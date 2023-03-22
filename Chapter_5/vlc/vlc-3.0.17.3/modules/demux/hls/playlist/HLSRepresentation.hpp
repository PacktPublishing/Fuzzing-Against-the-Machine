/*
 * HLSRepresentation.hpp
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

#ifndef HLSREPRESENTATION_H_
#define HLSREPRESENTATION_H_

#include "../../adaptive/playlist/BaseRepresentation.h"
#include "../../adaptive/tools/Properties.hpp"
#include "../../adaptive/StreamFormat.hpp"

namespace hls
{
    namespace playlist
    {
        class M3U8;

        using namespace adaptive;
        using namespace adaptive::playlist;

        class HLSRepresentation : public BaseRepresentation
        {
            friend class M3U8Parser;

            public:
                HLSRepresentation( BaseAdaptationSet * );
                virtual ~HLSRepresentation ();
                virtual StreamFormat getStreamFormat() const override;

                void setPlaylistUrl(const std::string &);
                Url getPlaylistUrl() const;
                bool isLive() const;
                bool initialized() const;
                virtual void scheduleNextUpdate(uint64_t, bool) override;
                virtual bool needsUpdate(uint64_t) const override;
                virtual void debug(vlc_object_t *, int) const override;
                virtual bool runLocalUpdates(SharedResources *) override;

                virtual uint64_t translateSegmentNumber(uint64_t, const BaseRepresentation *) const override;

            private:
                StreamFormat streamFormat;
                bool b_live;
                bool b_loaded;
                bool b_failed;
                mtime_t lastUpdateTime;
                time_t targetDuration;
                Url playlistUrl;
        };
    }
}

#endif /* HLSREPRESENTATION_H_ */
