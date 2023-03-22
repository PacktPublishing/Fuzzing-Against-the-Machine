/*
 * Retrieve.hpp
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
#ifndef RETRIEVE_HPP
#define RETRIEVE_HPP

#include <vlc_common.h>
#include <string>

namespace adaptive
{
    class SharedResources;
    namespace http
    {
        enum class ChunkType;
    };

    class Retrieve
    {
        public:
            static block_t * HTTP(SharedResources *, http::ChunkType, const std::string &uri);
    };
}

#endif // RETRIEVE_HPP
