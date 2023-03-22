/*****************************************************************************
 * es_out.c: Es Out handler for input.
 *****************************************************************************
 * Copyright (C) 2003-2004 VLC authors and VideoLAN
 * $Id: 112cf5fe1ddd757d78593af94f3ee4ae3e270240 $
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
 *          Jean-Paul Saman <jpsaman #_at_# m2x dot nl>
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

#include <stdio.h>
#include <assert.h>
#include <vlc_common.h>

#include <vlc_input.h>
#include <vlc_es_out.h>
#include <vlc_block.h>
#include <vlc_aout.h>
#include <vlc_fourcc.h>
#include <vlc_meta.h>

#include "input_internal.h"
#include "clock.h"
#include "decoder.h"
#include "es_out.h"
#include "event.h"
#include "info.h"
#include "item.h"

#include "../stream_output/stream_output.h"

#include <vlc_iso_lang.h>
/* FIXME we should find a better way than including that */
#include "../text/iso-639_def.h"

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/
typedef struct
{
    /* Program ID */
    int i_id;

    /* Number of es for this pgrm */
    int i_es;

    bool b_selected;
    bool b_scrambled;

    /* Clock for this program */
    input_clock_t *p_clock;

    mtime_t i_last_pcr;

    vlc_meta_t *p_meta;
} es_out_pgrm_t;

struct es_out_id_t
{
    /* ES ID */
    int       i_id;
    es_out_pgrm_t *p_pgrm;

    /* */
    bool b_scrambled;

    /* Channel in the track type */
    int         i_channel;
    es_format_t fmt;
    char        *psz_language;
    char        *psz_language_code;

    decoder_t   *p_dec;
    decoder_t   *p_dec_record;

    mtime_t     i_pts_level;

    /* Fields for Video with CC */
    struct
    {
        vlc_fourcc_t type;
        uint64_t     i_bitmap;    /* channels bitmap */
        es_out_id_t  *pp_es[64]; /* a max of 64 chans for CEA708 */
    } cc;

    /* Field for CC track from a master video */
    es_out_id_t *p_master;

    /* ID for the meta data */
    int         i_meta_id;
};

typedef struct
{
    int         i_count;    /* es count */
    es_out_id_t *p_main_es; /* current main es */
    enum es_out_policy_e e_policy;

    /* Parameters used for es selection */
    bool        b_autoselect; /* if we want to select an es when no user prefs */
    int         i_id;       /* es id as set by es fmt.id */
    int         i_demux_id; /* same as previous, demuxer set default value */
    int         i_channel;  /* es number in creation order */
    char        **ppsz_language;
} es_out_es_props_t;

struct es_out_sys_t
{
    input_thread_t *p_input;

    /* */
    vlc_mutex_t   lock;

    /* all programs */
    int           i_pgrm;
    es_out_pgrm_t **pgrm;
    es_out_pgrm_t *p_pgrm;  /* Master program */

    /* all es */
    int         i_id;
    int         i_es;
    es_out_id_t **es;

    /* mode gestion */
    bool  b_active;
    int         i_mode;

    es_out_es_props_t video, audio, sub;

    /* es/group to select */
    int         i_group_id;

    /* delay */
    int64_t i_audio_delay;
    int64_t i_spu_delay;

    /* Clock configuration */
    mtime_t     i_pts_delay;
    mtime_t     i_pts_jitter;
    int         i_cr_average;
    int         i_rate;

    /* */
    bool        b_paused;
    mtime_t     i_pause_date;

    /* Current preroll */
    mtime_t     i_preroll_end;

    /* Used for buffering */
    bool        b_buffering;
    mtime_t     i_buffering_extra_initial;
    mtime_t     i_buffering_extra_stream;
    mtime_t     i_buffering_extra_system;

    /* Record */
    sout_instance_t *p_sout_record;

    /* Used only to limit debugging output */
    int         i_prev_stream_level;
};

static es_out_id_t *EsOutAdd    ( es_out_t *, const es_format_t * );
static int          EsOutSend   ( es_out_t *, es_out_id_t *, block_t * );
static void         EsOutDel    ( es_out_t *, es_out_id_t * );
static int          EsOutControl( es_out_t *, int i_query, va_list );
static void         EsOutDelete ( es_out_t * );

static void         EsOutTerminate( es_out_t * );
static void         EsOutSelect( es_out_t *, es_out_id_t *es, bool b_force );
static void         EsOutUpdateInfo( es_out_t *, es_out_id_t *es, const es_format_t *, const vlc_meta_t * );
static int          EsOutSetRecord(  es_out_t *, bool b_record );

static bool EsIsSelected( es_out_id_t *es );
static void EsSelect( es_out_t *out, es_out_id_t *es );
static void EsDeleteInfo( es_out_t *, es_out_id_t *es );
static void EsUnselect( es_out_t *out, es_out_id_t *es, bool b_update );
static void EsOutDecoderChangeDelay( es_out_t *out, es_out_id_t *p_es );
static void EsOutDecodersChangePause( es_out_t *out, bool b_paused, mtime_t i_date );
static void EsOutProgramChangePause( es_out_t *out, bool b_paused, mtime_t i_date );
static void EsOutProgramsChangeRate( es_out_t *out );
static void EsOutDecodersStopBuffering( es_out_t *out, bool b_forced );
static void EsOutGlobalMeta( es_out_t *p_out, const vlc_meta_t *p_meta );
static void EsOutMeta( es_out_t *p_out, const vlc_meta_t *p_meta, const vlc_meta_t *p_progmeta );

static char *LanguageGetName( const char *psz_code );
static char *LanguageGetCode( const char *psz_lang );
static char **LanguageSplit( const char *psz_langs );
static int LanguageArrayIndex( char **ppsz_langs, const char *psz_lang );

static char *EsOutProgramGetMetaName( es_out_pgrm_t *p_pgrm );
static char *EsInfoCategoryName( es_out_id_t* es );

static inline int EsOutGetClosedCaptionsChannel( const es_format_t *p_fmt )
{
    int i_channel;
    if( p_fmt->i_codec == VLC_CODEC_CEA608 && p_fmt->subs.cc.i_channel < 4 )
        i_channel = p_fmt->subs.cc.i_channel;
    else if( p_fmt->i_codec == VLC_CODEC_CEA708 && p_fmt->subs.cc.i_channel < 64 )
        i_channel = p_fmt->subs.cc.i_channel;
    else
        i_channel = -1;
    return i_channel;
}
static inline bool EsFmtIsTeletext( const es_format_t *p_fmt )
{
    return p_fmt->i_cat == SPU_ES && p_fmt->i_codec == VLC_CODEC_TELETEXT;
}

/*****************************************************************************
 * Es category specific structs
 *****************************************************************************/
static es_out_es_props_t * GetPropsByCat( es_out_sys_t *p_sys, int i_cat )
{
    switch( i_cat )
    {
    case AUDIO_ES:
        return &p_sys->audio;
    case SPU_ES:
        return &p_sys->sub;
    case VIDEO_ES:
        return &p_sys->video;
    }
    return NULL;
}

static void EsOutPropsCleanup( es_out_es_props_t *p_props )
{
    if( p_props->ppsz_language )
    {
        for( int i = 0; p_props->ppsz_language[i]; i++ )
            free( p_props->ppsz_language[i] );
        free( p_props->ppsz_language );
    }
}

static void EsOutPropsInit( es_out_es_props_t *p_props,
                            bool autoselect,
                            input_thread_t *p_input,
                            enum es_out_policy_e e_default_policy,
                            const char *psz_trackidvar,
                            const char *psz_trackvar,
                            const char *psz_langvar,
                            const char *psz_debug )
{
    p_props->e_policy = e_default_policy;
    p_props->i_count = 0;
    p_props->b_autoselect = autoselect;
    p_props->i_id = (psz_trackidvar) ? var_GetInteger( p_input, psz_trackidvar ): -1;
    p_props->i_channel = (psz_trackvar) ? var_GetInteger( p_input, psz_trackvar ): -1;
    p_props->i_demux_id = -1;
    p_props->p_main_es = NULL;

    if( !input_priv(p_input)->b_preparsing && psz_langvar )
    {
        char *psz_string = var_GetString( p_input, psz_langvar );
        p_props->ppsz_language = LanguageSplit( psz_string );
        if( p_props->ppsz_language )
        {
            for( int i = 0; p_props->ppsz_language[i]; i++ )
                msg_Dbg( p_input, "selected %s language[%d] %s",
                         psz_debug, i, p_props->ppsz_language[i] );
        }
        free( psz_string );
    }
}

/*****************************************************************************
 * input_EsOutNew:
 *****************************************************************************/
es_out_t *input_EsOutNew( input_thread_t *p_input, int i_rate )
{
    es_out_t     *out = malloc( sizeof( *out ) );
    if( !out )
        return NULL;

    es_out_sys_t *p_sys = calloc( 1, sizeof( *p_sys ) );
    if( !p_sys )
    {
        free( out );
        return NULL;
    }

    out->pf_add     = EsOutAdd;
    out->pf_send    = EsOutSend;
    out->pf_del     = EsOutDel;
    out->pf_control = EsOutControl;
    out->pf_destroy = EsOutDelete;
    out->p_sys      = p_sys;

    vlc_mutex_init_recursive( &p_sys->lock );
    p_sys->p_input = p_input;

    p_sys->b_active = false;
    p_sys->i_mode   = ES_OUT_MODE_NONE;

    TAB_INIT( p_sys->i_pgrm, p_sys->pgrm );

    TAB_INIT( p_sys->i_es, p_sys->es );

    /* */
    EsOutPropsInit( &p_sys->video, true, p_input, ES_OUT_ES_POLICY_SIMULTANEOUS,
                    NULL, NULL, NULL, NULL );
    EsOutPropsInit( &p_sys->audio, true, p_input, ES_OUT_ES_POLICY_EXCLUSIVE,
                    "audio-track-id", "audio-track", "audio-language", "audio" );
    EsOutPropsInit( &p_sys->sub,  false, p_input, ES_OUT_ES_POLICY_EXCLUSIVE,
                    "sub-track-id", "sub-track", "sub-language", "sub" );

    p_sys->i_group_id = var_GetInteger( p_input, "program" );

    p_sys->i_pause_date = -1;

    p_sys->i_rate = i_rate;

    p_sys->b_buffering = true;
    p_sys->i_preroll_end = -1;
    p_sys->i_prev_stream_level = -1;

    return out;
}

/*****************************************************************************
 *
 *****************************************************************************/
static void EsOutDelete( es_out_t *out )
{
    es_out_sys_t *p_sys = out->p_sys;

    assert( !p_sys->i_es && !p_sys->i_pgrm && !p_sys->p_pgrm );
    EsOutPropsCleanup( &p_sys->audio );
    EsOutPropsCleanup( &p_sys->sub );

    vlc_mutex_destroy( &p_sys->lock );

    free( p_sys );
    free( out );
}

static void EsOutTerminate( es_out_t *out )
{
    es_out_sys_t *p_sys = out->p_sys;

    if( p_sys->p_sout_record )
        EsOutSetRecord( out, false );

    for( int i = 0; i < p_sys->i_es; i++ )
    {
        if( p_sys->es[i]->p_dec )
            input_DecoderDelete( p_sys->es[i]->p_dec );

        free( p_sys->es[i]->psz_language );
        free( p_sys->es[i]->psz_language_code );
        es_format_Clean( &p_sys->es[i]->fmt );

        free( p_sys->es[i] );
    }
    TAB_CLEAN( p_sys->i_es, p_sys->es );

    /* FIXME duplicate work EsOutProgramDel (but we cannot use it) add a EsOutProgramClean ? */
    for( int i = 0; i < p_sys->i_pgrm; i++ )
    {
        es_out_pgrm_t *p_pgrm = p_sys->pgrm[i];
        input_clock_Delete( p_pgrm->p_clock );
        if( p_pgrm->p_meta )
            vlc_meta_Delete( p_pgrm->p_meta );

        free( p_pgrm );
    }
    TAB_CLEAN( p_sys->i_pgrm, p_sys->pgrm );

    p_sys->p_pgrm = NULL;

    input_item_SetEpgOffline( input_priv(p_sys->p_input)->p_item );
    input_SendEventMetaEpg( p_sys->p_input );
}

static mtime_t EsOutGetWakeup( es_out_t *out )
{
    es_out_sys_t   *p_sys = out->p_sys;
    input_thread_t *p_input = p_sys->p_input;

    if( !p_sys->p_pgrm )
        return 0;

    /* We do not have a wake up date if the input cannot have its speed
     * controlled or sout is imposing its own or while buffering
     *
     * FIXME for !input_priv(p_input)->b_can_pace_control a wake-up time is still needed
     * to avoid too heavy buffering */
    if( !input_priv(p_input)->b_can_pace_control ||
        input_priv(p_input)->b_out_pace_control ||
        p_sys->b_buffering )
        return 0;

    return input_clock_GetWakeup( p_sys->p_pgrm->p_clock );
}

static es_out_id_t es_cat[DATA_ES];

static es_out_id_t *EsOutGetFromID( es_out_t *out, int i_id )
{
    if( i_id < 0 )
    {
        /* Special HACK, -i_id is the cat of the stream */
        return es_cat - i_id;
    }

    for( int i = 0; i < out->p_sys->i_es; i++ )
    {
        if( out->p_sys->es[i]->i_id == i_id )
            return out->p_sys->es[i];
    }
    return NULL;
}

static bool EsOutDecodersIsEmpty( es_out_t *out )
{
    es_out_sys_t      *p_sys = out->p_sys;

    if( p_sys->b_buffering && p_sys->p_pgrm )
    {
        EsOutDecodersStopBuffering( out, true );
        if( p_sys->b_buffering )
            return true;
    }

    for( int i = 0; i < p_sys->i_es; i++ )
    {
        es_out_id_t *es = p_sys->es[i];

        if( es->p_dec && !input_DecoderIsEmpty( es->p_dec ) )
            return false;
        if( es->p_dec_record && !input_DecoderIsEmpty( es->p_dec_record ) )
            return false;
    }
    return true;
}

static void EsOutSetDelay( es_out_t *out, int i_cat, int64_t i_delay )
{
    es_out_sys_t *p_sys = out->p_sys;

    if( i_cat == AUDIO_ES )
        p_sys->i_audio_delay = i_delay;
    else if( i_cat == SPU_ES )
        p_sys->i_spu_delay = i_delay;

    for( int i = 0; i < p_sys->i_es; i++ )
        EsOutDecoderChangeDelay( out, p_sys->es[i] );
}

