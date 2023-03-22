/**
 * @file x11.c
 * @brief X C Bindings video output module for VLC media player
 */
/*****************************************************************************
 * Copyright © 2009 Rémi Denis-Courmont
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
# include <config.h>
#endif

#include <stdlib.h>
#include <assert.h>

#include <xcb/xcb.h>
#include <xcb/shm.h>

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_vout_display.h>
#include <vlc_picture_pool.h>

#include "pictures.h"
#include "events.h"

static int  Open (vlc_object_t *);
static void Close (vlc_object_t *);

/*
 * Module descriptor
 */
vlc_module_begin ()
    set_shortname (N_("X11"))
    set_description (N_("X11 video output (XCB)"))
    set_category (CAT_VIDEO)
    set_subcategory (SUBCAT_VIDEO_VOUT)
    set_capability ("vout display", 100)
    set_callbacks (Open, Close)
    add_shortcut ("xcb-x11", "x11")

    add_obsolete_bool ("x11-shm") /* obsoleted since 2.0.0 */
vlc_module_end ()

/* This must be large enough to absorb the server display jitter.
 * But excessively large value is useless as direct rendering cannot be used
 * with XCB X11. */
#define MAX_PICTURES (3)

struct vout_display_sys_t
{
    xcb_connection_t *conn;
    vout_window_t *embed; /* VLC window */

    xcb_window_t window; /* drawable X window */
    xcb_gcontext_t gc; /* context to put images */
    xcb_shm_seg_t seg_base; /**< shared memory segment XID base */
    bool visible; /* whether to draw */
    uint8_t depth; /* useful bits per pixel */

    picture_pool_t *pool; /* picture pool */
};

static picture_pool_t *Pool (vout_display_t *, unsigned);
static void Display (vout_display_t *, picture_t *, subpicture_t *subpicture);
static int Control (vout_display_t *, int, va_list);

static void ResetPictures (vout_display_t *);

static const xcb_depth_t *FindDepth (const xcb_screen_t *scr,
                                     uint_fast8_t depth)
{
    xcb_depth_t *d = NULL;
    for (xcb_depth_iterator_t it = xcb_screen_allowed_depths_iterator (scr);
         it.rem > 0 && d == NULL;
         xcb_depth_next (&it))
    {
        if (it.data->depth == depth)
            d = it.data;
    }

    return d;
}


/**
 * Probe the X server.
 */
