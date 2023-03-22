/*
 * Representation.h
 *****************************************************************************
 * Copyright (C) 2010 - 2011 Klagenfurt University
 *
 * Created on: Aug 10, 2010
 * Authors: Christopher Mueller <christopher.mueller@itec.uni-klu.ac.at>
 *          Christian Timmerer  <christian.timmerer@itec.uni-klu.ac.at>
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

#ifndef BASEREPRESENTATION_H_
#define BASEREPRESENTATION_H_

#include <string>
#include <list>

#include "CommonAttributesElements.h"
#include "CodecDescription.hpp"
#include "SegmentInformation.hpp"
#include "../StreamFormat.hpp"

namespace adaptive
{
    class SharedResources;

    namespace playlist
    {
        class BaseAdaptationSet;
        class BasePlaylist;
        class SegmentTemplateSegment;

        class BaseRepresentation : public CommonAttributesElements,
                                   public SegmentInformation
        {
            public:
                BaseRepresentation( BaseAdaptationSet * );
                virtual ~BaseRepresentation ();

                virtual StreamFormat getStreamFormat() const;
                BaseAdaptationSet* getAdaptationSet();
                /*
                 *  @return The bitrate required for this representation
                 *          in bits per seconds.
                 *          Will be a valid value, as the parser refuses Representation
                 *          without bandwidth.
                 */
                uint64_t            getBandwidth            () const;
                void                setBandwidth            ( uint64_t bandwidth );
                const std::list<std::string> & getCodecs    () const;
                void                addCodecs               (const std::string &);
                void                getCodecsDesc           (CodecDescriptionList *) const;
                bool                consistentSegmentNumber () const;
                virtual void        pruneByPlaybackTime     (mtime_t) override;

                virtual mtime_t     getMinAheadTime         (uint64_t) const;
                virtual bool        needsUpdate             (uint64_t) const;
                virtual bool        needsIndex              () const;
                virtual bool        runLocalUpdates         (SharedResources *);
                virtual void        scheduleNextUpdate      (uint64_t, bool);

                virtual void        debug                   (vlc_object_t *,int = 0) const;

                /* for segment templates */
                virtual std::string contextualize(size_t, const std::string &,
                                                  const SegmentTemplate *) const;

                static bool         bwCompare(const BaseRepresentation *a,
                                              const BaseRepresentation *b);

                virtual uint64_t translateSegmentNumber(uint64_t, const BaseRepresentation *) const;
                bool getSegmentNumberByTime(mtime_t, uint64_t *) const;
                bool getPlaybackTimeDurationBySegmentNumber(uint64_t, mtime_t *, mtime_t *) const;
                bool getMediaPlaybackRange(mtime_t *rangeBegin,
                                                               mtime_t *rangeEnd,
                                                               mtime_t *rangeLength) const;
            protected:
                virtual CodecDescription * makeCodecDescription(const std::string &) const;
                virtual bool        validateCodec(const std::string &) const;
                BaseAdaptationSet                  *adaptationSet;
                uint64_t                            bandwidth;
                std::list<std::string>              codecs;
                bool                                b_consistent;
        };
    }
}

#endif /* BASEREPRESENTATION_H_ */
