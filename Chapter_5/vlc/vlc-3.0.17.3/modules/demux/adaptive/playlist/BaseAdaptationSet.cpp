/*
 * BaseAdaptationSet.cpp
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

#include "BaseAdaptationSet.h"
#include "BaseRepresentation.h"

#include <vlc_common.h>
#include <vlc_arrays.h>

#include "SegmentTemplate.h"
#include "BasePeriod.h"
#include "Inheritables.hpp"

#include <algorithm>

using namespace adaptive;
using namespace adaptive::playlist;

BaseAdaptationSet::BaseAdaptationSet(BasePeriod *period) :
    CommonAttributesElements(),
    SegmentInformation( period )
{
}

BaseAdaptationSet::~BaseAdaptationSet   ()
{
    vlc_delete_all( representations );
    childs.clear();
}

StreamFormat BaseAdaptationSet::getStreamFormat() const
{
    if (!representations.empty())
        return representations.front()->getStreamFormat();
    else
        return StreamFormat();
}

const std::vector<BaseRepresentation*>& BaseAdaptationSet::getRepresentations() const
{
    return representations;
}

BaseRepresentation * BaseAdaptationSet::getRepresentationByID(const ID &id) const
{
    for(auto it = representations.cbegin(); it != representations.cend(); ++it)
    {
        if((*it)->getID() == id)
            return *it;
    }
    return nullptr;
}

void BaseAdaptationSet::addRepresentation(BaseRepresentation *rep)
{
    representations.insert(std::upper_bound(representations.begin(),
                                            representations.end(),
                                            rep,
                                            BaseRepresentation::bwCompare),
                           rep);
    childs.push_back(rep);
}

const std::string & BaseAdaptationSet::getLang() const
{
    return lang;
}

void BaseAdaptationSet::setLang( const std::string &lang_ )
{
    std::size_t pos = lang.find_first_of('-');
    if(pos != std::string::npos && pos > 0)
        lang = lang_.substr(0, pos);
    else if(lang_.size() < 4)
        lang = lang_;
}

void BaseAdaptationSet::setSegmentAligned(bool b)
{
    segmentAligned = b;
}

void BaseAdaptationSet::setBitswitchAble(bool b)
{
    bitswitchAble = b;
}

bool BaseAdaptationSet::isSegmentAligned() const
{
    return !segmentAligned.isSet() || segmentAligned.value();
}

bool BaseAdaptationSet::isBitSwitchable() const
{
    return bitswitchAble.isSet() && segmentAligned.value();
}

void BaseAdaptationSet::setRole(const Role &r)
{
    role = r;
}

const Role & BaseAdaptationSet::getRole() const
{
    return role;
}

void BaseAdaptationSet::debug(vlc_object_t *obj, int indent) const
{
    std::string text(indent, ' ');
    text.append("BaseAdaptationSet ");
    text.append(id.str());
    msg_Dbg(obj, "%s", text.c_str());
    const AbstractSegmentBaseType *profile = getProfile();
    if(profile)
        profile->debug(obj, indent + 1);
    std::vector<BaseRepresentation *>::const_iterator k;
    for(k = representations.begin(); k != representations.end(); ++k)
        (*k)->debug(obj, indent + 1);
}
