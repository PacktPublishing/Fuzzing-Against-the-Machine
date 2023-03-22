/*****************************************************************************
 * video.c: transcoding stream output module (video)
 *****************************************************************************
 * Copyright (C) 2003-2009 VLC authors and VideoLAN
 * $Id: 9070babb2d33cdc6bdf26acfa41a10d8abd94480 $
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
 *          Gildas Bazin <gbazin@videolan.org>
 *          Jean-Paul Saman <jpsaman #_at_# m2x dot nl>
 *          Antoine Cellerier <dionoea at videolan dot org>
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

#include "transcode.h"

#include <math.h>
#include <vlc_meta.h>
#include <vlc_spu.h>
#include <vlc_modules.h>

#define ENC_FRAMERATE (25 * 1000)
#define ENC_FRAMERATE_BASE 1000

static const video_format_t* video_output_format( sout_stream_id_sys_t *id,
                                                  picture_t *p_pic )
{
    assert( id && p_pic );
    if( id->p_uf_chain )
        return &filter_chain_GetFmtOut( id->p_uf_chain )->video;
    else if( id->p_f_chain )
        return &filter_chain_GetFmtOut( id->p_f_chain )->video;
    else
        return &p_pic->format;
}

static int video_update_format_decoder( decoder_t *p_dec )
{
    sout_stream_t        *stream = (sout_stream_t*) p_dec->p_owner;
    sout_stream_sys_t    *sys    = stream->p_sys;
    sout_stream_id_sys_t *id     = p_dec->p_queue_ctx;
    filter_chain_t       *test_chain;

    filter_owner_t filter_owner = {
        .sys = sys,
    };

    /* will need proper chroma for get_buffer */
    p_dec->fmt_out.video.i_chroma = p_dec->fmt_out.i_codec;

    if( id->p_encoder->fmt_in.i_codec == p_dec->fmt_out.i_codec ||
        video_format_IsSimilar( &id->video_dec_out,
                                &p_dec->fmt_out.video ) )
        return 0;
    id->video_dec_out = p_dec->fmt_out.video;
    id->video_dec_out.p_palette = NULL;

    msg_Dbg( stream, "Checking if filter chain %4.4s -> %4.4s is possible",
                 (char *)&p_dec->fmt_out.i_codec, (char*)&id->p_encoder->fmt_in.i_codec );
    test_chain = filter_chain_NewVideo( stream, false, &filter_owner );
    filter_chain_Reset( test_chain, &p_dec->fmt_out, &p_dec->fmt_out );

    int chain_works = filter_chain_AppendConverter( test_chain, &p_dec->fmt_out,
                                  &id->p_encoder->fmt_in );
    filter_chain_Delete( test_chain );
    msg_Dbg( stream, "Filter chain testing done, input chroma %4.4s seems to be %s for transcode",
                     (char *)&p_dec->fmt_out.video.i_chroma,
                     chain_works == 0 ? "possible" : "not possible");
    return chain_works;
}

static picture_t *video_new_buffer_decoder( decoder_t *p_dec )
{
    return picture_NewFromFormat( &p_dec->fmt_out.video );
}

static picture_t *video_new_buffer_encoder( encoder_t *p_enc )
{
    return picture_NewFromFormat( &p_enc->fmt_in.video );
}

static picture_t *transcode_video_filter_buffer_new( filter_t *p_filter )
{
    p_filter->fmt_out.video.i_chroma = p_filter->fmt_out.i_codec;
    return picture_NewFromFormat( &p_filter->fmt_out.video );
}

static void* EncoderThread( void *obj )
{
    sout_stream_sys_t *p_sys = (sout_stream_sys_t*)obj;
    sout_stream_id_sys_t *id = p_sys->id_video;
    picture_t *p_pic = NULL;
    int canc = vlc_savecancel ();
    block_t *p_block = NULL;

    vlc_mutex_lock( &p_sys->lock_out );

    for( ;; )
    {
        while( !p_sys->b_abort &&
               (p_pic = picture_fifo_Pop( p_sys->pp_pics )) == NULL )
            vlc_cond_wait( &p_sys->cond, &p_sys->lock_out );
        vlc_sem_post( &p_sys->picture_pool_has_room );

        if( p_pic )
        {
            /* release lock while encoding */
            vlc_mutex_unlock( &p_sys->lock_out );
            p_block = id->p_encoder->pf_encode_video( id->p_encoder, p_pic );
            picture_Release( p_pic );
            vlc_mutex_lock( &p_sys->lock_out );

            block_ChainAppend( &p_sys->p_buffers, p_block );
        }

        if( p_sys->b_abort )
            break;
    }

    /*Encode what we have in the buffer on closing*/
    while( (p_pic = picture_fifo_Pop( p_sys->pp_pics )) != NULL )
    {
        vlc_sem_post( &p_sys->picture_pool_has_room );
        p_block = id->p_encoder->pf_encode_video( id->p_encoder, p_pic );
        picture_Release( p_pic );
        block_ChainAppend( &p_sys->p_buffers, p_block );
    }

    /*Now flush encoder*/
    do {
        p_block = id->p_encoder->pf_encode_video(id->p_encoder, NULL );
        block_ChainAppend( &p_sys->p_buffers, p_block );
    } while( p_block );

    vlc_mutex_unlock( &p_sys->lock_out );

    vlc_restorecancel (canc);

    return NULL;
}

