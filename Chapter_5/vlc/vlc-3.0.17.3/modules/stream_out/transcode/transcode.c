/*****************************************************************************
 * transcode.c: transcoding stream output module
 *****************************************************************************
 * Copyright (C) 2003-2009 VLC authors and VideoLAN
 * $Id: 0c3fe91b9a54f6a27dac03fbd7699a52ce3e66a9 $
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
 *          Gildas Bazin <gbazin@videolan.org>
 *          Jean-Paul Saman <jpsaman #_at_# m2x dot nl>
 *          Antoine Cellerier <dionoea at videolan dot org>
 *          Ilkka Ollakka <ileoo at videolan dot org>
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
#include <vlc_plugin.h>

#include <vlc_spu.h>

#include "transcode.h"

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
#define VENC_TEXT N_("Video encoder")
#define VENC_LONGTEXT N_( \
    "This is the video encoder module that will be used (and its associated "\
    "options).")
#define VCODEC_TEXT N_("Destination video codec")
#define VCODEC_LONGTEXT N_( \
    "This is the video codec that will be used.")
#define VB_TEXT N_("Video bitrate")
#define VB_LONGTEXT N_( \
    "Target bitrate of the transcoded video stream." )
#define SCALE_TEXT N_("Video scaling")
#define SCALE_LONGTEXT N_( \
    "Scale factor to apply to the video while transcoding (eg: 0.25)")
#define FPS_TEXT N_("Video frame-rate")
#define FPS_LONGTEXT N_( \
    "Target output frame rate for the video stream." )
#define DEINTERLACE_TEXT N_("Deinterlace video")
#define DEINTERLACE_LONGTEXT N_( \
    "Deinterlace the video before encoding." )
#define DEINTERLACE_MODULE_TEXT N_("Deinterlace module")
#define DEINTERLACE_MODULE_LONGTEXT N_( \
    "Specify the deinterlace module to use." )
#define WIDTH_TEXT N_("Video width")
#define WIDTH_LONGTEXT N_( \
    "Output video width." )
#define HEIGHT_TEXT N_("Video height")
#define HEIGHT_LONGTEXT N_( \
    "Output video height." )
#define MAXWIDTH_TEXT N_("Maximum video width")
#define MAXWIDTH_LONGTEXT N_( \
    "Maximum output video width." )
#define MAXHEIGHT_TEXT N_("Maximum video height")
#define MAXHEIGHT_LONGTEXT N_( \
    "Maximum output video height." )
#define VFILTER_TEXT N_("Video filter")
#define VFILTER_LONGTEXT N_( \
    "Video filters will be applied to the video streams (after overlays " \
    "are applied). You can enter a colon-separated list of filters." )

#define AENC_TEXT N_("Audio encoder")
#define AENC_LONGTEXT N_( \
    "This is the audio encoder module that will be used (and its associated "\
    "options).")
#define ACODEC_TEXT N_("Destination audio codec")
#define ACODEC_LONGTEXT N_( \
    "This is the audio codec that will be used.")
#define AB_TEXT N_("Audio bitrate")
#define AB_LONGTEXT N_( \
    "Target bitrate of the transcoded audio stream." )
#define ARATE_TEXT N_("Audio sample rate")
#define ARATE_LONGTEXT N_( \
 "Sample rate of the transcoded audio stream (11250, 22500, 44100 or 48000).")
#define ALANG_TEXT N_("Audio language")
#define ALANG_LONGTEXT N_( \
    "This is the language of the audio stream.")
#define ACHANS_TEXT N_("Audio channels")
#define ACHANS_LONGTEXT N_( \
    "Number of audio channels in the transcoded streams." )
#define AFILTER_TEXT N_("Audio filter")
#define AFILTER_LONGTEXT N_( \
    "Audio filters will be applied to the audio streams (after conversion " \
    "filters are applied). You can enter a colon-separated list of filters." )

#define SENC_TEXT N_("Subtitle encoder")
#define SENC_LONGTEXT N_( \
    "This is the subtitle encoder module that will be used (and its " \
    "associated options)." )
#define SCODEC_TEXT N_("Destination subtitle codec")
#define SCODEC_LONGTEXT N_( \
    "This is the subtitle codec that will be used." )

#define SFILTER_TEXT N_("Overlays")
#define SFILTER_LONGTEXT N_( \
    "This allows you to add overlays (also known as \"subpictures\") on the "\
    "transcoded video stream. The subpictures produced by the filters will "\
    "be overlayed directly onto the video. You can specify a colon-separated "\
    "list of subpicture modules." )

#define THREADS_TEXT N_("Number of threads")
#define THREADS_LONGTEXT N_( \
    "Number of threads used for the transcoding." )
#define HP_TEXT N_("High priority")
#define HP_LONGTEXT N_( \
    "Runs the optional encoder thread at the OUTPUT priority instead of " \
    "VIDEO." )
#define POOL_TEXT N_("Picture pool size")
#define POOL_LONGTEXT N_( "Defines how many pictures we allow to be in pool "\
    "between decoder/encoder threads when threads > 0" )


static const char *const ppsz_deinterlace_type[] =
{
    "deinterlace", "ffmpeg-deinterlace"
};

static int  Open ( vlc_object_t * );
static void Close( vlc_object_t * );

#define SOUT_CFG_PREFIX "sout-transcode-"

vlc_module_begin ()
    set_shortname( N_("Transcode"))
    set_description( N_("Transcode stream output") )
    set_capability( "sout stream", 50 )
    add_shortcut( "transcode" )
    set_callbacks( Open, Close )
    set_category( CAT_SOUT )
    set_subcategory( SUBCAT_SOUT_STREAM )
    set_section( N_("Video"), NULL )
    add_module( SOUT_CFG_PREFIX "venc", "encoder", NULL, VENC_TEXT,
                VENC_LONGTEXT, false )
    add_string( SOUT_CFG_PREFIX "vcodec", NULL, VCODEC_TEXT,
                VCODEC_LONGTEXT, false )
    add_integer( SOUT_CFG_PREFIX "vb", 0, VB_TEXT,
                 VB_LONGTEXT, false )
    add_float( SOUT_CFG_PREFIX "scale", 0, SCALE_TEXT,
               SCALE_LONGTEXT, false )
    add_string( SOUT_CFG_PREFIX "fps", NULL, FPS_TEXT,
               FPS_LONGTEXT, false )
    add_obsolete_bool( SOUT_CFG_PREFIX "hurry-up"); /* Since 2.2.0 */
    add_bool( SOUT_CFG_PREFIX "deinterlace", false, DEINTERLACE_TEXT,
              DEINTERLACE_LONGTEXT, false )
    add_string( SOUT_CFG_PREFIX "deinterlace-module", "deinterlace",
                DEINTERLACE_MODULE_TEXT, DEINTERLACE_MODULE_LONGTEXT,
                false )
        change_string_list( ppsz_deinterlace_type, ppsz_deinterlace_type )
    add_integer( SOUT_CFG_PREFIX "width", 0, WIDTH_TEXT,
                 WIDTH_LONGTEXT, true )
    add_integer( SOUT_CFG_PREFIX "height", 0, HEIGHT_TEXT,
                 HEIGHT_LONGTEXT, true )
    add_integer( SOUT_CFG_PREFIX "maxwidth", 0, MAXWIDTH_TEXT,
                 MAXWIDTH_LONGTEXT, true )
    add_integer( SOUT_CFG_PREFIX "maxheight", 0, MAXHEIGHT_TEXT,
                 MAXHEIGHT_LONGTEXT, true )
    add_module_list( SOUT_CFG_PREFIX "vfilter", "video filter",
                     NULL, VFILTER_TEXT, VFILTER_LONGTEXT, false )

    set_section( N_("Audio"), NULL )
    add_module( SOUT_CFG_PREFIX "aenc", "encoder", NULL, AENC_TEXT,
                AENC_LONGTEXT, false )
    add_string( SOUT_CFG_PREFIX "acodec", NULL, ACODEC_TEXT,
                ACODEC_LONGTEXT, false )
    add_integer( SOUT_CFG_PREFIX "ab", 96, AB_TEXT,
                 AB_LONGTEXT, false )
    add_string( SOUT_CFG_PREFIX "alang", NULL, ALANG_TEXT,
                ALANG_LONGTEXT, true )
    add_integer( SOUT_CFG_PREFIX "channels", 0, ACHANS_TEXT,
                 ACHANS_LONGTEXT, false )
        change_integer_range( 0, 9 )
    add_integer( SOUT_CFG_PREFIX "samplerate", 0, ARATE_TEXT,
                 ARATE_LONGTEXT, true )
        change_integer_range( 0, 48000 )
    add_obsolete_bool( SOUT_CFG_PREFIX "audio-sync" ) /*Since 2.2.0 */
    add_module_list( SOUT_CFG_PREFIX "afilter",  "audio filter",
                     NULL, AFILTER_TEXT, AFILTER_LONGTEXT, false )

    set_section( N_("Overlays/Subtitles"), NULL )
    add_module( SOUT_CFG_PREFIX "senc", "encoder", NULL, SENC_TEXT,
                SENC_LONGTEXT, false )
    add_string( SOUT_CFG_PREFIX "scodec", NULL, SCODEC_TEXT,
                SCODEC_LONGTEXT, false )
    add_bool( SOUT_CFG_PREFIX "soverlay", false, SCODEC_TEXT,
               SCODEC_LONGTEXT, false )
    add_module_list( SOUT_CFG_PREFIX "sfilter", "sub source",
                     NULL, SFILTER_TEXT, SFILTER_LONGTEXT, false )

    set_section( N_("Miscellaneous"), NULL )
    add_integer( SOUT_CFG_PREFIX "threads", 0, THREADS_TEXT,
                 THREADS_LONGTEXT, true )
    add_integer( SOUT_CFG_PREFIX "pool-size", 10, POOL_TEXT, POOL_LONGTEXT, true )
        change_integer_range( 1, 1000 )
    add_bool( SOUT_CFG_PREFIX "high-priority", false, HP_TEXT, HP_LONGTEXT,
              true )