static int EsOutSetRecord(  es_out_t *out, bool b_record )
{
    es_out_sys_t   *p_sys = out->p_sys;
    input_thread_t *p_input = p_sys->p_input;

    assert( ( b_record && !p_sys->p_sout_record ) || ( !b_record && p_sys->p_sout_record ) );

    if( b_record )
    {
        char *psz_path = var_CreateGetNonEmptyString( p_input, "input-record-path" );
        if( !psz_path )
        {
            if( var_CountChoices( p_input, "video-es" ) )
                psz_path = config_GetUserDir( VLC_VIDEOS_DIR );
            else if( var_CountChoices( p_input, "audio-es" ) )
                psz_path = config_GetUserDir( VLC_MUSIC_DIR );
            else
                psz_path = config_GetUserDir( VLC_DOWNLOAD_DIR );
        }

        char *psz_sout = NULL;  // TODO conf

        if( !psz_sout && psz_path )
        {
            char *psz_file = input_CreateFilename( p_input, psz_path, INPUT_RECORD_PREFIX, NULL );
            if( psz_file )
            {
                char* psz_file_esc = config_StringEscape( psz_file );
                if ( psz_file_esc )
                {
                    if( asprintf( &psz_sout, "#record{dst-prefix='%s'}", psz_file_esc ) < 0 )
                        psz_sout = NULL;
                    free( psz_file_esc );
                }
                free( psz_file );
            }
        }
        free( psz_path );

        if( !psz_sout )
            return VLC_EGENERIC;

#ifdef ENABLE_SOUT
        p_sys->p_sout_record = sout_NewInstance( p_input, psz_sout );
#endif
        free( psz_sout );

        if( !p_sys->p_sout_record )
            return VLC_EGENERIC;

        for( int i = 0; i < p_sys->i_es; i++ )
        {
            es_out_id_t *p_es = p_sys->es[i];

            if( !p_es->p_dec || p_es->p_master )
                continue;

            p_es->p_dec_record = input_DecoderNew( p_input, &p_es->fmt, p_es->p_pgrm->p_clock, p_sys->p_sout_record );
            if( p_es->p_dec_record && p_sys->b_buffering )
                input_DecoderStartWait( p_es->p_dec_record );
        }
    }
    else
    {
        for( int i = 0; i < p_sys->i_es; i++ )
        {
            es_out_id_t *p_es = p_sys->es[i];

            if( !p_es->p_dec_record )
                continue;

            input_DecoderDelete( p_es->p_dec_record );
            p_es->p_dec_record = NULL;
        }
#ifdef ENABLE_SOUT
        sout_DeleteInstance( p_sys->p_sout_record );
#endif
        p_sys->p_sout_record = NULL;
    }

    return VLC_SUCCESS;
}
static void EsOutChangePause( es_out_t *out, bool b_paused, mtime_t i_date )
{
    es_out_sys_t *p_sys = out->p_sys;

    /* XXX the order is important */
    if( b_paused )
    {
        EsOutDecodersChangePause( out, true, i_date );
        EsOutProgramChangePause( out, true, i_date );
    }
    else
    {
        if( p_sys->i_buffering_extra_initial > 0 )
        {
            mtime_t i_stream_start;
            mtime_t i_system_start;
            mtime_t i_stream_duration;
            mtime_t i_system_duration;
            int i_ret;
            i_ret = input_clock_GetState( p_sys->p_pgrm->p_clock,
                                          &i_stream_start, &i_system_start,
                                          &i_stream_duration, &i_system_duration );
            if( !i_ret )
            {
                /* FIXME pcr != exactly what wanted */
                const mtime_t i_used = /*(i_stream_duration - input_priv(p_sys->p_input)->i_pts_delay)*/ p_sys->i_buffering_extra_system - p_sys->i_buffering_extra_initial;
                i_date -= i_used;
            }
            p_sys->i_buffering_extra_initial = 0;
            p_sys->i_buffering_extra_stream = 0;
            p_sys->i_buffering_extra_system = 0;
        }
        EsOutProgramChangePause( out, false, i_date );
        EsOutDecodersChangePause( out, false, i_date );

        EsOutProgramsChangeRate( out );
    }
    p_sys->b_paused = b_paused;
    p_sys->i_pause_date = i_date;
}

static void EsOutChangeRate( es_out_t *out, int i_rate )
{
    es_out_sys_t      *p_sys = out->p_sys;

    p_sys->i_rate = i_rate;
    EsOutProgramsChangeRate( out );
}

static void EsOutChangePosition( es_out_t *out )
{
    es_out_sys_t      *p_sys = out->p_sys;

    input_SendEventCache( p_sys->p_input, 0.0 );

    for( int i = 0; i < p_sys->i_es; i++ )
    {
        es_out_id_t *p_es = p_sys->es[i];

        if( p_es->p_dec != NULL )
        {
            input_DecoderFlush( p_es->p_dec );
            if( !p_sys->b_buffering )
            {
                input_DecoderStartWait( p_es->p_dec );
                if( p_es->p_dec_record != NULL )
                    input_DecoderStartWait( p_es->p_dec_record );
            }
        }
        p_es->i_pts_level = VLC_TS_INVALID;
    }

    for( int i = 0; i < p_sys->i_pgrm; i++ ) {
        input_clock_Reset( p_sys->pgrm[i]->p_clock );
        p_sys->pgrm[i]->i_last_pcr = VLC_TS_INVALID;
    }

    p_sys->b_buffering = true;
    p_sys->i_buffering_extra_initial = 0;
    p_sys->i_buffering_extra_stream = 0;
    p_sys->i_buffering_extra_system = 0;
    p_sys->i_preroll_end = -1;
    p_sys->i_prev_stream_level = -1;
}



static void EsOutDecodersStopBuffering( es_out_t *out, bool b_forced )
{
    es_out_sys_t *p_sys = out->p_sys;

    mtime_t i_stream_start;
    mtime_t i_system_start;
    mtime_t i_stream_duration;
    mtime_t i_system_duration;
    if (input_clock_GetState( p_sys->p_pgrm->p_clock,
                                  &i_stream_start, &i_system_start,
                                  &i_stream_duration, &i_system_duration ))
        return;

    mtime_t i_preroll_duration = 0;
    if( p_sys->i_preroll_end >= 0 )
        i_preroll_duration = __MAX( p_sys->i_preroll_end - i_stream_start, 0 );

    const mtime_t i_buffering_duration = p_sys->i_pts_delay +
                                         i_preroll_duration +
                                         p_sys->i_buffering_extra_stream - p_sys->i_buffering_extra_initial;

    if( i_stream_duration <= i_buffering_duration && !b_forced )
    {
        double f_level;
        if (i_buffering_duration == 0)
            f_level = 0;
        else
            f_level = __MAX( (double)i_stream_duration / i_buffering_duration, 0 );
        input_SendEventCache( p_sys->p_input, f_level );

        int i_level = (int)(100 * f_level);
        if( p_sys->i_prev_stream_level != i_level )
        {
            msg_Dbg( p_sys->p_input, "Buffering %d%%", i_level );
            p_sys->i_prev_stream_level = i_level;
        }

        return;
    }
    input_SendEventCache( p_sys->p_input, 1.0 );

    msg_Dbg( p_sys->p_input, "Stream buffering done (%d ms in %d ms)",
              (int)(i_stream_duration/1000), (int)(i_system_duration/1000) );
    p_sys->b_buffering = false;
    p_sys->i_preroll_end = -1;
    p_sys->i_prev_stream_level = -1;

    if( p_sys->i_buffering_extra_initial > 0 )
    {
        /* FIXME wrong ? */
        return;
    }

    const mtime_t i_decoder_buffering_start = mdate();
    for( int i = 0; i < p_sys->i_es; i++ )
    {
        es_out_id_t *p_es = p_sys->es[i];

        if( !p_es->p_dec || p_es->fmt.i_cat == SPU_ES )
            continue;
        input_DecoderWait( p_es->p_dec );
        if( p_es->p_dec_record )
            input_DecoderWait( p_es->p_dec_record );
    }

    msg_Dbg( p_sys->p_input, "Decoder wait done in %d ms",
              (int)(mdate() - i_decoder_buffering_start)/1000 );

    /* Here is a good place to destroy unused vout with every demuxer */
    input_resource_TerminateVout( input_priv(p_sys->p_input)->p_resource );

    /* */
    const mtime_t i_wakeup_delay = 10*1000; /* FIXME CLEANUP thread wake up time*/
    const mtime_t i_current_date = p_sys->b_paused ? p_sys->i_pause_date : mdate();

    input_clock_ChangeSystemOrigin( p_sys->p_pgrm->p_clock, true,
                                    i_current_date + i_wakeup_delay - i_buffering_duration );

    for( int i = 0; i < p_sys->i_es; i++ )
    {
        es_out_id_t *p_es = p_sys->es[i];

        if( !p_es->p_dec )
            continue;

        input_DecoderStopWait( p_es->p_dec );
        if( p_es->p_dec_record )
            input_DecoderStopWait( p_es->p_dec_record );
    }
}
static void EsOutDecodersChangePause( es_out_t *out, bool b_paused, mtime_t i_date )
{
    es_out_sys_t *p_sys = out->p_sys;

    /* Pause decoders first */
    for( int i = 0; i < p_sys->i_es; i++ )
    {
        es_out_id_t *es = p_sys->es[i];

        if( es->p_dec )
        {
            input_DecoderChangePause( es->p_dec, b_paused, i_date );
            if( es->p_dec_record )
                input_DecoderChangePause( es->p_dec_record, b_paused, i_date );
        }
    }
}

static bool EsOutIsExtraBufferingAllowed( es_out_t *out )
{
    es_out_sys_t *p_sys = out->p_sys;

    size_t i_size = 0;
    for( int i = 0; i < p_sys->i_es; i++ )
    {
        es_out_id_t *p_es = p_sys->es[i];

        if( p_es->p_dec )
            i_size += input_DecoderGetFifoSize( p_es->p_dec );
        if( p_es->p_dec_record )
            i_size += input_DecoderGetFifoSize( p_es->p_dec_record );
    }
    //msg_Info( out, "----- EsOutIsExtraBufferingAllowed =% 5d KiB -- ", i_size / 1024 );

    /* TODO maybe we want to be able to tune it ? */
#if defined(OPTIMIZE_MEMORY)
    const size_t i_level_high = 512*1024;  /* 0.5 MiB */
#else
    const size_t i_level_high = 10*1024*1024; /* 10 MiB */
#endif
    return i_size < i_level_high;
}

static void EsOutProgramChangePause( es_out_t *out, bool b_paused, mtime_t i_date )
{
    es_out_sys_t *p_sys = out->p_sys;

    for( int i = 0; i < p_sys->i_pgrm; i++ )
        input_clock_ChangePause( p_sys->pgrm[i]->p_clock, b_paused, i_date );
}

static void EsOutDecoderChangeDelay( es_out_t *out, es_out_id_t *p_es )
{
    es_out_sys_t *p_sys = out->p_sys;

    mtime_t i_delay = 0;
    if( p_es->fmt.i_cat == AUDIO_ES )
        i_delay = p_sys->i_audio_delay;
    else if( p_es->fmt.i_cat == SPU_ES )
        i_delay = p_sys->i_spu_delay;
    else
        return;

    if( p_es->p_dec )
        input_DecoderChangeDelay( p_es->p_dec, i_delay );
    if( p_es->p_dec_record )
        input_DecoderChangeDelay( p_es->p_dec_record, i_delay );
}
static void EsOutProgramsChangeRate( es_out_t *out )
{
    es_out_sys_t      *p_sys = out->p_sys;

    for( int i = 0; i < p_sys->i_pgrm; i++ )
        input_clock_ChangeRate( p_sys->pgrm[i]->p_clock, p_sys->i_rate );
}

static void EsOutFrameNext( es_out_t *out )
{
    es_out_sys_t *p_sys = out->p_sys;
    es_out_id_t *p_es_video = NULL;

    if( p_sys->b_buffering )
    {
        msg_Warn( p_sys->p_input, "buffering, ignoring 'frame next'" );
        return;
    }

    assert( p_sys->b_paused );

    for( int i = 0; i < p_sys->i_es; i++ )
    {
        es_out_id_t *p_es = p_sys->es[i];

        if( p_es->fmt.i_cat == VIDEO_ES && p_es->p_dec )
        {
            p_es_video = p_es;
            break;
        }
    }

    if( !p_es_video )
    {
        msg_Warn( p_sys->p_input, "No video track selected, ignoring 'frame next'" );
        return;
    }

    mtime_t i_duration;
    input_DecoderFrameNext( p_es_video->p_dec, &i_duration );

    msg_Dbg( out->p_sys->p_input, "EsOutFrameNext consummed %d ms", (int)(i_duration/1000) );

    if( i_duration <= 0 )
        i_duration = 40*1000;

    /* FIXME it is not a clean way ? */
    if( p_sys->i_buffering_extra_initial <= 0 )
    {
        mtime_t i_stream_start;
        mtime_t i_system_start;
        mtime_t i_stream_duration;
        mtime_t i_system_duration;
        int i_ret;

        i_ret = input_clock_GetState( p_sys->p_pgrm->p_clock,
                                      &i_stream_start, &i_system_start,
                                      &i_stream_duration, &i_system_duration );
        if( i_ret )
            return;

        p_sys->i_buffering_extra_initial = 1 + i_stream_duration - p_sys->i_pts_delay; /* FIXME < 0 ? */
        p_sys->i_buffering_extra_system =
        p_sys->i_buffering_extra_stream = p_sys->i_buffering_extra_initial;
    }

    const int i_rate = input_clock_GetRate( p_sys->p_pgrm->p_clock );

    p_sys->b_buffering = true;
    p_sys->i_buffering_extra_system += i_duration;
    p_sys->i_buffering_extra_stream = p_sys->i_buffering_extra_initial +
                                      ( p_sys->i_buffering_extra_system - p_sys->i_buffering_extra_initial ) *
                                                INPUT_RATE_DEFAULT / i_rate;

    p_sys->i_preroll_end = -1;
    p_sys->i_prev_stream_level = -1;
}
static mtime_t EsOutGetBuffering( es_out_t *out )
{
    es_out_sys_t *p_sys = out->p_sys;
    mtime_t i_stream_duration, i_system_start;

    if( !p_sys->p_pgrm )
        return 0;
    else
    {
        mtime_t i_stream_start, i_system_duration;

        if( input_clock_GetState( p_sys->p_pgrm->p_clock,
                                  &i_stream_start, &i_system_start,
                                  &i_stream_duration, &i_system_duration ) )
            return 0;
    }

    mtime_t i_delay;

    if( p_sys->b_buffering && p_sys->i_buffering_extra_initial <= 0 )
    {
        i_delay = i_stream_duration;
    }
    else
    {
        mtime_t i_system_duration;

        if( p_sys->b_paused )
        {
            i_system_duration = p_sys->i_pause_date  - i_system_start;
            if( p_sys->i_buffering_extra_initial > 0 )
                i_system_duration += p_sys->i_buffering_extra_system - p_sys->i_buffering_extra_initial;
        }
        else
        {
            i_system_duration = mdate() - i_system_start;
        }

        const mtime_t i_consumed = i_system_duration * INPUT_RATE_DEFAULT / p_sys->i_rate - i_stream_duration;
        i_delay = p_sys->i_pts_delay - i_consumed;
    }
    if( i_delay < 0 )
        return 0;
    return i_delay;
}

