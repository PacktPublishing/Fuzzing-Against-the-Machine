/*****************************************************************************
 * fingerprinter.c: Audio fingerprinter module
 *****************************************************************************
 * Copyright (C) 2012 VLC authors and VideoLAN
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
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <assert.h>

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_stream.h>
#include <vlc_modules.h>
#include <vlc_meta.h>
#include <vlc_url.h>

#include <vlc/vlc.h>
#include <vlc_input.h>
#include <vlc_fingerprinter.h>
#include "webservices/acoustid.h"
#include "../stream_out/chromaprint_data.h"

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/

struct fingerprinter_sys_t
{
    vlc_thread_t thread;

    struct
    {
        vlc_array_t         queue;
        vlc_mutex_t         lock;
    } incoming, results;

    struct
    {
        vlc_array_t         queue;
        vlc_mutex_t         lock;
        vlc_cond_t          cond;
        bool                b_working;
    } processing;
};

static int  Open            (vlc_object_t *);
static void Close           (vlc_object_t *);
static void CleanSys        (fingerprinter_sys_t *);
static void *Run(void *);

/*****************************************************************************
 * Module descriptor
 ****************************************************************************/
vlc_module_begin ()
    set_category(CAT_ADVANCED)
    set_subcategory(SUBCAT_ADVANCED_MISC)
    set_shortname(N_("acoustid"))
    set_description(N_("Track fingerprinter (based on Acoustid)"))
    set_capability("fingerprinter", 10)
    set_callbacks(Open, Close)
vlc_module_end ()

/*****************************************************************************
 * Requests lifecycle
 *****************************************************************************/

static int EnqueueRequest( fingerprinter_thread_t *f, fingerprint_request_t *r )
{
    fingerprinter_sys_t *p_sys = f->p_sys;
    vlc_mutex_lock( &p_sys->incoming.lock );
    int i_ret = vlc_array_append( &p_sys->incoming.queue, r );
    vlc_mutex_unlock( &p_sys->incoming.lock );
    return i_ret;
}

static void QueueIncomingRequests( fingerprinter_sys_t *p_sys )
{
    vlc_mutex_lock( &p_sys->incoming.lock );

    for( size_t i = vlc_array_count( &p_sys->incoming.queue ); i > 0 ; i-- )
    {
        fingerprint_request_t *r = vlc_array_item_at_index( &p_sys->incoming.queue, i - 1 );
        if( vlc_array_append( &p_sys->processing.queue, r ) )
            fingerprint_request_Delete( r );
    }
    vlc_array_clear( &p_sys->incoming.queue );
    vlc_mutex_unlock(&p_sys->incoming.lock);
}

static fingerprint_request_t * GetResult( fingerprinter_thread_t *f )
{
    fingerprint_request_t *r = NULL;
    fingerprinter_sys_t *p_sys = f->p_sys;
    vlc_mutex_lock( &p_sys->results.lock );
    if ( vlc_array_count( &p_sys->results.queue ) )
    {
        r = vlc_array_item_at_index( &p_sys->results.queue, 0 );
        vlc_array_remove( &p_sys->results.queue, 0 );
    }
    vlc_mutex_unlock( &p_sys->results.lock );
    return r;
}

static void ApplyResult( fingerprint_request_t *p_r, size_t i_resultid )
{
    if ( i_resultid >= vlc_array_count( & p_r->results.metas_array ) ) return;

    vlc_meta_t *p_meta = (vlc_meta_t *)
            vlc_array_item_at_index( & p_r->results.metas_array, i_resultid );
    input_item_t *p_item = p_r->p_item;
    vlc_mutex_lock( &p_item->lock );
    vlc_meta_Merge( p_item->p_meta, p_meta );
    vlc_mutex_unlock( &p_item->lock );
}

static int InputEventHandler( vlc_object_t *p_this, char const *psz_cmd,
                              vlc_value_t oldval, vlc_value_t newval,
                              void *p_data )
{
    VLC_UNUSED( psz_cmd );
    VLC_UNUSED( oldval );
    input_thread_t *p_input = (input_thread_t *) p_this;
    fingerprinter_sys_t *p_sys = (fingerprinter_sys_t *) p_data;
    if( newval.i_int == INPUT_EVENT_STATE )
    {
        if( var_GetInteger( p_input, "state" ) >= PAUSE_S )
        {
            vlc_mutex_lock( &p_sys->processing.lock );
            p_sys->processing.b_working = false;
            vlc_cond_signal( &p_sys->processing.cond );
            vlc_mutex_unlock( &p_sys->processing.lock );
        }
    }
    return VLC_SUCCESS;
}

