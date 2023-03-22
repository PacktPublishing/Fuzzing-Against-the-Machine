/*
 * Parser.hpp
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
#ifndef PARSER_HPP
#define PARSER_HPP

#include "../../adaptive/playlist/SegmentBaseType.hpp"

#include <cstdlib>
#include <sstream>
#include <list>

#include <vlc_common.h>

namespace adaptive
{
    class SharedResources;

    namespace playlist
    {
        class SegmentInformation;
        class SegmentTemplate;
        class BasePeriod;
        class BaseAdaptationSet;
    }
}

namespace hls
{
    namespace playlist
    {
        using namespace adaptive::playlist;

        class M3U8;
        class AttributesTag;
        class Tag;
        class HLSRepresentation;

        class M3U8Parser
        {
            public:
                M3U8Parser             (adaptive::SharedResources *);
                virtual ~M3U8Parser    ();

                M3U8 *             parse  (vlc_object_t *p_obj, stream_t *p_stream, const std::string &);
                bool appendSegmentsFromPlaylistURI(vlc_object_t *, HLSRepresentation *);

            private:
                HLSRepresentation * createRepresentation(BaseAdaptationSet *, const AttributesTag *);
                void createAndFillRepresentation(vlc_object_t *, BaseAdaptationSet *,
                                                 const AttributesTag *, const std::list<Tag *>&);
                void parseSegments(vlc_object_t *, HLSRepresentation *, const std::list<Tag *>&);
                std::list<Tag *> parseEntries(stream_t *);
                adaptive::SharedResources *resources;
        };
    }
}

#endif // PARSER_HPP