static void EsOutESVarUpdateGeneric( es_out_t *out, int i_id,
                                     const es_format_t *fmt, const char *psz_language,
                                     bool b_delete )
{
    es_out_sys_t      *p_sys = out->p_sys;
    input_thread_t    *p_input = p_sys->p_input;
    vlc_value_t       val, text;

    if( b_delete )
    {
        if( EsFmtIsTeletext( fmt ) )
            input_SendEventTeletextDel( p_sys->p_input, i_id );

        input_SendEventEsDel( p_input, fmt->i_cat, i_id );
        return;
    }

    /* Get the number of ES already added */
    const char *psz_var;
    if( fmt->i_cat == AUDIO_ES )
        psz_var = "audio-es";
    else if( fmt->i_cat == VIDEO_ES )
        psz_var = "video-es";
    else
        psz_var = "spu-es";

    var_Change( p_input, psz_var, VLC_VAR_CHOICESCOUNT, &val, NULL );
    if( val.i_int == 0 )
    {
        vlc_value_t val2;

        /* First one, we need to add the "Disable" choice */
        val2.i_int = -1; text.psz_string = _("Disable");
        var_Change( p_input, psz_var, VLC_VAR_ADDCHOICE, &val2, &text );
        val.i_int++;
    }

    /* Take care of the ES description */
    if( fmt->psz_description && *fmt->psz_description )
    {
        if( psz_language && *psz_language )
        {
            if( asprintf( &text.psz_string, "%s - [%s]", fmt->psz_description,
                          psz_language ) == -1 )
                text.psz_string = NULL;
        }
        else text.psz_string = strdup( fmt->psz_description );
    }
    else
    {
        if( psz_language && *psz_language )
        {
            if( asprintf( &text.psz_string, "%s %"PRId64" - [%s]", _( "Track" ), val.i_int, psz_language ) == -1 )
                text.psz_string = NULL;
        }
        else
        {
            if( asprintf( &text.psz_string, "%s %"PRId64, _( "Track" ), val.i_int ) == -1 )
                text.psz_string = NULL;
        }
    }

    input_SendEventEsAdd( p_input, fmt->i_cat, i_id, text.psz_string );
    if( EsFmtIsTeletext( fmt ) )
    {
        char psz_page[3+1];
        snprintf( psz_page, sizeof(psz_page), "%d%2.2x",
                  fmt->subs.teletext.i_magazine,
                  fmt->subs.teletext.i_page );
        input_SendEventTeletextAdd( p_sys->p_input,
                                    i_id, fmt->subs.teletext.i_magazine >= 0 ? psz_page : NULL );
    }

    free( text.psz_string );
}

static void EsOutESVarUpdate( es_out_t *out, es_out_id_t *es,
                              bool b_delete )
{
    EsOutESVarUpdateGeneric( out, es->i_id, &es->fmt, es->psz_language, b_delete );
}

static bool EsOutIsProgramVisible( es_out_t *out, int i_group )
{
    return out->p_sys->i_group_id == 0 || out->p_sys->i_group_id == i_group;
}

/* EsOutProgramSelect:
 *  Select a program and update the object variable
 */
static void EsOutProgramSelect( es_out_t *out, es_out_pgrm_t *p_pgrm )
{
    es_out_sys_t      *p_sys = out->p_sys;
    input_thread_t    *p_input = p_sys->p_input;
    int               i;

    if( p_sys->p_pgrm == p_pgrm )
        return; /* Nothing to do */

    if( p_sys->p_pgrm )
    {
        es_out_pgrm_t *old = p_sys->p_pgrm;
        msg_Dbg( p_input, "unselecting program id=%d", old->i_id );

        for( i = 0; i < p_sys->i_es; i++ )
        {
            if( p_sys->es[i]->p_pgrm == old && EsIsSelected( p_sys->es[i] ) &&
                p_sys->i_mode != ES_OUT_MODE_ALL )
                EsUnselect( out, p_sys->es[i], true );
        }

        p_sys->audio.p_main_es = NULL;
        p_sys->video.p_main_es = NULL;
        p_sys->sub.p_main_es = NULL;
    }

    msg_Dbg( p_input, "selecting program id=%d", p_pgrm->i_id );

    /* Mark it selected */
    p_pgrm->b_selected = true;

    /* Switch master stream */
    p_sys->p_pgrm = p_pgrm;

    /* Update "program" */
    input_SendEventProgramSelect( p_input, p_pgrm->i_id );

    /* Update "es-*" */
    input_SendEventEsDel( p_input, AUDIO_ES, -1 );
    input_SendEventEsDel( p_input, VIDEO_ES, -1 );
    input_SendEventEsDel( p_input, SPU_ES, -1 );
    input_SendEventTeletextDel( p_input, -1 );
    input_SendEventProgramScrambled( p_input, p_pgrm->i_id, p_pgrm->b_scrambled );

    /* TODO event */
    var_SetInteger( p_input, "teletext-es", -1 );

    for( i = 0; i < p_sys->i_es; i++ )
    {
        if( p_sys->es[i]->p_pgrm == p_sys->p_pgrm )
        {
            EsOutESVarUpdate( out, p_sys->es[i], false );
            EsOutUpdateInfo( out, p_sys->es[i], &p_sys->es[i]->fmt, NULL );
        }

        EsOutSelect( out, p_sys->es[i], false );
    }

    /* Ensure the correct running EPG table is selected */
    input_item_ChangeEPGSource( input_priv(p_input)->p_item, p_pgrm->i_id );

    /* Update now playing */
    if( p_pgrm->p_meta )
    {
        input_item_SetESNowPlaying( input_priv(p_input)->p_item,
                                    vlc_meta_Get( p_pgrm->p_meta, vlc_meta_ESNowPlaying ) );
        input_item_SetPublisher( input_priv(p_input)->p_item,
                                 vlc_meta_Get( p_pgrm->p_meta, vlc_meta_Publisher ) );
        input_item_SetTitle( input_priv(p_input)->p_item,
                             vlc_meta_Get( p_pgrm->p_meta, vlc_meta_Title ) );
        input_SendEventMeta( p_input );
        /* FIXME: we probably want to replace every input meta */
    }
}

/* EsOutAddProgram:
 *  Add a program
 */
static es_out_pgrm_t *EsOutProgramAdd( es_out_t *out, int i_group )
{
    es_out_sys_t      *p_sys = out->p_sys;
    input_thread_t    *p_input = p_sys->p_input;

    es_out_pgrm_t *p_pgrm = malloc( sizeof( es_out_pgrm_t ) );
    if( !p_pgrm )
        return NULL;

    /* Init */
    p_pgrm->i_id = i_group;
    p_pgrm->i_es = 0;
    p_pgrm->b_selected = false;
    p_pgrm->b_scrambled = false;
    p_pgrm->i_last_pcr = VLC_TS_INVALID;
    p_pgrm->p_meta = NULL;
    p_pgrm->p_clock = input_clock_New( p_sys->i_rate );
    if( !p_pgrm->p_clock )
    {
        free( p_pgrm );
        return NULL;
    }
    if( p_sys->b_paused )
        input_clock_ChangePause( p_pgrm->p_clock, p_sys->b_paused, p_sys->i_pause_date );
    input_clock_SetJitter( p_pgrm->p_clock, p_sys->i_pts_delay, p_sys->i_cr_average );

    /* Append it */
    TAB_APPEND( p_sys->i_pgrm, p_sys->pgrm, p_pgrm );

    /* Update "program" variable */
    if( EsOutIsProgramVisible( out, i_group ) )
        input_SendEventProgramAdd( p_input, i_group, NULL );

    if( i_group == p_sys->i_group_id || ( !p_sys->p_pgrm && p_sys->i_group_id == 0 ) )
        EsOutProgramSelect( out, p_pgrm );

    return p_pgrm;
}

/* EsOutDelProgram:
 *  Delete a program
 */
static int EsOutProgramDel( es_out_t *out, int i_group )
{
    es_out_sys_t      *p_sys = out->p_sys;
    input_thread_t    *p_input = p_sys->p_input;
    es_out_pgrm_t     *p_pgrm = NULL;
    int               i;

    for( i = 0; i < p_sys->i_pgrm; i++ )
    {
        if( p_sys->pgrm[i]->i_id == i_group )
        {
            p_pgrm = p_sys->pgrm[i];
            break;
        }
    }

    if( p_pgrm == NULL )
        return VLC_EGENERIC;

    if( p_pgrm->i_es )
    {
        msg_Dbg( p_input, "can't delete program %d which still has %i ES",
                 i_group, p_pgrm->i_es );
        return VLC_EGENERIC;
    }

    TAB_REMOVE( p_sys->i_pgrm, p_sys->pgrm, p_pgrm );

    /* If program is selected we need to unselect it */
    if( p_sys->p_pgrm == p_pgrm )
        p_sys->p_pgrm = NULL;

    input_clock_Delete( p_pgrm->p_clock );

    if( p_pgrm->p_meta )
        vlc_meta_Delete( p_pgrm->p_meta );
    free( p_pgrm );

    /* Update "program" variable */
    input_SendEventProgramDel( p_input, i_group );

    return VLC_SUCCESS;
}

/* EsOutProgramFind
 */
static es_out_pgrm_t *EsOutProgramFind( es_out_t *p_out, int i_group )
{
    es_out_sys_t *p_sys = p_out->p_sys;

    for( int i = 0; i < p_sys->i_pgrm; i++ )
    {
        if( p_sys->pgrm[i]->i_id == i_group )
            return p_sys->pgrm[i];
    }
    return EsOutProgramAdd( p_out, i_group );
}

/* EsOutProgramMeta:
 */
static char *EsOutProgramGetMetaName( es_out_pgrm_t *p_pgrm )
{
    char *psz = NULL;
    if( p_pgrm->p_meta && vlc_meta_Get( p_pgrm->p_meta, vlc_meta_Title ) )
    {
        if( asprintf( &psz, _("%s [%s %d]"), vlc_meta_Get( p_pgrm->p_meta, vlc_meta_Title ),
                      _("Program"), p_pgrm->i_id ) == -1 )
            return NULL;
    }
    else
    {
        if( asprintf( &psz, "%s %d", _("Program"), p_pgrm->i_id ) == -1 )
            return NULL;
    }
    return psz;
}

static char *EsOutProgramGetProgramName( es_out_pgrm_t *p_pgrm )
{
    char *psz = NULL;
    if( p_pgrm->p_meta && vlc_meta_Get( p_pgrm->p_meta, vlc_meta_Title ) )
    {
        return strdup( vlc_meta_Get( p_pgrm->p_meta, vlc_meta_Title ) );
    }
    else
    {
        if( asprintf( &psz, "%s %d", _("Program"), p_pgrm->i_id ) == -1 )
            return NULL;
    }
    return psz;
}

static char *EsInfoCategoryName( es_out_id_t* es )
{
    char *psz_category;

    if( asprintf( &psz_category, _("Stream %d"), es->i_meta_id ) == -1 )
        return NULL;

    return psz_category;
}

static void EsOutProgramMeta( es_out_t *out, int i_group, const vlc_meta_t *p_meta )
{
    es_out_sys_t      *p_sys = out->p_sys;
    es_out_pgrm_t     *p_pgrm;
    input_thread_t    *p_input = p_sys->p_input;
    const char        *psz_title = NULL;
    const char        *psz_provider = NULL;
    int i;

    msg_Dbg( p_input, "EsOutProgramMeta: number=%d", i_group );

    /* Check against empty meta data (empty for what we handle) */
    if( !vlc_meta_Get( p_meta, vlc_meta_Title) &&
        !vlc_meta_Get( p_meta, vlc_meta_ESNowPlaying) &&
        !vlc_meta_Get( p_meta, vlc_meta_Publisher) )
    {
        return;
    }

    if( i_group < 0 )
    {
        EsOutGlobalMeta( out, p_meta );
        return;
    }

    /* Find program */
    if( !EsOutIsProgramVisible( out, i_group ) )
        return;
    p_pgrm = EsOutProgramFind( out, i_group );
    if( !p_pgrm )
        return;

    if( p_pgrm->p_meta )
    {
        const char *psz_current_title = vlc_meta_Get( p_pgrm->p_meta, vlc_meta_Title );
        const char *psz_new_title = vlc_meta_Get( p_meta, vlc_meta_Title );
        if( (psz_current_title != NULL && psz_new_title != NULL)
            ? strcmp(psz_new_title, psz_current_title)
            : (psz_current_title != psz_new_title) )
        {
            /* Remove old entries */
            char *psz_oldinfokey = EsOutProgramGetMetaName( p_pgrm );
            input_Control( p_input, INPUT_DEL_INFO, psz_oldinfokey, NULL );
            /* TODO update epg name ?
             * TODO update scrambled info name ? */
            free( psz_oldinfokey );
        }
        vlc_meta_Delete( p_pgrm->p_meta );
    }
    p_pgrm->p_meta = vlc_meta_New();
    if( p_pgrm->p_meta )
        vlc_meta_Merge( p_pgrm->p_meta, p_meta );

    if( p_sys->p_pgrm == p_pgrm )
    {
        EsOutMeta( out, NULL, p_meta );
    }
    /* */
    psz_title = vlc_meta_Get( p_meta, vlc_meta_Title);
    psz_provider = vlc_meta_Get( p_meta, vlc_meta_Publisher);

    /* Update the description text of the program */
    if( psz_title && *psz_title )
    {
        char *psz_text;
        if( psz_provider && *psz_provider )
        {
            if( asprintf( &psz_text, "%s [%s]", psz_title, psz_provider ) < 0 )
                psz_text = NULL;
        }
        else
        {
            psz_text = strdup( psz_title );
        }

        /* ugly but it works */
        if( psz_text )
        {
            input_SendEventProgramDel( p_input, i_group );
            input_SendEventProgramAdd( p_input, i_group, psz_text );
            if( p_sys->p_pgrm == p_pgrm )
                input_SendEventProgramSelect( p_input, i_group );
            free( psz_text );
        }
    }

    /* */
    char **ppsz_all_keys = vlc_meta_CopyExtraNames(p_meta );

    info_category_t *p_cat = NULL;
    if( psz_provider || ( ppsz_all_keys[0] && *ppsz_all_keys[0] ) )
    {
        char *psz_cat = EsOutProgramGetMetaName( p_pgrm );
        if( psz_cat )
            p_cat = info_category_New( psz_cat );
        free( psz_cat );
    }

    for( i = 0; ppsz_all_keys[i]; i++ )
    {
        if( p_cat )
            info_category_AddInfo( p_cat, vlc_gettext(ppsz_all_keys[i]), "%s",
                                   vlc_meta_GetExtra( p_meta, ppsz_all_keys[i] ) );
        free( ppsz_all_keys[i] );
    }
    free( ppsz_all_keys );

    if( psz_provider )
    {
        if( p_sys->p_pgrm == p_pgrm )
        {
            input_item_SetPublisher( input_priv(p_input)->p_item, psz_provider );
            input_SendEventMeta( p_input );
        }
        if( p_cat )
            info_category_AddInfo( p_cat, vlc_meta_TypeToLocalizedString(vlc_meta_Publisher),
                                   "%s",psz_provider );
    }
    if( p_cat )
        input_Control( p_input, INPUT_MERGE_INFOS, p_cat );
}