vlc_module_end ()

static const char *const ppsz_sout_options[] = {
    "venc", "vcodec", "vb",
    "scale", "fps", "width", "height", "vfilter", "deinterlace",
    "deinterlace-module", "threads", "aenc", "acodec", "ab", "alang",
    "afilter", "samplerate", "channels", "senc", "scodec", "soverlay",
    "sfilter", "high-priority", "maxwidth", "maxheight", "pool-size",
    NULL
};

/*****************************************************************************
 * Exported prototypes
 *****************************************************************************/
static sout_stream_id_sys_t *Add( sout_stream_t *, const es_format_t * );
static void              Del ( sout_stream_t *, sout_stream_id_sys_t * );
static int               Send( sout_stream_t *, sout_stream_id_sys_t *, block_t* );

/*****************************************************************************
 * Open:
 *****************************************************************************/
static int Open( vlc_object_t *p_this )
{
    sout_stream_t     *p_stream = (sout_stream_t*)p_this;
    sout_stream_sys_t *p_sys;
    char              *psz_string;

    if( !p_stream->p_next )
    {
        msg_Err( p_stream, "cannot create chain" );
        return VLC_EGENERIC;
    }
    p_sys = calloc( 1, sizeof( *p_sys ) );
    p_sys->i_master_drift = 0;

    config_ChainParse( p_stream, SOUT_CFG_PREFIX, ppsz_sout_options,
                   p_stream->p_cfg );

    /* Audio transcoding parameters */
    psz_string = var_GetString( p_stream, SOUT_CFG_PREFIX "aenc" );
    p_sys->psz_aenc = NULL;
    p_sys->p_audio_cfg = NULL;
    if( psz_string && *psz_string )
    {
        char *psz_next;
        psz_next = config_ChainCreate( &p_sys->psz_aenc, &p_sys->p_audio_cfg,
                                       psz_string );
        free( psz_next );
    }
    free( psz_string );

    psz_string = var_GetString( p_stream, SOUT_CFG_PREFIX "acodec" );
    p_sys->i_acodec = 0;
    if( psz_string && *psz_string )
    {
        char fcc[5] = "    \0";
        memcpy( fcc, psz_string, __MIN( strlen( psz_string ), 4 ) );
        p_sys->i_acodec = vlc_fourcc_GetCodecFromString( AUDIO_ES, fcc );
        msg_Dbg( p_stream, "Checking codec mapping for %s got %4.4s ", fcc, (char*)&p_sys->i_acodec);
    }
    free( psz_string );

    p_sys->psz_alang = var_GetNonEmptyString( p_stream, SOUT_CFG_PREFIX "alang" );

    p_sys->i_abitrate = var_GetInteger( p_stream, SOUT_CFG_PREFIX "ab" );
    if( p_sys->i_abitrate < 4000 ) p_sys->i_abitrate *= 1000;

    p_sys->i_sample_rate = var_GetInteger( p_stream, SOUT_CFG_PREFIX "samplerate" );

    p_sys->i_channels = var_GetInteger( p_stream, SOUT_CFG_PREFIX "channels" );

    if( p_sys->i_acodec )
    {
        if( ( p_sys->i_acodec == VLC_CODEC_MP3 ||
              p_sys->i_acodec == VLC_CODEC_MP2 ||
              p_sys->i_acodec == VLC_CODEC_MPGA ) && p_sys->i_channels > 2 )
        {
            msg_Warn( p_stream, "%d channels invalid for mp2/mp3, forcing to 2",
                      p_sys->i_channels );
            p_sys->i_channels = 2;
        }
        msg_Dbg( p_stream, "codec audio=%4.4s %dHz %d channels %dKb/s",
                 (char *)&p_sys->i_acodec, p_sys->i_sample_rate,
                 p_sys->i_channels, p_sys->i_abitrate / 1000 );
    }

    psz_string = var_GetString( p_stream, SOUT_CFG_PREFIX "afilter" );
    if( psz_string && *psz_string )
        p_sys->psz_af = strdup( psz_string );
    else
        p_sys->psz_af = NULL;
    free( psz_string );

    /* Video transcoding parameters */
    psz_string = var_GetString( p_stream, SOUT_CFG_PREFIX "venc" );
    p_sys->psz_venc = NULL;
    p_sys->p_video_cfg = NULL;
    if( psz_string && *psz_string )
    {
        char *psz_next;
        psz_next = config_ChainCreate( &p_sys->psz_venc, &p_sys->p_video_cfg,
                                   psz_string );
        free( psz_next );
    }
    free( psz_string );

    psz_string = var_GetString( p_stream, SOUT_CFG_PREFIX "vcodec" );
    p_sys->i_vcodec = 0;
    if( psz_string && *psz_string )
    {
        char fcc[5] = "    \0";
        memcpy( fcc, psz_string, __MIN( strlen( psz_string ), 4 ) );
        p_sys->i_vcodec = vlc_fourcc_GetCodecFromString( VIDEO_ES, fcc );
        msg_Dbg( p_stream, "Checking video codec mapping for %s got %4.4s ", fcc, (char*)&p_sys->i_vcodec);
    }
    free( psz_string );

    p_sys->i_vbitrate = var_GetInteger( p_stream, SOUT_CFG_PREFIX "vb" );
    if( p_sys->i_vbitrate < 16000 ) p_sys->i_vbitrate *= 1000;

    p_sys->f_scale = var_GetFloat( p_stream, SOUT_CFG_PREFIX "scale" );

    p_sys->b_master_sync = var_InheritURational( p_stream, &p_sys->fps_num, &p_sys->fps_den, SOUT_CFG_PREFIX "fps" ) == VLC_SUCCESS;

    p_sys->i_width = var_GetInteger( p_stream, SOUT_CFG_PREFIX "width" );

    p_sys->i_height = var_GetInteger( p_stream, SOUT_CFG_PREFIX "height" );

    p_sys->i_maxwidth = var_GetInteger( p_stream, SOUT_CFG_PREFIX "maxwidth" );

    p_sys->i_maxheight = var_GetInteger( p_stream, SOUT_CFG_PREFIX "maxheight" );

    psz_string = var_GetString( p_stream, SOUT_CFG_PREFIX "vfilter" );
    if( psz_string && *psz_string )
        p_sys->psz_vf2 = strdup(psz_string );
    else
        p_sys->psz_vf2 = NULL;
    free( psz_string );

    if( var_GetBool( p_stream, SOUT_CFG_PREFIX "deinterlace" ) )
        psz_string = var_GetString( p_stream,
                                    SOUT_CFG_PREFIX "deinterlace-module" );
    else
        psz_string = NULL;

    free( config_ChainCreate( &p_sys->psz_deinterlace,
                              &p_sys->p_deinterlace_cfg, psz_string ) );
    free( psz_string );

    p_sys->i_threads = var_GetInteger( p_stream, SOUT_CFG_PREFIX "threads" );
    p_sys->pool_size = var_GetInteger( p_stream, SOUT_CFG_PREFIX "pool-size" );
    p_sys->b_high_priority = var_GetBool( p_stream, SOUT_CFG_PREFIX "high-priority" );

    if( p_sys->i_vcodec )
    {
        msg_Dbg( p_stream, "codec video=%4.4s %dx%d scaling: %f %dkb/s",
                 (char *)&p_sys->i_vcodec, p_sys->i_width, p_sys->i_height,
                 p_sys->f_scale, p_sys->i_vbitrate / 1000 );
    }

    /* Subpictures transcoding parameters */
    p_sys->p_spu = NULL;
    p_sys->p_spu_blend = NULL;
    p_sys->psz_senc = NULL;
    p_sys->p_spu_cfg = NULL;
    p_sys->i_scodec = 0;

    psz_string = var_GetString( p_stream, SOUT_CFG_PREFIX "senc" );
    if( psz_string && *psz_string )
    {
        char *psz_next;
        psz_next = config_ChainCreate( &p_sys->psz_senc, &p_sys->p_spu_cfg,
                                   psz_string );
        free( psz_next );
    }
    free( psz_string );

    psz_string = var_GetString( p_stream, SOUT_CFG_PREFIX "scodec" );
    if( psz_string && *psz_string )
    {
        char fcc[5] = "    \0";
        memcpy( fcc, psz_string, __MIN( strlen( psz_string ), 4 ) );
        p_sys->i_scodec = vlc_fourcc_GetCodecFromString( SPU_ES, fcc );
        msg_Dbg( p_stream, "Checking spu codec mapping for %s got %4.4s ", fcc, (char*)&p_sys->i_scodec);
    }
    free( psz_string );

    if( p_sys->i_scodec )
    {
        msg_Dbg( p_stream, "codec spu=%4.4s", (char *)&p_sys->i_scodec );
    }

    p_sys->b_soverlay = var_GetBool( p_stream, SOUT_CFG_PREFIX "soverlay" );

    psz_string = var_GetString( p_stream, SOUT_CFG_PREFIX "sfilter" );
    if( psz_string && *psz_string )
    {
        p_sys->p_spu = spu_Create( p_stream, NULL );
        if( p_sys->p_spu )
            spu_ChangeSources( p_sys->p_spu, psz_string );
    }
    free( psz_string );

    p_stream->pf_add    = Add;
    p_stream->pf_del    = Del;
    p_stream->pf_send   = Send;
    p_stream->p_sys     = p_sys;

    return VLC_SUCCESS;
}