static int decoder_queue_video( decoder_t *p_dec, picture_t *p_pic )
{
    sout_stream_id_sys_t *id = p_dec->p_queue_ctx;

    vlc_mutex_lock(&id->fifo.lock);
    *id->fifo.pic.last = p_pic;
    id->fifo.pic.last = &p_pic->p_next;
    vlc_mutex_unlock(&id->fifo.lock);
    return 0;
}

static picture_t *transcode_dequeue_all_pics( sout_stream_id_sys_t *id )
{
    vlc_mutex_lock(&id->fifo.lock);
    picture_t *p_pics = id->fifo.pic.first;
    id->fifo.pic.first = NULL;
    id->fifo.pic.last = &id->fifo.pic.first;
    vlc_mutex_unlock(&id->fifo.lock);

    return p_pics;
}

static int transcode_video_new( sout_stream_t *p_stream, sout_stream_id_sys_t *id )
{
    sout_stream_sys_t *p_sys = p_stream->p_sys;

    /* Open decoder
     * Initialization of decoder structures
     */
    id->p_decoder->pf_decode = NULL;
    id->p_decoder->pf_queue_video = decoder_queue_video;
    id->p_decoder->p_queue_ctx = id;
    id->p_decoder->pf_get_cc = NULL;
    id->p_decoder->pf_vout_format_update = video_update_format_decoder;
    id->p_decoder->pf_vout_buffer_new = video_new_buffer_decoder;
    id->p_decoder->p_owner = (decoder_owner_sys_t*) p_stream;

    id->p_decoder->p_module =
        module_need( id->p_decoder, "video decoder", "$codec", false );

    if( !id->p_decoder->p_module )
    {
        msg_Err( p_stream, "cannot find video decoder" );
        return VLC_EGENERIC;
    }

    /*
     * Open encoder.
     * Because some info about the decoded input will only be available
     * once the first frame is decoded, we actually only test the availability
     * of the encoder here.
     */

    /* Initialization of encoder format structures */
    es_format_Init( &id->p_encoder->fmt_in, id->p_decoder->fmt_in.i_cat,
                    id->p_decoder->fmt_out.i_codec );

    /* The dimensions will be set properly later on.
     * Just put sensible values so we can test an encoder is available. */
    id->p_encoder->fmt_in.video.i_width =
        id->p_encoder->fmt_out.video.i_width
          ? id->p_encoder->fmt_out.video.i_width
          : id->p_decoder->fmt_in.video.i_width
            ? id->p_decoder->fmt_in.video.i_width : 16;
    id->p_encoder->fmt_in.video.i_height =
        id->p_encoder->fmt_out.video.i_height
          ? id->p_encoder->fmt_out.video.i_height
          : id->p_decoder->fmt_in.video.i_height
            ? id->p_decoder->fmt_in.video.i_height : 16;
    id->p_encoder->fmt_in.video.i_visible_width =
        id->p_encoder->fmt_out.video.i_visible_width
          ? id->p_encoder->fmt_out.video.i_visible_width
          : id->p_decoder->fmt_in.video.i_visible_width
            ? id->p_decoder->fmt_in.video.i_visible_width : id->p_encoder->fmt_in.video.i_width;
    id->p_encoder->fmt_in.video.i_visible_height =
        id->p_encoder->fmt_out.video.i_visible_height
          ? id->p_encoder->fmt_out.video.i_visible_height
          : id->p_decoder->fmt_in.video.i_visible_height
            ? id->p_decoder->fmt_in.video.i_visible_height : id->p_encoder->fmt_in.video.i_height;
    /* The same goes with frame rate. Some encoders need it to be initialized */
    id->p_encoder->fmt_in.video.i_frame_rate = ENC_FRAMERATE;
    id->p_encoder->fmt_in.video.i_frame_rate_base = ENC_FRAMERATE_BASE;

    id->p_encoder->i_threads = p_sys->i_threads;
    id->p_encoder->p_cfg = p_sys->p_video_cfg;

    id->p_encoder->p_module =
        module_need( id->p_encoder, "encoder", p_sys->psz_venc, true );
    if( !id->p_encoder->p_module )
    {
        msg_Err( p_stream, "cannot find video encoder (module:%s fourcc:%4.4s). Take a look few lines earlier to see possible reason.",
                 p_sys->psz_venc ? p_sys->psz_venc : "any",
                 (char *)&p_sys->i_vcodec );
        module_unneed( id->p_decoder, id->p_decoder->p_module );
        id->p_decoder->p_module = 0;
        return VLC_EGENERIC;
    }

    /* Close the encoder.
     * We'll open it only when we have the first frame. */
    module_unneed( id->p_encoder, id->p_encoder->p_module );
    if( id->p_encoder->fmt_out.p_extra )
    {
        free( id->p_encoder->fmt_out.p_extra );
        id->p_encoder->fmt_out.p_extra = NULL;
        id->p_encoder->fmt_out.i_extra = 0;
    }
    id->p_encoder->fmt_in.video.i_chroma = id->p_encoder->fmt_in.i_codec;
    id->p_encoder->p_module = NULL;

    if( p_sys->i_threads <= 0 )
        return VLC_SUCCESS;

    int i_priority = p_sys->b_high_priority ? VLC_THREAD_PRIORITY_OUTPUT :
                       VLC_THREAD_PRIORITY_VIDEO;
    p_sys->id_video = id;
    p_sys->pp_pics = picture_fifo_New();
    if( p_sys->pp_pics == NULL )
    {
        msg_Err( p_stream, "cannot create picture fifo" );
        module_unneed( id->p_decoder, id->p_decoder->p_module );
        id->p_decoder->p_module = NULL;
        return VLC_ENOMEM;
    }

    vlc_sem_init( &p_sys->picture_pool_has_room, p_sys->pool_size );
    vlc_mutex_init( &p_sys->lock_out );
    vlc_cond_init( &p_sys->cond );
    p_sys->p_buffers = NULL;
    p_sys->b_abort = false;
    if( vlc_clone( &p_sys->thread, EncoderThread, p_sys, i_priority ) )
    {
        msg_Err( p_stream, "cannot spawn encoder thread" );
        vlc_mutex_destroy( &p_sys->lock_out );
        vlc_cond_destroy( &p_sys->cond );
        picture_fifo_Delete( p_sys->pp_pics );
        module_unneed( id->p_decoder, id->p_decoder->p_module );
        id->p_decoder->p_module = NULL;
        return VLC_EGENERIC;
    }
    return VLC_SUCCESS;
}