static void EsOutProgramEpgEvent( es_out_t *out, int i_group, const vlc_epg_event_t *p_event )
{
    es_out_sys_t      *p_sys = out->p_sys;
    input_thread_t    *p_input = p_sys->p_input;
    input_item_t      *p_item = input_priv(p_input)->p_item;
    es_out_pgrm_t     *p_pgrm;

    /* Find program */
    if( !EsOutIsProgramVisible( out, i_group ) )
        return;
    p_pgrm = EsOutProgramFind( out, i_group );
    if( !p_pgrm )
        return;

    input_item_SetEpgEvent( p_item, p_event );
}

static void EsOutProgramEpg( es_out_t *out, int i_group, const vlc_epg_t *p_epg )
{
    es_out_sys_t      *p_sys = out->p_sys;
    input_thread_t    *p_input = p_sys->p_input;
    input_item_t      *p_item = input_priv(p_input)->p_item;
    es_out_pgrm_t     *p_pgrm;
    char *psz_cat;

    /* Find program */
    if( !EsOutIsProgramVisible( out, i_group ) )
        return;
    p_pgrm = EsOutProgramFind( out, i_group );
    if( !p_pgrm )
        return;

    /* Update info */
    psz_cat = EsOutProgramGetMetaName( p_pgrm );
    msg_Dbg( p_input, "EsOutProgramEpg: number=%d name=%s", i_group, psz_cat );

    /* Merge EPG */
    vlc_epg_t epg;

    epg = *p_epg;
    epg.psz_name = EsOutProgramGetProgramName( p_pgrm );

    input_item_SetEpg( p_item, &epg, p_sys->p_pgrm && (p_epg->i_source_id == p_sys->p_pgrm->i_id) );
    input_SendEventMetaEpg( p_sys->p_input );

    free( epg.psz_name );

    /* Update now playing */
    if( p_epg->b_present && p_pgrm->p_meta &&
       ( p_epg->p_current || p_epg->i_event == 0 ) )
    {
        vlc_meta_SetNowPlaying( p_pgrm->p_meta, NULL );
    }

    vlc_mutex_lock( &p_item->lock );
    for( int i = 0; i < p_item->i_epg; i++ )
    {
        const vlc_epg_t *p_tmp = p_item->pp_epg[i];

        if( p_tmp->b_present && p_tmp->i_source_id == p_pgrm->i_id )
        {
            const char *psz_name = ( p_tmp->p_current ) ? p_tmp->p_current->psz_name : NULL;
            if( !p_pgrm->p_meta )
                p_pgrm->p_meta = vlc_meta_New();
            if( p_pgrm->p_meta )
                vlc_meta_Set( p_pgrm->p_meta, vlc_meta_ESNowPlaying, psz_name );
            break;
        }
    }
    vlc_mutex_unlock( &p_item->lock );

    /* Update selected program input info */
    if( p_pgrm == p_sys->p_pgrm )
    {
        const char *psz_nowplaying = p_pgrm->p_meta ?
                                     vlc_meta_Get( p_pgrm->p_meta, vlc_meta_ESNowPlaying ) : NULL;

        input_item_SetESNowPlaying( input_priv(p_input)->p_item, psz_nowplaying );
        input_SendEventMeta( p_input );

        if( psz_nowplaying )
        {
            input_Control( p_input, INPUT_ADD_INFO, psz_cat,
                vlc_meta_TypeToLocalizedString(vlc_meta_ESNowPlaying), "%s", psz_nowplaying );
        }
        else
        {
            input_Control( p_input, INPUT_DEL_INFO, psz_cat,
                vlc_meta_TypeToLocalizedString(vlc_meta_ESNowPlaying) );
        }
    }

    free( psz_cat );
}

static void EsOutEpgTime( es_out_t *out, int64_t time )
{
    es_out_sys_t      *p_sys = out->p_sys;
    input_thread_t    *p_input = p_sys->p_input;
    input_item_t      *p_item = input_priv(p_input)->p_item;

    input_item_SetEpgTime( p_item, time );
}

static void EsOutProgramUpdateScrambled( es_out_t *p_out, es_out_pgrm_t *p_pgrm )
{
    es_out_sys_t    *p_sys = p_out->p_sys;
    input_thread_t  *p_input = p_sys->p_input;
    bool b_scrambled = false;

    for( int i = 0; i < p_sys->i_es; i++ )
    {
        if( p_sys->es[i]->p_pgrm == p_pgrm && p_sys->es[i]->b_scrambled )
        {
            b_scrambled = true;
            break;
        }
    }
    if( !p_pgrm->b_scrambled == !b_scrambled )
        return;

    p_pgrm->b_scrambled = b_scrambled;
    char *psz_cat = EsOutProgramGetMetaName( p_pgrm );

    if( b_scrambled )
        input_Control( p_input, INPUT_ADD_INFO, psz_cat, _("Scrambled"), _("Yes") );
    else
        input_Control( p_input, INPUT_DEL_INFO, psz_cat, _("Scrambled") );
    free( psz_cat );

    input_SendEventProgramScrambled( p_input, p_pgrm->i_id, b_scrambled );
}

static void EsOutMeta( es_out_t *p_out, const vlc_meta_t *p_meta, const vlc_meta_t *p_program_meta )
{
    es_out_sys_t    *p_sys = p_out->p_sys;
    input_thread_t  *p_input = p_sys->p_input;
    input_item_t *p_item = input_GetItem( p_input );

    vlc_mutex_lock( &p_item->lock );
    if( p_meta )
        vlc_meta_Merge( p_item->p_meta, p_meta );
    vlc_mutex_unlock( &p_item->lock );

    /* Check program meta to not override GROUP_META values */
    if( p_meta && (!p_program_meta || vlc_meta_Get( p_program_meta, vlc_meta_Title ) == NULL) &&
         vlc_meta_Get( p_meta, vlc_meta_Title ) != NULL )
        input_item_SetName( p_item, vlc_meta_Get( p_meta, vlc_meta_Title ) );

    const char *psz_arturl = NULL;
    char *psz_alloc = NULL;

    if( p_program_meta )
        psz_arturl = vlc_meta_Get( p_program_meta, vlc_meta_ArtworkURL );
    if( psz_arturl == NULL && p_meta )
        psz_arturl = vlc_meta_Get( p_meta, vlc_meta_ArtworkURL );

    if( psz_arturl == NULL ) /* restore/favor previously set item art URL */
        psz_arturl = psz_alloc = input_item_GetArtURL( p_item );

    if( psz_arturl != NULL )
        input_item_SetArtURL( p_item, psz_arturl );

    if( psz_arturl != NULL && !strncmp( psz_arturl, "attachment://", 13 ) )
    {   /* Clear art cover if streaming out.
         * FIXME: Why? Remove this when sout gets meta data support. */
        if( input_priv(p_input)->p_sout != NULL )
            input_item_SetArtURL( p_item, NULL );
        else
            input_ExtractAttachmentAndCacheArt( p_input, psz_arturl + 13 );
    }
    free( psz_alloc );

    input_item_SetPreparsed( p_item, true );

    input_SendEventMeta( p_input );
    /* TODO handle sout meta ? */
}

static void EsOutGlobalMeta( es_out_t *p_out, const vlc_meta_t *p_meta )
{
    es_out_sys_t    *p_sys = p_out->p_sys;
    EsOutMeta( p_out, p_meta,
               (p_sys->p_pgrm && p_sys->p_pgrm->p_meta) ? p_sys->p_pgrm->p_meta : NULL );
}

static es_out_id_t *EsOutAddSlave( es_out_t *out, const es_format_t *fmt, es_out_id_t *p_master )
{
    es_out_sys_t      *p_sys = out->p_sys;
    input_thread_t    *p_input = p_sys->p_input;

    if( fmt->i_group < 0 )
    {
        msg_Err( p_input, "invalid group number" );
        return NULL;
    }

    es_out_id_t   *es = malloc( sizeof( *es ) );
    es_out_pgrm_t *p_pgrm;
    int i;

    if( !es )
        return NULL;

    vlc_mutex_lock( &p_sys->lock );

    /* Search the program */
    p_pgrm = EsOutProgramFind( out, fmt->i_group );
    if( !p_pgrm )
    {
        vlc_mutex_unlock( &p_sys->lock );
        free( es );
        return NULL;
    }

    /* Increase ref count for program */
    p_pgrm->i_es++;

    /* Set up ES */
    es->p_pgrm = p_pgrm;
    es_format_Copy( &es->fmt, fmt );
    if( es->fmt.i_id < 0 )
        es->fmt.i_id = p_sys->i_id;
    if( !es->fmt.i_original_fourcc )
        es->fmt.i_original_fourcc = es->fmt.i_codec;

    es->i_id = es->fmt.i_id;
    es->i_meta_id = p_sys->i_id++; /* always incremented */
    es->b_scrambled = false;

    switch( es->fmt.i_cat )
    {
    case AUDIO_ES:
    {
        es->fmt.i_codec = vlc_fourcc_GetCodecAudio( es->fmt.i_codec,
                                                    es->fmt.audio.i_bitspersample );
        es->i_channel = p_sys->audio.i_count++;

        audio_replay_gain_t rg;
        memset( &rg, 0, sizeof(rg) );
        vlc_mutex_lock( &input_priv(p_input)->p_item->lock );
        vlc_audio_replay_gain_MergeFromMeta( &rg, input_priv(p_input)->p_item->p_meta );
        vlc_mutex_unlock( &input_priv(p_input)->p_item->lock );

        for( i = 0; i < AUDIO_REPLAY_GAIN_MAX; i++ )
        {
            if( !es->fmt.audio_replay_gain.pb_peak[i] )
            {
                es->fmt.audio_replay_gain.pb_peak[i] = rg.pb_peak[i];
                es->fmt.audio_replay_gain.pf_peak[i] = rg.pf_peak[i];
            }
            if( !es->fmt.audio_replay_gain.pb_gain[i] )
            {
                es->fmt.audio_replay_gain.pb_gain[i] = rg.pb_gain[i];
                es->fmt.audio_replay_gain.pf_gain[i] = rg.pf_gain[i];
            }
        }
        break;
    }

    case VIDEO_ES:
        es->fmt.i_codec = vlc_fourcc_GetCodec( es->fmt.i_cat, es->fmt.i_codec );
        es->i_channel = p_sys->video.i_count++;

        if( !es->fmt.video.i_visible_width || !es->fmt.video.i_visible_height )
        {
            es->fmt.video.i_visible_width = es->fmt.video.i_width;
            es->fmt.video.i_visible_height = es->fmt.video.i_height;
        }

        if( es->fmt.video.i_frame_rate && es->fmt.video.i_frame_rate_base )
            vlc_ureduce( &es->fmt.video.i_frame_rate,
                         &es->fmt.video.i_frame_rate_base,
                         es->fmt.video.i_frame_rate,
                         es->fmt.video.i_frame_rate_base, 0 );
        break;

    case SPU_ES:
        es->fmt.i_codec = vlc_fourcc_GetCodec( es->fmt.i_cat, es->fmt.i_codec );
        es->i_channel = p_sys->sub.i_count++;
        break;

    default:
        es->i_channel = 0;
        break;
    }
    es->psz_language = LanguageGetName( es->fmt.psz_language ); /* remember so we only need to do it once */
    es->psz_language_code = LanguageGetCode( es->fmt.psz_language );
    es->p_dec = NULL;
    es->p_dec_record = NULL;
    es->cc.type = 0;
    es->cc.i_bitmap = 0;
    es->p_master = p_master;
    es->i_pts_level = VLC_TS_INVALID;

    TAB_APPEND( p_sys->i_es, p_sys->es, es );

    if( es->p_pgrm == p_sys->p_pgrm )
        EsOutESVarUpdate( out, es, false );

    EsOutUpdateInfo( out, es, &es->fmt, NULL );
    EsOutSelect( out, es, false );

    if( es->b_scrambled )
        EsOutProgramUpdateScrambled( out, es->p_pgrm );

    vlc_mutex_unlock( &p_sys->lock );

    return es;
}

/* EsOutAdd:
 *  Add an es_out
 */
static es_out_id_t *EsOutAdd( es_out_t *out, const es_format_t *fmt )
{
    return EsOutAddSlave( out, fmt, NULL );
}

static bool EsIsSelected( es_out_id_t *es )
{
    if( es->p_master )
    {
        bool b_decode = false;
        if( es->p_master->p_dec )
        {
            int i_channel = EsOutGetClosedCaptionsChannel( &es->fmt );
            input_DecoderGetCcState( es->p_master->p_dec, es->fmt.i_codec,
                                     i_channel, &b_decode );
        }
        return b_decode;
    }
    else
    {
        return es->p_dec != NULL;
    }
}
static void EsCreateDecoder( es_out_t *out, es_out_id_t *p_es )
{
    es_out_sys_t   *p_sys = out->p_sys;
    input_thread_t *p_input = p_sys->p_input;

    p_es->p_dec = input_DecoderNew( p_input, &p_es->fmt, p_es->p_pgrm->p_clock, input_priv(p_input)->p_sout );
    if( p_es->p_dec )
    {
        if( p_sys->b_buffering )
            input_DecoderStartWait( p_es->p_dec );

        if( !p_es->p_master && p_sys->p_sout_record )
        {
            p_es->p_dec_record = input_DecoderNew( p_input, &p_es->fmt, p_es->p_pgrm->p_clock, p_sys->p_sout_record );
            if( p_es->p_dec_record && p_sys->b_buffering )
                input_DecoderStartWait( p_es->p_dec_record );
        }
    }

    EsOutDecoderChangeDelay( out, p_es );
}
static void EsDestroyDecoder( es_out_t *out, es_out_id_t *p_es )
{
    VLC_UNUSED(out);

    if( !p_es->p_dec )
        return;

    input_DecoderDelete( p_es->p_dec );
    p_es->p_dec = NULL;

    if( p_es->p_dec_record )
    {
        input_DecoderDelete( p_es->p_dec_record );
        p_es->p_dec_record = NULL;
    }
}

