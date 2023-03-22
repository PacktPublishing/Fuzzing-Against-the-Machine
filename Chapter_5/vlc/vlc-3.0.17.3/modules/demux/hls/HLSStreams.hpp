/*
 * HLSStreams.hpp
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
#ifndef HLSSTREAM_HPP
#define HLSSTREAM_HPP

#include "../adaptive/Streams.hpp"

namespace hls
{
    using namespace adaptive;

    class HLSStream : public AbstractStream
    {
        public:
            HLSStream(demux_t *);
            virtual ~HLSStream();

        protected:
            virtual block_t *checkBlock(block_t *, bool) override;
            virtual AbstractDemuxer * newDemux(vlc_object_t *, const StreamFormat &,
                                               es_out_t *, AbstractSourceStream *) const override;
            virtual bool setPosition(mtime_t, bool) override;

        private:
            static int ID3TAG_Parse_Handler(uint32_t, const uint8_t *, size_t, void *);
            int ParseID3Tag(uint32_t, const uint8_t *, size_t);
            int ParseID3PrivTag(const uint8_t *, size_t);
            void setMetadataTimeOffset(mtime_t);
            bool b_id3_timestamps_offset_set;
            vlc_meta_t *p_meta;
            bool b_meta_updated;
    };

    class HLSStreamFactory : public AbstractStreamFactory
    {
        public:
            virtual AbstractStream *create(demux_t *, const StreamFormat &,
                                   SegmentTracker *, AbstractConnectionManager *) const override;
    };

}
#endif // HLSSTREAMS_HPP