/*****************************************************************************
 * Close:
 *****************************************************************************/
static void Close( vlc_object_t * p_this )
{
    sout_stream_t       *p_stream = (sout_stream_t*)p_this;
    sout_stream_sys_t   *p_sys = p_stream->p_sys;

    free( p_sys->psz_af );

    config_ChainDestroy( p_sys->p_audio_cfg );
    free( p_sys->psz_aenc );
    free( p_sys->psz_alang );

    free( p_sys->psz_vf2 );

    config_ChainDestroy( p_sys->p_video_cfg );
    free( p_sys->psz_venc );

    config_ChainDestroy( p_sys->p_deinterlace_cfg );
    free( p_sys->psz_deinterlace );

    config_ChainDestroy( p_sys->p_spu_cfg );
    free( p_sys->psz_senc );

    if( p_sys->p_spu ) spu_Destroy( p_sys->p_spu );
    if( p_sys->p_spu_blend ) filter_DeleteBlend( p_sys->p_spu_blend );

    free( p_sys );
}

static void DeleteSoutStreamID( sout_stream_id_sys_t *id )
{
    if( id )
    {
        if( id->p_decoder )
        {
            es_format_Clean( &id->p_decoder->fmt_in );
            es_format_Clean( &id->p_decoder->fmt_out );
            vlc_object_release( id->p_decoder );
        }

        if( id->p_encoder )
        {
            es_format_Clean( &id->p_encoder->fmt_in );
            es_format_Clean( &id->p_encoder->fmt_out );
            vlc_object_release( id->p_encoder );
        }

        vlc_mutex_destroy(&id->fifo.lock);
        free( id );
    }
}