static void EsSelect( es_out_t *out, es_out_id_t *es )
{
    es_out_sys_t   *p_sys = out->p_sys;
    input_thread_t *p_input = p_sys->p_input;

    if( EsIsSelected( es ) )
    {
        msg_Warn( p_input, "ES 0x%x is already selected", es->i_id );
        return;
    }

    if( es->p_master )
    {
        int i_channel;
        if( !es->p_master->p_dec )
            return;

        i_channel = EsOutGetClosedCaptionsChannel( &es->fmt );

        if( i_channel == -1 ||
            input_DecoderSetCcState( es->p_master->p_dec, es->fmt.i_codec,
                                     i_channel, true ) )
            return;
    }
    else
    {
        const bool b_sout = input_priv(p_input)->p_sout != NULL;
        if( es->fmt.i_cat == VIDEO_ES || es->fmt.i_cat == SPU_ES )
        {
            if( !var_GetBool( p_input, b_sout ? "sout-video" : "video" ) )
            {
                msg_Dbg( p_input, "video is disabled, not selecting ES 0x%x",
                         es->i_id );
                return;
            }
        }
        else if( es->fmt.i_cat == AUDIO_ES )
        {
            if( !var_GetBool( p_input, b_sout ? "sout-audio" : "audio" ) )
            {
                msg_Dbg( p_input, "audio is disabled, not selecting ES 0x%x",
                         es->i_id );
                return;
            }
        }
        if( es->fmt.i_cat == SPU_ES )
        {
            if( !var_GetBool( p_input, b_sout ? "sout-spu" : "spu" ) )
            {
                msg_Dbg( p_input, "spu is disabled, not selecting ES 0x%x",
                         es->i_id );
                return;
            }
        }

        EsCreateDecoder( out, es );

        if( es->p_dec == NULL || es->p_pgrm != p_sys->p_pgrm )
            return;
    }

    /* Mark it as selected */
    input_SendEventEsSelect( p_input, es->fmt.i_cat, es->i_id );
    input_SendEventTeletextSelect( p_input, EsFmtIsTeletext( &es->fmt ) ? es->i_id : -1 );
}

static void EsDeleteCCChannels( es_out_t *out, es_out_id_t *parent )
{
    es_out_sys_t   *p_sys = out->p_sys;
    input_thread_t *p_input = p_sys->p_input;

    if( parent->cc.type == 0 )
        return;

    const int i_spu_id = var_GetInteger( p_input, "spu-es");

    uint64_t i_bitmap = parent->cc.i_bitmap;
    for( int i = 0; i_bitmap > 0; i++, i_bitmap >>= 1 )
    {
        if( (i_bitmap & 1) == 0 || !parent->cc.pp_es[i] )
            continue;

        if( i_spu_id == parent->cc.pp_es[i]->i_id )
        {
            /* Force unselection of the CC */
            input_SendEventEsSelect( p_input, SPU_ES, -1 );
        }
        EsOutDel( out, parent->cc.pp_es[i] );
    }

    parent->cc.i_bitmap = 0;
    parent->cc.type = 0;
}

static void EsUnselect( es_out_t *out, es_out_id_t *es, bool b_update )
{
    es_out_sys_t   *p_sys = out->p_sys;
    input_thread_t *p_input = p_sys->p_input;

    if( !EsIsSelected( es ) )
    {
        msg_Warn( p_input, "ES 0x%x is already unselected", es->i_id );
        return;
    }

    if( es->p_master )
    {
        if( es->p_master->p_dec )
        {
            int i_channel = EsOutGetClosedCaptionsChannel( &es->fmt );
            if( i_channel != -1 )
                input_DecoderSetCcState( es->p_master->p_dec, es->fmt.i_codec,
                                         i_channel, false );
        }
    }
    else
    {
        EsDeleteCCChannels( out, es );
        EsDestroyDecoder( out, es );
    }

    if( !b_update )
        return;

    /* Mark it as unselected */
    input_SendEventEsSelect( p_input, es->fmt.i_cat, -1 );
    if( EsFmtIsTeletext( &es->fmt ) )
        input_SendEventTeletextSelect( p_input, -1 );
}

/**
 * Select an ES given the current mode
 * XXX: you need to take a the lock before (stream.stream_lock)
 *
 * \param out The es_out structure
 * \param es es_out_id structure
 * \param b_force ...
 * \return nothing
 */
static void EsOutSelect( es_out_t *out, es_out_id_t *es, bool b_force )
{
    es_out_sys_t      *p_sys = out->p_sys;
    es_out_es_props_t *p_esprops = GetPropsByCat( p_sys, es->fmt.i_cat );

    if( !p_sys->b_active ||
        ( !b_force && es->fmt.i_priority < ES_PRIORITY_SELECTABLE_MIN ) )
    {
        return;
    }

    bool b_auto_unselect = p_esprops && p_sys->i_mode == ES_OUT_MODE_AUTO &&
                           p_esprops->e_policy == ES_OUT_ES_POLICY_EXCLUSIVE &&
                           p_esprops->p_main_es && p_esprops->p_main_es != es;

    if( p_sys->i_mode == ES_OUT_MODE_ALL || b_force )
    {
        if( !EsIsSelected( es ) )
        {
            if( b_auto_unselect )
                EsUnselect( out, p_esprops->p_main_es, false );

            EsSelect( out, es );
        }
    }
    else if( p_sys->i_mode == ES_OUT_MODE_PARTIAL )
    {
        char *prgms = var_GetNonEmptyString( p_sys->p_input, "programs" );
        if( prgms != NULL )
        {
            char *buf;

            for ( const char *prgm = strtok_r( prgms, ",", &buf );
                  prgm != NULL;
                  prgm = strtok_r( NULL, ",", &buf ) )
            {
                if( atoi( prgm ) == es->p_pgrm->i_id || b_force )
                {
                    if( !EsIsSelected( es ) )
                        EsSelect( out, es );
                    break;
                }
            }
            free( prgms );
        }
    }
    else if( p_sys->i_mode == ES_OUT_MODE_AUTO )
    {
        const es_out_id_t *wanted_es = NULL;

        if( es->p_pgrm != p_sys->p_pgrm || !p_esprops )
            return;

        /* user designated by ID ES have higher prio than everything */
        if ( p_esprops->i_id >= 0 )
        {
            if( es->i_id == p_esprops->i_id )
                wanted_es = es;
        }
        /* then per pos */
        else if( p_esprops->i_channel >= 0 )
        {
            if( p_esprops->i_channel == es->i_channel )
                wanted_es = es;
        }
        else if( p_esprops->ppsz_language )
        {
            /* If not deactivated */
            const int i_stop_idx = LanguageArrayIndex( p_esprops->ppsz_language, "none" );
            {
                int current_es_idx = ( p_esprops->p_main_es == NULL ) ? -1 :
                        LanguageArrayIndex( p_esprops->ppsz_language,
                                            p_esprops->p_main_es->psz_language_code );
                int es_idx = LanguageArrayIndex( p_esprops->ppsz_language,
                                                 es->psz_language_code );
                if( es_idx >= 0 && (i_stop_idx < 0 || i_stop_idx > es_idx) )
                {
                    /* Only select the language if it's in the list */
                    if( p_esprops->p_main_es == NULL ||
                        current_es_idx < 0 || /* current es was not selected by lang prefs */
                        es_idx < current_es_idx || /* current es has lower lang prio */
                        (  es_idx == current_es_idx && /* lang is same, but es has higher prio */
                           p_esprops->p_main_es->fmt.i_priority < es->fmt.i_priority ) )
                    {
                        wanted_es = es;
                    }
                }
                /* We did not find a language matching our prefs */
                else if( i_stop_idx < 0 ) /* If not fallback disabled by 'none' */
                {
                    /* Select if asked by demuxer */
                    if( current_es_idx < 0 ) /* No es is currently selected by lang pref */
                    {
                        /* If demux has specified a track */
                        if( p_esprops->i_demux_id >= 0 && es->i_id == p_esprops->i_demux_id )
                        {
                            wanted_es = es;
                        }
                        /* Otherwise, fallback by priority */
                        else if( p_esprops->p_main_es == NULL ||
                                 es->fmt.i_priority > p_esprops->p_main_es->fmt.i_priority )
                        {
                            if( p_esprops->b_autoselect )
                                wanted_es = es;
                        }
                    }
                }
            }

        }
        /* If there is no user preference, select the default subtitle
         * or adapt by ES priority */
        else if( p_esprops->i_demux_id >= 0 && es->i_id == p_esprops->i_demux_id )
        {
            wanted_es = es;
        }
        else if( p_esprops->p_main_es == NULL ||
                 es->fmt.i_priority > p_esprops->p_main_es->fmt.i_priority )
        {
            if( p_esprops->b_autoselect )
                wanted_es = es;
        }

        if( wanted_es == es && !EsIsSelected( es ) )
        {
            if( b_auto_unselect )
                EsUnselect( out, p_esprops->p_main_es, false );

            EsSelect( out, es );
        }
    }

    /* FIXME TODO handle priority here */
    if( p_esprops && p_sys->i_mode == ES_OUT_MODE_AUTO && EsIsSelected( es ) )
        p_esprops->p_main_es = es;
}

static void EsOutCreateCCChannels( es_out_t *out, vlc_fourcc_t codec, uint64_t i_bitmap,
                                   const char *psz_descfmt, es_out_id_t *parent )
{
    es_out_sys_t   *p_sys = out->p_sys;
    input_thread_t *p_input = p_sys->p_input;

    /* Only one type of captions is allowed ! */
    if( parent->cc.type && parent->cc.type != codec )
        return;

    uint64_t i_existingbitmap = parent->cc.i_bitmap;
    for( int i = 0; i_bitmap > 0; i++, i_bitmap >>= 1, i_existingbitmap >>= 1 )
    {
        es_format_t fmt;

        if( (i_bitmap & 1) == 0 || (i_existingbitmap & 1) )
            continue;

        msg_Dbg( p_input, "Adding CC track %d for es[%d]", 1+i, parent->i_id );

        es_format_Init( &fmt, SPU_ES, codec );
        fmt.subs.cc.i_channel = i;
        fmt.i_group = parent->fmt.i_group;
        if( asprintf( &fmt.psz_description, psz_descfmt, 1 + i ) == -1 )
            fmt.psz_description = NULL;

        es_out_id_t **pp_es = &parent->cc.pp_es[i];
        *pp_es = EsOutAddSlave( out, &fmt, parent );
        es_format_Clean( &fmt );

        /* */
        parent->cc.i_bitmap |= (1ULL << i);
        parent->cc.type = codec;

        /* Enable if user specified on command line */
        if (p_sys->sub.i_channel == i)
            EsOutSelect(out, *pp_es, true);
    }
}

/**
 * Send a block for the given es_out
 *
 * \param out the es_out to send from
 * \param es the es_out_id
 * \param p_block the data block to send
 */
static int EsOutSend( es_out_t *out, es_out_id_t *es, block_t *p_block )
{
    es_out_sys_t   *p_sys = out->p_sys;
    input_thread_t *p_input = p_sys->p_input;

    assert( p_block->p_next == NULL );

    if( libvlc_stats( p_input ) )
    {
        uint64_t i_total;

        vlc_mutex_lock( &input_priv(p_input)->counters.counters_lock );
        stats_Update( input_priv(p_input)->counters.p_demux_read,
                      p_block->i_buffer, &i_total );
        stats_Update( input_priv(p_input)->counters.p_demux_bitrate, i_total, NULL );

        /* Update number of corrupted data packats */
        if( p_block->i_flags & BLOCK_FLAG_CORRUPTED )
        {
            stats_Update( input_priv(p_input)->counters.p_demux_corrupted, 1, NULL );
        }
        /* Update number of discontinuities */
        if( p_block->i_flags & BLOCK_FLAG_DISCONTINUITY )
        {
            stats_Update( input_priv(p_input)->counters.p_demux_discontinuity, 1, NULL );
        }
        vlc_mutex_unlock( &input_priv(p_input)->counters.counters_lock );
    }

    vlc_mutex_lock( &p_sys->lock );

    /* Mark preroll blocks */
    if( p_sys->i_preroll_end >= 0 )
    {
        int64_t i_date = p_block->i_pts;
        if( p_block->i_pts <= VLC_TS_INVALID )
            i_date = p_block->i_dts;

        /* In some cases, the demuxer sends non dated packets.
           We use interpolation, previous, or pcr value to compare with
           preroll target timestamp */
        if( i_date == VLC_TS_INVALID )
        {
            if( es->i_pts_level != VLC_TS_INVALID )
                i_date = es->i_pts_level;
            else if( es->p_pgrm->i_last_pcr != VLC_TS_INVALID )
                i_date = es->p_pgrm->i_last_pcr;
        }

        if( i_date != VLC_TS_INVALID )
            es->i_pts_level = i_date + p_block->i_length;

        /* If i_date is still invalid (first/all non dated), expect to be in preroll */

        if( i_date == VLC_TS_INVALID ||
            es->i_pts_level < p_sys->i_preroll_end )
            p_block->i_flags |= BLOCK_FLAG_PREROLL;
    }

    if( !es->p_dec )
    {
        block_Release( p_block );
        vlc_mutex_unlock( &p_sys->lock );
        return VLC_SUCCESS;
    }

    /* Check for sout mode */
    if( input_priv(p_input)->p_sout )
    {
        /* FIXME review this, proper lock may be missing */
        if( input_priv(p_input)->p_sout->i_out_pace_nocontrol > 0 &&
            input_priv(p_input)->b_out_pace_control )
        {
            msg_Dbg( p_input, "switching to sync mode" );
            input_priv(p_input)->b_out_pace_control = false;
        }
        else if( input_priv(p_input)->p_sout->i_out_pace_nocontrol <= 0 &&
                 !input_priv(p_input)->b_out_pace_control )
        {
            msg_Dbg( p_input, "switching to async mode" );
            input_priv(p_input)->b_out_pace_control = true;
        }
    }

    /* Decode */
    if( es->p_dec_record )
    {
        block_t *p_dup = block_Duplicate( p_block );
        if( p_dup )
            input_DecoderDecode( es->p_dec_record, p_dup,
                                 input_priv(p_input)->b_out_pace_control );
    }
    input_DecoderDecode( es->p_dec, p_block,
                         input_priv(p_input)->b_out_pace_control );

    es_format_t fmt_dsc;
    vlc_meta_t  *p_meta_dsc;
    if( input_DecoderHasFormatChanged( es->p_dec, &fmt_dsc, &p_meta_dsc ) )
    {
        EsOutUpdateInfo( out, es, &fmt_dsc, p_meta_dsc );

        es_format_Clean( &fmt_dsc );
        if( p_meta_dsc )
            vlc_meta_Delete( p_meta_dsc );
    }

    /* Check CC status */
    decoder_cc_desc_t desc;

    input_DecoderGetCcDesc( es->p_dec, &desc );
    if( var_InheritInteger( p_input, "captions" ) == 708 )
        EsOutCreateCCChannels( out, VLC_CODEC_CEA708, desc.i_708_channels,
                               _("DTVCC Closed captions %u"), es );
    EsOutCreateCCChannels( out, VLC_CODEC_CEA608, desc.i_608_channels,
                           _("Closed captions %u"), es );

    vlc_mutex_unlock( &p_sys->lock );

    return VLC_SUCCESS;
}

