/*****************************************************************************
 * vhs.c : VHS effect video filter
 *****************************************************************************
 * Copyright (C) 2013      Vianney Boyer
 * $Id: 51bbcaafcbf24734cf01407ab9b2b4e3fa9737ce $
 *
 * Authors: Vianney Boyer <vlcvboyer -at- gmail -dot- com>
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
#   include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_filter.h>
#include <vlc_picture.h>
#include <vlc_rand.h>
#include <vlc_mtime.h>

#include "filter_picture.h"

static inline int64_t MOD(int64_t a, int64_t b) {
    return ( ( a % b ) + b ) % b; }

#define MAX_BLUE_RED_LINES 100

typedef struct {
    int32_t  i_offset;
    uint16_t i_intensity;
    bool     b_blue_red;
    mtime_t  i_stop_trigger;
} blue_red_line_t;

struct filter_sys_t {

    /* general data */
    bool b_init;
    int32_t  i_planes;
    int32_t *i_height; /* note: each plane may have different dimensions */
    int32_t *i_width;
    int32_t *i_visible_pitch;
    mtime_t  i_start_time;
    mtime_t  i_last_time;
    mtime_t  i_cur_time;

    /* sliding & offset effect */
    int32_t  i_phase_speed;
    int32_t  i_phase_ofs;
    int32_t  i_offset_ofs;
    int32_t  i_sliding_ofs;
    int32_t  i_sliding_speed;
    mtime_t  i_offset_trigger;
    mtime_t  i_sliding_trigger;
    mtime_t  i_sliding_stop_trig;
    bool     i_sliding_type_duplicate;

    /* blue red lines effect */
    mtime_t  i_BR_line_trigger;
    blue_red_line_t *p_BR_lines[MAX_BLUE_RED_LINES];

};

/*****************************************************************************
 * Prototypes
 *****************************************************************************/

static picture_t *Filter( filter_t *, picture_t * );

static int  vhs_allocate_data( filter_t *, picture_t * );
static void vhs_free_allocated_data( filter_t * );

static int  vhs_blue_red_line_effect( filter_t *, picture_t * );
static void vhs_blue_red_dots_effect( filter_t *, picture_t * );
static int  vhs_sliding_effect( filter_t *, picture_t * );

static int  vhs_sliding_effect_apply( filter_t *, picture_t * );

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/

static int  Open ( vlc_object_t * );
static void Close( vlc_object_t * );

vlc_module_begin()
    set_description( N_("VHS movie effect video filter") )
    set_shortname(   N_("VHS movie" ) )
    set_capability( "video filter", 0 )
    set_category( CAT_VIDEO )
    set_subcategory( SUBCAT_VIDEO_VFILTER )

    set_callbacks( Open, Close )
vlc_module_end()

/**
 * Open the filter
 */
static int Open( vlc_object_t *p_this )
{
    filter_t *p_filter = (filter_t*)p_this;
    filter_sys_t *p_sys;

    /* Assert video in match with video out */
    if( !es_format_IsSimilar( &p_filter->fmt_in, &p_filter->fmt_out ) ) {
        msg_Err( p_filter, "Input and output format does not match" );
        return VLC_EGENERIC;
    }

    /* Reject 0 bpp and unsupported chroma */
    const vlc_fourcc_t fourcc = p_filter->fmt_in.video.i_chroma;
    const vlc_chroma_description_t *p_chroma =
        vlc_fourcc_GetChromaDescription( p_filter->fmt_in.video.i_chroma );
    if( !p_chroma || p_chroma->pixel_size == 0
        || p_chroma->plane_count < 3 || p_chroma->pixel_size > 1
        || !vlc_fourcc_IsYUV( fourcc ) )
    {
        msg_Err( p_filter, "Unsupported chroma (%4.4s)", (char*)&fourcc );
        return VLC_EGENERIC;
    }

    /* Allocate structure */
    p_filter->p_sys = p_sys = calloc(1, sizeof(*p_sys) );
    if( unlikely( !p_sys ) )
        return VLC_ENOMEM;

    /* init data */
    p_filter->pf_video_filter = Filter;
    p_sys->i_start_time = p_sys->i_cur_time = p_sys->i_last_time = mdate();

    return VLC_SUCCESS;
}

