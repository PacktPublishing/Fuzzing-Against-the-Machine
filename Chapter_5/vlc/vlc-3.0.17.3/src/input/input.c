/*****************************************************************************
 * input.c: input thread
 *****************************************************************************
 * Copyright (C) 1998-2007 VLC authors and VideoLAN
 * $Id: 04e5b3bb7270369763bd67f8aacd2659683c4fc6 $
 *
 * Authors: Christophe Massiot <massiot@via.ecp.fr>
 *          Laurent Aimar <fenrir@via.ecp.fr>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

/*****************************************************************************
 * Preamble
 *****************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>

#include <limits.h>
#include <assert.h>
#include <sys/stat.h>

#include "input_internal.h"
#include "event.h"
#include "es_out.h"
#include "es_out_timeshift.h"
#include "demux.h"
#include "item.h"
#include "resource.h"
#include "stream.h"

#include <vlc_aout.h>
#include <vlc_sout.h>
#include <vlc_dialog.h>
#include <vlc_url.h>
#include <vlc_charset.h>
#include <vlc_fs.h>
#include <vlc_strings.h>
#include <vlc_modules.h>
#include <vlc_stream.h>
#include <vlc_stream_extractor.h>
#include <vlc_renderer_discovery.h>

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/
static  void *Run( void * );
static  void *Preparse( void * );

static input_thread_t * Create  ( vlc_object_t *, input_item_t *,
                                  const char *, bool, input_resource_t *,
                                  vlc_renderer_item_t * );
static  int             Init    ( input_thread_t *p_input );
static void             End     ( input_thread_t *p_input );
static void             MainLoop( input_thread_t *p_input, bool b_interactive );

static inline int ControlPop( input_thread_t *, int *, vlc_value_t *, mtime_t i_deadline, bool b_postpone_seek );
static void       ControlRelease( int i_type, vlc_value_t val );
static bool       ControlIsSeekRequest( int i_type );
static bool       Control( input_thread_t *, int, vlc_value_t );
static void       ControlPause( input_thread_t *, mtime_t );

static int  UpdateTitleSeekpointFromDemux( input_thread_t * );
static void UpdateGenericFromDemux( input_thread_t * );
static void UpdateTitleListfromDemux( input_thread_t * );

static void MRLSections( const char *, int *, int *, int *, int *);

static input_source_t *InputSourceNew( input_thread_t *, const char *,
                                       const char *psz_forced_demux,
                                       bool b_in_can_fail );
static void InputSourceDestroy( input_source_t * );
static void InputSourceMeta( input_thread_t *, input_source_t *, vlc_meta_t * );

/* TODO */
//static void InputGetAttachments( input_thread_t *, input_source_t * );
static void SlaveDemux( input_thread_t *p_input );
static void SlaveSeek( input_thread_t *p_input );

static void InputMetaUser( input_thread_t *p_input, vlc_meta_t *p_meta );
static void InputUpdateMeta( input_thread_t *p_input, demux_t *p_demux );
static void InputGetExtraFiles( input_thread_t *p_input,
                                int *pi_list, char ***pppsz_list,
                                const char **psz_access, const char *psz_path );

static void AppendAttachment( int *pi_attachment, input_attachment_t ***ppp_attachment,
                              const demux_t ***ppp_attachment_demux,
                              int i_new, input_attachment_t **pp_new, const demux_t *p_demux );

#define SLAVE_ADD_NOFLAG    0
#define SLAVE_ADD_FORCED    (1<<0)
#define SLAVE_ADD_CANFAIL   (1<<1)
#define SLAVE_ADD_SET_TIME  (1<<2)

static int input_SlaveSourceAdd( input_thread_t *, enum slave_type,
                                 const char *, unsigned );
static char *input_SubtitleFile2Uri( input_thread_t *, const char * );
static void input_ChangeState( input_thread_t *p_input, int i_state ); /* TODO fix name */

#undef input_Create
/**
 * Create a new input_thread_t.
 *
 * You need to call input_Start on it when you are done
 * adding callback on the variables/events you want to monitor.
 *
 * \param p_parent a vlc_object
 * \param p_item an input item
 * \param psz_log an optional prefix for this input logs
 * \param p_resource an optional input ressource
 * \return a pointer to the spawned input thread
 */
input_thread_t *input_Create( vlc_object_t *p_parent,
                              input_item_t *p_item,
                              const char *psz_log, input_resource_t *p_resource,
                              vlc_renderer_item_t *p_renderer )
{
    return Create( p_parent, p_item, psz_log, false, p_resource, p_renderer );
}

#undef input_Read
/**
 * Initialize an input thread and run it until it stops by itself.
 *
 * \param p_parent a vlc_object
 * \param p_item an input item
 * \return an error code, VLC_SUCCESS on success
 */
int input_Read( vlc_object_t *p_parent, input_item_t *p_item )
{
    input_thread_t *p_input = Create( p_parent, p_item, NULL, false, NULL, NULL );
    if( !p_input )
        return VLC_EGENERIC;

    if( !Init( p_input ) )
    {
        MainLoop( p_input, false );
        End( p_input );
    }

    vlc_object_release( p_input );
    return VLC_SUCCESS;
}

input_thread_t *input_CreatePreparser( vlc_object_t *parent,
                                       input_item_t *item )
{
    return Create( parent, item, NULL, true, NULL, NULL );
}

/**
 * Start a input_thread_t created by input_Create.
 *
 * You must not start an already running input_thread_t.
 *
 * \param the input thread to start
 */
int input_Start( input_thread_t *p_input )
{
    input_thread_private_t *priv = input_priv(p_input);
    void *(*func)(void *) = Run;

    if( priv->b_preparsing )
        func = Preparse;

    assert( !priv->is_running );
    /* Create thread and wait for its readiness. */
    priv->is_running = !vlc_clone( &priv->thread, func, priv,
                                   VLC_THREAD_PRIORITY_INPUT );
    if( !priv->is_running )
    {
        input_ChangeState( p_input, ERROR_S );
        msg_Err( p_input, "cannot create input thread" );
        return VLC_EGENERIC;
    }
    return VLC_SUCCESS;
}

/**
 * Request a running input thread to stop and die
 *
 * \param p_input the input thread to stop
 */
void input_Stop( input_thread_t *p_input )
{
    input_thread_private_t *sys = input_priv(p_input);

    vlc_mutex_lock( &sys->lock_control );
    /* Discard all pending controls */
    for( int i = 0; i < sys->i_control; i++ )
    {
        input_control_t *ctrl = &sys->control[i];
        ControlRelease( ctrl->i_type, ctrl->val );
    }
    sys->i_control = 0;
    sys->is_stopped = true;
    vlc_cond_signal( &sys->wait_control );
    vlc_mutex_unlock( &sys->lock_control );
    vlc_interrupt_kill( &sys->interrupt );
}

/**
 * Close an input
 *
 * It does not call input_Stop itself.
 */
void input_Close( input_thread_t *p_input )
{
    if( input_priv(p_input)->is_running )
        vlc_join( input_priv(p_input)->thread, NULL );
    vlc_interrupt_deinit( &input_priv(p_input)->interrupt );
    vlc_object_release( p_input );
}

/**
 * Input destructor (called when the object's refcount reaches 0).
 */
static void input_Destructor( vlc_object_t *obj )
{
    input_thread_t *p_input = (input_thread_t *)obj;
    input_thread_private_t *priv = input_priv(p_input);
#ifndef NDEBUG
    char * psz_name = input_item_GetName( priv->p_item );
    msg_Dbg( p_input, "Destroying the input for '%s'", psz_name);
    free( psz_name );
#endif

    if( priv->p_renderer )
        vlc_renderer_item_release( priv->p_renderer );
    if( priv->p_es_out_display )
        es_out_Delete( priv->p_es_out_display );

    if( priv->p_resource )
        input_resource_Release( priv->p_resource );
    if( priv->p_resource_private )
        input_resource_Release( priv->p_resource_private );

    input_item_Release( priv->p_item );

    vlc_mutex_destroy( &priv->counters.counters_lock );

    for( int i = 0; i < priv->i_control; i++ )
    {
        input_control_t *p_ctrl = &priv->control[i];
        ControlRelease( p_ctrl->i_type, p_ctrl->val );
    }

    vlc_cond_destroy( &priv->wait_control );
    vlc_mutex_destroy( &priv->lock_control );
}

/**
 * Get the item from an input thread
 * FIXME it does not increase ref count of the item.
 * if it is used after p_input is destroyed nothing prevent it from
 * being freed.
 */
input_item_t *input_GetItem( input_thread_t *p_input )
{
    assert( p_input != NULL );
    return input_priv(p_input)->p_item;
}

/*****************************************************************************
 * This function creates a new input, and returns a pointer
 * to its description. On error, it returns NULL.
 *
 * XXX Do not forget to update vlc_input.h if you add new variables.
 *****************************************************************************/
static input_thread_t *Create( vlc_object_t *p_parent, input_item_t *p_item,
                               const char *psz_header, bool b_preparsing,
                               input_resource_t *p_resource,
                               vlc_renderer_item_t *p_renderer )
{
    /* Allocate descriptor */
    input_thread_private_t *priv;

    priv = vlc_custom_create( p_parent, sizeof( *priv ), "input" );
    if( unlikely(priv == NULL) )
        return NULL;

    input_thread_t *p_input = &priv->input;

    char * psz_name = input_item_GetName( p_item );
    msg_Dbg( p_input, "Creating an input for %s'%s'",
             b_preparsing ? "preparsing " : "", psz_name);
    free( psz_name );

    /* Parse input options */
    input_item_ApplyOptions( VLC_OBJECT(p_input), p_item );

    p_input->obj.header = psz_header ? strdup( psz_header ) : NULL;

    /* Init Common fields */
    priv->b_preparsing = b_preparsing;
    priv->b_can_pace_control = true;
    priv->i_start = 0;
    priv->i_time  = 0;
    priv->i_stop  = 0;
    priv->i_title = 0;
    priv->title = NULL;
    priv->i_title_offset = input_priv(p_input)->i_seekpoint_offset = 0;
    priv->i_state = INIT_S;
    priv->is_running = false;
    priv->is_stopped = false;
    priv->b_recording = false;
    priv->i_rate = INPUT_RATE_DEFAULT;
    memset( &priv->bookmark, 0, sizeof(priv->bookmark) );
    TAB_INIT( priv->i_bookmark, priv->pp_bookmark );
    TAB_INIT( priv->i_attachment, priv->attachment );
    priv->attachment_demux = NULL;
    priv->p_sout   = NULL;
    priv->b_out_pace_control = false;
    priv->p_renderer = p_renderer && b_preparsing == false ?
                vlc_renderer_item_hold( p_renderer ) : NULL;

    priv->viewpoint_changed = false;
    /* Fetch the viewpoint from the mediaplayer or the playlist if any */
    vlc_viewpoint_t *p_viewpoint = var_InheritAddress( p_input, "viewpoint" );
    if (p_viewpoint != NULL)
        priv->viewpoint = *p_viewpoint;
    else
        vlc_viewpoint_init( &priv->viewpoint );

    input_item_Hold( p_item ); /* Released in Destructor() */
    priv->p_item = p_item;

    /* Init Input fields */
    priv->master = NULL;
    vlc_mutex_lock( &p_item->lock );

    if( !p_item->p_stats )
        p_item->p_stats = stats_NewInputStats( p_input );

    /* setup the preparse depth of the item
     * if we are preparsing, use the i_preparse_depth of the parent item */
    if( !priv->b_preparsing )
    {
        char *psz_rec = var_InheritString( p_parent, "recursive" );

        if( psz_rec != NULL )
        {
            if ( !strcasecmp( psz_rec, "none" ) )
                p_item->i_preparse_depth = 0;
            else if ( !strcasecmp( psz_rec, "collapse" ) )
                p_item->i_preparse_depth = 1;
            else
                p_item->i_preparse_depth = -1; /* default is expand */
            free (psz_rec);
        } else
            p_item->i_preparse_depth = -1;
    }
    else
        p_input->obj.flags |= OBJECT_FLAGS_QUIET | OBJECT_FLAGS_NOINTERACT;

    /* Make sure the interaction option is honored */
    if( !var_InheritBool( p_input, "interact" ) )
        p_input->obj.flags |= OBJECT_FLAGS_NOINTERACT;
    else if( p_item->b_preparse_interact )
    {
        /* If true, this item was asked explicitly to interact with the user
         * (via libvlc_MetadataRequest). Sub items created from this input won't
         * have this flag and won't interact with the user */
        p_input->obj.flags &= ~OBJECT_FLAGS_NOINTERACT;
    }

    vlc_mutex_unlock( &p_item->lock );

    /* No slave */
    priv->i_slave = 0;
    priv->slave   = NULL;

    /* */
    if( p_resource )
    {
        priv->p_resource_private = NULL;
        priv->p_resource = input_resource_Hold( p_resource );
    }
    else
    {
        priv->p_resource_private = input_resource_New( VLC_OBJECT( p_input ) );
        priv->p_resource = input_resource_Hold( priv->p_resource_private );
    }
    input_resource_SetInput( priv->p_resource, p_input );

    /* Init control buffer */
    vlc_mutex_init( &priv->lock_control );
    vlc_cond_init( &priv->wait_control );
    priv->i_control = 0;
    vlc_interrupt_init(&priv->interrupt);

    /* Create Object Variables for private use only */
    input_ConfigVarInit( p_input );

    /* Create Objects variables for public Get and Set */
    input_ControlVarInit( p_input );

    /* */
    if( !priv->b_preparsing )
    {
        char *psz_bookmarks = var_GetNonEmptyString( p_input, "bookmarks" );
        if( psz_bookmarks )
        {
            /* FIXME: have a common cfg parsing routine used by sout and others */
            char *psz_parser, *psz_start, *psz_end;
            psz_parser = psz_bookmarks;
            while( (psz_start = strchr( psz_parser, '{' ) ) )
            {
                 seekpoint_t *p_seekpoint;
                 char backup;
                 psz_start++;
                 psz_end = strchr( psz_start, '}' );
                 if( !psz_end ) break;
                 psz_parser = psz_end + 1;
                 backup = *psz_parser;
                 *psz_parser = 0;
                 *psz_end = ',';

                 p_seekpoint = vlc_seekpoint_New();

                 if( unlikely( p_seekpoint == NULL ) )
                     break;

                 while( (psz_end = strchr( psz_start, ',' ) ) )
                 {
                     *psz_end = 0;
                     if( !strncmp( psz_start, "name=", 5 ) )
                     {
                         free( p_seekpoint->psz_name );

                         p_seekpoint->psz_name = strdup(psz_start + 5);
                     }
                     else if( !strncmp( psz_start, "time=", 5 ) )
                     {
                         p_seekpoint->i_time_offset = atof(psz_start + 5) *
                                                        CLOCK_FREQ;
                     }
                     psz_start = psz_end + 1;
                }
                msg_Dbg( p_input, "adding bookmark: %s, time=%"PRId64,
                                  p_seekpoint->psz_name,
                                  p_seekpoint->i_time_offset );
                input_Control( p_input, INPUT_ADD_BOOKMARK, p_seekpoint );
                vlc_seekpoint_Delete( p_seekpoint );
                *psz_parser = backup;
            }
            free( psz_bookmarks );
        }
    }

    /* Remove 'Now playing' info as it is probably outdated */
    input_item_SetNowPlaying( p_item, NULL );
    input_item_SetESNowPlaying( p_item, NULL );
    input_SendEventMeta( p_input );

    /* */
    memset( &priv->counters, 0, sizeof( priv->counters ) );
    vlc_mutex_init( &priv->counters.counters_lock );

    priv->p_es_out_display = input_EsOutNew( p_input, priv->i_rate );
    priv->p_es_out = NULL;

    /* Set the destructor when we are sure we are initialized */
    vlc_object_set_destructor( p_input, input_Destructor );

    return p_input;
}