/*****************************************************************************
 * EsOutDel:
 *****************************************************************************/
static void EsOutDel( es_out_t *out, es_out_id_t *es )
{
    es_out_sys_t *p_sys = out->p_sys;
    bool b_reselect = false;
    int i;

    vlc_mutex_lock( &p_sys->lock );

    es_out_es_props_t *p_esprops = GetPropsByCat( p_sys, es->fmt.i_cat );

    /* We don't try to reselect */
    if( es->p_dec )
    {   /* FIXME: This might hold the ES output caller (i.e. the demux), and
         * the corresponding thread (typically the input thread), for a little
         * bit too long if the ES is deleted in the middle of a stream. */
        input_DecoderDrain( es->p_dec );
        while( !input_Stopped(p_sys->p_input) && !p_sys->b_buffering )
        {
            if( input_DecoderIsEmpty( es->p_dec ) &&
                ( !es->p_dec_record || input_DecoderIsEmpty( es->p_dec_record ) ))
                break;
            /* FIXME there should be a way to have auto deleted es, but there will be
             * a problem when another codec of the same type is created (mainly video) */
            msleep( 20*1000 );
        }
        EsUnselect( out, es, es->p_pgrm == p_sys->p_pgrm );
    }

    if( es->p_pgrm == p_sys->p_pgrm )
        EsOutESVarUpdate( out, es, true );

    EsDeleteInfo( out, es );

    TAB_REMOVE( p_sys->i_es, p_sys->es, es );

    /* Update program */
    es->p_pgrm->i_es--;
    if( es->p_pgrm->i_es == 0 )
        msg_Dbg( p_sys->p_input, "Program doesn't contain anymore ES" );

    if( es->b_scrambled )
        EsOutProgramUpdateScrambled( out, es->p_pgrm );

    /* */
    if( p_esprops )
    {
        if( p_esprops->p_main_es == es )
        {
            b_reselect = true;
            p_esprops->p_main_es = NULL;
        }
        p_esprops->i_count--;
    }

    /* Re-select another track when needed */
    if( b_reselect )
    {
        for( i = 0; i < p_sys->i_es; i++ )
        {
            if( es->fmt.i_cat == p_sys->es[i]->fmt.i_cat )
            {
                if( EsIsSelected(p_sys->es[i]) )
                {
                    input_SendEventEsSelect( p_sys->p_input, es->fmt.i_cat, p_sys->es[i]->i_id );
                    if( p_esprops->p_main_es == NULL )
                        p_esprops->p_main_es = p_sys->es[i];
                }
                else
                    EsOutSelect( out, p_sys->es[i], false );
            }
        }
    }

    free( es->psz_language );
    free( es->psz_language_code );

    es_format_Clean( &es->fmt );

    vlc_mutex_unlock( &p_sys->lock );

    free( es );
}

/**
 * Control query handler
 *
 * \param out the es_out to control
 * \param i_query A es_out query as defined in include/ninput.h
 * \param args a variable list of arguments for the query
 * \return VLC_SUCCESS or an error code
 */
static int EsOutControlLocked( es_out_t *out, int i_query, va_list args )
{
    es_out_sys_t *p_sys = out->p_sys;

    switch( i_query )
    {
    case ES_OUT_SET_ES_STATE:
    {
        es_out_id_t *es = va_arg( args, es_out_id_t * );
        bool b = va_arg( args, int );
        if( b && !EsIsSelected( es ) )
        {
            EsSelect( out, es );
            return EsIsSelected( es ) ? VLC_SUCCESS : VLC_EGENERIC;
        }
        else if( !b && EsIsSelected( es ) )
        {
            EsUnselect( out, es, es->p_pgrm == p_sys->p_pgrm );
            return VLC_SUCCESS;
        }
        return VLC_SUCCESS;
    }

    case ES_OUT_GET_ES_STATE:
    {
        es_out_id_t *es = va_arg( args, es_out_id_t * );
        bool *pb = va_arg( args, bool * );

        *pb = EsIsSelected( es );
        return VLC_SUCCESS;
    }

    case ES_OUT_SET_ES_CAT_POLICY:
    {
        enum es_format_category_e i_cat = va_arg( args, enum es_format_category_e );
        enum es_out_policy_e i_pol = va_arg( args, enum es_out_policy_e );
        es_out_es_props_t *p_esprops = GetPropsByCat( p_sys, i_cat );
        if( p_esprops == NULL )
            return VLC_EGENERIC;
        p_esprops->e_policy = i_pol;
        return VLC_SUCCESS;
    }

    case ES_OUT_GET_GROUP_FORCED:
    {
        int *pi_group = va_arg( args, int * );
        *pi_group = p_sys->i_group_id;
        return VLC_SUCCESS;
    }

    case ES_OUT_SET_MODE:
    {
        const int i_mode = va_arg( args, int );
        assert( i_mode == ES_OUT_MODE_NONE || i_mode == ES_OUT_MODE_ALL ||
                i_mode == ES_OUT_MODE_AUTO || i_mode == ES_OUT_MODE_PARTIAL ||
                i_mode == ES_OUT_MODE_END );

        if( i_mode != ES_OUT_MODE_NONE && !p_sys->b_active && p_sys->i_es > 0 )
        {
            /* XXX Terminate vout if there are tracks but no video one.
             * This one is not mandatory but is he earliest place where it
             * can be done */
            int i;
            for( i = 0; i < p_sys->i_es; i++ )
            {
                es_out_id_t *p_es = p_sys->es[i];
                if( p_es->fmt.i_cat == VIDEO_ES )
                    break;
            }
            if( i >= p_sys->i_es )
                input_resource_TerminateVout( input_priv(p_sys->p_input)->p_resource );
        }
        p_sys->b_active = i_mode != ES_OUT_MODE_NONE;
        p_sys->i_mode = i_mode;

        /* Reapply policy mode */
        for( int i = 0; i < p_sys->i_es; i++ )
        {
            if( EsIsSelected( p_sys->es[i] ) )
                EsUnselect( out, p_sys->es[i],
                            p_sys->es[i]->p_pgrm == p_sys->p_pgrm );
        }
        for( int i = 0; i < p_sys->i_es; i++ )
            EsOutSelect( out, p_sys->es[i], false );
        if( i_mode == ES_OUT_MODE_END )
            EsOutTerminate( out );
        return VLC_SUCCESS;
    }

    case ES_OUT_SET_ES:
    case ES_OUT_RESTART_ES:
    {
#define IGNORE_ES DATA_ES
        es_out_id_t *es = va_arg( args, es_out_id_t * );

        enum es_format_category_e i_cat;
        if( es == NULL )
            i_cat = UNKNOWN_ES;
        else if( es == es_cat + AUDIO_ES )
            i_cat = AUDIO_ES;
        else if( es == es_cat + VIDEO_ES )
            i_cat = VIDEO_ES;
        else if( es == es_cat + SPU_ES )
            i_cat = SPU_ES;
        else
            i_cat = IGNORE_ES;

        for( int i = 0; i < p_sys->i_es; i++ )
        {
            if( i_cat == IGNORE_ES )
            {
                if( es == p_sys->es[i] )
                {
                    if( i_query == ES_OUT_RESTART_ES && p_sys->es[i]->p_dec )
                    {
                        EsDestroyDecoder( out, p_sys->es[i] );
                        EsCreateDecoder( out, p_sys->es[i] );
                    }
                    else if( i_query == ES_OUT_SET_ES )
                    {
                        EsOutSelect( out, es, true );
                    }
                    break;
                }
            }
            else
            {
                if( i_cat == UNKNOWN_ES || p_sys->es[i]->fmt.i_cat == i_cat )
                {
                    if( EsIsSelected( p_sys->es[i] ) )
                    {
                        if( i_query == ES_OUT_RESTART_ES )
                        {
                            if( p_sys->es[i]->p_dec )
                            {
                                EsDestroyDecoder( out, p_sys->es[i] );
                                EsCreateDecoder( out, p_sys->es[i] );
                            }
                        }
                        else
                        {
                            EsUnselect( out, p_sys->es[i],
                                        p_sys->es[i]->p_pgrm == p_sys->p_pgrm );
                        }
                    }
                }
            }
        }
        return VLC_SUCCESS;
    }
    case ES_OUT_STOP_ALL_ES:
    {
        int *selected_es = vlc_alloc(p_sys->i_es + 1, sizeof(int));
        if (!selected_es)
            return VLC_ENOMEM;
        selected_es[0] = p_sys->i_es;
        for( int i = 0; i < p_sys->i_es; i++ )
        {
            if( EsIsSelected( p_sys->es[i] ) )
            {
                EsDestroyDecoder( out, p_sys->es[i] );
                selected_es[i + 1] = p_sys->es[i]->i_id;
            }
            else
                selected_es[i + 1] = -1;
        }
        *va_arg( args, void **) = selected_es;
        return VLC_SUCCESS;
    }
    case ES_OUT_START_ALL_ES:
    {
        int *selected_es = va_arg( args, void * );
        int count = selected_es[0];
        for( int i = 0; i < count; ++i )
        {
            int i_id = selected_es[i + 1];
            if( i_id != -1 )
            {
                es_out_id_t *p_es = EsOutGetFromID( out, i_id );
                EsCreateDecoder( out, p_es );
            }
        }
        free(selected_es);
        return VLC_SUCCESS;
    }

    case ES_OUT_SET_ES_DEFAULT:
    {
        es_out_id_t *es = va_arg( args, es_out_id_t * );

        if( es == NULL )
        {
            /*p_sys->i_default_video_id = -1;*/
            /*p_sys->i_default_audio_id = -1;*/
            p_sys->sub.i_demux_id = -1;
        }
        else if( es == es_cat + AUDIO_ES )
        {
            /*p_sys->i_default_video_id = -1;*/
        }
        else if( es == es_cat + VIDEO_ES )
        {
            /*p_sys->i_default_audio_id = -1;*/
        }
        else if( es == es_cat + SPU_ES )
        {
            p_sys->sub.i_demux_id = -1;
        }
        else
        {
            /*if( es->fmt.i_cat == VIDEO_ES )
                p_sys->i_default_video_id = es->i_id;
            else
            if( es->fmt.i_cat == AUDIO_ES )
                p_sys->i_default_audio_id = es->i_id;
            else*/
            if( es->fmt.i_cat == SPU_ES )
                p_sys->sub.i_demux_id = es->i_id;
        }
        return VLC_SUCCESS;
    }

    case ES_OUT_SET_PCR:
    case ES_OUT_SET_GROUP_PCR:
    {
        es_out_pgrm_t *p_pgrm = NULL;
        int            i_group = 0;
        int64_t        i_pcr;

        /* Search program */
        if( i_query == ES_OUT_SET_PCR )
        {
            p_pgrm = p_sys->p_pgrm;
            if( !p_pgrm )
                p_pgrm = EsOutProgramAdd( out, i_group );   /* Create it */
        }
        else
        {
            i_group = va_arg( args, int );
            p_pgrm = EsOutProgramFind( out, i_group );
        }
        if( !p_pgrm )
            return VLC_EGENERIC;

        i_pcr = va_arg( args, int64_t );
        if( i_pcr <= VLC_TS_INVALID )
        {
            msg_Err( p_sys->p_input, "Invalid PCR value in ES_OUT_SET_(GROUP_)PCR !" );
            return VLC_EGENERIC;
        }

        p_pgrm->i_last_pcr = i_pcr;

        /* TODO do not use mdate() but proper stream acquisition date */
        bool b_late;
        input_clock_Update( p_pgrm->p_clock, VLC_OBJECT(p_sys->p_input),
                            &b_late,
                            input_priv(p_sys->p_input)->b_can_pace_control || p_sys->b_buffering,
                            EsOutIsExtraBufferingAllowed( out ),
                            i_pcr, mdate() );

        if( !p_sys->p_pgrm )
            return VLC_SUCCESS;

        if( p_sys->b_buffering )
        {
            /* Check buffering state on master clock update */
            EsOutDecodersStopBuffering( out, false );
        }
        else if( p_pgrm == p_sys->p_pgrm )
        {
            if( b_late && ( !input_priv(p_sys->p_input)->p_sout ||
                            !input_priv(p_sys->p_input)->b_out_pace_control ) )
            {
                const mtime_t i_pts_delay_base = p_sys->i_pts_delay - p_sys->i_pts_jitter;
                mtime_t i_pts_delay = input_clock_GetJitter( p_pgrm->p_clock );

                /* Avoid dangerously high value */
                const mtime_t i_jitter_max = INT64_C(1000) * var_InheritInteger( p_sys->p_input, "clock-jitter" );
                if( i_pts_delay > __MIN( i_pts_delay_base + i_jitter_max, INPUT_PTS_DELAY_MAX ) )
                {
                    msg_Err( p_sys->p_input,
                             "ES_OUT_SET_(GROUP_)PCR  is called too late (jitter of %d ms ignored)",
                             (int)(i_pts_delay - i_pts_delay_base) / 1000 );
                    i_pts_delay = p_sys->i_pts_delay;

                    /* reset clock */
                    for( int i = 0; i < p_sys->i_pgrm; i++ )
                      input_clock_Reset( p_sys->pgrm[i]->p_clock );
                }
                else
                {
                    msg_Err( p_sys->p_input,
                             "ES_OUT_SET_(GROUP_)PCR  is called too late (pts_delay increased to %d ms)",
                             (int)(i_pts_delay/1000) );

                    /* Force a rebufferization when we are too late */

                    /* It is not really good, as we throw away already buffered data
                     * TODO have a mean to correctly reenter bufferization */
                    es_out_Control( out, ES_OUT_RESET_PCR );
                }

                es_out_SetJitter( out, i_pts_delay_base, i_pts_delay - i_pts_delay_base, p_sys->i_cr_average );
            }
        }
        return VLC_SUCCESS;
    }

    case ES_OUT_RESET_PCR:
        msg_Dbg( p_sys->p_input, "ES_OUT_RESET_PCR called" );
        EsOutChangePosition( out );
        return VLC_SUCCESS;

    case ES_OUT_SET_GROUP:
    {
        int i = va_arg( args, int );
        for( int j = 0; j < p_sys->i_pgrm; j++ )
        {
            es_out_pgrm_t *p_pgrm = p_sys->pgrm[j];
            if( p_pgrm->i_id == i )
            {
                EsOutProgramSelect( out, p_pgrm );
                return VLC_SUCCESS;
            }
        }
        return VLC_EGENERIC;
    }

    case ES_OUT_SET_ES_FMT:
    {
        /* This ain't pretty but is need by some demuxers (eg. Ogg )
         * to update the p_extra data */
        es_out_id_t *es = va_arg( args, es_out_id_t * );
        es_format_t *p_fmt = va_arg( args, es_format_t * );
        if( es == NULL )
            return VLC_EGENERIC;

        es_format_Clean( &es->fmt );
        es_format_Copy( &es->fmt, p_fmt );

        if( es->p_dec )
        {
            EsDestroyDecoder( out, es );
            EsCreateDecoder( out, es );
        }

        return VLC_SUCCESS;
    }

    case ES_OUT_SET_ES_SCRAMBLED_STATE:
    {
        es_out_id_t *es = va_arg( args, es_out_id_t * );
        bool b_scrambled = (bool)va_arg( args, int );

        if( !es->b_scrambled != !b_scrambled )
        {
            es->b_scrambled = b_scrambled;
            EsOutProgramUpdateScrambled( out, es->p_pgrm );
        }
        return VLC_SUCCESS;
    }

    case ES_OUT_SET_NEXT_DISPLAY_TIME:
    {
        const int64_t i_date = va_arg( args, int64_t );

        if( i_date < 0 )
            return VLC_EGENERIC;

        p_sys->i_preroll_end = i_date;

        return VLC_SUCCESS;
    }
    case ES_OUT_SET_GROUP_META:
    {
        int i_group = va_arg( args, int );
        const vlc_meta_t *p_meta = va_arg( args, const vlc_meta_t * );

        EsOutProgramMeta( out, i_group, p_meta );
        return VLC_SUCCESS;
    }
    case ES_OUT_SET_GROUP_EPG:
    {
        int i_group = va_arg( args, int );
        const vlc_epg_t *p_epg = va_arg( args, const vlc_epg_t * );

        EsOutProgramEpg( out, i_group, p_epg );
        return VLC_SUCCESS;
    }
    case ES_OUT_SET_GROUP_EPG_EVENT:
    {
        int i_group = va_arg( args, int );
        const vlc_epg_event_t *p_evt = va_arg( args, const vlc_epg_event_t * );

        EsOutProgramEpgEvent( out, i_group, p_evt );
        return VLC_SUCCESS;
    }
    case ES_OUT_SET_EPG_TIME:
    {
        int64_t i64 = va_arg( args, int64_t );

        EsOutEpgTime( out, i64 );
        return VLC_SUCCESS;
    }

    case ES_OUT_DEL_GROUP:
    {
        int i_group = va_arg( args, int );

        return EsOutProgramDel( out, i_group );
    }

    case ES_OUT_SET_META:
    {
        const vlc_meta_t *p_meta = va_arg( args, const vlc_meta_t * );

        EsOutGlobalMeta( out, p_meta );
        return VLC_SUCCESS;
    }

    case ES_OUT_GET_WAKE_UP:
    {
        mtime_t *pi_wakeup = va_arg( args, mtime_t* );
        *pi_wakeup = EsOutGetWakeup( out );
        return VLC_SUCCESS;
    }

    case ES_OUT_SET_ES_BY_ID:
    case ES_OUT_RESTART_ES_BY_ID:
    case ES_OUT_SET_ES_DEFAULT_BY_ID:
    {
        const int i_id = va_arg( args, int );
        es_out_id_t *p_es = EsOutGetFromID( out, i_id );
        int i_new_query = 0;

        switch( i_query )
        {
        case ES_OUT_SET_ES_BY_ID:         i_new_query = ES_OUT_SET_ES; break;
        case ES_OUT_RESTART_ES_BY_ID:     i_new_query = ES_OUT_RESTART_ES; break;
        case ES_OUT_SET_ES_DEFAULT_BY_ID: i_new_query = ES_OUT_SET_ES_DEFAULT; break;
        default:
          vlc_assert_unreachable();
        }
        /* TODO if the lock is made non recursive it should be changed */
        int i_ret = es_out_Control( out, i_new_query, p_es );

        /* Clean up vout after user action (in active mode only).
         * FIXME it does not work well with multiple video windows */
        if( p_sys->b_active )
            input_resource_TerminateVout( input_priv(p_sys->p_input)->p_resource );
        return i_ret;
    }

    case ES_OUT_GET_ES_OBJECTS_BY_ID:
    {
        const int i_id = va_arg( args, int );
        es_out_id_t *p_es = EsOutGetFromID( out, i_id );
        if( !p_es )
            return VLC_EGENERIC;

        vlc_object_t    **pp_decoder = va_arg( args, vlc_object_t ** );
        vout_thread_t   **pp_vout    = va_arg( args, vout_thread_t ** );
        audio_output_t **pp_aout    = va_arg( args, audio_output_t ** );
        if( p_es->p_dec )
        {
            if( pp_decoder )
                *pp_decoder = vlc_object_hold( p_es->p_dec );
            input_DecoderGetObjects( p_es->p_dec, pp_vout, pp_aout );
        }
        else
        {
            if( pp_decoder )
                *pp_decoder = NULL;
            if( pp_vout )
                *pp_vout = NULL;
            if( pp_aout )
                *pp_aout = NULL;
        }
        return VLC_SUCCESS;
    }

    case ES_OUT_GET_BUFFERING:
    {
        bool *pb = va_arg( args, bool* );
        *pb = p_sys->b_buffering;
        return VLC_SUCCESS;
    }

    case ES_OUT_GET_EMPTY:
    {
        bool *pb = va_arg( args, bool* );
        *pb = EsOutDecodersIsEmpty( out );
        return VLC_SUCCESS;
    }

    case ES_OUT_SET_DELAY:
    {
        const int i_cat = va_arg( args, int );
        const mtime_t i_delay = va_arg( args, mtime_t );
        EsOutSetDelay( out, i_cat, i_delay );
        return VLC_SUCCESS;
    }

    case ES_OUT_SET_RECORD_STATE:
    {
        bool b = va_arg( args, int );
        return EsOutSetRecord( out, b );
    }

    case ES_OUT_SET_PAUSE_STATE:
    {
        const bool b_source_paused = (bool)va_arg( args, int );
        const bool b_paused = (bool)va_arg( args, int );
        const mtime_t i_date = va_arg( args, mtime_t );

        assert( !b_source_paused == !b_paused );
        EsOutChangePause( out, b_paused, i_date );

        return VLC_SUCCESS;
    }

    case ES_OUT_SET_RATE:
    {
        const int i_src_rate = va_arg( args, int );
        const int i_rate = va_arg( args, int );

        assert( i_src_rate == i_rate );
        EsOutChangeRate( out, i_rate );

        return VLC_SUCCESS;
    }

    case ES_OUT_SET_TIME:
    {
        const mtime_t i_date = va_arg( args, mtime_t );

        assert( i_date == -1 );
        EsOutChangePosition( out );

        return VLC_SUCCESS;
    }

    case ES_OUT_SET_FRAME_NEXT:
        EsOutFrameNext( out );
        return VLC_SUCCESS;

    case ES_OUT_SET_TIMES:
    {
        double f_position = va_arg( args, double );
        mtime_t i_time = va_arg( args, mtime_t );
        mtime_t i_length = va_arg( args, mtime_t );

        input_SendEventLength( p_sys->p_input, i_length );

        if( !p_sys->b_buffering )
        {
            mtime_t i_delay;

            /* Fix for buffering delay */
            if( !input_priv(p_sys->p_input)->p_sout ||
                !input_priv(p_sys->p_input)->b_out_pace_control )
                i_delay = EsOutGetBuffering( out );
            else
                i_delay = 0;

            i_time -= i_delay;
            if( i_time < 0 )
                i_time = 0;

            if( i_length > 0 )
                f_position -= (double)i_delay / i_length;
            if( f_position < 0 )
                f_position = 0;

            input_SendEventPosition( p_sys->p_input, f_position, i_time );
        }
        return VLC_SUCCESS;
    }
    case ES_OUT_SET_JITTER:
    {
        mtime_t i_pts_delay  = va_arg( args, mtime_t );
        mtime_t i_pts_jitter = va_arg( args, mtime_t );
        int     i_cr_average = va_arg( args, int );

        bool b_change_clock =
            i_pts_delay + i_pts_jitter != p_sys->i_pts_delay ||
            i_cr_average != p_sys->i_cr_average;

        assert( i_pts_jitter >= 0 );
        p_sys->i_pts_delay  = i_pts_delay + i_pts_jitter;
        p_sys->i_pts_jitter = i_pts_jitter;
        p_sys->i_cr_average = i_cr_average;

        for( int i = 0; i < p_sys->i_pgrm && b_change_clock; i++ )
            input_clock_SetJitter( p_sys->pgrm[i]->p_clock,
                                   i_pts_delay + i_pts_jitter, i_cr_average );
        return VLC_SUCCESS;
    }

    case ES_OUT_GET_PCR_SYSTEM:
    {
        if( p_sys->b_buffering )
            return VLC_EGENERIC;

        es_out_pgrm_t *p_pgrm = p_sys->p_pgrm;
        if( !p_pgrm )
            return VLC_EGENERIC;

        mtime_t *pi_system = va_arg( args, mtime_t *);
        mtime_t *pi_delay  = va_arg( args, mtime_t *);
        input_clock_GetSystemOrigin( p_pgrm->p_clock, pi_system, pi_delay );
        return VLC_SUCCESS;
    }

    case ES_OUT_MODIFY_PCR_SYSTEM:
    {
        if( p_sys->b_buffering )
            return VLC_EGENERIC;

        es_out_pgrm_t *p_pgrm = p_sys->p_pgrm;
        if( !p_pgrm )
            return VLC_EGENERIC;

        const bool    b_absolute = va_arg( args, int );
        const mtime_t i_system   = va_arg( args, mtime_t );
        input_clock_ChangeSystemOrigin( p_pgrm->p_clock, b_absolute, i_system );
        return VLC_SUCCESS;
    }
    case ES_OUT_SET_EOS:
    {
        for (int i = 0; i < p_sys->i_es; i++) {
            es_out_id_t *id = p_sys->es[i];
            if (id->p_dec != NULL)
                input_DecoderDrain(id->p_dec);
        }
        return VLC_SUCCESS;
    }

    case ES_OUT_POST_SUBNODE:
    {
        input_item_node_t *node = va_arg(args, input_item_node_t *);
        input_item_node_PostAndDelete(node);
        return VLC_SUCCESS;
    }

    default:
        msg_Err( p_sys->p_input, "unknown query 0x%x in %s", i_query,
                 __func__  );
        return VLC_EGENERIC;
    }
}
static int EsOutControl( es_out_t *out, int i_query, va_list args )
{
    es_out_sys_t *p_sys = out->p_sys;
    int i_ret;

    vlc_mutex_lock( &p_sys->lock );
    i_ret = EsOutControlLocked( out, i_query, args );
    vlc_mutex_unlock( &p_sys->lock );

    return i_ret;
}

