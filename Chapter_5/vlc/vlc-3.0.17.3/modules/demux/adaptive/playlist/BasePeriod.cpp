/*
 * BasePeriod.cpp
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "BasePeriod.h"
#include "BasePlaylist.hpp"
#include "SegmentBaseType.hpp"
#include "../Streams.hpp"

#include <vlc_common.h>
#include <vlc_arrays.h>
#include <algorithm>
#include <cassert>

using namespace adaptive::playlist;

BasePeriod::BasePeriod(BasePlaylist *playlist_) :
    SegmentInformation( playlist_ )
{
    duration.Set(0);
    startTime.Set(0);
    playlist = playlist_;
}

BasePeriod::~BasePeriod ()
{
    vlc_delete_all( adaptationSets );
    childs.clear();
}

BasePlaylist *BasePeriod::getPlaylist() const
{
    return playlist;
}

const std::vector<BaseAdaptationSet*>&  BasePeriod::getAdaptationSets() const
{
    return adaptationSets;
}

void BasePeriod::addAdaptationSet(BaseAdaptationSet *adaptationSet)
{
    auto p = std::find_if(adaptationSets.begin(), adaptationSets.end(),
        [adaptationSet](BaseAdaptationSet *s){
            return adaptationSet->getRole() < s->getRole(); });
    adaptationSets.insert(p, adaptationSet);
    childs.push_back(adaptationSet);
}

BaseAdaptationSet *BasePeriod::getAdaptationSetByID(const adaptive::ID &id) const
{
    for(auto it = adaptationSets.cbegin(); it!= adaptationSets.cend(); ++it)
    {
        if( (*it)->getID() == id )
            return *it;
    }
    return nullptr;
}

void BasePeriod::debug(vlc_object_t *obj, int indent) const
{
    std::string text(indent, ' ');
    text.append("Period");
    msg_Dbg(obj, "%s", text.c_str());
    const AbstractSegmentBaseType *profile = getProfile();
    if(profile)
        profile->debug(obj, indent + 1);
    std::vector<BaseAdaptationSet *>::const_iterator k;
    for(k = adaptationSets.begin(); k != adaptationSets.end(); ++k)
        (*k)->debug(obj, indent + 1);
}

mtime_t BasePeriod::getPeriodStart() const
{
    return startTime.Get();
}

mtime_t BasePeriod::getPeriodDuration() const
{
    return duration.Get();
}