/*****************************************************************************
 * Run: main thread loop
 * This is the "normal" thread that spawns the input processing chain,
 * reads the stream, cleans up and waits
 *****************************************************************************/
static void *Run( void *data )
{
    input_thread_private_t *priv = data;
    input_thread_t *p_input = &priv->input;

    vlc_interrupt_set(&priv->interrupt);

    if( !Init( p_input ) )
    {
        if( priv->b_can_pace_control && priv->b_out_pace_control )
        {
            /* We don't want a high input priority here or we'll
             * end-up sucking up all the CPU time */
            vlc_set_priority( priv->thread, VLC_THREAD_PRIORITY_LOW );
        }

        MainLoop( p_input, true ); /* FIXME it can be wrong (like with VLM) */

        /* Clean up */
        End( p_input );
    }

    input_SendEventDead( p_input );
    return NULL;
}

static void *Preparse( void *data )
{
    input_thread_private_t *priv = data;
    input_thread_t *p_input = &priv->input;

    vlc_interrupt_set(&priv->interrupt);

    if( !Init( p_input ) )
    {   /* if the demux is a playlist, call Mainloop that will call
         * demux_Demux in order to fetch sub items */
        bool b_is_playlist = false;

        if ( input_item_ShouldPreparseSubItems( priv->p_item )
          && demux_Control( priv->master->p_demux, DEMUX_IS_PLAYLIST,
                            &b_is_playlist ) )
            b_is_playlist = false;
        if( b_is_playlist )
            MainLoop( p_input, false );
        End( p_input );
    }

    input_SendEventDead( p_input );
    return NULL;
}

bool input_Stopped( input_thread_t *input )
{
    input_thread_private_t *sys = input_priv(input);
    bool ret;

    vlc_mutex_lock( &sys->lock_control );
    ret = sys->is_stopped;
    vlc_mutex_unlock( &sys->lock_control );
    return ret;
}

/*****************************************************************************
 * Main loop: Fill buffers from access, and demux
 *****************************************************************************/

/**
 * MainLoopDemux
 * It asks the demuxer to demux some data
 */
static void MainLoopDemux( input_thread_t *p_input, bool *pb_changed )
{
    input_thread_private_t* p_priv = input_priv(p_input);
    demux_t *p_demux = p_priv->master->p_demux;
    int i_ret = VLC_DEMUXER_SUCCESS;

    *pb_changed = false;

    if( p_priv->i_stop > 0 )
    {
        if( demux_Control( p_demux, DEMUX_GET_TIME, &p_priv->i_time ) )
            p_priv->i_time = 0;

        if( p_priv->i_stop <= p_priv->i_time )
            i_ret = VLC_DEMUXER_EOF;
    }

    if( i_ret == VLC_DEMUXER_SUCCESS )
        i_ret = demux_Demux( p_demux );

    i_ret = i_ret > 0 ? VLC_DEMUXER_SUCCESS : ( i_ret < 0 ? VLC_DEMUXER_EGENERIC : VLC_DEMUXER_EOF);

    if( i_ret == VLC_DEMUXER_SUCCESS )
    {
        if( demux_TestAndClearFlags( p_demux, INPUT_UPDATE_TITLE_LIST ) )
            UpdateTitleListfromDemux( p_input );

        if( p_priv->master->b_title_demux )
        {
            i_ret = UpdateTitleSeekpointFromDemux( p_input );
            *pb_changed = true;
        }

        UpdateGenericFromDemux( p_input );
    }

    if( i_ret == VLC_DEMUXER_EOF )
    {
        msg_Dbg( p_input, "EOF reached" );
        p_priv->master->b_eof = true;
        es_out_Eos(p_priv->p_es_out);
    }
    else if( i_ret == VLC_DEMUXER_EGENERIC )
    {
        input_ChangeState( p_input, ERROR_S );
    }
    else if( p_priv->i_slave > 0 )
        SlaveDemux( p_input );
}

static int MainLoopTryRepeat( input_thread_t *p_input )
{
    int i_repeat = var_GetInteger( p_input, "input-repeat" );
    if( i_repeat <= 0 )
        return VLC_EGENERIC;

    vlc_value_t val;

    msg_Dbg( p_input, "repeating the same input (%d)", i_repeat );
    if( i_repeat > 0 )
    {
        i_repeat--;
        var_SetInteger( p_input, "input-repeat", i_repeat );
    }

    /* Seek to start title/seekpoint */
    val.i_int = input_priv(p_input)->master->i_title_start -
        input_priv(p_input)->master->i_title_offset;
    if( val.i_int < 0 || val.i_int >= input_priv(p_input)->master->i_title )
        val.i_int = 0;
    input_ControlPush( p_input,
                       INPUT_CONTROL_SET_TITLE, &val );

    val.i_int = input_priv(p_input)->master->i_seekpoint_start -
        input_priv(p_input)->master->i_seekpoint_offset;
    if( val.i_int > 0 /* TODO: check upper boundary */ )
        input_ControlPush( p_input,
                           INPUT_CONTROL_SET_SEEKPOINT, &val );

    /* Seek to start position */
    if( input_priv(p_input)->i_start > 0 )
    {
        val.i_int = input_priv(p_input)->i_start;
        input_ControlPush( p_input, INPUT_CONTROL_SET_TIME, &val );
    }
    else
    {
        val.f_float = 0.f;
        input_ControlPush( p_input, INPUT_CONTROL_SET_POSITION, &val );
    }

    return VLC_SUCCESS;
}

/**
 * Update timing infos and statistics.
 */
static void MainLoopStatistics( input_thread_t *p_input )
{
    double f_position = 0.0;
    mtime_t i_time = 0;
    mtime_t i_length = 0;

    /* update input status variables */
    if( demux_Control( input_priv(p_input)->master->p_demux,
                       DEMUX_GET_POSITION, &f_position ) )
        f_position = 0.0;

    if( demux_Control( input_priv(p_input)->master->p_demux,
                       DEMUX_GET_TIME, &i_time ) )
        i_time = 0;
    input_priv(p_input)->i_time = i_time;

    if( demux_Control( input_priv(p_input)->master->p_demux,
                       DEMUX_GET_LENGTH, &i_length ) )
        i_length = 0;

    es_out_SetTimes( input_priv(p_input)->p_es_out, f_position, i_time, i_length );

    /* update current bookmark */
    vlc_mutex_lock( &input_priv(p_input)->p_item->lock );
    input_priv(p_input)->bookmark.i_time_offset = i_time;
    vlc_mutex_unlock( &input_priv(p_input)->p_item->lock );

    stats_ComputeInputStats( p_input, input_priv(p_input)->p_item->p_stats );
    input_SendEventStatistics( p_input );
}

/**
 * MainLoop
 * The main input loop.
 */
static void MainLoop( input_thread_t *p_input, bool b_interactive )
{
    mtime_t i_intf_update = 0;
    mtime_t i_last_seek_mdate = 0;

    if( b_interactive && var_InheritBool( p_input, "start-paused" ) )
        ControlPause( p_input, mdate() );

    bool b_pause_after_eof = b_interactive &&
                           var_InheritBool( p_input, "play-and-pause" );
    bool b_paused_at_eof = false;

    demux_t *p_demux = input_priv(p_input)->master->p_demux;
    const bool b_can_demux = p_demux->pf_demux != NULL;

    while( !input_Stopped( p_input ) && input_priv(p_input)->i_state != ERROR_S )
    {
        mtime_t i_wakeup = -1;
        bool b_paused = input_priv(p_input)->i_state == PAUSE_S;
        /* FIXME if input_priv(p_input)->i_state == PAUSE_S the access/access_demux
         * is paused -> this may cause problem with some of them
         * The same problem can be seen when seeking while paused */
        if( b_paused )
            b_paused = !es_out_GetBuffering( input_priv(p_input)->p_es_out )
                    || input_priv(p_input)->master->b_eof;

        if( !b_paused )
        {
            if( !input_priv(p_input)->master->b_eof )
            {
                bool b_force_update = false;

                MainLoopDemux( p_input, &b_force_update );

                if( b_can_demux )
                    i_wakeup = es_out_GetWakeup( input_priv(p_input)->p_es_out );
                if( b_force_update )
                    i_intf_update = 0;

                b_paused_at_eof = false;
            }
            else if( !es_out_GetEmpty( input_priv(p_input)->p_es_out ) )
            {
                msg_Dbg( p_input, "waiting decoder fifos to empty" );
                i_wakeup = mdate() + INPUT_IDLE_SLEEP;
            }
            /* Pause after eof only if the input is pausable.
             * This way we won't trigger timeshifting for nothing */
            else if( b_pause_after_eof && input_priv(p_input)->b_can_pause )
            {
                if( b_paused_at_eof )
                    break;

                vlc_value_t val = { .i_int = PAUSE_S };

                msg_Dbg( p_input, "pausing at EOF (pause after each)");
                Control( p_input, INPUT_CONTROL_SET_STATE, val );

                b_paused = true;
                b_paused_at_eof = true;
            }
            else
            {
                if( MainLoopTryRepeat( p_input ) )
                    break;
            }

            /* Update interface and statistics */
            mtime_t now = mdate();
            if( now >= i_intf_update )
            {
                MainLoopStatistics( p_input );
                i_intf_update = now + INT64_C(250000);
            }
        }

        /* Handle control */
        for( ;; )
        {
            mtime_t i_deadline = i_wakeup;

            /* Postpone seeking until ES buffering is complete or at most
             * 125 ms. */
            bool b_postpone = es_out_GetBuffering( input_priv(p_input)->p_es_out )
                            && !input_priv(p_input)->master->b_eof;
            if( b_postpone )
            {
                mtime_t now = mdate();

                /* Recheck ES buffer level every 20 ms when seeking */
                if( now < i_last_seek_mdate + INT64_C(125000)
                 && (i_deadline < 0 || i_deadline > now + INT64_C(20000)) )
                    i_deadline = now + INT64_C(20000);
                else
                    b_postpone = false;
            }

            int i_type;
            vlc_value_t val;

            if( ControlPop( p_input, &i_type, &val, i_deadline, b_postpone ) )
            {
                if( b_postpone )
                    continue;
                break; /* Wake-up time reached */
            }

#ifndef NDEBUG
            msg_Dbg( p_input, "control type=%d", i_type );
#endif
            if( Control( p_input, i_type, val ) )
            {
                if( ControlIsSeekRequest( i_type ) )
                    i_last_seek_mdate = mdate();
                i_intf_update = 0;
            }

            /* Update the wakeup time */
            if( i_wakeup != 0 )
                i_wakeup = es_out_GetWakeup( input_priv(p_input)->p_es_out );
        }
    }
}