static void transcode_video_filter_init( sout_stream_t *p_stream,
                                         sout_stream_id_sys_t *id )
{
    filter_owner_t owner = {
        .sys = p_stream->p_sys,
        .video = {
            .buffer_new = transcode_video_filter_buffer_new,
        },
    };
    const es_format_t *p_fmt_out = &id->p_decoder->fmt_out;

    id->p_encoder->fmt_in.video.i_chroma = id->p_encoder->fmt_in.i_codec;
    id->p_f_chain = filter_chain_NewVideo( p_stream, false, &owner );
    filter_chain_Reset( id->p_f_chain, p_fmt_out, p_fmt_out );

    /* Check that we have visible_width/height*/
    if( !id->p_decoder->fmt_out.video.i_visible_height )
        id->p_decoder->fmt_out.video.i_visible_height = id->p_decoder->fmt_out.video.i_height;
    if( !id->p_decoder->fmt_out.video.i_visible_width )
        id->p_decoder->fmt_out.video.i_visible_width = id->p_decoder->fmt_out.video.i_width;

    /* Deinterlace */
    if( p_stream->p_sys->psz_deinterlace != NULL )
    {
        filter_chain_AppendFilter( id->p_f_chain,
                                   p_stream->p_sys->psz_deinterlace,
                                   p_stream->p_sys->p_deinterlace_cfg,
                                   &id->p_decoder->fmt_out,
                                   &id->p_decoder->fmt_out );

        p_fmt_out = filter_chain_GetFmtOut( id->p_f_chain );
    }
    if( p_stream->p_sys->b_master_sync )
    {
        filter_chain_AppendFilter( id->p_f_chain,
                                   "fps",
                                   NULL,
                                   p_fmt_out,
                                   &id->p_encoder->fmt_in );

        p_fmt_out = filter_chain_GetFmtOut( id->p_f_chain );
    }

    if( p_stream->p_sys->psz_vf2 )
    {
        id->p_uf_chain = filter_chain_NewVideo( p_stream, true, &owner );
        filter_chain_Reset( id->p_uf_chain, p_fmt_out,
                            &id->p_encoder->fmt_in );
        if( p_fmt_out->video.i_chroma != id->p_encoder->fmt_in.video.i_chroma )
        {
            filter_chain_AppendConverter( id->p_uf_chain, p_fmt_out,
                                           &id->p_encoder->fmt_in );
        }
        filter_chain_AppendFromString( id->p_uf_chain, p_stream->p_sys->psz_vf2 );
        p_fmt_out = filter_chain_GetFmtOut( id->p_uf_chain );
        es_format_Copy( &id->p_encoder->fmt_in, p_fmt_out );
        id->p_encoder->fmt_out.video.i_width =
            id->p_encoder->fmt_in.video.i_width;
        id->p_encoder->fmt_out.video.i_height =
            id->p_encoder->fmt_in.video.i_height;
        id->p_encoder->fmt_out.video.i_sar_num =
            id->p_encoder->fmt_in.video.i_sar_num;
        id->p_encoder->fmt_out.video.i_sar_den =
            id->p_encoder->fmt_in.video.i_sar_den;
    }

    /* Keep colorspace etc info along */
    id->p_encoder->fmt_in.video.space     = id->p_decoder->fmt_out.video.space;
    id->p_encoder->fmt_in.video.transfer  = id->p_decoder->fmt_out.video.transfer;
    id->p_encoder->fmt_in.video.primaries = id->p_decoder->fmt_out.video.primaries;
    id->p_encoder->fmt_in.video.b_color_range_full = id->p_decoder->fmt_out.video.b_color_range_full;
}