static int Open (vlc_object_t *obj)
{
    vout_display_t *vd = (vout_display_t *)obj;
    vout_display_sys_t *sys = malloc (sizeof (*sys));
    if (unlikely(sys == NULL))
        return VLC_ENOMEM;

    vd->sys = sys;
    sys->pool = NULL;

    /* Get window, connect to X server */
    xcb_connection_t *conn;
    const xcb_screen_t *scr;
    sys->embed = vlc_xcb_parent_Create(vd, &conn, &scr);
    if (sys->embed == NULL)
    {
        free (sys);
        return VLC_EGENERIC;
    }
    sys->conn = conn;

    const xcb_setup_t *setup = xcb_get_setup (conn);

    /* Determine our pixel format */
    video_format_t fmt_pic;
    xcb_visualid_t vid = 0;
    sys->depth = 0;

    for (const xcb_format_t *fmt = xcb_setup_pixmap_formats (setup),
             *end = fmt + xcb_setup_pixmap_formats_length (setup);
         fmt < end;
         fmt++)
    {
        if (fmt->depth <= sys->depth)
            continue; /* no better than earlier format */

        video_format_ApplyRotation(&fmt_pic, &vd->fmt);

        /* Check that the pixmap format is supported by VLC. */
        switch (fmt->depth)
        {
          case 32:
            if (fmt->bits_per_pixel != 32)
                continue;
            fmt_pic.i_chroma = VLC_CODEC_ARGB;
            break;
          case 24:
            if (fmt->bits_per_pixel == 32)
                fmt_pic.i_chroma = VLC_CODEC_RGB32;
            else if (fmt->bits_per_pixel == 24)
                fmt_pic.i_chroma = VLC_CODEC_RGB24;
            else
                continue;
            break;
          case 16:
            if (fmt->bits_per_pixel != 16)
                continue;
            fmt_pic.i_chroma = VLC_CODEC_RGB16;
            break;
          case 15:
            if (fmt->bits_per_pixel != 16)
                continue;
            fmt_pic.i_chroma = VLC_CODEC_RGB15;
            break;
          case 8:
            if (fmt->bits_per_pixel != 8)
                continue;
            fmt_pic.i_chroma = VLC_CODEC_RGB8;
            break;
          default:
            continue;
        }

        /* Byte sex is a non-issue for 8-bits. It can be worked around with
         * RGB masks for 24-bits. Too bad for 15-bits and 16-bits. */
        if (fmt->bits_per_pixel == 16 && setup->image_byte_order != ORDER)
            continue;

        /* Make sure the X server is sane */
        assert (fmt->bits_per_pixel > 0);
        if (unlikely(fmt->scanline_pad % fmt->bits_per_pixel))
            continue;

        /* Check that the selected screen supports this depth */
        const xcb_depth_t *d = FindDepth (scr, fmt->depth);
        if (d == NULL)
            continue;

        /* Find a visual type for the selected depth */
        const xcb_visualtype_t *vt = xcb_depth_visuals (d);

        /* First try True Color class */
        for (int i = xcb_depth_visuals_length (d); i > 0; i--)
        {
            if (vt->_class == XCB_VISUAL_CLASS_TRUE_COLOR)
            {
                fmt_pic.i_rmask = vt->red_mask;
                fmt_pic.i_gmask = vt->green_mask;
                fmt_pic.i_bmask = vt->blue_mask;
            found_visual:
                vid = vt->visual_id;
                msg_Dbg (vd, "using X11 visual ID 0x%"PRIx32, vid);
                sys->depth = fmt->depth;
                msg_Dbg (vd, " %"PRIu8" bits depth", sys->depth);
                msg_Dbg (vd, " %"PRIu8" bits per pixel", fmt->bits_per_pixel);
                msg_Dbg (vd, " %"PRIu8" bits line pad", fmt->scanline_pad);
                goto found_format;
            }
            vt++;
        }

        /* Then try Static Gray class */
        if (fmt->depth != 8)
            continue;
        vt = xcb_depth_visuals (d);
        for (int i = xcb_depth_visuals_length (d); i > 0 && !vid; i--)
        {
            if (vt->_class == XCB_VISUAL_CLASS_STATIC_GRAY)
            {
                fmt_pic.i_chroma = VLC_CODEC_GREY;
                goto found_visual;
            }
            vt++;
        }
    }

    msg_Err (obj, "no supported pixel format & visual");
    goto error;

found_format:;
    /* Create colormap (needed to select non-default visual) */
    xcb_colormap_t cmap;
    if (vid != scr->root_visual)
    {
        cmap = xcb_generate_id (conn);
        xcb_create_colormap (conn, XCB_COLORMAP_ALLOC_NONE,
                             cmap, scr->root, vid);
    }
    else
        cmap = scr->default_colormap;

    /* Create window */
    sys->window = xcb_generate_id (conn);
    sys->gc = xcb_generate_id (conn);
    xcb_pixmap_t pixmap = xcb_generate_id (conn);
    {
        const uint32_t mask =
            XCB_CW_BACK_PIXMAP |
            XCB_CW_BACK_PIXEL |
            XCB_CW_BORDER_PIXMAP |
            XCB_CW_BORDER_PIXEL |
            XCB_CW_EVENT_MASK |
            XCB_CW_COLORMAP;
        const uint32_t values[] = {
            /* XCB_CW_BACK_PIXMAP */
            pixmap,
            /* XCB_CW_BACK_PIXEL */
            scr->black_pixel,
            /* XCB_CW_BORDER_PIXMAP */
            pixmap,
            /* XCB_CW_BORDER_PIXEL */
            scr->black_pixel,
            /* XCB_CW_EVENT_MASK */
            XCB_EVENT_MASK_VISIBILITY_CHANGE,
            /* XCB_CW_COLORMAP */
            cmap,
        };
        xcb_void_cookie_t c;

        xcb_create_pixmap (conn, sys->depth, pixmap, scr->root, 1, 1);
        c = xcb_create_window_checked (conn, sys->depth, sys->window,
                                       sys->embed->handle.xid, 0, 0,
                                       vd->cfg->display.width,
                                       vd->cfg->display.height, 0,
                                       XCB_WINDOW_CLASS_INPUT_OUTPUT,
                                       vid, mask, values);
        xcb_map_window (conn, sys->window);
        /* Create graphic context (I wonder why the heck do we need this) */
        xcb_create_gc (conn, sys->gc, sys->window, 0, NULL);

        if (vlc_xcb_error_Check(vd, conn, "cannot create X11 window", c))
            goto error;
    }
    msg_Dbg (vd, "using X11 window %08"PRIx32, sys->window);
    msg_Dbg (vd, "using X11 graphic context %08"PRIx32, sys->gc);

    sys->visible = false;
    if (XCB_shm_Check (obj, conn))
    {
        sys->seg_base = xcb_generate_id (conn);
        for (unsigned i = 1; i < MAX_PICTURES; i++)
             xcb_generate_id (conn);
    }
    else
        sys->seg_base = 0;

    /* Setup vout_display_t once everything is fine */
    vd->info.has_pictures_invalid = true;

    vd->fmt = fmt_pic;
    vd->pool = Pool;
    vd->prepare = NULL;
    vd->display = Display;
    vd->control = Control;

    return VLC_SUCCESS;

error:
    Close (obj);
    return VLC_EGENERIC;
}


