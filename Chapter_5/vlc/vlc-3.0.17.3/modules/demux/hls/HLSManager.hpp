/*
 * HLSManager.hpp
 *****************************************************************************
 * Copyright © 2015 - VideoLAN and VLC authors
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
#ifndef HLSMANAGER_HPP
#define HLSMANAGER_HPP

#include "../adaptive/PlaylistManager.h"
#include "../adaptive/logic/AbstractAdaptationLogic.h"
#include "playlist/M3U8.hpp"

namespace hls
{
    using namespace adaptive;

    class HLSManager : public PlaylistManager
    {
        public:
            HLSManager( demux_t *,
                        SharedResources *,
                        playlist::M3U8 *,
                        AbstractStreamFactory *,
                        logic::AbstractAdaptationLogic::LogicType type );
            virtual ~HLSManager();
            static bool isHTTPLiveStreaming(stream_t *);
    };

}

#endif // HLSMANAGER_HPP