/* Take care of the scaling and chroma conversions. */
static int conversion_video_filter_append( sout_stream_id_sys_t *id,
                                           picture_t *p_pic )
{
    const video_format_t *p_vid_out = video_output_format( id, p_pic );

    if( ( p_vid_out->i_chroma != id->p_encoder->fmt_in.video.i_chroma ) ||
        ( p_vid_out->i_width != id->p_encoder->fmt_in.video.i_width ) ||
        ( p_vid_out->i_height != id->p_encoder->fmt_in.video.i_height ) )
    {
        es_format_t fmt_out;
        es_format_Init( &fmt_out, VIDEO_ES, p_vid_out->i_chroma );
        fmt_out.video = *p_vid_out;
        return filter_chain_AppendConverter( id->p_uf_chain ? id->p_uf_chain : id->p_f_chain,
                                             &fmt_out, &id->p_encoder->fmt_in );
    }
    return VLC_SUCCESS;
}

static void transcode_video_framerate_init( sout_stream_t *p_stream,
                                            sout_stream_id_sys_t *id,
                                            const video_format_t *p_vid_out )
{
    /* Handle frame rate conversion */
    if( !id->p_encoder->fmt_out.video.i_frame_rate ||
        !id->p_encoder->fmt_out.video.i_frame_rate_base )
    {
        if( p_vid_out->i_frame_rate &&
            p_vid_out->i_frame_rate_base )
        {
            id->p_encoder->fmt_out.video.i_frame_rate =
                p_vid_out->i_frame_rate;
            id->p_encoder->fmt_out.video.i_frame_rate_base =
                p_vid_out->i_frame_rate_base;
        }
        else
        {
            /* Pick a sensible default value */
            id->p_encoder->fmt_out.video.i_frame_rate = ENC_FRAMERATE;
            id->p_encoder->fmt_out.video.i_frame_rate_base = ENC_FRAMERATE_BASE;
        }
    }

    id->p_encoder->fmt_in.video.i_frame_rate =
        id->p_encoder->fmt_out.video.i_frame_rate;
    id->p_encoder->fmt_in.video.i_frame_rate_base =
        id->p_encoder->fmt_out.video.i_frame_rate_base;

    vlc_ureduce( &id->p_encoder->fmt_in.video.i_frame_rate,
        &id->p_encoder->fmt_in.video.i_frame_rate_base,
        id->p_encoder->fmt_in.video.i_frame_rate,
        id->p_encoder->fmt_in.video.i_frame_rate_base,
        0 );
     msg_Dbg( p_stream, "source fps %u/%u, destination %u/%u",
        id->p_decoder->fmt_out.video.i_frame_rate,
        id->p_decoder->fmt_out.video.i_frame_rate_base,
        id->p_encoder->fmt_in.video.i_frame_rate,
        id->p_encoder->fmt_in.video.i_frame_rate_base );
}