/****************************************************************************
 * LanguageGetName: try to expend iso639 into plain name
 ****************************************************************************/
static char *LanguageGetName( const char *psz_code )
{
    const iso639_lang_t *pl;

    if( psz_code == NULL || !strcmp( psz_code, "und" ) )
    {
        return strdup( "" );
    }

    if( strlen( psz_code ) == 2 )
    {
        pl = GetLang_1( psz_code );
    }
    else if( strlen( psz_code ) == 3 )
    {
        pl = GetLang_2B( psz_code );
        if( !strcmp( pl->psz_iso639_1, "??" ) )
        {
            pl = GetLang_2T( psz_code );
        }
    }
    else
    {
        char *lang = LanguageGetCode( psz_code );
        pl = GetLang_1( lang );
        free( lang );
    }

    if( !strcmp( pl->psz_iso639_1, "??" ) )
    {
       return strdup( psz_code );
    }
    else
    {
        return strdup( vlc_gettext(pl->psz_eng_name) );
    }
}

/* Get a 2 char code */
static char *LanguageGetCode( const char *psz_lang )
{
    const iso639_lang_t *pl;

    if( psz_lang == NULL || *psz_lang == '\0' )
        return strdup("??");

    for( pl = p_languages; pl->psz_eng_name != NULL; pl++ )
    {
        if( !strcasecmp( pl->psz_eng_name, psz_lang ) ||
            !strcasecmp( pl->psz_iso639_1, psz_lang ) ||
            !strcasecmp( pl->psz_iso639_2T, psz_lang ) ||
            !strcasecmp( pl->psz_iso639_2B, psz_lang ) )
            return strdup( pl->psz_iso639_1 );
    }

    return strdup("??");
}

static char **LanguageSplit( const char *psz_langs )
{
    char *psz_dup;
    char *psz_parser;
    char **ppsz = NULL;
    int i_psz = 0;

    if( psz_langs == NULL ) return NULL;

    psz_parser = psz_dup = strdup(psz_langs);

    while( psz_parser && *psz_parser )
    {
        char *psz;
        char *psz_code;

        psz = strchr(psz_parser, ',' );
        if( psz ) *psz++ = '\0';

        if( !strcmp( psz_parser, "any" ) )
        {
            TAB_APPEND( i_psz, ppsz, strdup("any") );
        }
        else if( !strcmp( psz_parser, "none" ) )
        {
            TAB_APPEND( i_psz, ppsz, strdup("none") );
        }
        else
        {
            psz_code = LanguageGetCode( psz_parser );
            if( strcmp( psz_code, "??" ) )
            {
                TAB_APPEND( i_psz, ppsz, psz_code );
            }
            else
            {
                free( psz_code );
            }
        }

        psz_parser = psz;
    }

    if( i_psz )
    {
        TAB_APPEND( i_psz, ppsz, NULL );
    }

    free( psz_dup );
    return ppsz;
}

static int LanguageArrayIndex( char **ppsz_langs, const char *psz_lang )
{
    if( !ppsz_langs || !psz_lang )
        return -1;

    for( int i = 0; ppsz_langs[i]; i++ )
    {
        if( !strcasecmp( ppsz_langs[i], psz_lang ) ||
            ( !strcasecmp( ppsz_langs[i], "any" ) && strcasecmp( psz_lang, "none") ) )
            return i;
        if( !strcasecmp( ppsz_langs[i], "none" ) )
            break;
    }

    return -1;
}

/****************************************************************************
 * EsOutUpdateInfo:
 * - add meta info to the playlist item
 ****************************************************************************/