static void InitStatistics( input_thread_t *p_input )
{
    input_thread_private_t *priv = input_priv(p_input);

    if( priv->b_preparsing ) return;

    /* Prepare statistics */
#define INIT_COUNTER( c, compute ) free( priv->counters.p_##c ); \
    priv->counters.p_##c = \
 stats_CounterCreate( STATS_##compute);
    if( libvlc_stats( p_input ) )
    {
        INIT_COUNTER( read_bytes, COUNTER );
        INIT_COUNTER( read_packets, COUNTER );
        INIT_COUNTER( demux_read, COUNTER );
        INIT_COUNTER( input_bitrate, DERIVATIVE );
        INIT_COUNTER( demux_bitrate, DERIVATIVE );
        INIT_COUNTER( demux_corrupted, COUNTER );
        INIT_COUNTER( demux_discontinuity, COUNTER );
        INIT_COUNTER( played_abuffers, COUNTER );
        INIT_COUNTER( lost_abuffers, COUNTER );
        INIT_COUNTER( displayed_pictures, COUNTER );
        INIT_COUNTER( lost_pictures, COUNTER );
        INIT_COUNTER( decoded_audio, COUNTER );
        INIT_COUNTER( decoded_video, COUNTER );
        INIT_COUNTER( decoded_sub, COUNTER );
        priv->counters.p_sout_send_bitrate = NULL;
        priv->counters.p_sout_sent_packets = NULL;
        priv->counters.p_sout_sent_bytes = NULL;
    }
}

#ifdef ENABLE_SOUT
static int InitSout( input_thread_t * p_input )
{
    input_thread_private_t *priv = input_priv(p_input);

    if( priv->b_preparsing )
        return VLC_SUCCESS;

    /* Find a usable sout and attach it to p_input */
    char *psz = var_GetNonEmptyString( p_input, "sout" );
    if( priv->p_renderer )
    {
        /* Keep sout if it comes from a renderer and if the user didn't touch
         * the sout config */
        bool keep_sout = psz == NULL;
        free(psz);

        const char *psz_renderer_sout = vlc_renderer_item_sout( priv->p_renderer );
        if( asprintf( &psz, "#%s", psz_renderer_sout ) < 0 )
            return VLC_ENOMEM;
        if( keep_sout )
            var_SetBool( p_input, "sout-keep", true );
    }
    if( psz && strncasecmp( priv->p_item->psz_uri, "vlc:", 4 ) )
    {
        priv->p_sout  = input_resource_RequestSout( priv->p_resource, NULL, psz );
        if( priv->p_sout == NULL )
        {
            input_ChangeState( p_input, ERROR_S );
            msg_Err( p_input, "cannot start stream output instance, " \
                              "aborting" );
            free( psz );
            return VLC_EGENERIC;
        }
        if( libvlc_stats( p_input ) )
        {
            INIT_COUNTER( sout_sent_packets, COUNTER );
            INIT_COUNTER( sout_sent_bytes, COUNTER );
            INIT_COUNTER( sout_send_bitrate, DERIVATIVE );
        }
    }
    else
    {
        input_resource_RequestSout( priv->p_resource, NULL, NULL );
    }
    free( psz );

    return VLC_SUCCESS;
}
#endif

static void InitTitle( input_thread_t * p_input )
{
    input_thread_private_t *priv = input_priv(p_input);
    input_source_t *p_master = priv->master;

    if( priv->b_preparsing )
        return;

    vlc_mutex_lock( &priv->p_item->lock );
    /* Create global title (from master) */
    priv->i_title = p_master->i_title;
    priv->title   = p_master->title;
    priv->i_title_offset = p_master->i_title_offset;
    priv->i_seekpoint_offset = p_master->i_seekpoint_offset;
    if( priv->i_title > 0 )
    {
        /* Setup variables */
        input_ControlVarNavigation( p_input );
        input_SendEventTitle( p_input, 0 );
    }

    /* Global flag */
    priv->b_can_pace_control = p_master->b_can_pace_control;
    priv->b_can_pause        = p_master->b_can_pause;
    priv->b_can_rate_control = p_master->b_can_rate_control;
    vlc_mutex_unlock( &priv->p_item->lock );
}

static void StartTitle( input_thread_t * p_input )
{
    input_thread_private_t *priv = input_priv(p_input);
    vlc_value_t val;

    /* Start title/chapter */
    val.i_int = priv->master->i_title_start - priv->master->i_title_offset;
    if( val.i_int > 0 && val.i_int < priv->master->i_title )
        input_ControlPush( p_input, INPUT_CONTROL_SET_TITLE, &val );

    val.i_int = priv->master->i_seekpoint_start -
                priv->master->i_seekpoint_offset;
    if( val.i_int > 0 /* TODO: check upper boundary */ )
        input_ControlPush( p_input, INPUT_CONTROL_SET_SEEKPOINT, &val );

    /* Start/stop/run time */
    priv->i_start = llroundf(1000000.f
                                     * var_GetFloat( p_input, "start-time" ));
    priv->i_stop  = llroundf(1000000.f
                                     * var_GetFloat( p_input, "stop-time" ));
    if( priv->i_stop <= 0 )
    {
        priv->i_stop = llroundf(1000000.f
                                     * var_GetFloat( p_input, "run-time" ));
        if( priv->i_stop < 0 )
        {
            msg_Warn( p_input, "invalid run-time ignored" );
            priv->i_stop = 0;
        }
        else
            priv->i_stop += priv->i_start;
    }

    if( priv->i_start > 0 )
    {
        vlc_value_t s;

        msg_Dbg( p_input, "starting at time: %"PRId64"s",
                 priv->i_start / CLOCK_FREQ );

        s.i_int = priv->i_start;
        input_ControlPush( p_input, INPUT_CONTROL_SET_TIME, &s );
    }
    if( priv->i_stop > 0 && priv->i_stop <= priv->i_start )
    {
        msg_Warn( p_input, "invalid stop-time ignored" );
        priv->i_stop = 0;
    }
    priv->b_fast_seek = var_GetBool( p_input, "input-fast-seek" );
}

static int SlaveCompare(const void *a, const void *b)
{
    const input_item_slave_t *p_slave0 = *((const input_item_slave_t **) a);
    const input_item_slave_t *p_slave1 = *((const input_item_slave_t **) b);

    if( p_slave0 == NULL || p_slave1 == NULL )
    {
        /* Put NULL (or rejected) subs at the end */
        return p_slave0 == NULL ? 1 : p_slave1 == NULL ? -1 : 0;
    }

    if( p_slave0->i_priority > p_slave1->i_priority )
        return -1;

    if( p_slave0->i_priority < p_slave1->i_priority )
        return 1;

    return 0;
}

static bool SlaveExists( input_item_slave_t **pp_slaves, int i_slaves,
                         const char *psz_uri)
{
    for( int i = 0; i < i_slaves; i++ )
    {
        if( pp_slaves[i] != NULL
         && !strcmp( pp_slaves[i]->psz_uri, psz_uri ) )
            return true;
    }
    return false;
}

static void SetSubtitlesOptions( input_thread_t *p_input )
{
    /* Get fps and set it if not already set */
    const float f_fps = input_priv(p_input)->master->f_fps;
    if( f_fps > 1.f )
    {
        var_Create( p_input, "sub-original-fps", VLC_VAR_FLOAT );
        var_SetFloat( p_input, "sub-original-fps", f_fps );

        float f_requested_fps = var_CreateGetFloat( p_input, "sub-fps" );
        if( f_requested_fps != f_fps )
        {
            var_Create( p_input, "sub-fps", VLC_VAR_FLOAT|
                                            VLC_VAR_DOINHERIT );
            var_SetFloat( p_input, "sub-fps", f_requested_fps );
        }
    }

    const int i_delay = var_CreateGetInteger( p_input, "sub-delay" );
    if( i_delay != 0 )
        var_SetInteger( p_input, "spu-delay", (mtime_t)i_delay * 100000 );
}

static enum slave_type DeduceSlaveType( input_thread_t *p_input,
                                        const char *psz_uri )
{
    vlc_url_t parsed_uri;
    if( vlc_UrlParse( &parsed_uri, psz_uri ) != VLC_SUCCESS ||
        parsed_uri.psz_path == NULL )
    {
        goto fail;
    }

    enum slave_type i_type;
    if( !input_item_slave_GetType( parsed_uri.psz_path, &i_type ) )
        goto fail;

    vlc_UrlClean( &parsed_uri );
    return i_type;

fail:
    msg_Dbg( p_input, "Can't deduce slave type of \"%s\" with file extension.",
             psz_uri );
    vlc_UrlClean( &parsed_uri );
    return SLAVE_TYPE_AUDIO;
}

static void GetVarSlaves( input_thread_t *p_input,
                          input_item_slave_t ***ppp_slaves, int *p_slaves )
{
    char *psz = var_GetNonEmptyString( p_input, "input-slave" );
    if( !psz )
        return;

    input_item_slave_t **pp_slaves = *ppp_slaves;
    int i_slaves = *p_slaves;

    char *psz_org = psz;
    while( psz && *psz )
    {
        while( *psz == ' ' || *psz == '#' )
            psz++;

        char *psz_delim = strchr( psz, '#' );
        if( psz_delim )
            *psz_delim++ = '\0';

        if( *psz == 0 )
            break;

        char *uri = strstr(psz, "://")
                                   ? strdup( psz ) : vlc_path2uri( psz, NULL );
        psz = psz_delim;
        if( uri == NULL )
            continue;

        const enum slave_type i_type = DeduceSlaveType( p_input, uri );
        input_item_slave_t *p_slave =
            input_item_slave_New( uri, i_type, SLAVE_PRIORITY_USER );
        free( uri );

        if( unlikely( p_slave == NULL ) )
            break;
        TAB_APPEND(i_slaves, pp_slaves, p_slave);
    }
    free( psz_org );

    *ppp_slaves = pp_slaves; /* in case of realloc */
    *p_slaves = i_slaves;
}

static void LoadSlaves( input_thread_t *p_input )
{
    input_item_slave_t **pp_slaves;
    int i_slaves;
    TAB_INIT( i_slaves, pp_slaves );

    /* Look for and add slaves */

    char *psz_subtitle = var_GetNonEmptyString( p_input, "sub-file" );
    if( psz_subtitle != NULL )
    {
        msg_Dbg( p_input, "forced subtitle: %s", psz_subtitle );
        char *psz_uri = input_SubtitleFile2Uri( p_input, psz_subtitle );
        free( psz_subtitle );
        psz_subtitle = NULL;
        if( psz_uri != NULL )
        {
            input_item_slave_t *p_slave =
                input_item_slave_New( psz_uri, SLAVE_TYPE_SPU,
                                      SLAVE_PRIORITY_USER );
            free( psz_uri );
            if( p_slave )
            {
                TAB_APPEND(i_slaves, pp_slaves, p_slave);
                psz_subtitle = p_slave->psz_uri;
            }
        }
    }

    if( var_GetBool( p_input, "sub-autodetect-file" ) )
    {
        /* Add local subtitles */
        char *psz_autopath = var_GetNonEmptyString( p_input, "sub-autodetect-path" );

        if( subtitles_Detect( p_input, psz_autopath, input_priv(p_input)->p_item->psz_uri,
                              &pp_slaves, &i_slaves ) == VLC_SUCCESS )
        {
            /* check that we did not add the subtitle through sub-file */
            if( psz_subtitle != NULL )
            {
                for( int i = 1; i < i_slaves; i++ )
                {
                    input_item_slave_t *p_curr = pp_slaves[i];
                    if( p_curr != NULL
                     && !strcmp( psz_subtitle, p_curr->psz_uri ) )
                    {
                        /* reject current sub */
                        input_item_slave_Delete( p_curr );
                        pp_slaves[i] = NULL;
                    }
                }
            }
        }
        free( psz_autopath );
    }

    /* Add slaves from the "input-slave" option */
    GetVarSlaves( p_input, &pp_slaves, &i_slaves );

    /* Add slaves found by the directory demuxer or via libvlc */
    input_item_t *p_item = input_priv(p_input)->p_item;
    vlc_mutex_lock( &p_item->lock );

    /* Move item slaves to local pp_slaves */
    for( int i = 0; i < p_item->i_slaves; i++ )
    {
        input_item_slave_t *p_slave = p_item->pp_slaves[i];
        if( !SlaveExists( pp_slaves, i_slaves, p_slave->psz_uri ) )
            TAB_APPEND(i_slaves, pp_slaves, p_slave);
        else
            input_item_slave_Delete( p_slave );
    }
    /* Slaves that are successfully loaded will be added back to the item */
    TAB_CLEAN( p_item->i_slaves, p_item->pp_slaves );
    vlc_mutex_unlock( &p_item->lock );

    if( i_slaves > 0 )
        qsort( pp_slaves, i_slaves, sizeof (input_item_slave_t*),
               SlaveCompare );

    /* add all detected slaves */
    bool p_forced[2] = { false, false };
    static_assert( SLAVE_TYPE_AUDIO <= 1 && SLAVE_TYPE_SPU <= 1,
                   "slave type size mismatch");
    for( int i = 0; i < i_slaves && pp_slaves[i] != NULL; i++ )
    {
        input_item_slave_t *p_slave = pp_slaves[i];
        /* Slaves added via options should not fail */
        unsigned i_flags = p_slave->i_priority != SLAVE_PRIORITY_USER
                           ? SLAVE_ADD_CANFAIL : SLAVE_ADD_NOFLAG;
        bool b_forced = false;

        /* Force the first subtitle with the highest priority or with the
         * forced flag */
        if( !p_forced[p_slave->i_type]
         && ( p_slave->b_forced || p_slave->i_priority == SLAVE_PRIORITY_USER ) )
        {
            i_flags |= SLAVE_ADD_FORCED;
            b_forced = true;
        }

        if( input_SlaveSourceAdd( p_input, p_slave->i_type, p_slave->psz_uri,
                                  i_flags ) == VLC_SUCCESS )
        {
            input_item_AddSlave( input_priv(p_input)->p_item, p_slave );
            if( b_forced )
                p_forced[p_slave->i_type] = true;
        }
        else
            input_item_slave_Delete( p_slave );
    }
    TAB_CLEAN( i_slaves, pp_slaves );

    /* Load subtitles from attachments */
    int i_attachment = 0;
    input_attachment_t **pp_attachment = NULL;

    vlc_mutex_lock( &input_priv(p_input)->p_item->lock );
    for( int i = 0; i < input_priv(p_input)->i_attachment; i++ )
    {
        const input_attachment_t *a = input_priv(p_input)->attachment[i];
        if( !strcmp( a->psz_mime, "application/x-srt" ) )
            TAB_APPEND( i_attachment, pp_attachment,
                        vlc_input_attachment_New( a->psz_name, NULL,
                                                  a->psz_description, NULL, 0 ) );
    }
    vlc_mutex_unlock( &input_priv(p_input)->p_item->lock );

    if( i_attachment > 0 )
        var_Create( p_input, "sub-description", VLC_VAR_STRING );
    for( int i = 0; i < i_attachment; i++ )
    {
        input_attachment_t *a = pp_attachment[i];
        if( !a )
            continue;
        char *psz_mrl;
        if( a->psz_name[0] &&
            asprintf( &psz_mrl, "attachment://%s", a->psz_name ) >= 0 )
        {
            var_SetString( p_input, "sub-description", a->psz_description ? a->psz_description : "");

            /* Force the first subtitle from attachment if there is no
             * subtitles already forced */
            if( input_SlaveSourceAdd( p_input, SLAVE_TYPE_SPU, psz_mrl,
                                      p_forced[ SLAVE_TYPE_SPU ] ?
                                      SLAVE_ADD_NOFLAG : SLAVE_ADD_FORCED ) == VLC_SUCCESS )
                p_forced[ SLAVE_TYPE_SPU ] = true;

            free( psz_mrl );
            /* Don't update item slaves for attachements */
        }
        vlc_input_attachment_Delete( a );
    }
    free( pp_attachment );
    if( i_attachment > 0 )
        var_Destroy( p_input, "sub-description" );
}

static void UpdatePtsDelay( input_thread_t *p_input )
{
    input_thread_private_t *p_sys = input_priv(p_input);

    /* Get max pts delay from input source */
    mtime_t i_pts_delay = p_sys->master->i_pts_delay;
    for( int i = 0; i < p_sys->i_slave; i++ )
        i_pts_delay = __MAX( i_pts_delay, p_sys->slave[i]->i_pts_delay );

    if( i_pts_delay < 0 )
        i_pts_delay = 0;

    /* Take care of audio/spu delay */
    const mtime_t i_audio_delay = var_GetInteger( p_input, "audio-delay" );
    const mtime_t i_spu_delay   = var_GetInteger( p_input, "spu-delay" );
    const mtime_t i_extra_delay = __MIN( i_audio_delay, i_spu_delay );
    if( i_extra_delay < 0 )
        i_pts_delay -= i_extra_delay;

    /* Update cr_average depending on the caching */
    const int i_cr_average = var_GetInteger( p_input, "cr-average" ) * i_pts_delay / DEFAULT_PTS_DELAY;

    /* */
    es_out_SetDelay( input_priv(p_input)->p_es_out_display, AUDIO_ES, i_audio_delay );
    es_out_SetDelay( input_priv(p_input)->p_es_out_display, SPU_ES, i_spu_delay );
    es_out_SetJitter( input_priv(p_input)->p_es_out, i_pts_delay, 0, i_cr_average );
}

static void InitPrograms( input_thread_t * p_input )
{
    int i_es_out_mode;
    vlc_list_t list;

    /* Compute correct pts_delay */
    UpdatePtsDelay( p_input );

    /* Set up es_out */
    i_es_out_mode = ES_OUT_MODE_AUTO;
    if( input_priv(p_input)->p_sout && !input_priv(p_input)->p_renderer )
    {
        char *prgms;

        if( (prgms = var_GetNonEmptyString( p_input, "programs" )) != NULL )
        {
            char *buf;

            TAB_INIT( list.i_count, list.p_values );
            for( const char *prgm = strtok_r( prgms, ",", &buf );
                 prgm != NULL;
                 prgm = strtok_r( NULL, ",", &buf ) )
            {
                vlc_value_t val = { .i_int = atoi( prgm ) };
                TAB_APPEND(list.i_count, list.p_values, val);
            }

            if( list.i_count > 0 )
                i_es_out_mode = ES_OUT_MODE_PARTIAL;
                /* Note : we should remove the "program" callback. */

            free( prgms );
        }
        else if( var_GetBool( p_input, "sout-all" ) )
        {
            i_es_out_mode = ES_OUT_MODE_ALL;
        }
    }
    es_out_SetMode( input_priv(p_input)->p_es_out, i_es_out_mode );

    /* Inform the demuxer about waited group (needed only for DVB) */
    if( i_es_out_mode == ES_OUT_MODE_ALL )
    {
        demux_Control( input_priv(p_input)->master->p_demux, DEMUX_SET_GROUP, -1, NULL );
    }
    else if( i_es_out_mode == ES_OUT_MODE_PARTIAL )
    {
        demux_Control( input_priv(p_input)->master->p_demux, DEMUX_SET_GROUP, -1,
                       &list );
        TAB_CLEAN( list.i_count, list.p_values );
    }
    else
    {
        demux_Control( input_priv(p_input)->master->p_demux, DEMUX_SET_GROUP,
                       es_out_GetGroupForced( input_priv(p_input)->p_es_out ), NULL );
    }
}

static int Init( input_thread_t * p_input )
{
    input_thread_private_t *priv = input_priv(p_input);
    input_source_t *master;

    if( var_Type( p_input->obj.parent, "meta-file" ) )
    {
        msg_Dbg( p_input, "Input is a meta file: disabling unneeded options" );
        var_SetString( p_input, "sout", "" );
        var_SetBool( p_input, "sout-all", false );
        var_SetString( p_input, "input-slave", "" );
        var_SetInteger( p_input, "input-repeat", 0 );
        var_SetString( p_input, "sub-file", "" );
        var_SetBool( p_input, "sub-autodetect-file", false );
    }

    InitStatistics( p_input );
#ifdef ENABLE_SOUT
    if( InitSout( p_input ) )
        goto error;
#endif

    /* Create es out */
    priv->p_es_out = input_EsOutTimeshiftNew( p_input, priv->p_es_out_display,
                                              priv->i_rate );
    if( priv->p_es_out == NULL )
        goto error;

    /* */
    input_ChangeState( p_input, OPENING_S );
    input_SendEventCache( p_input, 0.0 );

    /* */
    master = InputSourceNew( p_input, priv->p_item->psz_uri, NULL, false );
    if( master == NULL )
        goto error;
    priv->master = master;

    InitTitle( p_input );

    /* Load master infos */
    /* Init length */
    mtime_t i_length;
    if( demux_Control( master->p_demux, DEMUX_GET_LENGTH, &i_length ) )
        i_length = 0;
    if( i_length <= 0 )
        i_length = input_item_GetDuration( priv->p_item );
    input_SendEventLength( p_input, i_length );

    input_SendEventPosition( p_input, 0.0, 0 );

    if( !priv->b_preparsing )
    {
        StartTitle( p_input );
        SetSubtitlesOptions( p_input );
        LoadSlaves( p_input );
        InitPrograms( p_input );

        double f_rate = var_InheritFloat( p_input, "rate" );
        if( f_rate != 0.0 && f_rate != 1.0 )
        {
            vlc_value_t val = { .i_int = INPUT_RATE_DEFAULT / f_rate };
            input_ControlPush( p_input, INPUT_CONTROL_SET_RATE, &val );
        }
    }

    if( !priv->b_preparsing && priv->p_sout )
    {
        priv->b_out_pace_control = priv->p_sout->i_out_pace_nocontrol > 0;

        msg_Dbg( p_input, "starting in %ssync mode",
                 priv->b_out_pace_control ? "a" : "" );
    }

    vlc_meta_t *p_meta = vlc_meta_New();
    if( p_meta != NULL )
    {
        /* Get meta data from users */
        InputMetaUser( p_input, p_meta );

        /* Get meta data from master input */
        InputSourceMeta( p_input, master, p_meta );

        /* And from slave */
        for( int i = 0; i < priv->i_slave; i++ )
            InputSourceMeta( p_input, priv->slave[i], p_meta );

        es_out_ControlSetMeta( priv->p_es_out, p_meta );
        vlc_meta_Delete( p_meta );
    }

    msg_Dbg( p_input, "`%s' successfully opened",
             input_priv(p_input)->p_item->psz_uri );

    /* initialization is complete */
    input_ChangeState( p_input, PLAYING_S );

    return VLC_SUCCESS;

error:
    input_ChangeState( p_input, ERROR_S );

    if( input_priv(p_input)->p_es_out )
        es_out_Delete( input_priv(p_input)->p_es_out );
    es_out_SetMode( input_priv(p_input)->p_es_out_display, ES_OUT_MODE_END );
    if( input_priv(p_input)->p_resource )
    {
        if( input_priv(p_input)->p_sout )
            input_resource_RequestSout( input_priv(p_input)->p_resource,
                                         input_priv(p_input)->p_sout, NULL );
        input_resource_SetInput( input_priv(p_input)->p_resource, NULL );
        if( input_priv(p_input)->p_resource_private )
            input_resource_Terminate( input_priv(p_input)->p_resource_private );
    }

    if( !priv->b_preparsing && libvlc_stats( p_input ) )
    {
#define EXIT_COUNTER( c ) do { if( input_priv(p_input)->counters.p_##c ) \
                                   stats_CounterClean( input_priv(p_input)->counters.p_##c );\
                               input_priv(p_input)->counters.p_##c = NULL; } while(0)
        EXIT_COUNTER( read_bytes );
        EXIT_COUNTER( read_packets );
        EXIT_COUNTER( demux_read );
        EXIT_COUNTER( input_bitrate );
        EXIT_COUNTER( demux_bitrate );
        EXIT_COUNTER( demux_corrupted );
        EXIT_COUNTER( demux_discontinuity );
        EXIT_COUNTER( played_abuffers );
        EXIT_COUNTER( lost_abuffers );
        EXIT_COUNTER( displayed_pictures );
        EXIT_COUNTER( lost_pictures );
        EXIT_COUNTER( decoded_audio );
        EXIT_COUNTER( decoded_video );
        EXIT_COUNTER( decoded_sub );

        if( input_priv(p_input)->p_sout )
        {
            EXIT_COUNTER( sout_sent_packets );
            EXIT_COUNTER( sout_sent_bytes );
            EXIT_COUNTER( sout_send_bitrate );
        }
#undef EXIT_COUNTER
    }

    /* Mark them deleted */
    input_priv(p_input)->p_es_out = NULL;
    input_priv(p_input)->p_sout = NULL;

    return VLC_EGENERIC;
}

/*****************************************************************************
 * End: end the input thread
 *****************************************************************************/
static void End( input_thread_t * p_input )
{
    input_thread_private_t *priv = input_priv(p_input);

    /* We are at the end */
    input_ChangeState( p_input, END_S );

    /* Clean control variables */
    input_ControlVarStop( p_input );

    /* Stop es out activity */
    es_out_SetMode( priv->p_es_out, ES_OUT_MODE_NONE );

    /* Delete slave */
    for( int i = 0; i < priv->i_slave; i++ )
        InputSourceDestroy( priv->slave[i] );
    free( priv->slave );

    /* Clean up master */
    InputSourceDestroy( priv->master );
    priv->i_title = 0;
    priv->title = NULL;
    priv->i_title_offset = 0;
    priv->i_seekpoint_offset = 0;

    /* Unload all modules */
    if( priv->p_es_out )
        es_out_Delete( priv->p_es_out );
    es_out_SetMode( priv->p_es_out_display, ES_OUT_MODE_END );

    if( !priv->b_preparsing )
    {
#define CL_CO( c ) \
do { \
    stats_CounterClean( priv->counters.p_##c ); \
    priv->counters.p_##c = NULL; \
} while (0)

        if( libvlc_stats( p_input ) )
        {
            /* make sure we are up to date */
            stats_ComputeInputStats( p_input, priv->p_item->p_stats );
            CL_CO( read_bytes );
            CL_CO( read_packets );
            CL_CO( demux_read );
            CL_CO( input_bitrate );
            CL_CO( demux_bitrate );
            CL_CO( demux_corrupted );
            CL_CO( demux_discontinuity );
            CL_CO( played_abuffers );
            CL_CO( lost_abuffers );
            CL_CO( displayed_pictures );
            CL_CO( lost_pictures );
            CL_CO( decoded_audio) ;
            CL_CO( decoded_video );
            CL_CO( decoded_sub) ;
        }

        /* Close optional stream output instance */
        if( priv->p_sout )
        {
            CL_CO( sout_sent_packets );
            CL_CO( sout_sent_bytes );
            CL_CO( sout_send_bitrate );
        }
#undef CL_CO
    }

    vlc_mutex_lock( &priv->p_item->lock );
    if( priv->i_attachment > 0 )
    {
        for( int i = 0; i < priv->i_attachment; i++ )
            vlc_input_attachment_Delete( priv->attachment[i] );
        TAB_CLEAN( priv->i_attachment, priv->attachment );
        free( priv->attachment_demux);
        priv->attachment_demux = NULL;
    }

    /* clean bookmarks */
    for( int i = 0; i < priv->i_bookmark; ++i )
        vlc_seekpoint_Delete( priv->pp_bookmark[i] );
    TAB_CLEAN( priv->i_bookmark, priv->pp_bookmark );

    vlc_mutex_unlock( &input_priv(p_input)->p_item->lock );

    /* */
    input_resource_RequestSout( input_priv(p_input)->p_resource,
                                 input_priv(p_input)->p_sout, NULL );
    input_resource_SetInput( input_priv(p_input)->p_resource, NULL );
    if( input_priv(p_input)->p_resource_private )
        input_resource_Terminate( input_priv(p_input)->p_resource_private );
}

/*****************************************************************************
 * Control
 *****************************************************************************/
void input_ControlPush( input_thread_t *p_input,
                        int i_type, vlc_value_t *p_val )
{
    input_thread_private_t *sys = input_priv(p_input);

    vlc_mutex_lock( &sys->lock_control );
    if( sys->is_stopped || sys->i_control >= INPUT_CONTROL_FIFO_SIZE )
    {
        if( sys->is_stopped )
            msg_Dbg( p_input, "input control stopped, trashing type=%d",
                     i_type );
        else
            msg_Err( p_input, "input control fifo overflow, trashing type=%d",
                     i_type );
        if( p_val )
            ControlRelease( i_type, *p_val );
    }
    else
    {
        input_control_t c;
        c.i_type = i_type;
        if( p_val )
            c.val = *p_val;
        else
            memset( &c.val, 0, sizeof(c.val) );

        sys->control[sys->i_control++] = c;

        vlc_cond_signal( &sys->wait_control );
    }
    vlc_mutex_unlock( &sys->lock_control );
}

static int ControlGetReducedIndexLocked( input_thread_t *p_input )
{
    const int i_lt = input_priv(p_input)->control[0].i_type;
    int i;
    for( i = 1; i < input_priv(p_input)->i_control; i++ )
    {
        const int i_ct = input_priv(p_input)->control[i].i_type;

        if( i_lt == i_ct &&
            ( i_ct == INPUT_CONTROL_SET_STATE ||
              i_ct == INPUT_CONTROL_SET_RATE ||
              i_ct == INPUT_CONTROL_SET_POSITION ||
              i_ct == INPUT_CONTROL_SET_TIME ||
              i_ct == INPUT_CONTROL_SET_PROGRAM ||
              i_ct == INPUT_CONTROL_SET_TITLE ||
              i_ct == INPUT_CONTROL_SET_SEEKPOINT ||
              i_ct == INPUT_CONTROL_SET_BOOKMARK ) )
        {
            continue;
        }
        else
        {
            /* TODO but that's not that important
                - merge SET_X with SET_X_CMD
                - ignore SET_SEEKPOINT/SET_POSITION/SET_TIME before a SET_TITLE
                - ignore SET_SEEKPOINT/SET_POSITION/SET_TIME before another among them
                - ?
                */
            break;
        }
    }
    return i - 1;
}


static inline int ControlPop( input_thread_t *p_input,
                              int *pi_type, vlc_value_t *p_val,
                              mtime_t i_deadline, bool b_postpone_seek )
{
    input_thread_private_t *p_sys = input_priv(p_input);

    vlc_mutex_lock( &p_sys->lock_control );
    while( p_sys->i_control <= 0 ||
           ( b_postpone_seek && ControlIsSeekRequest( p_sys->control[0].i_type ) ) )
    {
        if( p_sys->is_stopped )
        {
            vlc_mutex_unlock( &p_sys->lock_control );
            return VLC_EGENERIC;
        }

        if( i_deadline >= 0 )
        {
            if( vlc_cond_timedwait( &p_sys->wait_control, &p_sys->lock_control,
                                    i_deadline ) )
            {
                vlc_mutex_unlock( &p_sys->lock_control );
                return VLC_EGENERIC;
            }
        }
        else
            vlc_cond_wait( &p_sys->wait_control, &p_sys->lock_control );
    }

    /* */
    const int i_index = ControlGetReducedIndexLocked( p_input );

    for( int i = 0; i < i_index; ++i )
    {
        /* Release Reduced controls */
        ControlRelease( p_sys->control[i].i_type, p_sys->control[i].val );
    }

    /* */
    *pi_type = p_sys->control[i_index].i_type;
    *p_val   = p_sys->control[i_index].val;

    p_sys->i_control -= i_index + 1;
    if( p_sys->i_control > 0 )
        memmove( &p_sys->control[0], &p_sys->control[i_index+1],
                 sizeof(*p_sys->control) * p_sys->i_control );
    vlc_mutex_unlock( &p_sys->lock_control );

    return VLC_SUCCESS;
}
static bool ControlIsSeekRequest( int i_type )
{
    switch( i_type )
    {
    case INPUT_CONTROL_SET_POSITION:
    case INPUT_CONTROL_SET_TIME:
    case INPUT_CONTROL_SET_TITLE:
    case INPUT_CONTROL_SET_TITLE_NEXT:
    case INPUT_CONTROL_SET_TITLE_PREV:
    case INPUT_CONTROL_SET_SEEKPOINT:
    case INPUT_CONTROL_SET_SEEKPOINT_NEXT:
    case INPUT_CONTROL_SET_SEEKPOINT_PREV:
    case INPUT_CONTROL_SET_BOOKMARK:
    case INPUT_CONTROL_NAV_ACTIVATE:
    case INPUT_CONTROL_NAV_UP:
    case INPUT_CONTROL_NAV_DOWN:
    case INPUT_CONTROL_NAV_LEFT:
    case INPUT_CONTROL_NAV_RIGHT:
    case INPUT_CONTROL_NAV_POPUP:
    case INPUT_CONTROL_NAV_MENU:
        return true;
    default:
        return false;
    }
}

static void ControlRelease( int i_type, vlc_value_t val )
{
    switch( i_type )
    {
    case INPUT_CONTROL_ADD_SLAVE:
        if( val.p_address )
            input_item_slave_Delete( val.p_address );
        break;
    case INPUT_CONTROL_SET_VIEWPOINT:
    case INPUT_CONTROL_SET_INITIAL_VIEWPOINT:
    case INPUT_CONTROL_UPDATE_VIEWPOINT:
        free( val.p_address );
        break;
    case INPUT_CONTROL_SET_RENDERER:
        if( val.p_address )
            vlc_renderer_item_release( val.p_address );
        break;

    default:
        break;
    }
}

/* Pause input */
static void ControlPause( input_thread_t *p_input, mtime_t i_control_date )
{
    int i_state = PAUSE_S;

    if( input_priv(p_input)->b_can_pause )
    {
        demux_t *p_demux = input_priv(p_input)->master->p_demux;

        if( demux_Control( p_demux, DEMUX_SET_PAUSE_STATE, true ) )
        {
            msg_Warn( p_input, "cannot set pause state" );
            return;
        }
    }

    /* */
    if( es_out_SetPauseState( input_priv(p_input)->p_es_out, input_priv(p_input)->b_can_pause,
                              true, i_control_date ) )
    {
        msg_Warn( p_input, "cannot set pause state at es_out level" );
        return;
    }

    /* Switch to new state */
    input_ChangeState( p_input, i_state );
}

static void ControlUnpause( input_thread_t *p_input, mtime_t i_control_date )
{
    if( input_priv(p_input)->b_can_pause )
    {
        demux_t *p_demux = input_priv(p_input)->master->p_demux;

        if( demux_Control( p_demux, DEMUX_SET_PAUSE_STATE, false ) )
        {
            msg_Err( p_input, "cannot resume" );
            input_ChangeState( p_input, ERROR_S );
            return;
        }
    }

    /* Switch to play */
    input_ChangeState( p_input, PLAYING_S );
    es_out_SetPauseState( input_priv(p_input)->p_es_out, false, false, i_control_date );
}

static void ViewpointApply( input_thread_t *p_input )
{
    input_thread_private_t *priv = input_priv(p_input);

    vlc_viewpoint_clip( &priv->viewpoint );

    vout_thread_t **pp_vout;
    size_t i_vout;
    input_resource_HoldVouts( priv->p_resource, &pp_vout, &i_vout );

    for( size_t i = 0; i < i_vout; ++i )
    {
        var_SetAddress( pp_vout[i], "viewpoint", &priv->viewpoint );
        /* This variable can only be read from callbacks */
        var_Change( pp_vout[i], "viewpoint", VLC_VAR_SETVALUE,
                    &(vlc_value_t) { .p_address = NULL }, NULL );
        vlc_object_release( pp_vout[i] );
    }
    free( pp_vout );

    audio_output_t *p_aout = input_resource_HoldAout( priv->p_resource );
    if( p_aout )
    {

        var_SetAddress( p_aout, "viewpoint", &priv->viewpoint );
        /* This variable can only be read from callbacks */
        var_Change( p_aout, "viewpoint", VLC_VAR_SETVALUE,
                    &(vlc_value_t) { .p_address = NULL }, NULL );
        vlc_object_release( p_aout );
    }
}

/* XXX: The two following functions are copied from the hotkeys module. The OSD
 * handling need to be handled by the core, in the future input manager for
 * example. */
static void ControlNavDisplayVolume( vout_thread_t *p_vout, float vol )
{
    vout_FlushSubpictureChannel( p_vout, VOUT_SPU_CHANNEL_OSD );
    vout_OSDMessage( p_vout, VOUT_SPU_CHANNEL_OSD, _( "Volume %ld%%" ),
                     lroundf( vol * 100.f ) );
}

static void ControlNavDisplayPosition( vout_thread_t *p_vout,
                                       input_thread_t *p_input )
{
    vout_FlushSubpictureChannel( p_vout, VOUT_SPU_CHANNEL_OSD );

    int64_t t = var_GetInteger( p_input, "time" ) / CLOCK_FREQ;
    int64_t l = var_GetInteger( p_input, "length" ) / CLOCK_FREQ;

    char psz_time[MSTRTIME_MAX_SIZE];
    secstotimestr( psz_time, t );

    if( l > 0 )
    {
        char psz_duration[MSTRTIME_MAX_SIZE];

        secstotimestr( psz_duration, l );
        vout_OSDMessage( p_vout, VOUT_SPU_CHANNEL_OSD,
                         "%s / %s", psz_time, psz_duration );
    }
    else if( t > 0 )
    {
        vout_OSDMessage( p_vout, VOUT_SPU_CHANNEL_OSD, "%s", psz_time );
    }
}

static void ControlNav( input_thread_t *p_input, int i_type )
{
    input_thread_private_t *priv = input_priv(p_input);

    if( !demux_Control( priv->master->p_demux, i_type
                        - INPUT_CONTROL_NAV_ACTIVATE + DEMUX_NAV_ACTIVATE ) )
        return; /* The demux handled the navigation control */

    /* Handle Up/Down/Left/Right if the demux can't navigate */
    vlc_viewpoint_t vp = {};
    int vol_direction = 0;
    int seek_direction = 0;
    switch( i_type )
    {
        case INPUT_CONTROL_NAV_UP:
            vol_direction = 1;
            vp.pitch = -1.f;
            break;
        case INPUT_CONTROL_NAV_DOWN:
            vol_direction = -1;
            vp.pitch = 1.f;
            break;
        case INPUT_CONTROL_NAV_LEFT:
            seek_direction = -1;
            vp.yaw = -1.f;
            break;
        case INPUT_CONTROL_NAV_RIGHT:
            seek_direction = 1;
            vp.yaw = 1.f;
            break;
        case INPUT_CONTROL_NAV_ACTIVATE:
        case INPUT_CONTROL_NAV_POPUP:
        case INPUT_CONTROL_NAV_MENU:
            return;
        default:
            vlc_assert_unreachable();
    }

    /* Try to change the viewpoint if possible */
    vout_thread_t **pp_vout;
    size_t i_vout;
    bool b_viewpoint_ch = false;
    input_resource_HoldVouts( priv->p_resource, &pp_vout, &i_vout );
    for( size_t i = 0; i < i_vout; ++i )
    {
        if( !b_viewpoint_ch
         && var_GetBool( pp_vout[i], "viewpoint-changeable" ) )
            b_viewpoint_ch = true;
    }

    if( b_viewpoint_ch )
    {
        priv->viewpoint_changed = true;
        priv->viewpoint.yaw   += vp.yaw;
        priv->viewpoint.pitch += vp.pitch;
        priv->viewpoint.roll  += vp.roll;
        priv->viewpoint.fov   += vp.fov;
        ViewpointApply( p_input );
        goto clean;
    }

    /* Seek or change volume if the input doesn't have navigation or viewpoint */
    if( seek_direction != 0 )
    {
        mtime_t it = var_InheritInteger( p_input, "short-jump-size" );
        var_SetInteger( p_input, "time-offset", it * seek_direction * CLOCK_FREQ );
        if( i_vout > 0 )
            ControlNavDisplayPosition( pp_vout[0], p_input );
    }
    else
    {
        assert( vol_direction != 0 );
        audio_output_t *p_aout = input_resource_HoldAout( priv->p_resource );
        if( p_aout )
        {
            float new_vol;
            aout_VolumeUpdate( p_aout, vol_direction, &new_vol );
            vlc_object_release( p_aout );
            if( i_vout > 0 )
                ControlNavDisplayVolume( pp_vout[0], new_vol );
        }
    }

clean:
    for( size_t i = 0; i < i_vout; ++i )
        vlc_object_release( pp_vout[i] );
    free( pp_vout );
}

#ifdef ENABLE_SOUT
static void ControlUpdateRenderer( input_thread_t *p_input, bool b_enable )
{
    if( b_enable )
    {
        if( InitSout( p_input ) != VLC_SUCCESS )
        {
            msg_Err( p_input, "Failed to start sout" );
            return;
        }
    }
    else
    {
        input_resource_RequestSout( input_priv(p_input)->p_resource,
                                    input_priv(p_input)->p_sout, NULL );
        input_priv(p_input)->p_sout = NULL;
    }
}
#endif

static void ControlInsertDemuxFilter( input_thread_t* p_input, const char* psz_demux_chain )
{
    input_source_t *p_inputSource = input_priv(p_input)->master;
    demux_t *p_filtered_demux = demux_FilterChainNew( p_inputSource->p_demux, psz_demux_chain );
    if ( p_filtered_demux != NULL )
        p_inputSource->p_demux = p_filtered_demux;
    else if ( psz_demux_chain != NULL )
        msg_Dbg(p_input, "Failed to create demux filter %s", psz_demux_chain);
}

static bool Control( input_thread_t *p_input,
                     int i_type, vlc_value_t val )
{
    const mtime_t i_control_date = mdate();
    /* FIXME b_force_update is abused, it should be carefully checked */
    bool b_force_update = false;

    if( !p_input )
        return b_force_update;

    switch( i_type )
    {
        case INPUT_CONTROL_SET_POSITION:
        {
            if( input_priv(p_input)->b_recording )
            {
                msg_Err( p_input, "INPUT_CONTROL_SET_POSITION ignored while recording" );
                break;
            }

            float f_pos = val.f_float;
            if( f_pos < 0.f )
                f_pos = 0.f;
            else if( f_pos > 1.f )
                f_pos = 1.f;
            /* Reset the decoders states and clock sync (before calling the demuxer */
            es_out_SetTime( input_priv(p_input)->p_es_out, -1 );
            if( demux_Control( input_priv(p_input)->master->p_demux, DEMUX_SET_POSITION,
                               (double) f_pos, !input_priv(p_input)->b_fast_seek ) )
            {
                msg_Err( p_input, "INPUT_CONTROL_SET_POSITION "
                         "%2.1f%% failed", (double)(f_pos * 100.f) );
            }
            else
            {
                if( input_priv(p_input)->i_slave > 0 )
                    SlaveSeek( p_input );
                input_priv(p_input)->master->b_eof = false;

                b_force_update = true;
            }
            break;
        }

        case INPUT_CONTROL_SET_TIME:
        {
            int64_t i_time;
            int i_ret;

            if( input_priv(p_input)->b_recording )
            {
                msg_Err( p_input, "INPUT_CONTROL_SET_TIME ignored while recording" );
                break;
            }

            i_time = val.i_int;
            if( i_time < 0 )
                i_time = 0;

            /* Reset the decoders states and clock sync (before calling the demuxer */
            es_out_SetTime( input_priv(p_input)->p_es_out, -1 );

            i_ret = demux_Control( input_priv(p_input)->master->p_demux,
                                   DEMUX_SET_TIME, i_time,
                                   !input_priv(p_input)->b_fast_seek );
            if( i_ret )
            {
                int64_t i_length;

                /* Emulate it with a SET_POS */
                if( !demux_Control( input_priv(p_input)->master->p_demux,
                                    DEMUX_GET_LENGTH, &i_length ) && i_length > 0 )
                {
                    double f_pos = (double)i_time / (double)i_length;
                    f_pos = VLC_CLIP(f_pos, 0.0, 1.0);
                    i_ret = demux_Control( input_priv(p_input)->master->p_demux,
                                            DEMUX_SET_POSITION, f_pos,
                                            !input_priv(p_input)->b_fast_seek );
                }
            }
            if( i_ret )
            {
                msg_Warn( p_input, "INPUT_CONTROL_SET_TIME %"PRId64
                         " failed or not possible", i_time );
            }
            else
            {
                if( input_priv(p_input)->i_slave > 0 )
                    SlaveSeek( p_input );
                input_priv(p_input)->master->b_eof = false;

                b_force_update = true;
            }
            break;
        }

        case INPUT_CONTROL_SET_STATE:
            switch( val.i_int )
            {
                case PLAYING_S:
                    if( input_priv(p_input)->i_state == PAUSE_S )
                    {
                        ControlUnpause( p_input, i_control_date );
                        b_force_update = true;
                    }
                    break;
                case PAUSE_S:
                    if( input_priv(p_input)->i_state == PLAYING_S )
                    {
                        ControlPause( p_input, i_control_date );
                        b_force_update = true;
                    }
                    break;
                default:
                    msg_Err( p_input, "invalid INPUT_CONTROL_SET_STATE" );
            }
            break;

        case INPUT_CONTROL_SET_RATE:
        {
            /* Get rate and direction */
            long long i_rate = llabs( val.i_int );
            int i_rate_sign = val.i_int < 0 ? -1 : 1;

            /* Check rate bound */
            if( i_rate < INPUT_RATE_MIN )
            {
                msg_Dbg( p_input, "cannot set rate faster" );
                i_rate = INPUT_RATE_MIN;
            }
            else if( i_rate > INPUT_RATE_MAX )
            {
                msg_Dbg( p_input, "cannot set rate slower" );
                i_rate = INPUT_RATE_MAX;
            }

            /* Apply direction */
            if( i_rate_sign < 0 )
            {
                if( input_priv(p_input)->master->b_rescale_ts )
                {
                    msg_Dbg( p_input, "cannot set negative rate" );
                    i_rate = input_priv(p_input)->i_rate;
                    assert( i_rate > 0 );
                }
                else
                {
                    i_rate *= i_rate_sign;
                }
            }

            if( i_rate != INPUT_RATE_DEFAULT &&
                ( ( !input_priv(p_input)->b_can_rate_control && !input_priv(p_input)->master->b_rescale_ts ) ||
                  ( input_priv(p_input)->p_sout && !input_priv(p_input)->b_out_pace_control ) ) )
            {
                msg_Dbg( p_input, "cannot change rate" );
                i_rate = INPUT_RATE_DEFAULT;
            }
            if( i_rate != input_priv(p_input)->i_rate &&
                !input_priv(p_input)->b_can_pace_control && input_priv(p_input)->b_can_rate_control )
            {
                if( !input_priv(p_input)->master->b_rescale_ts )
                    es_out_Control( input_priv(p_input)->p_es_out, ES_OUT_RESET_PCR );

                if( demux_Control( input_priv(p_input)->master->p_demux, DEMUX_SET_RATE,
                                   &i_rate ) )
                {
                    msg_Warn( p_input, "ACCESS/DEMUX_SET_RATE failed" );
                    i_rate = input_priv(p_input)->i_rate;
                }
            }

            /* */
            if( i_rate != input_priv(p_input)->i_rate )
            {
                input_priv(p_input)->i_rate = i_rate;
                input_SendEventRate( p_input, i_rate );

                if( input_priv(p_input)->master->b_rescale_ts )
                {
                    const int i_rate_source = (input_priv(p_input)->b_can_pace_control || input_priv(p_input)->b_can_rate_control ) ? i_rate : INPUT_RATE_DEFAULT;
                    es_out_SetRate( input_priv(p_input)->p_es_out, i_rate_source, i_rate );
                }

                b_force_update = true;
            }
            break;
        }

        case INPUT_CONTROL_SET_PROGRAM:
            /* No need to force update, es_out does it if needed */
            es_out_Control( input_priv(p_input)->p_es_out,
                            ES_OUT_SET_GROUP, val.i_int );

            demux_Control( input_priv(p_input)->master->p_demux, DEMUX_SET_GROUP, val.i_int,
                            NULL );
            break;

        case INPUT_CONTROL_SET_ES:
            /* No need to force update, es_out does it if needed */
            es_out_Control( input_priv(p_input)->p_es_out_display,
                            ES_OUT_SET_ES_BY_ID, (int)val.i_int );

            demux_Control( input_priv(p_input)->master->p_demux, DEMUX_SET_ES, (int)val.i_int );
            break;

        case INPUT_CONTROL_RESTART_ES:
            es_out_Control( input_priv(p_input)->p_es_out_display,
                            ES_OUT_RESTART_ES_BY_ID, (int)val.i_int );
            break;

        case INPUT_CONTROL_SET_VIEWPOINT:
        case INPUT_CONTROL_SET_INITIAL_VIEWPOINT:
        case INPUT_CONTROL_UPDATE_VIEWPOINT:
        {
            input_thread_private_t *priv = input_priv(p_input);
            const vlc_viewpoint_t *p_vp = val.p_address;

            if ( i_type == INPUT_CONTROL_SET_INITIAL_VIEWPOINT )
            {

                /* Set the initial viewpoint if it had not been changed by the
                 * user. */
                if( !priv->viewpoint_changed )
                    priv->viewpoint = *p_vp;
                /* Update viewpoints of aout and every vouts in all cases. */
            }
            else if ( i_type == INPUT_CONTROL_SET_VIEWPOINT)
            {
                priv->viewpoint_changed = true;
                priv->viewpoint = *p_vp;
            }
            else
            {
                priv->viewpoint_changed = true;
                priv->viewpoint.yaw   += p_vp->yaw;
                priv->viewpoint.pitch += p_vp->pitch;
                priv->viewpoint.roll  += p_vp->roll;
                priv->viewpoint.fov   += p_vp->fov;
            }

            ViewpointApply( p_input );
            break;
        }

        case INPUT_CONTROL_SET_AUDIO_DELAY:
            input_SendEventAudioDelay( p_input, val.i_int );
            UpdatePtsDelay( p_input );
            break;

        case INPUT_CONTROL_SET_SPU_DELAY:
            input_SendEventSubtitleDelay( p_input, val.i_int );
            UpdatePtsDelay( p_input );
            break;

        case INPUT_CONTROL_SET_TITLE:
        case INPUT_CONTROL_SET_TITLE_NEXT:
        case INPUT_CONTROL_SET_TITLE_PREV:
        {
            if( input_priv(p_input)->b_recording )
            {
                msg_Err( p_input, "INPUT_CONTROL_SET_TITLE(*) ignored while recording" );
                break;
            }
            if( input_priv(p_input)->master->i_title <= 0 )
                break;

            int i_title = demux_GetTitle( input_priv(p_input)->master->p_demux );
            if( i_type == INPUT_CONTROL_SET_TITLE_PREV )
                i_title--;
            else if( i_type == INPUT_CONTROL_SET_TITLE_NEXT )
                i_title++;
            else
                i_title = val.i_int;
            if( i_title < 0 || i_title >= input_priv(p_input)->master->i_title )
                break;

            es_out_SetTime( input_priv(p_input)->p_es_out, -1 );
            demux_Control( input_priv(p_input)->master->p_demux,
                           DEMUX_SET_TITLE, i_title );
            input_SendEventTitle( p_input, i_title );
            break;
        }
        case INPUT_CONTROL_SET_SEEKPOINT:
        case INPUT_CONTROL_SET_SEEKPOINT_NEXT:
        case INPUT_CONTROL_SET_SEEKPOINT_PREV:
        {
            if( input_priv(p_input)->b_recording )
            {
                msg_Err( p_input, "INPUT_CONTROL_SET_SEEKPOINT(*) ignored while recording" );
                break;
            }
            if( input_priv(p_input)->master->i_title <= 0 )
                break;

            demux_t *p_demux = input_priv(p_input)->master->p_demux;

            int i_title = demux_GetTitle( p_demux );
            int i_seekpoint = demux_GetSeekpoint( p_demux );

            if( i_type == INPUT_CONTROL_SET_SEEKPOINT_PREV )
            {
                int64_t i_seekpoint_time = input_priv(p_input)->master->title[i_title]->seekpoint[i_seekpoint]->i_time_offset;
                int64_t i_input_time = var_GetInteger( p_input, "time" );
                if( i_seekpoint_time >= 0 && i_input_time >= 0 )
                {
                    if( i_input_time < i_seekpoint_time + 3000000 )
                        i_seekpoint--;
                }
                else
                    i_seekpoint--;
            }
            else if( i_type == INPUT_CONTROL_SET_SEEKPOINT_NEXT )
                i_seekpoint++;
            else
                i_seekpoint = val.i_int;
            if( i_seekpoint < 0
             || i_seekpoint >= input_priv(p_input)->master->title[i_title]->i_seekpoint )
                break;

            es_out_SetTime( input_priv(p_input)->p_es_out, -1 );
            demux_Control( input_priv(p_input)->master->p_demux,
                           DEMUX_SET_SEEKPOINT, i_seekpoint );
            input_SendEventSeekpoint( p_input, i_title, i_seekpoint );
            if( input_priv(p_input)->i_slave > 0 )
                SlaveSeek( p_input );
            break;
        }

        case INPUT_CONTROL_ADD_SLAVE:
            if( val.p_address )
            {
                input_item_slave_t *p_item_slave  = val.p_address;
                unsigned i_flags = SLAVE_ADD_CANFAIL | SLAVE_ADD_SET_TIME;
                if( p_item_slave->b_forced )
                    i_flags |= SLAVE_ADD_FORCED;

                if( input_SlaveSourceAdd( p_input, p_item_slave->i_type,
                                          p_item_slave->psz_uri, i_flags )
                                          == VLC_SUCCESS )
                {
                    /* Update item slaves */
                    input_item_AddSlave( input_priv(p_input)->p_item, p_item_slave );
                    /* The slave is now owned by the item */
                    val.p_address = NULL;
                }
            }
            break;

        case INPUT_CONTROL_SET_RECORD_STATE:
            if( !!input_priv(p_input)->b_recording != !!val.b_bool )
            {
                if( input_priv(p_input)->master->b_can_stream_record )
                {
                    if( demux_Control( input_priv(p_input)->master->p_demux,
                                       DEMUX_SET_RECORD_STATE, val.b_bool ) )
                        val.b_bool = false;
                }
                else
                {
                    if( es_out_SetRecordState( input_priv(p_input)->p_es_out_display, val.b_bool ) )
                        val.b_bool = false;
                }
                input_priv(p_input)->b_recording = val.b_bool;

                input_SendEventRecord( p_input, val.b_bool );

                b_force_update = true;
            }
            break;

        case INPUT_CONTROL_SET_FRAME_NEXT:
            if( input_priv(p_input)->i_state == PAUSE_S )
            {
                es_out_SetFrameNext( input_priv(p_input)->p_es_out );
            }
            else if( input_priv(p_input)->i_state == PLAYING_S )
            {
                ControlPause( p_input, i_control_date );
            }
            else
            {
                msg_Err( p_input, "invalid state for frame next" );
            }
            b_force_update = true;
            break;

        case INPUT_CONTROL_SET_BOOKMARK:
        {
            mtime_t time_offset = -1;

            vlc_mutex_lock( &input_priv(p_input)->p_item->lock );
            if( val.i_int >= 0 && val.i_int < input_priv(p_input)->i_bookmark )
            {
                const seekpoint_t *p_bookmark = input_priv(p_input)->pp_bookmark[val.i_int];
                time_offset = p_bookmark->i_time_offset;
            }
            vlc_mutex_unlock( &input_priv(p_input)->p_item->lock );

            if( time_offset < 0 )
            {
                msg_Err( p_input, "invalid bookmark %"PRId64, val.i_int );
                break;
            }

            val.i_int = time_offset;
            b_force_update = Control( p_input, INPUT_CONTROL_SET_TIME, val );
            break;
        }
        case INPUT_CONTROL_SET_RENDERER:
        {
#ifdef ENABLE_SOUT
            vlc_renderer_item_t *p_item = val.p_address;
            input_thread_private_t *p_priv = input_priv( p_input );
            // We do not support switching from a renderer to another for now
            if ( p_item == NULL && p_priv->p_renderer == NULL )
                break;

            void *context;
            if( es_out_Control( input_priv(p_input)->p_es_out_display,
                                ES_OUT_STOP_ALL_ES, &context ) != VLC_SUCCESS )
                break;

            if ( p_priv->p_renderer )
            {
                ControlUpdateRenderer( p_input, false );
                demux_FilterDisable( p_priv->master->p_demux,
                        vlc_renderer_item_demux_filter( p_priv->p_renderer ) );
                vlc_renderer_item_release( p_priv->p_renderer );
                p_priv->p_renderer = NULL;
            }
            if( p_item != NULL )
            {
                p_priv->p_renderer = vlc_renderer_item_hold( p_item );
                ControlUpdateRenderer( p_input, true );
                if( !demux_FilterEnable( p_priv->master->p_demux,
                                vlc_renderer_item_demux_filter( p_priv->p_renderer ) ) )
                {
                    ControlInsertDemuxFilter( p_input,
                                        vlc_renderer_item_demux_filter( p_item ) );
                }
                input_resource_TerminateVout( p_priv->p_resource );
            }
            es_out_Control( input_priv(p_input)->p_es_out_display, ES_OUT_START_ALL_ES,
                            context );
#endif
            break;
        }

        case INPUT_CONTROL_NAV_ACTIVATE:
        case INPUT_CONTROL_NAV_UP:
        case INPUT_CONTROL_NAV_DOWN:
        case INPUT_CONTROL_NAV_LEFT:
        case INPUT_CONTROL_NAV_RIGHT:
        case INPUT_CONTROL_NAV_POPUP:
        case INPUT_CONTROL_NAV_MENU:
            ControlNav( p_input, i_type );
            break;

        default:
            msg_Err( p_input, "not yet implemented" );
            break;
    }

    ControlRelease( i_type, val );
    return b_force_update;
}

/*****************************************************************************
 * UpdateTitleSeekpoint
 *****************************************************************************/
static int UpdateTitleSeekpoint( input_thread_t *p_input,
                                 int i_title, int i_seekpoint )
{
    int i_title_end = input_priv(p_input)->master->i_title_end -
                        input_priv(p_input)->master->i_title_offset;
    int i_seekpoint_end = input_priv(p_input)->master->i_seekpoint_end -
                            input_priv(p_input)->master->i_seekpoint_offset;

    if( i_title_end >= 0 && i_seekpoint_end >= 0 )
    {
        if( i_title > i_title_end ||
            ( i_title == i_title_end && i_seekpoint > i_seekpoint_end ) )
            return VLC_DEMUXER_EOF;
    }
    else if( i_seekpoint_end >= 0 )
    {
        if( i_seekpoint > i_seekpoint_end )
            return VLC_DEMUXER_EOF;
    }
    else if( i_title_end >= 0 )
    {
        if( i_title > i_title_end )
            return VLC_DEMUXER_EOF;
    }
    return VLC_DEMUXER_SUCCESS;
}
/*****************************************************************************
 * Update*FromDemux:
 *****************************************************************************/
static int UpdateTitleSeekpointFromDemux( input_thread_t *p_input )
{
    demux_t *p_demux = input_priv(p_input)->master->p_demux;

    /* TODO event-like */
    if( demux_TestAndClearFlags( p_demux, INPUT_UPDATE_TITLE ) )
        input_SendEventTitle( p_input, demux_GetTitle( p_demux ) );

    if( demux_TestAndClearFlags( p_demux, INPUT_UPDATE_SEEKPOINT ) )
        input_SendEventSeekpoint( p_input, demux_GetTitle( p_demux ),
                                  demux_GetSeekpoint( p_demux ) );

    return UpdateTitleSeekpoint( p_input,
                                 demux_GetTitle( p_demux ),
                                 demux_GetSeekpoint( p_demux ) );
}

static void UpdateGenericFromDemux( input_thread_t *p_input )
{
    demux_t *p_demux = input_priv(p_input)->master->p_demux;

    if( demux_TestAndClearFlags( p_demux, INPUT_UPDATE_META ) )
        InputUpdateMeta( p_input, p_demux );

    {
        double quality;
        double strength;

        if( !demux_Control( p_demux, DEMUX_GET_SIGNAL, &quality, &strength ) )
            input_SendEventSignal( p_input, quality, strength );
    }
}

static void UpdateTitleListfromDemux( input_thread_t *p_input )
{
    input_thread_private_t *priv = input_priv(p_input);
    input_source_t *in = priv->master;

    vlc_mutex_lock( &priv->p_item->lock );
    /* Delete the preexisting titles */
    if( in->i_title > 0 )
    {
        for( int i = 0; i < in->i_title; i++ )
            vlc_input_title_Delete( in->title[i] );
        TAB_CLEAN( in->i_title, in->title );
        priv->i_title = 0;
        priv->title = NULL;
        in->b_title_demux = false;
    }

    /* Get the new title list */
    if( demux_Control( in->p_demux, DEMUX_GET_TITLE_INFO,
                       &in->title, &in->i_title,
                       &in->i_title_offset, &in->i_seekpoint_offset ) )
        TAB_INIT( in->i_title, in->title );
    else
        in->b_title_demux = true;

    vlc_mutex_unlock( &priv->p_item->lock );

    InitTitle( p_input );
}

static int
InputStreamHandleAnchor( input_source_t *source, stream_t **stream,
                         char const *anchor )
{
    char const* extra;
    if( stream_extractor_AttachParsed( stream, anchor, &extra ) )
    {
        msg_Err( source, "unable to attach stream-extractors for %s",
            (*stream)->psz_url );

        return VLC_EGENERIC;
    }

    if( vlc_stream_directory_Attach( stream, NULL ) )
        msg_Dbg( source, "attachment of directory-extractor failed for %s",
            (*stream)->psz_url );

    MRLSections( extra ? extra : "",
        &source->i_title_start, &source->i_title_end,
        &source->i_seekpoint_start, &source->i_seekpoint_end );

    return VLC_SUCCESS;
}

static demux_t *InputDemuxNew( input_thread_t *p_input, input_source_t *p_source,
                               const char *psz_access, const char *psz_demux,
                               const char *psz_path, const char *psz_anchor )
{
    input_thread_private_t *priv = input_priv(p_input );
    demux_t *p_demux = NULL;

    /* first, try to create an access demux */
    p_demux = demux_NewAdvanced( VLC_OBJECT( p_source ), p_input,
                                 psz_access, psz_demux, psz_path,
                                 NULL, priv->p_es_out, priv->b_preparsing );
    if( p_demux )
    {
        MRLSections( psz_anchor,
            &p_source->i_title_start, &p_source->i_title_end,
            &p_source->i_seekpoint_start, &p_source->i_seekpoint_end );

        return p_demux;
    }

    /* not an access-demux: create the underlying access stream */
    char *psz_base_mrl;

    if( asprintf( &psz_base_mrl, "%s://%s", psz_access, psz_path ) < 0 )
        return NULL;

    char *psz_filters = var_InheritString( p_source, "stream-filter" );
    stream_t* p_stream = stream_AccessNew( VLC_OBJECT( p_source ), p_input,
                                           priv->b_preparsing,
                                           psz_base_mrl );
    FREENULL( psz_base_mrl );

    if( p_stream == NULL )
        goto error;

    /* attach explicit stream filters to stream */
    if( psz_filters )
        p_stream = stream_FilterChainNew( p_stream, psz_filters );

    FREENULL( psz_filters );

    /* handle anchors */
    if( InputStreamHandleAnchor( p_source, &p_stream, psz_anchor ) )
        goto error;

    /* attach conditional record stream-filter */
    if( var_InheritBool( p_source, "input-record-native" ) )
        p_stream = stream_FilterChainNew( p_stream, "record" );

    /* create a regular demux with the access stream created */
    p_demux = demux_NewAdvanced( VLC_OBJECT( p_source ), p_input,
                                 psz_access, psz_demux, psz_path,
                                 p_stream, priv->p_es_out,
                                 priv->b_preparsing );
    if( p_demux )
        return p_demux;

error:
    free( psz_base_mrl );
    free( psz_filters );

    if( p_stream )
        vlc_stream_Delete( p_stream );

    return NULL;
}

/*****************************************************************************
 * InputSourceNew:
 *****************************************************************************/
static input_source_t *InputSourceNew( input_thread_t *p_input,
                                       const char *psz_mrl,
                                       const char *psz_forced_demux,
                                       bool b_in_can_fail )
{
    input_thread_private_t *priv = input_priv(p_input);
    input_source_t *in = vlc_custom_create( p_input, sizeof( *in ),
                                            "input source" );
    if( unlikely(in == NULL) )
        return NULL;

    const char *psz_access, *psz_demux, *psz_path, *psz_anchor = NULL;

    assert( psz_mrl );
    char *psz_dup = strdup( psz_mrl );
    char *psz_demux_var = NULL;

    if( psz_dup == NULL )
    {
        vlc_object_release( in );
        return NULL;
    }

    /* Split uri */
    input_SplitMRL( &psz_access, &psz_demux, &psz_path, &psz_anchor, psz_dup );

    if( psz_demux == NULL || psz_demux[0] == '\0' )
        psz_demux = psz_demux_var = var_InheritString( in, "demux" );

    if( psz_forced_demux != NULL )
        psz_demux = psz_forced_demux;

    if( psz_demux == NULL )
        psz_demux = "any";

    msg_Dbg( p_input, "`%s' gives access `%s' demux `%s' path `%s'",
             psz_mrl, psz_access, psz_demux, psz_path );

    if( input_priv(p_input)->master == NULL /* XXX ugly */)
    {   /* On master stream only, use input-list */
        char *str = var_InheritString( p_input, "input-list" );
        if( str != NULL )
        {
            char *list;

            var_Create( p_input, "concat-list", VLC_VAR_STRING );
            if( likely(asprintf( &list, "%s://%s,%s", psz_access, psz_path,
                                 str ) >= 0) )
            {
                 var_SetString( p_input, "concat-list", list );
                 free( list );
            }
            free( str );
            psz_access = "concat";
        }
    }

    if( strcasecmp( psz_access, "concat" ) )
    {   /* Autodetect extra files if none specified */
        int count;
        char **tab;

        TAB_INIT( count, tab );
        InputGetExtraFiles( p_input, &count, &tab, &psz_access, psz_path );
        if( count > 0 )
        {
            char *list = NULL;

            for( int i = 0; i < count; i++ )
            {
                char *str;
                if( asprintf( &str, "%s,%s", list ? list : psz_mrl,
                              tab[i] ) < 0 )
                    break;

                free( tab[i] );
                free( list );
                list = str;
            }

            var_Create( p_input, "concat-list", VLC_VAR_STRING );
            if( likely(list != NULL) )
            {
                var_SetString( p_input, "concat-list", list );
                free( list );
            }
        }
        TAB_CLEAN( count, tab );
    }

    in->p_demux = InputDemuxNew( p_input, in, psz_access, psz_demux,
                                 psz_path, psz_anchor );

    free( psz_demux_var );
    free( psz_dup );

    if( in->p_demux == NULL )
    {
        if( !b_in_can_fail && !input_Stopped( p_input ) )
            vlc_dialog_display_error( p_input, _("Your input can't be opened"),
                                      _("VLC is unable to open the MRL '%s'."
                                      " Check the log for details."), psz_mrl );
        vlc_object_release( in );
        return NULL;
    }

    char *psz_demux_chain = NULL;
    if( priv->p_renderer )
    {
        const char* psz_renderer_demux = vlc_renderer_item_demux_filter(
                    priv->p_renderer );
        if( psz_renderer_demux )
            psz_demux_chain = strdup( psz_renderer_demux );
    }
    if( !psz_demux_chain )
        psz_demux_chain = var_GetNonEmptyString(p_input, "demux-filter");
    if( psz_demux_chain != NULL ) /* add the chain of demux filters */
    {
        in->p_demux = demux_FilterChainNew( in->p_demux, psz_demux_chain );
        free( psz_demux_chain );

        if( in->p_demux == NULL )
        {
            msg_Err(p_input, "Failed to create demux filter");
            vlc_object_release( in );
            return NULL;
        }
    }

    /* Get infos from (access_)demux */
    bool b_can_seek;
    if( demux_Control( in->p_demux, DEMUX_CAN_SEEK, &b_can_seek ) )
        b_can_seek = false;
    var_SetBool( p_input, "can-seek", b_can_seek );

    if( demux_Control( in->p_demux, DEMUX_CAN_CONTROL_PACE,
                       &in->b_can_pace_control ) )
        in->b_can_pace_control = false;

    assert( in->p_demux->pf_demux != NULL || !in->b_can_pace_control );

    if( !in->b_can_pace_control )
    {
        if( demux_Control( in->p_demux, DEMUX_CAN_CONTROL_RATE,
                           &in->b_can_rate_control ) )
        {
            in->b_can_rate_control = false;
            in->b_rescale_ts = true;
        }
        else
            in->b_rescale_ts = !in->b_can_rate_control;
    }
    else
    {
        in->b_can_rate_control = true;
        in->b_rescale_ts = true;
    }

    demux_Control( in->p_demux, DEMUX_CAN_PAUSE, &in->b_can_pause );

    var_SetBool( p_input, "can-pause", in->b_can_pause || !in->b_can_pace_control ); /* XXX temporary because of es_out_timeshift*/
    var_SetBool( p_input, "can-rate", !in->b_can_pace_control || in->b_can_rate_control ); /* XXX temporary because of es_out_timeshift*/
    var_SetBool( p_input, "can-rewind", !in->b_rescale_ts && !in->b_can_pace_control && in->b_can_rate_control );

    /* Set record capabilities */
    if( demux_Control( in->p_demux, DEMUX_CAN_RECORD, &in->b_can_stream_record ) )
        in->b_can_stream_record = false;
#ifdef ENABLE_SOUT
    if( !var_GetBool( p_input, "input-record-native" ) )
        in->b_can_stream_record = false;
    var_SetBool( p_input, "can-record", true );
#else
    var_SetBool( p_input, "can-record", in->b_can_stream_record );
#endif

    /* get attachment
     * FIXME improve for b_preparsing: move it after GET_META and check psz_arturl */
    if( !input_priv(p_input)->b_preparsing )
    {
        if( demux_Control( in->p_demux, DEMUX_GET_TITLE_INFO,
                           &in->title, &in->i_title,
                           &in->i_title_offset, &in->i_seekpoint_offset ))
        {
            TAB_INIT( in->i_title, in->title );
        }
        else
        {
            in->b_title_demux = true;
        }

        int i_attachment;
        input_attachment_t **attachment;
        if( !demux_Control( in->p_demux, DEMUX_GET_ATTACHMENTS,
                             &attachment, &i_attachment ) )
        {
            vlc_mutex_lock( &input_priv(p_input)->p_item->lock );
            AppendAttachment( &input_priv(p_input)->i_attachment, &input_priv(p_input)->attachment, &input_priv(p_input)->attachment_demux,
                              i_attachment, attachment, in->p_demux );
            vlc_mutex_unlock( &input_priv(p_input)->p_item->lock );
        }

        demux_Control( in->p_demux, DEMUX_GET_PTS_DELAY, &in->i_pts_delay );
        if( in->i_pts_delay > INPUT_PTS_DELAY_MAX )
            in->i_pts_delay = INPUT_PTS_DELAY_MAX;
        else if( in->i_pts_delay < 0 )
            in->i_pts_delay = 0;
    }

    if( demux_Control( in->p_demux, DEMUX_GET_FPS, &in->f_fps ) )
        in->f_fps = 0.f;

    if( var_GetInteger( p_input, "clock-synchro" ) != -1 )
        in->b_can_pace_control = !var_GetInteger( p_input, "clock-synchro" );

    return in;
}

/*****************************************************************************
 * InputSourceDestroy:
 *****************************************************************************/
static void InputSourceDestroy( input_source_t *in )
{
    int i;

    if( in->p_demux )
        demux_Delete( in->p_demux );

    if( in->i_title > 0 )
    {
        for( i = 0; i < in->i_title; i++ )
            vlc_input_title_Delete( in->title[i] );
        TAB_CLEAN( in->i_title, in->title );
    }

    vlc_object_release( in );
}

/*****************************************************************************
 * InputSourceMeta:
 *****************************************************************************/
static void InputSourceMeta( input_thread_t *p_input,
                             input_source_t *p_source, vlc_meta_t *p_meta )
{
    demux_t *p_demux = p_source->p_demux;

    /* XXX Remember that checking against p_item->p_meta->i_status & ITEM_PREPARSED
     * is a bad idea */

    bool has_meta = false;

    /* Read demux meta */
    if( !demux_Control( p_demux, DEMUX_GET_META, p_meta ) )
        has_meta = true;

    bool has_unsupported;
    if( demux_Control( p_demux, DEMUX_HAS_UNSUPPORTED_META, &has_unsupported ) )
        has_unsupported = true;

    /* If the demux report unsupported meta data, or if we don't have meta data
     * try an external "meta reader" */
    if( has_meta && !has_unsupported )
        return;

    demux_meta_t *p_demux_meta =
        vlc_custom_create( p_source, sizeof( *p_demux_meta ), "demux meta" );
    if( unlikely(p_demux_meta == NULL) )
        return;
    p_demux_meta->p_item = input_priv(p_input)->p_item;

    module_t *p_id3 = module_need( p_demux_meta, "meta reader", NULL, false );
    if( p_id3 )
    {
        if( p_demux_meta->p_meta )
        {
            vlc_meta_Merge( p_meta, p_demux_meta->p_meta );
            vlc_meta_Delete( p_demux_meta->p_meta );
        }

        if( p_demux_meta->i_attachments > 0 )
        {
            vlc_mutex_lock( &input_priv(p_input)->p_item->lock );
            AppendAttachment( &input_priv(p_input)->i_attachment, &input_priv(p_input)->attachment, &input_priv(p_input)->attachment_demux,
                              p_demux_meta->i_attachments, p_demux_meta->attachments, p_demux);
            vlc_mutex_unlock( &input_priv(p_input)->p_item->lock );
        }
        module_unneed( p_demux, p_id3 );
    }
    vlc_object_release( p_demux_meta );
}


static void SlaveDemux( input_thread_t *p_input )
{
    int64_t i_time;
    int i;

    if( demux_Control( input_priv(p_input)->master->p_demux, DEMUX_GET_TIME, &i_time ) )
    {
        msg_Err( p_input, "demux doesn't like DEMUX_GET_TIME" );
        return;
    }

    for( i = 0; i < input_priv(p_input)->i_slave; i++ )
    {
        input_source_t *in = input_priv(p_input)->slave[i];
        int i_ret;

        if( in->b_eof )
            continue;

        /* Call demux_Demux until we have read enough data */
        if( demux_Control( in->p_demux, DEMUX_SET_NEXT_DEMUX_TIME, i_time ) )
        {
            for( ;; )
            {
                int64_t i_stime;
                if( demux_Control( in->p_demux, DEMUX_GET_TIME, &i_stime ) )
                {
                    msg_Err( p_input, "slave[%d] doesn't like "
                             "DEMUX_GET_TIME -> EOF", i );
                    i_ret = 0;
                    break;
                }

                if( i_stime >= i_time )
                {
                    i_ret = 1;
                    break;
                }

                if( ( i_ret = demux_Demux( in->p_demux ) ) <= 0 )
                    break;
            }
        }
        else
        {
            i_ret = demux_Demux( in->p_demux );
        }

        if( i_ret <= 0 )
        {
            msg_Dbg( p_input, "slave %d EOF", i );
            in->b_eof = true;
        }
    }
}

static void SlaveSeek( input_thread_t *p_input )
{
    int64_t i_time;
    int i;

    if( demux_Control( input_priv(p_input)->master->p_demux, DEMUX_GET_TIME, &i_time ) )
    {
        msg_Err( p_input, "demux doesn't like DEMUX_GET_TIME" );
        return;
    }

    for( i = 0; i < input_priv(p_input)->i_slave; i++ )
    {
        input_source_t *in = input_priv(p_input)->slave[i];

        if( demux_Control( in->p_demux, DEMUX_SET_TIME, i_time, true ) )
        {
            if( !in->b_eof )
                msg_Err( p_input, "seek failed for slave %d -> EOF", i );
            in->b_eof = true;
        }
        else
        {
            in->b_eof = false;
        }
    }
}

/*****************************************************************************
 * InputMetaUser:
 *****************************************************************************/
static void InputMetaUser( input_thread_t *p_input, vlc_meta_t *p_meta )
{
    static const struct { int i_meta; const char *psz_name; } p_list[] = {
        { vlc_meta_Title,       "meta-title" },
        { vlc_meta_Artist,      "meta-artist" },
        { vlc_meta_Genre,       "meta-genre" },
        { vlc_meta_Copyright,   "meta-copyright" },
        { vlc_meta_Description, "meta-description" },
        { vlc_meta_Date,        "meta-date" },
        { vlc_meta_URL,         "meta-url" },
        { 0, NULL }
    };

    /* Get meta information from user */
    for( int i = 0; p_list[i].psz_name; i++ )
    {
        char *psz_string = var_GetNonEmptyString( p_input, p_list[i].psz_name );
        if( !psz_string )
            continue;

        EnsureUTF8( psz_string );
        vlc_meta_Set( p_meta, p_list[i].i_meta, psz_string );
        free( psz_string );
    }
}

static void AppendAttachment( int *pi_attachment, input_attachment_t ***ppp_attachment,
                              const demux_t ***ppp_attachment_demux,
                              int i_new, input_attachment_t **pp_new, const demux_t *p_demux )
{
    int i_attachment = *pi_attachment;
    int i;

    input_attachment_t **pp_att = realloc( *ppp_attachment,
                    sizeof(*pp_att) * ( i_attachment + i_new ) );
    if( likely(pp_att) )
    {
        *ppp_attachment = pp_att;
        const demux_t **pp_attdmx = realloc( *ppp_attachment_demux,
                        sizeof(*pp_attdmx) * ( i_attachment + i_new ) );
        if( likely(pp_attdmx) )
        {
            *ppp_attachment_demux = pp_attdmx;

            for( i = 0; i < i_new; i++ )
            {
                pp_att[i_attachment] = pp_new[i];
                pp_attdmx[i_attachment++] = p_demux;
            }
            /* */
            *pi_attachment = i_attachment;
            free( pp_new );
            return;
        }
    }

    /* on alloc errors */
    for( i = 0; i < i_new; i++ )
        vlc_input_attachment_Delete( pp_new[i] );
    free( pp_new );
}

/*****************************************************************************
 * InputUpdateMeta: merge p_item meta data with p_meta taking care of
 * arturl and locking issue.
 *****************************************************************************/
static void InputUpdateMeta( input_thread_t *p_input, demux_t *p_demux )
{
    vlc_meta_t *p_meta = vlc_meta_New();
    if( unlikely(p_meta == NULL) )
        return;

    demux_Control( p_demux, DEMUX_GET_META, p_meta );

    /* If metadata changed, then the attachments might have changed.
       We need to update them in case they contain album art. */
    input_attachment_t **attachment;
    int i_attachment;

    if( !demux_Control( p_demux, DEMUX_GET_ATTACHMENTS,
                        &attachment, &i_attachment ) )
    {
        vlc_mutex_lock( &input_priv(p_input)->p_item->lock );
        if( input_priv(p_input)->i_attachment > 0 )
        {
            int j = 0;
            for( int i = 0; i < input_priv(p_input)->i_attachment; i++ )
            {
                if( input_priv(p_input)->attachment_demux[i] == p_demux )
                    vlc_input_attachment_Delete( input_priv(p_input)->attachment[i] );
                else
                {
                    input_priv(p_input)->attachment[j] = input_priv(p_input)->attachment[i];
                    input_priv(p_input)->attachment_demux[j] = input_priv(p_input)->attachment_demux[i];
                    j++;
                }
            }
            input_priv(p_input)->i_attachment = j;
        }
        AppendAttachment( &input_priv(p_input)->i_attachment, &input_priv(p_input)->attachment, &input_priv(p_input)->attachment_demux,
                          i_attachment, attachment, p_demux );
        vlc_mutex_unlock( &input_priv(p_input)->p_item->lock );
    }

    es_out_ControlSetMeta( input_priv(p_input)->p_es_out, p_meta );
    vlc_meta_Delete( p_meta );
}

/*****************************************************************************
 * InputGetExtraFiles
 *  Autodetect extra input list
 *****************************************************************************/
static void InputGetExtraFilesPattern( input_thread_t *p_input,
                                       int *pi_list, char ***pppsz_list,
                                       const char *psz_path,
                                       const char *psz_match,
                                       const char *psz_format,
                                       int i_start, int i_stop )
{
    int i_list;
    char **ppsz_list;
    TAB_INIT( i_list, ppsz_list );

    char *psz_base = strdup( psz_path );
    if( !psz_base )
        goto exit;

    /* Remove the extension */
    char *psz_end = &psz_base[strlen(psz_base)-strlen(psz_match)];
    assert( psz_end >= psz_base);
    *psz_end = '\0';

    /* Try to list files */
    for( int i = i_start; i <= i_stop; i++ )
    {
        char *psz_probe;
        if( asprintf( &psz_probe, psz_format, psz_base, i ) < 0 )
            break;

        char *filepath = get_path( psz_probe );

        struct stat st;
        if( filepath == NULL ||
            vlc_stat( filepath, &st ) || !S_ISREG( st.st_mode ) || !st.st_size )
        {
            free( filepath );
            free( psz_probe );
            break;
        }

        msg_Dbg( p_input, "Detected extra file `%s'", filepath );

        char* psz_uri = vlc_path2uri( filepath, NULL );
        if( psz_uri )
            TAB_APPEND( i_list, ppsz_list, psz_uri );

        free( filepath );
        free( psz_probe );
    }
    free( psz_base );
exit:
    *pi_list = i_list;
    *pppsz_list = ppsz_list;
}

static void InputGetExtraFiles( input_thread_t *p_input,
                                int *pi_list, char ***pppsz_list,
                                const char **ppsz_access, const char *psz_path )
{
    static const struct pattern
    {
        const char *psz_access_force;
        const char *psz_match;
        const char *psz_format;
        int i_start;
        int i_stop;
    } patterns[] = {
        /* XXX the order is important */
        { "concat", ".001", "%s.%.3d", 2, 999 },
        { NULL, ".part1.rar","%s.part%.1d.rar", 2, 9 },
        { NULL, ".part01.rar","%s.part%.2d.rar", 2, 99, },
        { NULL, ".part001.rar", "%s.part%.3d.rar", 2, 999 },
        { NULL, ".rar", "%s.r%.2d", 0, 99 },
        { "concat", ".mts", "%s.mts%d", 1, 999 },
    };

    TAB_INIT( *pi_list, *pppsz_list );

    if( ( **ppsz_access && strcmp( *ppsz_access, "file" ) ) || !psz_path )
        return;

    const size_t i_path = strlen(psz_path);

    for( size_t i = 0; i < ARRAY_SIZE( patterns ); ++i )
    {
        const struct pattern* pat = &patterns[i];
        const size_t i_ext = strlen( pat->psz_match );

        if( i_path < i_ext )
            continue;

        if( !strcmp( &psz_path[i_path-i_ext], pat->psz_match ) )
        {
            InputGetExtraFilesPattern( p_input, pi_list, pppsz_list, psz_path,
                pat->psz_match, pat->psz_format, pat->i_start, pat->i_stop );

            if( *pi_list > 0 && pat->psz_access_force )
                *ppsz_access = pat->psz_access_force;
            return;
        }
    }
}

/* */
static void input_ChangeState( input_thread_t *p_input, int i_state )
{
    if( input_priv(p_input)->i_state == i_state )
        return;

    input_priv(p_input)->i_state = i_state;
    if( i_state == ERROR_S )
        input_item_SetErrorWhenReading( input_priv(p_input)->p_item, true );
    else if ( i_state == PLAYING_S )
        input_item_SetErrorWhenReading( input_priv(p_input)->p_item, false );
    input_SendEventState( p_input, i_state );
}


/*****************************************************************************
 * MRLSplit: parse the access, demux and url part of the
 *           Media Resource Locator.
 *****************************************************************************/
void input_SplitMRL( const char **access, const char **demux,
                     const char **path, const char **anchor, char *buf )
{
    char *p;

    /* Separate <path> from <access>[/<demux>]:// */
    p = strstr( buf, "://" );
    if( p != NULL )
    {
        *p = '\0';
        p += 3; /* skips "://" */
        *path = p;

        /* Remove HTML anchor if present (not supported).
         * The hash symbol itself should be URI-encoded. */
        p = strchr( p, '#' );
        if( p != NULL )
        {
            *(p++) = '\0';
            *anchor = p;
        }
        else
            *anchor = "";
    }
    else
    {
#ifndef NDEBUG
        fprintf( stderr, "%s(\"%s\") probably not a valid URI!\n", __func__,
                 buf );
#endif
        /* Note: this is a valid non const pointer to "": */
        *path = buf + strlen( buf );
    }

    /* Separate access from demux */
    p = strchr( buf, '/' );
    if( p != NULL )
    {
        *(p++) = '\0';
        if( p[0] == '$' )
            p++;
        *demux = p;
    }
    else
        *demux = "";

    /* We really don't want module name substitution here! */
    p = buf;
    if( p[0] == '$' )
        p++;
    *access = p;
}

static const char *MRLSeekPoint( const char *str, int *title, int *chapter )
{
    char *end;
    unsigned long u;

    /* Look for the title */
    u = strtoul( str, &end, 0 );
    *title = (str == end || u > (unsigned long)INT_MAX) ? -1 : (int)u;
    str = end;

    /* Look for the chapter */
    if( *str == ':' )
    {
        str++;
        u = strtoul( str, &end, 0 );
        *chapter = (str == end || u > (unsigned long)INT_MAX) ? -1 : (int)u;
        str = end;
    }
    else
        *chapter = -1;

    return str;
}


/*****************************************************************************
 * MRLSections: parse title and seekpoint info from the Media Resource Locator.
 *
 * Syntax:
 * [url][@[title_start][:chapter_start][-[title_end][:chapter_end]]]
 *****************************************************************************/
static void MRLSections( const char *p,
                         int *pi_title_start, int *pi_title_end,
                         int *pi_chapter_start, int *pi_chapter_end )
{
    *pi_title_start = *pi_title_end = *pi_chapter_start = *pi_chapter_end = -1;

    int title_start, chapter_start, title_end, chapter_end;

    if( !p )
        return;

    if( *p != '-' )
        p = MRLSeekPoint( p, &title_start, &chapter_start );
    else
        title_start = chapter_start = -1;

    if( *p == '-' )
        p = MRLSeekPoint( p + 1, &title_end, &chapter_end );
    else
        title_end = chapter_end = -1;

    if( *p ) /* syntax error */
        return;

    *pi_title_start = title_start;
    *pi_title_end = title_end;
    *pi_chapter_start = chapter_start;
    *pi_chapter_end = chapter_end;
}

static int input_SlaveSourceAdd( input_thread_t *p_input,
                                 enum slave_type i_type, const char *psz_uri,
                                 unsigned i_flags )
{
    vlc_value_t count;
    const char *psz_es;
    const char *psz_forced_demux;
    const bool b_can_fail = i_flags & SLAVE_ADD_CANFAIL;
    const bool b_forced = i_flags & SLAVE_ADD_FORCED;
    const bool b_set_time = i_flags & SLAVE_ADD_SET_TIME;

    switch( i_type )
    {
    case SLAVE_TYPE_SPU:
        psz_es = "spu-es";
        psz_forced_demux = "subtitle";
        break;
    case SLAVE_TYPE_AUDIO:
        psz_es = "audio-es";
        psz_forced_demux = NULL;
        break;
    default:
        vlc_assert_unreachable();
    }

    if( b_forced )
        var_Change( p_input, psz_es, VLC_VAR_CHOICESCOUNT, &count, NULL );

    msg_Dbg( p_input, "loading %s slave: %s (forced: %d)", psz_es, psz_uri,
             b_forced );

    input_source_t *p_source = InputSourceNew( p_input, psz_uri,
                                               psz_forced_demux,
                                               b_can_fail || psz_forced_demux );

    if( psz_forced_demux && p_source == NULL )
        p_source = InputSourceNew( p_input, psz_uri, NULL, b_can_fail );

    if( p_source == NULL )
    {
        msg_Warn( p_input, "failed to add %s as slave", psz_uri );
        return VLC_EGENERIC;
    }

    if( i_type == SLAVE_TYPE_AUDIO )
    {
        if( b_set_time )
        {
            int64_t i_time;

            /* Set position */
            if( demux_Control( input_priv(p_input)->master->p_demux,
                               DEMUX_GET_TIME, &i_time ) )
            {
                msg_Err( p_input, "demux doesn't like DEMUX_GET_TIME" );
                InputSourceDestroy( p_source );
                return VLC_EGENERIC;
            }

            if( demux_Control( p_source->p_demux,
                               DEMUX_SET_TIME, i_time, true ) )
            {
                msg_Err( p_input, "seek failed for new slave" );
                InputSourceDestroy( p_source );
                return VLC_EGENERIC;
            }
        }

        /* Get meta (access and demux) */
        InputUpdateMeta( p_input, p_source->p_demux );
    }

    TAB_APPEND( input_priv(p_input)->i_slave, input_priv(p_input)->slave, p_source );

    if( !b_forced )
        return VLC_SUCCESS;

    /* Select the ES */
    vlc_value_t list;

    if( var_Change( p_input, psz_es, VLC_VAR_GETCHOICES, &list, NULL ) )
        return VLC_SUCCESS;

    if( count.i_int == 0 )
        count.i_int++;
    /* if it was first one, there is disable too */

    if( count.i_int < list.p_list->i_count )
    {
        const int i_id = list.p_list->p_values[count.i_int].i_int;

        es_out_Control( input_priv(p_input)->p_es_out_display, ES_OUT_SET_ES_DEFAULT_BY_ID, i_id );
        es_out_Control( input_priv(p_input)->p_es_out_display, ES_OUT_SET_ES_BY_ID, i_id );
    }
    var_FreeList( &list, NULL );

    return VLC_SUCCESS;
}

static char *input_SubtitleFile2Uri( input_thread_t *p_input,
                                     const char *psz_subtitle )
{
    /* if we are provided a subtitle.sub file,
     * see if we don't have a subtitle.idx and use it instead */
    char *psz_idxpath = NULL;
    char *psz_extension = strrchr( psz_subtitle, '.');
    if( psz_extension && strcmp( psz_extension, ".sub" ) == 0 )
    {
        psz_idxpath = strdup( psz_subtitle );
        if( psz_idxpath )
        {
            struct stat st;

            psz_extension = psz_extension - psz_subtitle + psz_idxpath;
            strcpy( psz_extension, ".idx" );

            if( !vlc_stat( psz_idxpath, &st ) && S_ISREG( st.st_mode ) )
            {
                msg_Dbg( p_input, "using %s as subtitle file instead of %s",
                         psz_idxpath, psz_subtitle );
                psz_subtitle = psz_idxpath;
            }
        }
    }

    char *psz_uri = vlc_path2uri( psz_subtitle, NULL );
    free( psz_idxpath );
    return psz_uri;
}

/*****************************************************************************
 * Statistics
 *****************************************************************************/
void input_UpdateStatistic( input_thread_t *p_input,
                            input_statistic_t i_type, int i_delta )
{
    assert( input_priv(p_input)->i_state != INIT_S );

    vlc_mutex_lock( &input_priv(p_input)->counters.counters_lock);
    switch( i_type )
    {
#define I(c) stats_Update( input_priv(p_input)->counters.c, i_delta, NULL )
    case INPUT_STATISTIC_DECODED_VIDEO:
        I(p_decoded_video);
        break;
    case INPUT_STATISTIC_DECODED_AUDIO:
        I(p_decoded_audio);
        break;
    case INPUT_STATISTIC_DECODED_SUBTITLE:
        I(p_decoded_sub);
        break;
    case INPUT_STATISTIC_SENT_PACKET:
        I(p_sout_sent_packets);
        break;
#undef I
    case INPUT_STATISTIC_SENT_BYTE:
    {
        uint64_t bytes;

        stats_Update( input_priv(p_input)->counters.p_sout_sent_bytes, i_delta, &bytes );
        stats_Update( input_priv(p_input)->counters.p_sout_send_bitrate, bytes, NULL );
        break;
    }
    default:
        msg_Err( p_input, "Invalid statistic type %d (internal error)", i_type );
        break;
    }
    vlc_mutex_unlock( &input_priv(p_input)->counters.counters_lock);
}

/**/
/* TODO FIXME nearly the same logic that snapshot code */
char *input_CreateFilename(input_thread_t *input, const char *dir,
                           const char *filenamefmt, const char *ext)
{
    char *path;
    char *filename = str_format(input, filenamefmt);
    if (unlikely(filename == NULL))
        return NULL;

    filename_sanitize(filename);

    if (((ext != NULL)
            ? asprintf(&path, "%s"DIR_SEP"%s.%s", dir, filename, ext)
            : asprintf(&path, "%s"DIR_SEP"%s", dir, filename)) < 0)
        path = NULL;

    free(filename);
    return path;
}