/**
 * Close the filter
 */
static void Close( vlc_object_t *p_this ) {
    filter_t *p_filter = (filter_t*)p_this;
    filter_sys_t *p_sys = p_filter->p_sys;

    /* Free allocated memory */
    vhs_free_allocated_data( p_filter );
    free( p_sys );
}

/**
 * Filter a picture
 */
static picture_t *Filter( filter_t *p_filter, picture_t *p_pic_in ) {
    if( unlikely( !p_pic_in || !p_filter) )
        return NULL;

    filter_sys_t *p_sys = p_filter->p_sys;

    picture_t *p_pic_out = filter_NewPicture( p_filter );
    if( unlikely( !p_pic_out ) ) {
        picture_Release( p_pic_in );
        return NULL;
    }

   /*
    * manage time
    */
    p_sys->i_last_time = p_sys->i_cur_time;
    p_sys->i_cur_time = mdate();

   /*
    * allocate data
    */
    if ( unlikely( !p_sys->b_init ) )
        if ( unlikely( vhs_allocate_data( p_filter, p_pic_in ) != VLC_SUCCESS ) ) {
            picture_Release( p_pic_in );
            return NULL;
        }
    p_sys->b_init = true;

   /*
    * preset output pic: raw copy src to dst
    */
    picture_CopyPixels(p_pic_out, p_pic_in);

   /*
    * apply effects on picture
    */
    if ( unlikely( vhs_blue_red_line_effect( p_filter, p_pic_out ) != VLC_SUCCESS ) )
        return CopyInfoAndRelease( p_pic_out, p_pic_in );

    if ( unlikely( vhs_sliding_effect(p_filter, p_pic_out ) != VLC_SUCCESS ) )
        return CopyInfoAndRelease( p_pic_out, p_pic_in );

    vhs_blue_red_dots_effect( p_filter, p_pic_out );

    return CopyInfoAndRelease( p_pic_out, p_pic_in );
}

/*
 * Allocate data
 */
static int vhs_allocate_data( filter_t *p_filter, picture_t *p_pic_in ) {
    filter_sys_t *p_sys = p_filter->p_sys;

    vhs_free_allocated_data( p_filter );

   /*
    * take into account different characteristics for each plane
    */
    p_sys->i_planes = p_pic_in->i_planes;
    p_sys->i_height = calloc( p_sys->i_planes, sizeof(int32_t) );
    p_sys->i_width  = calloc( p_sys->i_planes, sizeof(int32_t) );
    p_sys->i_visible_pitch = calloc( p_sys->i_planes, sizeof(int32_t) );

    if( unlikely( !p_sys->i_height || !p_sys->i_width || !p_sys->i_visible_pitch ) ) {
        vhs_free_allocated_data( p_filter );
        return VLC_ENOMEM;
    }

    for ( int32_t i_p = 0; i_p < p_sys->i_planes; i_p++) {
        p_sys->i_visible_pitch [i_p] = (int) p_pic_in->p[i_p].i_visible_pitch;
        p_sys->i_height[i_p] = (int) p_pic_in->p[i_p].i_visible_lines;
        p_sys->i_width [i_p] = (int) p_pic_in->p[i_p].i_visible_pitch / p_pic_in->p[i_p].i_pixel_pitch;
    }
    return VLC_SUCCESS;
}

/**
 * Free allocated data
 */
static void vhs_free_allocated_data( filter_t *p_filter ) {
    filter_sys_t *p_sys = p_filter->p_sys;

    for ( uint32_t i_b = 0; i_b < MAX_BLUE_RED_LINES; i_b++ )
        FREENULL( p_sys->p_BR_lines[i_b] );

    p_sys->i_planes = 0;
    FREENULL( p_sys->i_height );
    FREENULL( p_sys->i_width );
    FREENULL( p_sys->i_visible_pitch );
}


/**
 * Horizontal blue or red lines random management and effect
 */