static sout_stream_id_sys_t *Add( sout_stream_t *p_stream,
                                  const es_format_t *p_fmt )
{
    sout_stream_sys_t *p_sys = p_stream->p_sys;
    sout_stream_id_sys_t *id;

    id = calloc( 1, sizeof( sout_stream_id_sys_t ) );
    if( !id )
        goto error;

    vlc_mutex_init(&id->fifo.lock);
    id->id = NULL;
    id->p_decoder = NULL;
    id->p_encoder = NULL;

    /* Create decoder object */
    id->p_decoder = vlc_object_create( p_stream, sizeof( decoder_t ) );
    if( !id->p_decoder )
        goto error;
    id->p_decoder->p_module = NULL;
    es_format_Init( &id->p_decoder->fmt_out, p_fmt->i_cat, 0 );
    es_format_Copy( &id->p_decoder->fmt_in, p_fmt );
    id->p_decoder->b_frame_drop_allowed = false;

    /* Create encoder object */
    id->p_encoder = sout_EncoderCreate( p_stream );
    if( !id->p_encoder )
        goto error;
    id->p_encoder->p_module = NULL;

    /* Create destination format */
    es_format_Init( &id->p_encoder->fmt_in, p_fmt->i_cat, 0 );
    es_format_Init( &id->p_encoder->fmt_out, p_fmt->i_cat, 0 );
    id->p_encoder->fmt_out.i_id    = p_fmt->i_id;
    id->p_encoder->fmt_out.i_group = p_fmt->i_group;

    if( p_sys->psz_alang )
        id->p_encoder->fmt_out.psz_language = strdup( p_sys->psz_alang );
    else if( p_fmt->psz_language )
        id->p_encoder->fmt_out.psz_language = strdup( p_fmt->psz_language );

    bool success;

    if( p_fmt->i_cat == AUDIO_ES && p_sys->i_acodec )
        success = transcode_audio_add(p_stream, p_fmt, id);
    else if( p_fmt->i_cat == VIDEO_ES && p_sys->i_vcodec )
        success = transcode_video_add(p_stream, p_fmt, id);
    else if( ( p_fmt->i_cat == SPU_ES ) &&
             ( p_sys->i_scodec || p_sys->b_soverlay ) )
        success = transcode_spu_add(p_stream, p_fmt, id);
    else
    {
        msg_Dbg( p_stream, "not transcoding a stream (fcc=`%4.4s')",
                 (char*)&p_fmt->i_codec );
        id->id = sout_StreamIdAdd( p_stream->p_next, p_fmt );
        id->b_transcode = false;

        success = id->id;
    }

    if(!success)
        goto error;

    return id;

error:
    DeleteSoutStreamID( id );
    return NULL;
}

