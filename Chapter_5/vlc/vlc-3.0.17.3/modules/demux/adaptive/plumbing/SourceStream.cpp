/*
 * SourceStream.cpp
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

#include "SourceStream.hpp"

#include "../AbstractSource.hpp"
#include "../http/Chunk.h"
#include <vlc_stream.h>
#include <vlc_demux.h>

using namespace adaptive;

AbstractChunksSourceStream::AbstractChunksSourceStream(vlc_object_t *p_obj_, AbstractSource *source_)
    : b_eof( false )
    , p_obj( p_obj_ )
    , source( source_ )
{ }

AbstractChunksSourceStream::~AbstractChunksSourceStream()
{

}

void AbstractChunksSourceStream::Reset()
{
    b_eof = false;
}

ssize_t AbstractChunksSourceStream::read_Callback(stream_t *s, void *buf, size_t size)
{
    AbstractChunksSourceStream *me = reinterpret_cast<AbstractChunksSourceStream *>(s->p_sys);
    return me->Read(reinterpret_cast<uint8_t *>(buf), size);
}

int AbstractChunksSourceStream::seek_Callback(stream_t *s, uint64_t i_pos)
{
    AbstractChunksSourceStream *me = reinterpret_cast<AbstractChunksSourceStream *>(s->p_sys);
    return me->Seek(i_pos);
}

int AbstractChunksSourceStream::control_Callback(stream_t *, int i_query, va_list args)
{
    switch( i_query )
    {
        case STREAM_GET_SIZE:
            *(va_arg( args, uint64_t * )) = 0;
            return VLC_SUCCESS;

        case STREAM_CAN_SEEK:
        case STREAM_CAN_FASTSEEK:
        case STREAM_CAN_PAUSE:
        case STREAM_CAN_CONTROL_PACE:
            *va_arg( args, bool * ) = false;
            return VLC_SUCCESS;

        case STREAM_GET_PTS_DELAY:
            *(va_arg( args, mtime_t * )) = DEFAULT_PTS_DELAY;
            return VLC_SUCCESS;

        default:
            break;
    }
    return VLC_EGENERIC;
}

void AbstractChunksSourceStream::delete_Callback(stream_t *)
{
}

stream_t * AbstractChunksSourceStream::makeStream()
{
    stream_t *p_stream = vlc_stream_CommonNew( p_obj, delete_Callback );
    if(p_stream)
    {
        p_stream->pf_control = control_Callback;
        p_stream->pf_read = read_Callback;
        p_stream->pf_readdir = nullptr;
        p_stream->pf_seek = seek_Callback;
        p_stream->p_sys = reinterpret_cast<stream_sys_t*>(this);
    }
    return p_stream;
}

ChunksSourceStream::ChunksSourceStream(vlc_object_t *p_obj_, AbstractSource *source_)
    : AbstractChunksSourceStream(p_obj_, source_)
{
    p_block = nullptr;
}

ChunksSourceStream::~ChunksSourceStream()
{
    if(p_block)
        block_Release(p_block);
}

void ChunksSourceStream::Reset()
{
    if(p_block)
        block_Release(p_block);
    p_block = nullptr;
    AbstractChunksSourceStream::Reset();
}

size_t ChunksSourceStream::Peek(const uint8_t **pp, size_t sz)
{
    if(!b_eof && !p_block)
    {
        p_block = source->readNextBlock();
        b_eof = !p_block;
    }
    if(!p_block)
        return 0;
    *pp = p_block->p_buffer;
    return std::min(p_block->i_buffer, sz);
}

ssize_t ChunksSourceStream::Read(uint8_t *buf, size_t size)
{
    size_t i_copied = 0;
    size_t i_toread = size;

    while(i_toread && !b_eof)
    {
        if(!p_block && !(p_block = source->readNextBlock()))
        {
            b_eof = true;
            break;
        }

        if(p_block->i_buffer > i_toread)
        {
            if(buf)
                memcpy(buf + i_copied, p_block->p_buffer, i_toread);
            i_copied += i_toread;
            p_block->p_buffer += i_toread;
            p_block->i_buffer -= i_toread;
            i_toread = 0;
        }
        else
        {
            if(buf)
                memcpy(buf + i_copied, p_block->p_buffer, p_block->i_buffer);
            i_copied += p_block->i_buffer;
            i_toread -= p_block->i_buffer;
            block_Release(p_block);
            p_block = nullptr;
        }
    }

    return i_copied;
}

int ChunksSourceStream::Seek(uint64_t)
{
    return VLC_EGENERIC;
}

BufferedChunksSourceStream::BufferedChunksSourceStream(vlc_object_t *p_obj_, AbstractSource *source_)
    : AbstractChunksSourceStream( p_obj_, source_ )
{
    i_global_offset = 0;
    i_bytestream_offset = 0;
    block_BytestreamInit( &bs );
    p_peekdata = nullptr;
}

BufferedChunksSourceStream::~BufferedChunksSourceStream()
{
    block_BytestreamEmpty( &bs );
    invalidatePeek();
}

void BufferedChunksSourceStream::Reset()
{
    block_BytestreamEmpty( &bs );
    i_bytestream_offset = 0;
    i_global_offset = 0;
    invalidatePeek();
    AbstractChunksSourceStream::Reset();
}

ssize_t BufferedChunksSourceStream::doRead(uint8_t *buf, size_t i_toread)
{
    size_t i_remain = block_BytestreamRemaining(&bs) - i_bytestream_offset;
    if(i_remain < i_toread)
    {
        fillByteStream(i_bytestream_offset + i_toread);
        i_remain = block_BytestreamRemaining(&bs) - i_bytestream_offset;
        if(i_remain == 0)
            return 0;
    }

    if(i_remain < i_toread)
        i_toread = i_remain;

    if(buf)
        block_PeekOffsetBytes(&bs, i_bytestream_offset, buf, i_toread);

    return i_toread;
}

ssize_t BufferedChunksSourceStream::Read(uint8_t *buf, size_t i_toread)
{
    invalidatePeek();

    ssize_t i_read = doRead(buf, i_toread);
    if(i_read <= 0)
        return i_read;

    i_bytestream_offset += i_read;
    i_toread -= i_read;

    if(i_bytestream_offset > MAX_BACKEND)
    {
        const size_t i_drop = i_bytestream_offset - MAX_BACKEND;
        if(i_drop >= MIN_BACKEND_CLEANUP) /* Dont flush for few bytes */
        {
            block_GetBytes(&bs, nullptr, i_drop);
            block_BytestreamFlush(&bs);
            i_bytestream_offset -= i_drop;
            i_global_offset += i_drop;
        }
    }
    return i_read;
}

