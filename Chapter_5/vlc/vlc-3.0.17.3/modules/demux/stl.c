/*****************************************************************************
 * stl.c: EBU STL demuxer
 *****************************************************************************
 * Copyright (C) 2010 Laurent Aimar
 * $Id: 238b515cc66f481b889d219a14b1e871e7440aff $
 *
 * Authors: Laurent Aimar <fenrir _AT_ videolan _DOT_ org>
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
#include <assert.h>

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_demux.h>

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
static int  Open (vlc_object_t *);
static void Close(vlc_object_t *);

vlc_module_begin()
    set_description(N_("EBU STL subtitles parser"))
    set_category(CAT_INPUT)
    set_subcategory(SUBCAT_INPUT_DEMUX)
    set_capability("demux", 1)
    set_callbacks(Open, Close)
    add_shortcut("stl", "subtitle")
vlc_module_end()

/*****************************************************************************
 * Local definitions/prototypes
 *****************************************************************************/
typedef struct {
    mtime_t start;
    mtime_t stop;
    size_t  blocknumber;
    size_t  count;
} stl_entry_t;

struct demux_sys_t {
    size_t      count;
    stl_entry_t *index;

    es_out_id_t *es;

    size_t      current;
    int64_t     next_date;
    bool        b_slave;
    bool        b_first_time;
};

static size_t ParseInteger(uint8_t *data, size_t size)
{
    char tmp[16];
    assert(size < sizeof(tmp));
    memcpy(tmp, data, size);
    tmp[size] = '\0';

    return strtol(tmp, NULL, 10);
}
static int64_t ParseTimeCode(uint8_t *data, double fps)
{
    return INT64_C(1000000) * (data[0] * 3600 +
                               data[1] *   60 +
                               data[2] *    1 +
                               data[3] /  fps);
}
static int64_t ParseTextTimeCode(uint8_t *data, double fps)
{
    uint8_t tmp[4];
    for (int i = 0; i < 4; i++)
        tmp[i] = ParseInteger(&data[2 * i], 2);
    return ParseTimeCode(tmp, fps);
}

static int Control(demux_t *demux, int query, va_list args)
{
    demux_sys_t *sys = demux->p_sys;
    switch(query) {
    case DEMUX_CAN_SEEK:
        return vlc_stream_vaControl(demux->s, query, args);
    case DEMUX_GET_LENGTH: {
        int64_t *l = va_arg(args, int64_t *);
        *l = sys->count > 0 ? sys->index[sys->count-1].stop : 0;
        return VLC_SUCCESS;
    }
    case DEMUX_GET_TIME: {
        int64_t *t = va_arg(args, int64_t *);
        *t = sys->next_date - var_GetInteger(demux->obj.parent, "spu-delay");
        if( *t < 0 )
            *t = sys->next_date;
        return VLC_SUCCESS;
    }
    case DEMUX_SET_NEXT_DEMUX_TIME: {
        sys->b_slave = true;
        sys->next_date = va_arg(args, int64_t);
        return VLC_SUCCESS;
    }
    case DEMUX_SET_TIME: {
        int64_t t = va_arg(args, int64_t);
        for( size_t i = 0; i + 1< sys->count; i++ )
        {
            if( sys->index[i + 1].start >= t &&
                vlc_stream_Seek(demux->s, 1024 + 128LL * sys->index[i].blocknumber) == VLC_SUCCESS )
            {
                sys->current = i;
                sys->next_date = t;
                sys->b_first_time = true;
                return VLC_SUCCESS;
            }
        }
        break;
    }
    case DEMUX_SET_POSITION:
    {
        double f = va_arg( args, double );
        if(sys->count && sys->index[sys->count-1].stop > 0)
        {
            int64_t i64 = f * sys->index[sys->count-1].stop;
            return demux_Control(demux, DEMUX_SET_TIME, i64);
        }
        break;
    }
    case DEMUX_GET_POSITION:
    {
        double *pf = va_arg(args, double *);
        if(sys->current >= sys->count)
        {
            *pf = 1.0;
        }
        else if(sys->count > 0 && sys->index[sys->count-1].stop > 0)
        {
            *pf = sys->next_date - var_GetInteger(demux->obj.parent, "spu-delay");
            if(*pf < 0)
               *pf = sys->next_date;
            *pf /= sys->index[sys->count-1].stop;
        }
        else
        {
            *pf = 0.0;
        }
        return VLC_SUCCESS;
    }
    default:
        break;
    }
    return VLC_EGENERIC;
}