static void EsOutUpdateInfo( es_out_t *out, es_out_id_t *es, const es_format_t *fmt, const vlc_meta_t *p_meta )
{
    es_out_sys_t   *p_sys = out->p_sys;
    input_thread_t *p_input = p_sys->p_input;
    const es_format_t *p_fmt_es = &es->fmt;
    lldiv_t         div;

    if( es->fmt.i_cat == fmt->i_cat )
    {
        es_format_t update = *fmt;
        update.i_id = es->i_meta_id;
        update.i_codec = es->fmt.i_codec;
        update.i_original_fourcc = es->fmt.i_original_fourcc;

        /* Update infos that could have been lost by the decoder (no need to
         * dup them since input_item_UpdateTracksInfo() will do it). */
        if (update.psz_language == NULL)
            update.psz_language = es->fmt.psz_language;
        if (update.psz_description == NULL)
            update.psz_description = es->fmt.psz_description;
        if (update.i_cat == SPU_ES)
        {
            if (update.subs.psz_encoding == NULL)
                update.subs.psz_encoding = es->fmt.subs.psz_encoding;
            if (update.subs.p_style == NULL)
                update.subs.p_style = es->fmt.subs.p_style;
        }
        if (update.i_extra_languages == 0)
        {
            assert(update.p_extra_languages == NULL);
            update.i_extra_languages = es->fmt.i_extra_languages;
            update.p_extra_languages = es->fmt.p_extra_languages;
        }

        /* No need to update codec specific data */
        update.i_extra = 0;
        update.p_extra = NULL;

        input_item_UpdateTracksInfo(input_GetItem(p_input), &update);
    }

    /* Create category */
    char* psz_cat = EsInfoCategoryName( es );

    if( unlikely( !psz_cat ) )
        return;

    info_category_t* p_cat = info_category_New( psz_cat );

    free( psz_cat );

    if( unlikely( !p_cat ) )
        return;

    /* Add information */
    if( es->i_meta_id != es->i_id )
        info_category_AddInfo( p_cat, _("Original ID"),
                       "%d", es->i_id );

    const vlc_fourcc_t i_codec_fourcc = ( p_fmt_es->i_original_fourcc )?
                               p_fmt_es->i_original_fourcc : p_fmt_es->i_codec;
    const char *psz_codec_description =
        vlc_fourcc_GetDescription( p_fmt_es->i_cat, i_codec_fourcc );
    if( psz_codec_description && *psz_codec_description )
        info_category_AddInfo( p_cat, _("Codec"), "%s (%.4s)",
                               psz_codec_description, (char*)&i_codec_fourcc );
    else if ( i_codec_fourcc != VLC_FOURCC(0,0,0,0) )
        info_category_AddInfo( p_cat, _("Codec"), "%.4s",
                               (char*)&i_codec_fourcc );

    if( es->psz_language && *es->psz_language )
        info_category_AddInfo( p_cat, _("Language"), "%s",
                               es->psz_language );
    if( fmt->psz_description && *fmt->psz_description )
        info_category_AddInfo( p_cat, _("Description"), "%s",
                               fmt->psz_description );

    switch( fmt->i_cat )
    {
    case AUDIO_ES:
        info_category_AddInfo( p_cat, _("Type"), _("Audio") );

        if( fmt->audio.i_physical_channels )
            info_category_AddInfo( p_cat, _("Channels"), "%s",
                                   _( aout_FormatPrintChannels( &fmt->audio ) ) );

        if( fmt->audio.i_rate != 0 )
        {
            info_category_AddInfo( p_cat, _("Sample rate"), _("%u Hz"),
                                   fmt->audio.i_rate );
            /* FIXME that should be removed or improved ! (used by text/strings.c) */
            var_SetInteger( p_input, "sample-rate", fmt->audio.i_rate );
        }

        unsigned int i_bitspersample = fmt->audio.i_bitspersample;
        if( i_bitspersample == 0 )
            i_bitspersample = aout_BitsPerSample( p_fmt_es->i_codec );
        if( i_bitspersample != 0 )
            info_category_AddInfo( p_cat, _("Bits per sample"), "%u",
                                   i_bitspersample );

        if( fmt->i_bitrate != 0 )
        {
            info_category_AddInfo( p_cat, _("Bitrate"), _("%u kb/s"),
                                   fmt->i_bitrate / 1000 );
            /* FIXME that should be removed or improved ! (used by text/strings.c) */
            var_SetInteger( p_input, "bit-rate", fmt->i_bitrate );
        }
        for( int i = 0; i < AUDIO_REPLAY_GAIN_MAX; i++ )
        {
            const audio_replay_gain_t *p_rg = &fmt->audio_replay_gain;
            if( !p_rg->pb_gain[i] )
                continue;
            const char *psz_name;
            if( i == AUDIO_REPLAY_GAIN_TRACK )
                psz_name = _("Track replay gain");
            else
                psz_name = _("Album replay gain");
            info_category_AddInfo( p_cat, psz_name, _("%.2f dB"),
                                   p_rg->pf_gain[i] );
        }
        break;

    case VIDEO_ES:
        info_category_AddInfo( p_cat, _("Type"), _("Video") );

        if( fmt->video.i_visible_width > 0 &&
            fmt->video.i_visible_height > 0 )
            info_category_AddInfo( p_cat, _("Video resolution"), "%ux%u",
                                   fmt->video.i_visible_width,
                                   fmt->video.i_visible_height);

        if( fmt->video.i_width > 0 && fmt->video.i_height > 0 )
            info_category_AddInfo( p_cat, _("Buffer dimensions"), "%ux%u",
                                   fmt->video.i_width, fmt->video.i_height );

       if( fmt->video.i_frame_rate > 0 &&
           fmt->video.i_frame_rate_base > 0 )
       {
           div = lldiv( (float)fmt->video.i_frame_rate /
                               fmt->video.i_frame_rate_base * 1000000,
                               1000000 );
           if( div.rem > 0 )
               info_category_AddInfo( p_cat, _("Frame rate"), "%"PRId64".%06u",
                                      div.quot, (unsigned int )div.rem );
           else
               info_category_AddInfo( p_cat, _("Frame rate"), "%"PRId64,
                                      div.quot );
       }
       if( fmt->i_codec != p_fmt_es->i_codec )
       {
           const char *psz_chroma_description =
                vlc_fourcc_GetDescription( VIDEO_ES, fmt->i_codec );
           if( psz_chroma_description )
               info_category_AddInfo( p_cat, _("Decoded format"), "%s",
                                      psz_chroma_description );
       }
       {
           static const char orient_names[][13] = {
               N_("Top left"), N_("Left top"),
               N_("Right bottom"), N_("Top right"),
               N_("Bottom left"), N_("Bottom right"),
               N_("Left bottom"), N_("Right top"),
           };
           info_category_AddInfo( p_cat, _("Orientation"), "%s",
                                  _(orient_names[fmt->video.orientation]) );
       }
       if( fmt->video.primaries != COLOR_PRIMARIES_UNDEF )
       {
           static const char primaries_names[][32] = {
               [COLOR_PRIMARIES_UNDEF] = N_("Undefined"),
               [COLOR_PRIMARIES_BT601_525] =
                   N_("ITU-R BT.601 (525 lines, 60 Hz)"),
               [COLOR_PRIMARIES_BT601_625] =
                   N_("ITU-R BT.601 (625 lines, 50 Hz)"),
               [COLOR_PRIMARIES_BT709] = "ITU-R BT.709",
               [COLOR_PRIMARIES_BT2020] = "ITU-R BT.2020",
               [COLOR_PRIMARIES_DCI_P3] = "DCI/P3 D65",
               [COLOR_PRIMARIES_BT470_M] = "ITU-R BT.470 M",
           };
           static_assert(ARRAY_SIZE(primaries_names) == COLOR_PRIMARIES_MAX+1,
                         "Color primiaries table mismatch");
           info_category_AddInfo( p_cat, _("Color primaries"), "%s",
                                  _(primaries_names[fmt->video.primaries]) );
       }
       if( fmt->video.transfer != TRANSFER_FUNC_UNDEF )
       {
           static const char func_names[][20] = {
               [TRANSFER_FUNC_UNDEF] = N_("Undefined"),
               [TRANSFER_FUNC_LINEAR] = N_("Linear"),
               [TRANSFER_FUNC_SRGB] = "sRGB",
               [TRANSFER_FUNC_BT470_BG] = "ITU-R BT.470 BG",
               [TRANSFER_FUNC_BT470_M] = "ITU-R BT.470 M",
               [TRANSFER_FUNC_BT709] = "ITU-R BT.709",
               [TRANSFER_FUNC_SMPTE_ST2084] = "SMPTE ST2084 (PQ)",
               [TRANSFER_FUNC_SMPTE_240] = "SMPTE 240M",
               [TRANSFER_FUNC_HLG] = N_("Hybrid Log-Gamma"),
           };
           static_assert(ARRAY_SIZE(func_names) == TRANSFER_FUNC_MAX+1,
                         "Transfer functions table mismatch");
           info_category_AddInfo( p_cat, _("Color transfer function"), "%s",
                                  _(func_names[fmt->video.transfer]) );
       }
       if( fmt->video.space != COLOR_SPACE_UNDEF )
       {
           static const char space_names[][16] = {
               [COLOR_SPACE_UNDEF] = N_("Undefined"),
               [COLOR_SPACE_BT601] = "ITU-R BT.601",
               [COLOR_SPACE_BT709] = "ITU-R BT.709",
               [COLOR_SPACE_BT2020] = "ITU-R BT.2020",
           };
           static_assert(ARRAY_SIZE(space_names) == COLOR_SPACE_MAX+1,
                         "Color space table mismatch");
           info_category_AddInfo( p_cat, _("Color space"), _("%s Range"),
                                  _(space_names[fmt->video.space]),
                       _(fmt->video.b_color_range_full ? "Full" : "Limited") );
       }
       if( fmt->video.chroma_location != CHROMA_LOCATION_UNDEF )
       {
           static const char c_loc_names[][16] = {
               [CHROMA_LOCATION_UNDEF] = N_("Undefined"),
               [CHROMA_LOCATION_LEFT] = N_("Left"),
               [CHROMA_LOCATION_CENTER] = N_("Center"),
               [CHROMA_LOCATION_TOP_LEFT] = N_("Top Left"),
               [CHROMA_LOCATION_TOP_CENTER] = N_("Top Center"),
               [CHROMA_LOCATION_BOTTOM_LEFT] =N_("Bottom Left"),
               [CHROMA_LOCATION_BOTTOM_CENTER] = N_("Bottom Center"),
           };
           static_assert(ARRAY_SIZE(c_loc_names) == CHROMA_LOCATION_MAX+1,
                         "Chroma location table mismatch");
           info_category_AddInfo( p_cat, _("Chroma location"), "%s",
                   _(c_loc_names[fmt->video.chroma_location]) );
       }
       if( fmt->video.projection_mode != PROJECTION_MODE_RECTANGULAR )
       {
           const char *psz_loc_name = NULL;
           switch (fmt->video.projection_mode)
           {
           case PROJECTION_MODE_RECTANGULAR:
               psz_loc_name = N_("Rectangular");
               break;
           case PROJECTION_MODE_EQUIRECTANGULAR:
               psz_loc_name = N_("Equirectangular");
               break;
           case PROJECTION_MODE_CUBEMAP_LAYOUT_STANDARD:
               psz_loc_name = N_("Cubemap");
               break;
           default:
               vlc_assert_unreachable();
               break;
           }
           info_category_AddInfo( p_cat, _("Projection"), "%s", _(psz_loc_name) );

           info_category_AddInfo( p_cat, vlc_pgettext("ViewPoint", "Yaw"),
                                  "%.2f", fmt->video.pose.yaw );
           info_category_AddInfo( p_cat, vlc_pgettext("ViewPoint", "Pitch"),
                                  "%.2f", fmt->video.pose.pitch );
           info_category_AddInfo( p_cat, vlc_pgettext("ViewPoint", "Roll"),
                                  "%.2f", fmt->video.pose.roll );
           info_category_AddInfo( p_cat,
                                  vlc_pgettext("ViewPoint", "Field of view"),
                                  "%.2f", fmt->video.pose.fov );
       }
       if ( fmt->video.mastering.max_luminance )
       {
           info_category_AddInfo( p_cat, _("Max. luminance"), "%.4f cd/m²",
               fmt->video.mastering.max_luminance / 10000.f );
       }
       if ( fmt->video.mastering.min_luminance )
       {
           info_category_AddInfo( p_cat, _("Min. luminance"), "%.4f cd/m²",
               fmt->video.mastering.min_luminance / 10000.f );
       }
       if ( fmt->video.mastering.primaries[4] &&
            fmt->video.mastering.primaries[5] )
       {
           float x = (float)fmt->video.mastering.primaries[4] / 50000.f;
           float y = (float)fmt->video.mastering.primaries[5] / 50000.f;
           info_category_AddInfo( p_cat, _("Primary R"), "x=%.4f y=%.4f", x, y );
       }
       if ( fmt->video.mastering.primaries[0] &&
            fmt->video.mastering.primaries[1] )
       {
           float x = (float)fmt->video.mastering.primaries[0] / 50000.f;
           float y = (float)fmt->video.mastering.primaries[1] / 50000.f;
           info_category_AddInfo( p_cat, _("Primary G"), "x=%.4f y=%.4f", x, y );
       }
       if ( fmt->video.mastering.primaries[2] &&
            fmt->video.mastering.primaries[3] )
       {
           float x = (float)fmt->video.mastering.primaries[2] / 50000.f;
           float y = (float)fmt->video.mastering.primaries[3] / 50000.f;
           info_category_AddInfo( p_cat, _("Primary B"), "x=%.4f y=%.4f", x, y );
       }
       if ( fmt->video.mastering.white_point[0] &&
            fmt->video.mastering.white_point[1] )
       {
           float x = (float)fmt->video.mastering.white_point[0] / 50000.f;
           float y = (float)fmt->video.mastering.white_point[1] / 50000.f;
           info_category_AddInfo( p_cat, _("White point"), "x=%.4f y=%.4f", x, y );
       }
       if ( fmt->video.lighting.MaxCLL )
       {
           info_category_AddInfo( p_cat, "MaxCLL", "%d cd/m²",
                                  fmt->video.lighting.MaxCLL );
       }
       if ( fmt->video.lighting.MaxFALL )
       {
           info_category_AddInfo( p_cat, "MaxFALL", "%d cd/m²",
                                  fmt->video.lighting.MaxFALL );
       }
       break;

    case SPU_ES:
        info_category_AddInfo( p_cat, _("Type"), _("Subtitle") );
        break;

    default:
        break;
    }

    /* Append generic meta */
    if( p_meta )
    {
        char **ppsz_all_keys = vlc_meta_CopyExtraNames( p_meta );
        for( int i = 0; ppsz_all_keys && ppsz_all_keys[i]; i++ )
        {
            char *psz_key = ppsz_all_keys[i];
            const char *psz_value = vlc_meta_GetExtra( p_meta, psz_key );

            if( psz_value )
                info_category_AddInfo( p_cat, vlc_gettext(psz_key), "%s",
                                       vlc_gettext(psz_value) );
            free( psz_key );
        }
        free( ppsz_all_keys );
    }
    /* */
    input_Control( p_input, INPUT_REPLACE_INFOS, p_cat );
}

static void EsDeleteInfo( es_out_t *out, es_out_id_t *es )
{
    char* psz_info_category;

    if( likely( psz_info_category = EsInfoCategoryName( es ) ) )
    {
        input_Control( out->p_sys->p_input, INPUT_DEL_INFO,
          psz_info_category, NULL );

        free( psz_info_category );
    }
}