static int vhs_blue_red_line_effect( filter_t *p_filter, picture_t *p_pic_out ) {
    filter_sys_t *p_sys = p_filter->p_sys;

#define BR_LINES_GENERATOR_PERIOD ( CLOCK_FREQ * 50 )
#define BR_LINES_DURATION         ( CLOCK_FREQ * 1/50 )

    /* generate new blue or red lines */
    if ( p_sys->i_BR_line_trigger <= p_sys->i_cur_time ) {
        for ( uint32_t i_b = 0; i_b < MAX_BLUE_RED_LINES; i_b++ )
            if (p_sys->p_BR_lines[i_b] == NULL) {
                /* allocate data */
                p_sys->p_BR_lines[i_b] = calloc( 1, sizeof(blue_red_line_t) );
                if ( unlikely( !p_sys->p_BR_lines[i_b] ) )
                    return VLC_ENOMEM;

                /* set random parameters */
                p_sys->p_BR_lines[i_b]->i_offset = (unsigned)vlc_mrand48()
                                                 % __MAX( 1, p_sys->i_height[Y_PLANE] - 10 )
                                                 + 5;

                p_sys->p_BR_lines[i_b]->b_blue_red = (unsigned)vlc_mrand48() & 0x01;

                p_sys->p_BR_lines[i_b]->i_stop_trigger = p_sys->i_cur_time
                                                       + (uint64_t)vlc_mrand48() % BR_LINES_DURATION
                                                       + BR_LINES_DURATION / 2;

                break;
            }
        p_sys->i_BR_line_trigger = p_sys->i_cur_time
                                 + (uint64_t)vlc_mrand48() % BR_LINES_GENERATOR_PERIOD
                                 + BR_LINES_GENERATOR_PERIOD / 2;
    }


    /* manage and apply current blue/red lines */
    for ( uint8_t i_b = 0; i_b < MAX_BLUE_RED_LINES; i_b++ )
        if ( p_sys->p_BR_lines[i_b] ) {
            /* remove outdated ones */
            if ( p_sys->p_BR_lines[i_b]->i_stop_trigger <= p_sys->i_cur_time ) {
                FREENULL( p_sys->p_BR_lines[i_b] );
                continue;
            }

            /* otherwise apply */
            for ( int32_t i_p=0; i_p < p_sys->i_planes; i_p++ ) {
                uint32_t i_pix_ofs = p_sys->p_BR_lines[i_b]->i_offset
                                   * p_pic_out->p[i_p].i_visible_lines
                                   / p_sys->i_height[Y_PLANE]
                                   * p_pic_out->p[i_p].i_pitch;

                switch ( i_p ) {
                  case Y_PLANE:
                    memset( &p_pic_out->p[i_p].p_pixels[i_pix_ofs], 127,
                            p_pic_out->p[i_p].i_visible_pitch);
                    break;
                  case U_PLANE:
                    memset( &p_pic_out->p[i_p].p_pixels[i_pix_ofs],
                            (p_sys->p_BR_lines[i_b]->b_blue_red?255:0),
                            p_pic_out->p[i_p].i_visible_pitch);
                    break;
                  case V_PLANE:
                    memset( &p_pic_out->p[i_p].p_pixels[i_pix_ofs],
                            (p_sys->p_BR_lines[i_b]->b_blue_red?0:255),
                            p_pic_out->p[i_p].i_visible_pitch);
                    break;
                }

            }
        }
    return VLC_SUCCESS;
}

/**
 * insert randomly blue and red dots on the picture
 */