/**
 * Disconnect from the X server.
 */
static void Close (vlc_object_t *obj)
{
    vout_display_t *vd = (vout_display_t *)obj;
    vout_display_sys_t *sys = vd->sys;

    ResetPictures (vd);

    /* colormap, window and context are garbage-collected by X */
    xcb_disconnect (sys->conn);
    vout_display_DeleteWindow (vd, sys->embed);
    free (sys);
}

/**
 * Return a direct buffer
 */
static picture_pool_t *Pool (vout_display_t *vd, unsigned requested_count)
{
    vout_display_sys_t *sys = vd->sys;
    (void)requested_count;

    if (sys->pool)
        return sys->pool;

    vout_display_place_t place;

    vout_display_PlacePicture (&place, &vd->source, vd->cfg, false);

    /* */
    const uint32_t values[] = { place.x, place.y, place.width, place.height };
    xcb_configure_window (sys->conn, sys->window,
                          XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |
                          XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                          values);

    picture_t *pic = picture_NewFromFormat (&vd->fmt);
    if (!pic)
        return NULL;

    assert (pic->i_planes == 1);

    picture_resource_t res = {
        .p = {
            [0] = {
                .i_lines = pic->p->i_lines,
                .i_pitch = pic->p->i_pitch,
            },
        },
    };
    picture_Release (pic);

    unsigned count;
    picture_t *pic_array[MAX_PICTURES];
    const size_t size = res.p->i_pitch * res.p->i_lines;
    for (count = 0; count < MAX_PICTURES; count++)
    {
        xcb_shm_seg_t seg = (sys->seg_base != 0) ? (sys->seg_base + count) : 0;

        if (XCB_picture_Alloc (vd, &res, size, sys->conn, seg))
            break;
        pic_array[count] = XCB_picture_NewFromResource (&vd->fmt, &res,
                                                        sys->conn);
        if (unlikely(pic_array[count] == NULL))
            break;
    }
    xcb_flush (sys->conn);

    if (count == 0)
        return NULL;

    sys->pool = picture_pool_New (count, pic_array);
    if (unlikely(sys->pool == NULL))
        while (count > 0)
            picture_Release(pic_array[--count]);
    return sys->pool;
}

/**
 * Sends an image to the X server.
 */