static void Del( sout_stream_t *p_stream, sout_stream_id_sys_t *id )
{
    if( id->b_transcode )
    {
        switch( id->p_decoder->fmt_in.i_cat )
        {
        case AUDIO_ES:
            Send( p_stream, id, NULL );
            transcode_audio_close( id );
            break;
        case VIDEO_ES:
            Send( p_stream, id, NULL );
            transcode_video_close( p_stream, id );
            break;
        case SPU_ES:
            transcode_spu_close( p_stream, id );
            break;
        default:
            break;
        }
    }

    if( id->id ) sout_StreamIdDel( p_stream->p_next, id->id );

    DeleteSoutStreamID( id );
}

static int Send( sout_stream_t *p_stream, sout_stream_id_sys_t *id,
                 block_t *p_buffer )
{
    block_t *p_out = NULL;

    if( id->b_error )
        goto error;

    if( !id->b_transcode )
    {
        if( id->id )
            return sout_StreamIdSend( p_stream->p_next, id->id, p_buffer );
        else
            goto error;
    }

    switch( id->p_decoder->fmt_in.i_cat )
    {
    case AUDIO_ES:
        if( transcode_audio_process( p_stream, id, p_buffer, &p_out )
            != VLC_SUCCESS )
        {
            return VLC_EGENERIC;
        }
        break;

    case VIDEO_ES:
        if( transcode_video_process( p_stream, id, p_buffer, &p_out )
            != VLC_SUCCESS )
        {
            return VLC_EGENERIC;
        }
        break;

    case SPU_ES:
        if ( transcode_spu_process( p_stream, id, p_buffer, &p_out ) !=
            VLC_SUCCESS )
        {
            return VLC_EGENERIC;
        }
        break;

    default:
        goto error;
    }

    if( p_out )
        return sout_StreamIdSend( p_stream->p_next, id->id, p_out );
    return VLC_SUCCESS;
error:
    if( p_buffer )
        block_Release( p_buffer );
    return VLC_EGENERIC;
}