static void vhs_blue_red_dots_effect( filter_t *p_filter, picture_t *p_pic_out ) {
#define BR_DOTS_RATIO 10000

    filter_sys_t *p_sys = p_filter->p_sys;

    for ( int32_t i_dots = 0;
          i_dots < p_sys->i_width[Y_PLANE] * p_sys->i_height[Y_PLANE] / BR_DOTS_RATIO;
          i_dots++) {

        uint32_t i_length = (unsigned)vlc_mrand48()
                          % __MAX( 1, p_sys->i_width[Y_PLANE] / 30 ) + 1;

        uint16_t i_x = (unsigned)vlc_mrand48()
                     % __MAX( 1, p_sys->i_width[Y_PLANE] - i_length );
        uint16_t i_y = (unsigned)vlc_mrand48() % p_sys->i_height[Y_PLANE];
        bool b_color = ( ( (unsigned)vlc_mrand48() % 2 ) == 0);

        for ( int32_t i_p = 0; i_p < p_sys->i_planes; i_p++ ) {
            uint32_t i_pix_ofs = i_y
                               * p_pic_out->p[i_p].i_visible_lines
                               / p_sys->i_height[Y_PLANE]
                               * p_pic_out->p[i_p].i_pitch
                               + i_x
                               * p_pic_out->p[i_p].i_pixel_pitch;

            uint32_t i_length_in_plane = i_length
                                       * p_pic_out->p[i_p].i_visible_pitch
                                       / p_pic_out->p[Y_PLANE].i_visible_pitch;

            switch ( i_p ) {
              case Y_PLANE:
                memset( &p_pic_out->p[i_p].p_pixels[i_pix_ofs], 127,
                        i_length_in_plane );
                break;
              case U_PLANE:
                memset( &p_pic_out->p[i_p].p_pixels[i_pix_ofs],
                        (b_color?255:0),
                        i_length_in_plane );
                break;
              case V_PLANE:
                memset( &p_pic_out->p[i_p].p_pixels[i_pix_ofs],
                        (b_color?0:255),
                        i_length_in_plane );
                break;
            }
        }
    }
}

/**
* sliding effects
*/
static int vhs_sliding_effect( filter_t *p_filter, picture_t *p_pic_out ) {
    filter_sys_t *p_sys = p_filter->p_sys;

    /**
    * one shot offset section
    */

#define OFFSET_AVERAGE_PERIOD   (10 * CLOCK_FREQ)

    /* start trigger to be (re)initialized */
    if ( p_sys->i_offset_trigger == 0
         || p_sys->i_sliding_speed != 0 ) { /* do not mix sliding and offset */

        /* random trigger for offset effect */
        p_sys->i_offset_trigger = p_sys->i_cur_time
                                + ((uint64_t) vlc_mrand48() ) % OFFSET_AVERAGE_PERIOD
                                + OFFSET_AVERAGE_PERIOD / 2;
        p_sys->i_offset_ofs = 0;
    } else if (p_sys->i_offset_trigger <= p_sys->i_cur_time) {
        /* trigger for offset effect occurs */
        p_sys->i_offset_trigger = 0;
        p_sys->i_offset_ofs = (uint32_t)vlc_mrand48()
                            % p_sys->i_height[Y_PLANE];
    }
    else
        p_sys->i_offset_ofs = 0;


    /**
    * phase section
    */

#define MAX_PHASE_OFS (p_sys->i_height[Y_PLANE]*100/15)

    p_sys->i_phase_speed += MOD( (int32_t)vlc_mrand48(), 3) - 1;
    p_sys->i_phase_ofs   += p_sys->i_phase_speed;
    p_sys->i_phase_ofs    = VLC_CLIP( p_sys->i_phase_ofs, -MAX_PHASE_OFS, +MAX_PHASE_OFS);
    if ( abs( p_sys->i_phase_ofs ) >= MAX_PHASE_OFS )
        p_sys->i_phase_speed = 0;


    /**
    * sliding section
    */

#define SLIDING_AVERAGE_PERIOD   (20 * CLOCK_FREQ)
#define SLIDING_AVERAGE_DURATION ( 3 * CLOCK_FREQ)

    /* start trigger to be (re)initialized */
    if ( ( p_sys->i_sliding_stop_trig  == 0 ) &&
         ( p_sys->i_sliding_trigger    == 0 ) &&
         ( p_sys->i_sliding_speed      == 0 ) ) {

        /* random trigger which enable sliding effect */
        p_sys->i_sliding_trigger = p_sys->i_cur_time
                                 + (uint64_t)vlc_mrand48() % SLIDING_AVERAGE_PERIOD
                                 + SLIDING_AVERAGE_PERIOD / 2;
    }

    /* start trigger just occurs */
    else if ( ( p_sys->i_sliding_stop_trig  == 0 ) &&
              ( p_sys->i_sliding_trigger    <= p_sys->i_cur_time ) &&
              ( p_sys->i_sliding_speed      == 0 ) ) {

        /* init sliding parameters */
        p_sys->i_sliding_trigger = 0;
        p_sys->i_sliding_stop_trig = p_sys->i_cur_time
                                   + (uint64_t)vlc_mrand48() % SLIDING_AVERAGE_DURATION
                                   + SLIDING_AVERAGE_DURATION / 2;
        p_sys->i_sliding_ofs = 0;
        /* note: sliding speed unit = image per 100 s */
        p_sys->i_sliding_speed = MOD( (int32_t)vlc_mrand48(), 1001 ) - 500;
        p_sys->i_sliding_type_duplicate = (unsigned)vlc_mrand48() & 0x01;
    }

    /* stop trigger disabling sliding effect occurs */
    else if ( ( p_sys->i_sliding_stop_trig  <= p_sys->i_cur_time )
              && ( p_sys->i_sliding_trigger == 0 ) ) {

        /* first increase speed to ensure we will stop sliding on plain pict */
        if ( abs( p_sys->i_sliding_speed ) < 5 )
            p_sys->i_sliding_speed += 1;

        /* check if offset is close to 0 and then ready to stop */
        if ( abs( p_sys->i_sliding_ofs ) < abs( p_sys->i_sliding_speed
             * p_sys->i_height[Y_PLANE]
             * ( p_sys->i_cur_time - p_sys->i_last_time ) / CLOCK_FREQ )
             || abs( p_sys->i_sliding_ofs ) < p_sys->i_height[Y_PLANE] * 100 / 20 ) {

            /* reset sliding parameters */
            p_sys->i_sliding_ofs = p_sys->i_sliding_speed = 0;
            p_sys->i_sliding_trigger = p_sys->i_sliding_stop_trig = 0;
            p_sys->i_sliding_type_duplicate = false;
        }
    }

    /* update offset */
    p_sys->i_sliding_ofs = MOD( p_sys->i_sliding_ofs
                                + p_sys->i_sliding_speed * p_sys->i_height[Y_PLANE]
                                * ( p_sys->i_cur_time - p_sys->i_last_time)
                                / CLOCK_FREQ,
                                p_sys->i_height[Y_PLANE] * 100 );

    return vhs_sliding_effect_apply( p_filter, p_pic_out );
}