static void transcode_video_size_init( sout_stream_t *p_stream,
                                     sout_stream_id_sys_t *id,
                                     const video_format_t *p_vid_out )
{
    sout_stream_sys_t *p_sys = p_stream->p_sys;

    /* Calculate scaling
     * width/height of source */
    int i_src_visible_width = p_vid_out->i_visible_width;
    int i_src_visible_height = p_vid_out->i_visible_height;

    if (i_src_visible_width == 0)
        i_src_visible_width = p_vid_out->i_width;
    if (i_src_visible_height == 0)
        i_src_visible_height = p_vid_out->i_height;


    /* with/height scaling */
    float f_scale_width = 1;
    float f_scale_height = 1;

    /* aspect ratio */
    float f_aspect = (double)p_vid_out->i_sar_num *
                     p_vid_out->i_width /
                     p_vid_out->i_sar_den /
                     p_vid_out->i_height;

    msg_Dbg( p_stream, "decoder aspect is %f:1", f_aspect );

    /* Change f_aspect from source frame to source pixel */
    f_aspect = f_aspect * i_src_visible_height / i_src_visible_width;
    msg_Dbg( p_stream, "source pixel aspect is %f:1", f_aspect );

    /* Calculate scaling factor for specified parameters */
    if( id->p_encoder->fmt_out.video.i_visible_width <= 0 &&
        id->p_encoder->fmt_out.video.i_visible_height <= 0 && p_sys->f_scale )
    {
        /* Global scaling. Make sure width will remain a factor of 16 */
        float f_real_scale;
        int  i_new_height;
        int i_new_width = i_src_visible_width * p_sys->f_scale;

        if( i_new_width % 16 <= 7 && i_new_width >= 16 )
            i_new_width -= i_new_width % 16;
        else
            i_new_width += 16 - i_new_width % 16;

        f_real_scale = (float)( i_new_width ) / (float) i_src_visible_width;

        i_new_height = __MAX( 16, i_src_visible_height * (float)f_real_scale );

        f_scale_width = f_real_scale;
        f_scale_height = (float) i_new_height / (float) i_src_visible_height;
    }
    else if( id->p_encoder->fmt_out.video.i_visible_width > 0 &&
             id->p_encoder->fmt_out.video.i_visible_height <= 0 )
    {
        /* Only width specified */
        f_scale_width = (float)id->p_encoder->fmt_out.video.i_visible_width/i_src_visible_width;
        f_scale_height = f_scale_width;
    }
    else if( id->p_encoder->fmt_out.video.i_visible_width <= 0 &&
             id->p_encoder->fmt_out.video.i_visible_height > 0 )
    {
         /* Only height specified */
         f_scale_height = (float)id->p_encoder->fmt_out.video.i_visible_height/i_src_visible_height;
         f_scale_width = f_scale_height;
     }
     else if( id->p_encoder->fmt_out.video.i_visible_width > 0 &&
              id->p_encoder->fmt_out.video.i_visible_height > 0 )
     {
         /* Width and height specified */
         f_scale_width = (float)id->p_encoder->fmt_out.video.i_visible_width/i_src_visible_width;
         f_scale_height = (float)id->p_encoder->fmt_out.video.i_visible_height/i_src_visible_height;
     }

     /* check maxwidth and maxheight */
     if( p_sys->i_maxwidth && f_scale_width > (float)p_sys->i_maxwidth /
                                                     i_src_visible_width )
     {
         f_scale_width = (float)p_sys->i_maxwidth / i_src_visible_width;
     }

     if( p_sys->i_maxheight && f_scale_height > (float)p_sys->i_maxheight /
                                                       i_src_visible_height )
     {
         f_scale_height = (float)p_sys->i_maxheight / i_src_visible_height;
     }


     /* Change aspect ratio from source pixel to scaled pixel */
     f_aspect = f_aspect * f_scale_height / f_scale_width;
     msg_Dbg( p_stream, "scaled pixel aspect is %f:1", f_aspect );

     /* f_scale_width and f_scale_height are now final */
     /* Calculate width, height from scaling
      * Make sure its multiple of 2
      */
     /* width/height of output stream */
     int i_dst_visible_width =  lroundf(f_scale_width*i_src_visible_width);
     int i_dst_visible_height = lroundf(f_scale_height*i_src_visible_height);
     int i_dst_width =  lroundf(f_scale_width*p_vid_out->i_width);
     int i_dst_height = lroundf(f_scale_height*p_vid_out->i_height);

     if( i_dst_visible_width & 1 ) ++i_dst_visible_width;
     if( i_dst_visible_height & 1 ) ++i_dst_visible_height;
     if( i_dst_width & 1 ) ++i_dst_width;
     if( i_dst_height & 1 ) ++i_dst_height;

     /* Store calculated values */
     id->p_encoder->fmt_out.video.i_width = i_dst_width;
     id->p_encoder->fmt_out.video.i_visible_width = i_dst_visible_width;
     id->p_encoder->fmt_out.video.i_height = i_dst_height;
     id->p_encoder->fmt_out.video.i_visible_height = i_dst_visible_height;

     id->p_encoder->fmt_in.video.i_width = i_dst_width;
     id->p_encoder->fmt_in.video.i_visible_width = i_dst_visible_width;
     id->p_encoder->fmt_in.video.i_height = i_dst_height;
     id->p_encoder->fmt_in.video.i_visible_height = i_dst_visible_height;

     msg_Dbg( p_stream, "source %ix%i, destination %ix%i",
         i_src_visible_width, i_src_visible_height,
         i_dst_visible_width, i_dst_visible_height
     );
}