static int Demux(demux_t *demux)
{
    demux_sys_t *sys = demux->p_sys;

    int64_t i_barrier = sys->next_date - var_GetInteger(demux->obj.parent, "spu-delay");
    if(i_barrier < 0)
        i_barrier = sys->next_date;

    while(sys->current < sys->count &&
          sys->index[sys->current].start <= i_barrier)
    {
        stl_entry_t *s = &sys->index[sys->current];

        if (!sys->b_slave && sys->b_first_time)
        {
            es_out_SetPCR(demux->out, VLC_TS_0 + i_barrier);
            sys->b_first_time = false;
        }

        /* Might be a gap in block # */
        const uint64_t i_pos = 1024 + 128LL * s->blocknumber;
        if(i_pos != vlc_stream_Tell(demux->s) &&
           vlc_stream_Seek( demux->s, i_pos ) != VLC_SUCCESS )
            return VLC_DEMUXER_EOF;

        block_t *b = vlc_stream_Block(demux->s, 128);
        if (b && b->i_buffer == 128)
        {
            b->i_dts =
            b->i_pts = VLC_TS_0 + s->start;
            if (s->stop > s->start)
                b->i_length = s->stop - s->start;
            es_out_Send(demux->out, sys->es, b);
        }
        else
        {
            if(b)
                block_Release(b);
            return VLC_DEMUXER_EOF;
        }
        sys->current++;
    }

    if (!sys->b_slave)
    {
        es_out_SetPCR(demux->out, VLC_TS_0 + i_barrier);
        sys->next_date += CLOCK_FREQ / 8;
    }

    return sys->current < sys->count ? VLC_DEMUXER_SUCCESS : VLC_DEMUXER_EOF;
}

static int Open(vlc_object_t *object)
{
    demux_t *demux = (demux_t*)object;

    const uint8_t *peek;
    if (vlc_stream_Peek(demux->s, &peek, 11) != 11)
        return VLC_EGENERIC;

    bool is_stl_25 = !memcmp(&peek[3], "STL25.01", 8);
    bool is_stl_30 = !memcmp(&peek[3], "STL30.01", 8);
    if (!is_stl_25 && !is_stl_30)
        return VLC_EGENERIC;
    const double fps = is_stl_25 ? 25 : 30;

    uint8_t header[1024];
    if (vlc_stream_Read(demux->s, header, sizeof(header)) != sizeof(header)) {
        msg_Err(demux, "Incomplete EBU STL header");
        return VLC_EGENERIC;
    }
    const int cct = ParseInteger(&header[12], 2);
    const mtime_t program_start = ParseTextTimeCode(&header[256], fps);
    const size_t tti_count = ParseInteger(&header[238], 5);
    if (!tti_count)
        return VLC_EGENERIC;
    msg_Dbg(demux, "Detected EBU STL : CCT=%d TTI=%zu start=%8.8s %"PRId64, cct, tti_count, &header[256], program_start);

    demux_sys_t *sys = malloc(sizeof(*sys));
    if(!sys)
        return VLC_EGENERIC;

    sys->b_slave   = false;
    sys->b_first_time = true;
    sys->next_date = 0;
    sys->current   = 0;
    sys->count     = 0;
    sys->index     = calloc(tti_count, sizeof(*sys->index));
    if(!sys->index)
    {
        free(sys);
        return VLC_EGENERIC;
    }

    bool comment = false;
    stl_entry_t *s = &sys->index[0];
    s->count = 0;

    for (size_t i = 0; i < tti_count; i++) {
        uint8_t tti[16];
        if (vlc_stream_Read(demux->s, tti, 16) != 16 ||
            vlc_stream_Read(demux->s, NULL, 112) != 112) {
            msg_Warn(demux, "Incomplete EBU STL file");
            break;
        }
        const int ebn = tti[3];
        if (ebn >= 0xf0 && ebn <= 0xfd)
            continue;
        if (ebn == 0xfe)
            continue;

        if (s->count <= 0) {
            comment  = tti[15] != 0;
            s->start = ParseTimeCode(&tti[5], fps) - program_start;
            s->stop  = ParseTimeCode(&tti[9], fps) - program_start;
            s->blocknumber = i;
        }
        s->count++;
        if (ebn == 0xff && !comment)
            s = &sys->index[++sys->count];
        if (ebn == 0xff && sys->count < tti_count)
            s->count = 0;
    }

    demux->p_sys = sys;
    if (sys->count == 0 ||
        vlc_stream_Seek(demux->s, 1024 + 128LL * sys->index[0].blocknumber) != VLC_SUCCESS)
    {
        Close(object);
        return VLC_EGENERIC;
    }

    es_format_t fmt;
    es_format_Init(&fmt, SPU_ES, VLC_CODEC_EBU_STL);
    fmt.i_extra = sizeof(header);
    fmt.p_extra = header;

    sys->es = es_out_Add(demux->out, &fmt);
    fmt.i_extra = 0;
    fmt.p_extra = NULL;
    es_format_Clean(&fmt);

    if(sys->es == NULL)
    {
        Close(object);
        return VLC_EGENERIC;
    }

    demux->p_sys      = sys;
    demux->pf_demux   = Demux;
    demux->pf_control = Control;
    return VLC_SUCCESS;
}

static void Close(vlc_object_t *object)
{
    demux_t *demux = (demux_t*)object;
    demux_sys_t *sys = demux->p_sys;

    free(sys->index);
    free(sys);
}