/**
* apply both sliding and offset effect
*/
static int vhs_sliding_effect_apply( filter_t *p_filter, picture_t *p_pic_out )
{
    filter_sys_t *p_sys = p_filter->p_sys;

    for ( uint8_t i_p = 0; i_p < p_pic_out->i_planes; i_p++ ) {
        /* first allocate temporary buffer for swap operation */
        uint8_t *p_temp_buf;
        if ( !p_sys->i_sliding_type_duplicate ) {
            p_temp_buf= calloc( p_pic_out->p[i_p].i_lines
                                * p_pic_out->p[i_p].i_pitch, sizeof(uint8_t) );
            if ( unlikely( !p_temp_buf ) )
                return VLC_ENOMEM;
            memcpy( p_temp_buf, p_pic_out->p[i_p].p_pixels,
                    p_pic_out->p[i_p].i_lines * p_pic_out->p[i_p].i_pitch );
        }
        else
            p_temp_buf = p_pic_out->p[i_p].p_pixels;

        /* copy lines to output_pic */
        for ( int32_t i_y = 0; i_y < p_pic_out->p[i_p].i_visible_lines; i_y++ )
        {
            int32_t i_ofs = p_sys->i_offset_ofs + p_sys->i_sliding_ofs;

            if ( ( p_sys->i_sliding_speed == 0 ) || !p_sys->i_sliding_type_duplicate )
                i_ofs += p_sys->i_phase_ofs;

            i_ofs  = MOD( i_ofs / 100, p_sys->i_height[Y_PLANE] );
            i_ofs *= p_pic_out->p[i_p].i_visible_lines;
            i_ofs /= p_sys->i_height[Y_PLANE];

            memcpy( &p_pic_out->p[i_p].p_pixels[ i_y * p_pic_out->p[i_p].i_pitch ],
                    &p_temp_buf[ ( ( i_y + i_ofs ) % p_pic_out->p[i_p].i_visible_lines ) * p_pic_out->p[i_p].i_pitch ],
                    p_pic_out->p[i_p].i_visible_pitch );
        }
        if ( !p_sys->i_sliding_type_duplicate )
            free(p_temp_buf);
    }

    return VLC_SUCCESS;
}