static void transcode_video_sar_init( sout_stream_t *p_stream,
                                     sout_stream_id_sys_t *id,
                                     const video_format_t *p_vid_out )
{
    int i_src_visible_width = p_vid_out->i_visible_width;
    int i_src_visible_height = p_vid_out->i_visible_height;

    if (i_src_visible_width == 0)
        i_src_visible_width = p_vid_out->i_width;
    if (i_src_visible_height == 0)
        i_src_visible_height = p_vid_out->i_height;

    /* Check whether a particular aspect ratio was requested */
    if( id->p_encoder->fmt_out.video.i_sar_num <= 0 ||
        id->p_encoder->fmt_out.video.i_sar_den <= 0 )
    {
        vlc_ureduce( &id->p_encoder->fmt_out.video.i_sar_num,
                     &id->p_encoder->fmt_out.video.i_sar_den,
                     (uint64_t)p_vid_out->i_sar_num * id->p_encoder->fmt_out.video.i_width * p_vid_out->i_height,
                     (uint64_t)p_vid_out->i_sar_den * id->p_encoder->fmt_out.video.i_height * p_vid_out->i_width,
                     0 );
    }
    else
    {
        vlc_ureduce( &id->p_encoder->fmt_out.video.i_sar_num,
                     &id->p_encoder->fmt_out.video.i_sar_den,
                     id->p_encoder->fmt_out.video.i_sar_num,
                     id->p_encoder->fmt_out.video.i_sar_den,
                     0 );
    }

    id->p_encoder->fmt_in.video.i_sar_num =
        id->p_encoder->fmt_out.video.i_sar_num;
    id->p_encoder->fmt_in.video.i_sar_den =
        id->p_encoder->fmt_out.video.i_sar_den;

    msg_Dbg( p_stream, "encoder aspect is %i:%i",
             id->p_encoder->fmt_out.video.i_sar_num * id->p_encoder->fmt_out.video.i_width,
             id->p_encoder->fmt_out.video.i_sar_den * id->p_encoder->fmt_out.video.i_height );

}

static void transcode_video_encoder_init( sout_stream_t *p_stream,
                                          sout_stream_id_sys_t *id,
                                          picture_t *p_pic )
{
    const video_format_t *p_vid_out = video_output_format( id, p_pic );

    id->p_encoder->fmt_in.video.orientation =
        id->p_encoder->fmt_out.video.orientation =
        id->p_decoder->fmt_in.video.orientation;

    transcode_video_framerate_init( p_stream, id, p_vid_out );

    transcode_video_size_init( p_stream, id, p_vid_out );
    transcode_video_sar_init( p_stream, id, p_vid_out );

    msg_Dbg( p_stream, "source chroma: %4.4s, destination %4.4s",
             (const char *)&id->p_decoder->fmt_out.video.i_chroma,
             (const char *)&id->p_encoder->fmt_in.video.i_chroma);
}

static int transcode_video_encoder_open( sout_stream_t *p_stream,
                                         sout_stream_id_sys_t *id )
{
    sout_stream_sys_t *p_sys = p_stream->p_sys;


    msg_Dbg( p_stream, "destination (after video filters) %ix%i",
             id->p_encoder->fmt_in.video.i_width,
             id->p_encoder->fmt_in.video.i_height );

    id->p_encoder->p_module =
        module_need( id->p_encoder, "encoder", p_sys->psz_venc, true );
    if( !id->p_encoder->p_module )
    {
        msg_Err( p_stream, "cannot find video encoder (module:%s fourcc:%4.4s)",
                 p_sys->psz_venc ? p_sys->psz_venc : "any",
                 (char *)&p_sys->i_vcodec );
        return VLC_EGENERIC;
    }

    id->p_encoder->fmt_in.video.i_chroma = id->p_encoder->fmt_in.i_codec;

    /*  */
    id->p_encoder->fmt_out.i_codec =
        vlc_fourcc_GetCodec( VIDEO_ES, id->p_encoder->fmt_out.i_codec );

    id->id = sout_StreamIdAdd( p_stream->p_next, &id->p_encoder->fmt_out );
    if( !id->id )
    {
        msg_Err( p_stream, "cannot add this stream" );
        return VLC_EGENERIC;
    }

    return VLC_SUCCESS;
}

void transcode_video_close( sout_stream_t *p_stream,
                                   sout_stream_id_sys_t *id )
{
    if( p_stream->p_sys->i_threads >= 1 && !p_stream->p_sys->b_abort )
    {
        vlc_mutex_lock( &p_stream->p_sys->lock_out );
        p_stream->p_sys->b_abort = true;
        vlc_cond_signal( &p_stream->p_sys->cond );
        vlc_mutex_unlock( &p_stream->p_sys->lock_out );

        vlc_join( p_stream->p_sys->thread, NULL );

        picture_fifo_Delete( p_stream->p_sys->pp_pics );
        block_ChainRelease( p_stream->p_sys->p_buffers );
    }

    if( p_stream->p_sys->i_threads >= 1 )
    {
        vlc_mutex_destroy( &p_stream->p_sys->lock_out );
        vlc_cond_destroy( &p_stream->p_sys->cond );
    }

    /* Close decoder */
    if( id->p_decoder->p_module )
        module_unneed( id->p_decoder, id->p_decoder->p_module );
    if( id->p_decoder->p_description )
        vlc_meta_Delete( id->p_decoder->p_description );

    /* Close encoder */
    if( id->p_encoder->p_module )
        module_unneed( id->p_encoder, id->p_encoder->p_module );

    /* Close filters */
    if( id->p_f_chain )
        filter_chain_Delete( id->p_f_chain );
    if( id->p_uf_chain )
        filter_chain_Delete( id->p_uf_chain );
}