int BufferedChunksSourceStream::Seek(uint64_t i_seek)
{
    if(i_seek < i_global_offset) /* can't seek into discarded data */
    {
        msg_Err(p_obj, "tried to seek back in cache %" PRIu64 " < %" PRIu64,
                i_seek, i_global_offset);
        return VLC_EGENERIC;
    }
    size_t i_bsseekoffset = i_seek - i_global_offset;
    fillByteStream(i_bsseekoffset);
    if(block_BytestreamRemaining(&bs) < i_bsseekoffset)
    {
        msg_Err(p_obj, "tried to seek too far in cache %" PRIu64 " < %" PRIu64 " < %" PRIu64,
                i_global_offset, i_seek, i_global_offset + block_BytestreamRemaining(&bs));
        return VLC_EGENERIC;
    }
    invalidatePeek();
    i_bytestream_offset = i_seek - i_global_offset;
    return VLC_SUCCESS;
}

size_t BufferedChunksSourceStream::Peek(const uint8_t **pp, size_t sz)
{
    sz = std::min(sz, (size_t)MAX_BACKEND);
    invalidatePeek();
    p_peekdata = block_Alloc(sz);
    if(!p_peekdata)
        return 0;
    ssize_t i_read = doRead(p_peekdata->p_buffer, sz);
    if(i_read <= 0)
    {
        invalidatePeek();
        return 0;
    }
    *pp = p_peekdata->p_buffer;
    return i_read;
}

void BufferedChunksSourceStream::fillByteStream(size_t level)
{
    while(!b_eof && level > block_BytestreamRemaining(&bs))
    {
        block_t *p_block = source->readNextBlock();
        b_eof = !p_block;
        if(p_block)
            block_BytestreamPush(&bs, p_block);
    }
}

void BufferedChunksSourceStream::invalidatePeek()
{
    if(p_peekdata)
    {
        block_Release(p_peekdata);
        p_peekdata = nullptr;
    }
}