static void DoFingerprint( fingerprinter_thread_t *p_fingerprinter,
                           acoustid_fingerprint_t *fp,
                           const char *psz_uri )
{
    input_item_t *p_item = input_item_New( NULL, NULL );
    if ( unlikely(p_item == NULL) )
         return;

    char *psz_sout_option;
    /* Note: need at -max- 2 channels, but we can't guess it before playing */
    /* the stereo upmix could make the mono tracks fingerprint to differ :/ */
    if ( asprintf( &psz_sout_option,
                   "sout=#transcode{acodec=%s,channels=2}:chromaprint",
                   ( VLC_CODEC_S16L == VLC_CODEC_S16N ) ? "s16l" : "s16b" )
         == -1 )
    {
        input_item_Release( p_item );
        return;
    }

    input_item_AddOption( p_item, psz_sout_option, VLC_INPUT_OPTION_TRUSTED );
    free( psz_sout_option );
    input_item_AddOption( p_item, "vout=dummy", VLC_INPUT_OPTION_TRUSTED );
    input_item_AddOption( p_item, "aout=dummy", VLC_INPUT_OPTION_TRUSTED );
    if ( fp->i_duration )
    {
        if ( asprintf( &psz_sout_option, "stop-time=%u", fp->i_duration ) == -1 )
        {
            input_item_Release( p_item );
            return;
        }
        input_item_AddOption( p_item, psz_sout_option, VLC_INPUT_OPTION_TRUSTED );
        free( psz_sout_option );
    }
    input_item_SetURI( p_item, psz_uri ) ;

    input_thread_t *p_input = input_Create( p_fingerprinter, p_item, "fingerprinter", NULL, NULL );
    input_item_Release( p_item );

    if( p_input == NULL )
        return;

    chromaprint_fingerprint_t chroma_fingerprint;

    chroma_fingerprint.psz_fingerprint = NULL;
    chroma_fingerprint.i_duration = fp->i_duration;

    var_Create( p_input, "fingerprint-data", VLC_VAR_ADDRESS );
    var_SetAddress( p_input, "fingerprint-data", &chroma_fingerprint );

    var_AddCallback( p_input, "intf-event", InputEventHandler, p_fingerprinter->p_sys );

    if( input_Start( p_input ) != VLC_SUCCESS )
    {
        var_DelCallback( p_input, "intf-event", InputEventHandler, p_fingerprinter->p_sys );
        input_Close( p_input );
    }
    else
    {
        p_fingerprinter->p_sys->processing.b_working = true;
        while( p_fingerprinter->p_sys->processing.b_working )
        {
            vlc_cond_wait( &p_fingerprinter->p_sys->processing.cond,
                           &p_fingerprinter->p_sys->processing.lock );
        }
        var_DelCallback( p_input, "intf-event", InputEventHandler, p_fingerprinter->p_sys );
        input_Stop( p_input );
        input_Close( p_input );

        fp->psz_fingerprint = chroma_fingerprint.psz_fingerprint;
        if( !fp->i_duration ) /* had not given hint */
            fp->i_duration = chroma_fingerprint.i_duration;
    }
}

/*****************************************************************************
 * Open:
 *****************************************************************************/
static int Open(vlc_object_t *p_this)
{
    fingerprinter_thread_t *p_fingerprinter = (fingerprinter_thread_t*) p_this;
    fingerprinter_sys_t *p_sys = calloc(1, sizeof(fingerprinter_sys_t));

    if ( !p_sys )
        return VLC_ENOMEM;

    p_fingerprinter->p_sys = p_sys;

    vlc_array_init( &p_sys->incoming.queue );
    vlc_mutex_init( &p_sys->incoming.lock );

    vlc_array_init( &p_sys->processing.queue );
    vlc_mutex_init( &p_sys->processing.lock );
    vlc_cond_init( &p_sys->processing.cond );

    vlc_array_init( &p_sys->results.queue );
    vlc_mutex_init( &p_sys->results.lock );

    p_fingerprinter->pf_enqueue = EnqueueRequest;
    p_fingerprinter->pf_getresults = GetResult;
    p_fingerprinter->pf_apply = ApplyResult;

    var_Create( p_fingerprinter, "results-available", VLC_VAR_BOOL );
    if( vlc_clone( &p_sys->thread, Run, p_fingerprinter,
                   VLC_THREAD_PRIORITY_LOW ) )
    {
        msg_Err( p_fingerprinter, "cannot spawn fingerprinter thread" );
        goto error;
    }

    return VLC_SUCCESS;

error:
    CleanSys( p_sys );
    free( p_sys );
    return VLC_EGENERIC;
}

/*****************************************************************************
 * Close:
 *****************************************************************************/
static void Close(vlc_object_t *p_this)
{
    fingerprinter_thread_t   *p_fingerprinter = (fingerprinter_thread_t*) p_this;
    fingerprinter_sys_t *p_sys = p_fingerprinter->p_sys;

    vlc_cancel( p_sys->thread );
    vlc_join( p_sys->thread, NULL );

    CleanSys( p_sys );
    free( p_sys );
}