static void Display (vout_display_t *vd, picture_t *pic, subpicture_t *subpicture)
{
    vout_display_sys_t *sys = vd->sys;
    xcb_shm_seg_t segment = XCB_picture_GetSegment(pic);
    xcb_void_cookie_t ck;

    vlc_xcb_Manage(vd, sys->conn, &sys->visible);

    if (!sys->visible)
        goto out;
    if (segment != 0)
        ck = xcb_shm_put_image_checked (sys->conn, sys->window, sys->gc,
          /* real width */ pic->p->i_pitch / pic->p->i_pixel_pitch,
         /* real height */ pic->p->i_lines,
                   /* x */ vd->fmt.i_x_offset,
                   /* y */ vd->fmt.i_y_offset,
               /* width */ vd->fmt.i_visible_width,
              /* height */ vd->fmt.i_visible_height,
                           0, 0, sys->depth, XCB_IMAGE_FORMAT_Z_PIXMAP,
                           0, segment, 0);
    else
    {
        const size_t offset = vd->fmt.i_y_offset * pic->p->i_pitch;
        const unsigned lines = pic->p->i_lines - vd->fmt.i_y_offset;

        ck = xcb_put_image_checked (sys->conn, XCB_IMAGE_FORMAT_Z_PIXMAP,
                       sys->window, sys->gc,
                       pic->p->i_pitch / pic->p->i_pixel_pitch,
                       lines, -vd->fmt.i_x_offset, 0, 0, sys->depth,
                       pic->p->i_pitch * lines, pic->p->p_pixels + offset);
    }

    /* Wait for reply. This makes sure that the X server gets CPU time to
     * display the picture. xcb_flush() is *not* sufficient: especially with
     * shared memory the PUT requests are so short that many of them can fit in
     * X11 socket output buffer before the kernel preempts VLC. */
    xcb_generic_error_t *e = xcb_request_check (sys->conn, ck);
    if (e != NULL)
    {
        msg_Dbg (vd, "%s: X11 error %d", "cannot put image", e->error_code);
        free (e);
    }

    /* FIXME might be WAY better to wait in some case (be carefull with
     * VOUT_DISPLAY_RESET_PICTURES if done) + does not work with
     * vout_display wrapper. */
out:
    picture_Release (pic);
    (void)subpicture;
}

static int Control (vout_display_t *vd, int query, va_list ap)
{
    vout_display_sys_t *sys = vd->sys;

    switch (query)
    {
    case VOUT_DISPLAY_CHANGE_DISPLAY_SIZE:
    {
        const vout_display_cfg_t *p_cfg =
            va_arg (ap, const vout_display_cfg_t *);
        vout_display_place_t place;

        vout_display_PlacePicture (&place, &vd->source, p_cfg, false);

        if (place.width  != vd->fmt.i_visible_width ||
            place.height != vd->fmt.i_visible_height)
        {
            vout_display_SendEventPicturesInvalid (vd);
            return VLC_SUCCESS;
        }

        /* Move the picture within the window */
        const uint32_t values[] = { place.x, place.y };
        xcb_configure_window (sys->conn, sys->window,
                              XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
                              values);
        return VLC_SUCCESS;
    }

    case VOUT_DISPLAY_CHANGE_ZOOM:
    case VOUT_DISPLAY_CHANGE_DISPLAY_FILLED:
    case VOUT_DISPLAY_CHANGE_SOURCE_ASPECT:
    case VOUT_DISPLAY_CHANGE_SOURCE_CROP:
        /* I am not sure it is always necessary, but it is way simpler ... */
        vout_display_SendEventPicturesInvalid (vd);
        return VLC_SUCCESS;

    case VOUT_DISPLAY_RESET_PICTURES:
    {
        ResetPictures (vd);

        vout_display_place_t place;
        vout_display_PlacePicture (&place, &vd->source, vd->cfg, false);

        video_format_t src;
        video_format_ApplyRotation(&src, &vd->source);

        vd->fmt.i_width  = src.i_width  * place.width / src.i_visible_width;
        vd->fmt.i_height = src.i_height * place.height / src.i_visible_height;

        vd->fmt.i_visible_width  = place.width;
        vd->fmt.i_visible_height = place.height;
        vd->fmt.i_x_offset = src.i_x_offset * place.width / src.i_visible_width;
        vd->fmt.i_y_offset = src.i_y_offset * place.height / src.i_visible_height;
        return VLC_SUCCESS;
    }

    default:
        msg_Err (vd, "Unknown request in XCB vout display");
        return VLC_EGENERIC;
    }
}

static void ResetPictures (vout_display_t *vd)
{
    vout_display_sys_t *sys = vd->sys;

    if (!sys->pool)
        return;

    if (sys->seg_base != 0)
        for (unsigned i = 0; i < MAX_PICTURES; i++)
            xcb_shm_detach (sys->conn, sys->seg_base + i);

    picture_pool_Release (sys->pool);
    sys->pool = NULL;
}