static void OutputFrame( sout_stream_t *p_stream, picture_t *p_pic, sout_stream_id_sys_t *id, block_t **out )
{
    sout_stream_sys_t *p_sys = p_stream->p_sys;

    /*
     * Encoding
     */
    /* Check if we have a subpicture to overlay */
    if( p_sys->p_spu )
    {
        video_format_t fmt = id->p_encoder->fmt_in.video;
        if( fmt.i_visible_width <= 0 || fmt.i_visible_height <= 0 )
        {
            fmt.i_visible_width  = fmt.i_width;
            fmt.i_visible_height = fmt.i_height;
            fmt.i_x_offset       = 0;
            fmt.i_y_offset       = 0;
        }

        subpicture_t *p_subpic = spu_Render( p_sys->p_spu, NULL, &fmt,
                                             &id->p_decoder->fmt_out.video,
                                             p_pic->date, p_pic->date, false );

        /* Overlay subpicture */
        if( p_subpic )
        {
            if( filter_chain_IsEmpty( id->p_f_chain ) )
            {
                /* We can't modify the picture, we need to duplicate it,
                 * in this point the picture is already p_encoder->fmt.in format*/
                picture_t *p_tmp = video_new_buffer_encoder( id->p_encoder );
                if( likely( p_tmp ) )
                {
                    picture_Copy( p_tmp, p_pic );
                    picture_Release( p_pic );
                    p_pic = p_tmp;
                }
            }
            if( unlikely( !p_sys->p_spu_blend ) )
                p_sys->p_spu_blend = filter_NewBlend( VLC_OBJECT( p_sys->p_spu ), &fmt );
            if( likely( p_sys->p_spu_blend ) )
                picture_BlendSubpicture( p_pic, p_sys->p_spu_blend, p_subpic );
            subpicture_Delete( p_subpic );
        }
    }

    if( p_sys->i_threads == 0 )
    {
        block_t *p_block;

        p_block = id->p_encoder->pf_encode_video( id->p_encoder, p_pic );
        block_ChainAppend( out, p_block );
    }

    if( p_sys->i_threads )
    {
        vlc_sem_wait( &p_sys->picture_pool_has_room );
        vlc_mutex_lock( &p_sys->lock_out );
        picture_fifo_Push( p_sys->pp_pics, p_pic );
        vlc_cond_signal( &p_sys->cond );
        vlc_mutex_unlock( &p_sys->lock_out );
    }

    if ( p_sys->i_threads == 0 )
        picture_Release( p_pic );
}

