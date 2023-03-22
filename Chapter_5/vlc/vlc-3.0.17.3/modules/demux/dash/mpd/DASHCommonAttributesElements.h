/*****************************************************************************
 * CommonAttributesElements.h: Defines the common attributes and elements
 *                             for some Dash elements.
 *****************************************************************************
 * Copyright © 2011 VideoLAN
 *
 * Authors: Hugo Beauzée-Luyssen <hugo@beauzee.fr>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
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

#ifndef DASHCOMMONATTRIBUTESELEMENTS_H
#define DASHCOMMONATTRIBUTESELEMENTS_H

#include <list>
#include <string>

namespace dash
{
    namespace mpd
    {
        class   ContentDescription;

        class DASHCommonAttributesElements
        {
            public:
                DASHCommonAttributesElements();
                virtual ~DASHCommonAttributesElements();
                int                             getParX() const;
                void                            setParX( int parX );
                int                             getParY() const;
                void                            setParY( int parY );
                int                             getFrameRate() const;
                void                            setFrameRate( int frameRate );
                const std::list<std::string>&   getNumberOfChannels() const;
                void                            addChannel( const std::string &channel );
                const std::list<int>&           getSamplingRates() const;
                void                            addSampleRate( int sampleRate );
                const std::list<ContentDescription*>&   getContentProtections() const;
                void                                    addContentProtection( ContentDescription *desc );
                const std::list<ContentDescription*>&   getAccessibilities() const;
                void                                    addAccessibility( ContentDescription *desc );
                const std::list<ContentDescription*>&   getRatings() const;
                void                                    addRating( ContentDescription* desc );
                const std::list<ContentDescription*>&   getViewpoints() const;
                void                                    addViewpoint( ContentDescription *desc );

            protected:
                int                                 parX;
                int                                 parY;
                int                                 frameRate;
                std::list<std::string>              channels;
                std::list<int>                      sampleRates;
                std::list<ContentDescription*>      contentProtections;
                std::list<ContentDescription*>      accessibilities;
                std::list<ContentDescription*>      ratings;
                std::list<ContentDescription*>      viewpoints;
        };
    }
}

#endif // DASHCOMMONATTRIBUTESELEMENTS_H
