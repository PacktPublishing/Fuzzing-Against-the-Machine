/*
 * Demuxer.cpp
 *****************************************************************************
 * Copyright © 2015 - VideoLAN and VLC Authors
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
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

#include "Demuxer.hpp"

#include <vlc_stream.h>
#include <vlc_demux.h>
#include "SourceStream.hpp"
#include "../StreamFormat.hpp"
#include "CommandsQueue.hpp"
#include "../AbstractSource.hpp"

using namespace adaptive;

AbstractDemuxer::AbstractDemuxer()
{
    b_startsfromzero = false;
    b_reinitsonseek = true;
    b_candetectswitches = true;
    b_alwaysrestarts = false;
}

AbstractDemuxer::~AbstractDemuxer()
{

}

bool AbstractDemuxer::alwaysStartsFromZero() const
{
    return b_startsfromzero;
}

bool AbstractDemuxer::bitstreamSwitchCompatible() const
{
    return b_candetectswitches;
}

bool AbstractDemuxer::needsRestartOnEachSegment() const
{
    return b_alwaysrestarts;
}

void AbstractDemuxer::setBitstreamSwitchCompatible( bool b )
{
    b_candetectswitches = b;
}

void AbstractDemuxer::setRestartsOnEachSegment( bool b )
{
    b_alwaysrestarts = b;
}

bool AbstractDemuxer::needsRestartOnSeek() const
{
    return b_reinitsonseek;
}

AbstractDemuxer::Status AbstractDemuxer::returnCode(int i_ret)
{
    switch(i_ret)
    {
        case VLC_DEMUXER_SUCCESS:
            return Status::Success;
        case VLC_DEMUXER_EGENERIC:
            return Status::Eof;
        default:
            return Status::Error;
    };
}

Demuxer::Demuxer(vlc_object_t *p_obj_, const std::string &name_,
                 es_out_t *out, AbstractSourceStream *source)
    : AbstractDemuxer()
{
    p_es_out = out;
    name = name_;
    p_obj = p_obj_;
    p_demux = nullptr;
    b_eof = false;
    sourcestream = source;

    if(name == "mp4")
    {
        b_candetectswitches = false;
        b_startsfromzero = true;
    }
    else if(name == "aac")
    {
        b_candetectswitches = false;
    }
}

Demuxer::~Demuxer()
{
    if(p_demux)
        demux_Delete(p_demux);
}

bool Demuxer::create()
{
    stream_t *p_newstream = sourcestream->makeStream();
    if(!p_newstream)
        return false;

    p_demux = demux_New( p_obj, name.c_str(), "",
                         p_newstream, p_es_out );
    if(!p_demux)
    {
        vlc_stream_Delete(p_newstream);
        b_eof = true;
        return false;
    }
    else
    {
        b_eof = false;
    }

    return true;
}

void Demuxer::destroy()
{
    if(p_demux)
    {
        demux_Delete(p_demux);
        p_demux = nullptr;
    }
    sourcestream->Reset();
}

void Demuxer::drain()
{
    while(p_demux && demux_Demux(p_demux) == VLC_DEMUXER_SUCCESS);
}

Demuxer::Status Demuxer::demux(mtime_t)
{
    if(!p_demux || b_eof)
        return Status::Eof;
    int i_ret = demux_Demux(p_demux);
    if(i_ret != VLC_DEMUXER_SUCCESS)
        b_eof = true;
    return returnCode(i_ret);
}

SlaveDemuxer::SlaveDemuxer(vlc_object_t *p_obj, const std::string &name,
                           es_out_t *out, AbstractSourceStream *source)
    : Demuxer(p_obj, name, out, source)
{
    length = VLC_TS_INVALID;
    b_reinitsonseek = false;
    b_startsfromzero = false;
}

SlaveDemuxer::~SlaveDemuxer()
{

}

bool SlaveDemuxer::create()
{
    if(Demuxer::create())
    {
        length = VLC_TS_INVALID;
        if(demux_Control(p_demux, DEMUX_GET_LENGTH, &length) != VLC_SUCCESS)
            b_eof = true;
        return true;
    }
    return false;
}

AbstractDemuxer::Status SlaveDemuxer::demux(mtime_t nz_deadline)
{
    /* Always call with increment or buffering will get slow stuck */
    mtime_t i_next_demux_time = VLC_TS_0 + nz_deadline + CLOCK_FREQ / 4;
    if( demux_Control(p_demux, DEMUX_SET_NEXT_DEMUX_TIME, i_next_demux_time ) != VLC_SUCCESS )
    {
        b_eof = true;
        return Status::Eof;
    }
    Status status = Demuxer::demux(i_next_demux_time);
    es_out_Control(p_es_out, ES_OUT_SET_GROUP_PCR, 0, i_next_demux_time);
    return status;
}
