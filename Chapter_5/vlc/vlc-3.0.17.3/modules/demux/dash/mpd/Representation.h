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

#ifndef DASHREPRESENTATION_H_
#define DASHREPRESENTATION_H_

#include "DASHCommonAttributesElements.h"
#include "../../adaptive/playlist/SegmentBaseType.hpp"
#include "../../adaptive/playlist/BaseRepresentation.h"

namespace dash
{
    namespace mpd
    {
        class AdaptationSet;
        class TrickModeType;
        class MPD;

        using namespace adaptive;
        using namespace adaptive::playlist;

        class Representation : public BaseRepresentation,
                               public DASHCommonAttributesElements
        {
            public:
                Representation( AdaptationSet * );
                virtual ~Representation ();

                virtual StreamFormat getStreamFormat() const override;
                int                 getQualityRanking       () const;
                void                setQualityRanking       ( int qualityRanking );
                const std::list<const Representation*>&     getDependencies() const;
                void                addDependency           ( const Representation* dep );
                /**
                 * @return  This SegmentInfo for this Representation.
                 *          It cannot be NULL, or without any Segments in it.
                 *          It can however have a NULL InitSegment
                 */
                TrickModeType*      getTrickModeType        () const;

                void                setTrickMode( TrickModeType *trickModeType );

                /* for segment templates */
                virtual std::string contextualize(size_t, const std::string &,
                                                  const SegmentTemplate *) const override;

            private:
                int                                 qualityRanking;
                std::list<const Representation*>    dependencies;
                TrickModeType                       *trickModeType;

                /* for contextualize() */
                stime_t getScaledTimeBySegmentNumber(uint64_t, const SegmentTemplate *) const;
        };
    }
}

#endif /* DASHREPRESENTATION_H_ */
