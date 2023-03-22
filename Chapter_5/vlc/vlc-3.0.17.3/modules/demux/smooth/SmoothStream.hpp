/*
 * SmoothStream.hpp
 *****************************************************************************
 * Copyright (C) 2015 - VideoLAN and VLC authors
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
#ifndef SMOOTHSTREAM_HPP
#define SMOOTHSTREAM_HPP

#include "../adaptive/Streams.hpp"

namespace smooth
{
    using namespace adaptive;

    class SmoothStream : public AbstractStream
    {
        public:
            SmoothStream(demux_t *);

        protected:
            virtual block_t *checkBlock(block_t *, bool) override;
            virtual AbstractDemuxer * newDemux(vlc_object_t *, const StreamFormat &,
                                               es_out_t *, AbstractSourceStream *) const override;
    };

    class SmoothStreamFactory : public AbstractStreamFactory
    {
        public:
            virtual AbstractStream *create(demux_t*, const StreamFormat &,
                                   SegmentTracker *, AbstractConnectionManager *) const override;
    };

}

#endif // SMOOTHSTREAM_HPP