int transcode_video_process( sout_stream_t *p_stream, sout_stream_id_sys_t *id,
                                    block_t *in, block_t **out )
{
    sout_stream_sys_t *p_sys = p_stream->p_sys;
    *out = NULL;

    int ret = id->p_decoder->pf_decode( id->p_decoder, in );
    if( ret != VLCDEC_SUCCESS )
        return VLC_EGENERIC;

    picture_t *p_pics = transcode_dequeue_all_pics( id );
    if( p_pics == NULL )
        goto end;

    do
    {
        picture_t *p_pic = p_pics;
        p_pics = p_pics->p_next;
        p_pic->p_next = NULL;

        if( id->b_error )
        {
            picture_Release( p_pic );
            continue;
        }

        if( unlikely (
             id->p_encoder->p_module && p_pic &&
             !video_format_IsSimilar( &id->fmt_input_video, &p_pic->format )
            )
          )
        {
            msg_Info( p_stream, "aspect-ratio changed, reiniting. %i -> %i : %i -> %i.",
                        id->fmt_input_video.i_sar_num, p_pic->format.i_sar_num,
                        id->fmt_input_video.i_sar_den, p_pic->format.i_sar_den
                    );
            /* Close filters */
            if( id->p_f_chain )
                filter_chain_Delete( id->p_f_chain );
            id->p_f_chain = NULL;
            if( id->p_uf_chain )
                filter_chain_Delete( id->p_uf_chain );
            id->p_uf_chain = NULL;

            /* Reinitialize filters */
            id->p_encoder->fmt_out.video.i_visible_width  = p_sys->i_width & ~1;
            id->p_encoder->fmt_out.video.i_visible_height = p_sys->i_height & ~1;
            id->p_encoder->fmt_out.video.i_sar_num = id->p_encoder->fmt_out.video.i_sar_den = 0;

            transcode_video_encoder_init( p_stream, id, p_pic );
            transcode_video_filter_init( p_stream, id );
            if( conversion_video_filter_append( id, p_pic ) != VLC_SUCCESS )
                goto error;
            memcpy( &id->fmt_input_video, &p_pic->format, sizeof(video_format_t));
        }


        if( unlikely( !id->p_encoder->p_module && p_pic ) )
        {
            if( id->p_f_chain )
                filter_chain_Delete( id->p_f_chain );
            if( id->p_uf_chain )
                filter_chain_Delete( id->p_uf_chain );
            id->p_f_chain = id->p_uf_chain = NULL;

            transcode_video_encoder_init( p_stream, id, p_pic );
            transcode_video_filter_init( p_stream, id );
            if( conversion_video_filter_append( id, p_pic ) != VLC_SUCCESS )
                goto error;
            memcpy( &id->fmt_input_video, &p_pic->format, sizeof(video_format_t));

            if( transcode_video_encoder_open( p_stream, id ) != VLC_SUCCESS )
                goto error;
        }

        /* Run the filter and output chains; first with the picture,
         * and then with NULL as many times as we need until they
         * stop outputting frames.
         */
        for ( ;; ) {
            picture_t *p_filtered_pic = p_pic;

            /* Run filter chain */
            if( id->p_f_chain )
                p_filtered_pic = filter_chain_VideoFilter( id->p_f_chain, p_filtered_pic );
            if( !p_filtered_pic )
                break;

            for ( ;; ) {
                picture_t *p_user_filtered_pic = p_filtered_pic;

                /* Run user specified filter chain */
                if( id->p_uf_chain )
                    p_user_filtered_pic = filter_chain_VideoFilter( id->p_uf_chain, p_user_filtered_pic );
                if( !p_user_filtered_pic )
                    break;

                OutputFrame( p_stream, p_user_filtered_pic, id, out );

                p_filtered_pic = NULL;
            }

            p_pic = NULL;
        }
        continue;
error:
        if( p_pic )
            picture_Release( p_pic );
        id->b_error = true;
    } while( p_pics );

    if( p_sys->i_threads >= 1 )
    {
        /* Pick up any return data the encoder thread wants to output. */
        vlc_mutex_lock( &p_sys->lock_out );
        *out = p_sys->p_buffers;
        p_sys->p_buffers = NULL;
        vlc_mutex_unlock( &p_sys->lock_out );
    }

end:
    /* Drain encoder */
    if( unlikely( !id->b_error && in == NULL ) )
    {
        if( p_sys->i_threads == 0 )
        {
            if( id->p_encoder->p_module )
            {
                block_t *p_block;
                do {
                    p_block = id->p_encoder->pf_encode_video(id->p_encoder, NULL );
                    block_ChainAppend( out, p_block );
                } while( p_block );
            }
        }
        else
        {
            msg_Dbg( p_stream, "Flushing thread and waiting that");
            vlc_mutex_lock( &p_stream->p_sys->lock_out );
            p_stream->p_sys->b_abort = true;
            vlc_cond_signal( &p_stream->p_sys->cond );
            vlc_mutex_unlock( &p_stream->p_sys->lock_out );

            vlc_join( p_stream->p_sys->thread, NULL );
            vlc_mutex_lock( &p_sys->lock_out );
            *out = p_sys->p_buffers;
            p_sys->p_buffers = NULL;
            vlc_mutex_unlock( &p_sys->lock_out );

            msg_Dbg( p_stream, "Flushing done");
        }
    }

    return id->b_error ? VLC_EGENERIC : VLC_SUCCESS;
}

bool transcode_video_add( sout_stream_t *p_stream, const es_format_t *p_fmt,
                                sout_stream_id_sys_t *id )
{
    sout_stream_sys_t *p_sys = p_stream->p_sys;

    msg_Dbg( p_stream,
             "creating video transcoding from fcc=`%4.4s' to fcc=`%4.4s'",
             (char*)&p_fmt->i_codec, (char*)&p_sys->i_vcodec );

    id->fifo.pic.first = NULL;
    id->fifo.pic.last = &id->fifo.pic.first;

    /* Complete destination format */
    id->p_encoder->fmt_out.i_codec = p_sys->i_vcodec;
    id->p_encoder->fmt_out.video.i_visible_width  = p_sys->i_width & ~1;
    id->p_encoder->fmt_out.video.i_visible_height = p_sys->i_height & ~1;
    id->p_encoder->fmt_out.i_bitrate = p_sys->i_vbitrate;

    /* Build decoder -> filter -> encoder chain */
    if( transcode_video_new( p_stream, id ) )
    {
        msg_Err( p_stream, "cannot create video chain" );
        return false;
    }

    /* Stream will be added later on because we don't know
     * all the characteristics of the decoded stream yet */
    id->b_transcode = true;

    if( p_sys->fps_num )
    {
        id->p_encoder->fmt_in.video.i_frame_rate = id->p_encoder->fmt_out.video.i_frame_rate = (p_sys->fps_num );
        id->p_encoder->fmt_in.video.i_frame_rate_base = id->p_encoder->fmt_out.video.i_frame_rate_base = (p_sys->fps_den ? p_sys->fps_den : 1);
    }

    return true;
}