static void CleanSys( fingerprinter_sys_t *p_sys )
{
    for ( size_t i = 0; i < vlc_array_count( &p_sys->incoming.queue ); i++ )
        fingerprint_request_Delete( vlc_array_item_at_index( &p_sys->incoming.queue, i ) );
    vlc_array_clear( &p_sys->incoming.queue );
    vlc_mutex_destroy( &p_sys->incoming.lock );

    for ( size_t i = 0; i < vlc_array_count( &p_sys->processing.queue ); i++ )
        fingerprint_request_Delete( vlc_array_item_at_index( &p_sys->processing.queue, i ) );
    vlc_array_clear( &p_sys->processing.queue );
    vlc_mutex_destroy( &p_sys->processing.lock );
    vlc_cond_destroy( &p_sys->processing.cond );

    for ( size_t i = 0; i < vlc_array_count( &p_sys->results.queue ); i++ )
        fingerprint_request_Delete( vlc_array_item_at_index( &p_sys->results.queue, i ) );
    vlc_array_clear( &p_sys->results.queue );
    vlc_mutex_destroy( &p_sys->results.lock );
}

static void fill_metas_with_results( fingerprint_request_t *p_r, acoustid_fingerprint_t *p_f )
{
    for( unsigned int i=0 ; i < p_f->results.count; i++ )
    {
        acoustid_result_t *p_result = & p_f->results.p_results[ i ];
        for ( unsigned int j=0 ; j < p_result->recordings.count; j++ )
        {
            acoustid_mb_result_t *p_record = & p_result->recordings.p_recordings[ j ];
            vlc_meta_t *p_meta = vlc_meta_New();
            if ( p_meta )
            {
                vlc_meta_Set( p_meta, vlc_meta_Title, p_record->psz_title );
                vlc_meta_Set( p_meta, vlc_meta_Artist, p_record->psz_artist );
                vlc_meta_AddExtra( p_meta, "musicbrainz-id", p_record->s_musicbrainz_id );
                if( vlc_array_append( & p_r->results.metas_array, p_meta ) )
                    vlc_meta_Delete( p_meta );
            }
        }
    }
}

/*****************************************************************************
 * Run :
 *****************************************************************************/
static void *Run( void *opaque )
{
    fingerprinter_thread_t *p_fingerprinter = opaque;
    fingerprinter_sys_t *p_sys = p_fingerprinter->p_sys;

    vlc_mutex_lock( &p_sys->processing.lock );
    mutex_cleanup_push( &p_sys->processing.lock );

    /* main loop */
    for (;;)
    {
        msleep( CLOCK_FREQ );

        QueueIncomingRequests( p_sys );

        vlc_testcancel();

        bool results_available = false;
        while( vlc_array_count( &p_sys->processing.queue ) )
        {
            int canc = vlc_savecancel();
            fingerprint_request_t *p_data = vlc_array_item_at_index( &p_sys->processing.queue, 0 );

            char *psz_uri = input_item_GetURI( p_data->p_item );
            if ( psz_uri != NULL )
            {
                 acoustid_fingerprint_t acoustid_print;

                 memset( &acoustid_print , 0, sizeof (acoustid_print) );
                /* overwrite with hint, as in this case, fingerprint's session will be truncated */
                if ( p_data->i_duration )
                     acoustid_print.i_duration = p_data->i_duration;

                DoFingerprint( p_fingerprinter, &acoustid_print, psz_uri );
                free( psz_uri );

                acoustid_config_t cfg = { .p_obj = VLC_OBJECT(p_fingerprinter),
                                          .psz_server = NULL, .psz_apikey = NULL };
                acoustid_lookup_fingerprint( &cfg, &acoustid_print );
                fill_metas_with_results( p_data, &acoustid_print );

                for( unsigned j = 0; j < acoustid_print.results.count; j++ )
                     acoustid_result_release( &acoustid_print.results.p_results[j] );
                if( acoustid_print.results.count )
                    free( acoustid_print.results.p_results );
                free( acoustid_print.psz_fingerprint );
            }
            vlc_restorecancel(canc);

            /* copy results */
            vlc_mutex_lock( &p_sys->results.lock );
            if( vlc_array_append( &p_sys->results.queue, p_data ) )
                fingerprint_request_Delete( p_data );
            else
                results_available = true;
            vlc_mutex_unlock( &p_sys->results.lock );

            // the fingerprint request must not exist both in the
            // processing and results queue, even in case of thread
            // cancellation, so remove it immediately
            vlc_array_remove( &p_sys->processing.queue, 0 );

            vlc_testcancel();
        }

        if ( results_available )
        {
            var_TriggerCallback( p_fingerprinter, "results-available" );
        }
    }

    vlc_cleanup_pop();
    vlc_assert_unreachable();
}
