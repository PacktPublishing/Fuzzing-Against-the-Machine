/*
 * NearOptimalAdaptationLogic.cpp
 *****************************************************************************
 * Copyright (C) 2017 - VideoLAN Authors
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

#include "NearOptimalAdaptationLogic.hpp"
#include "Representationselectors.hpp"

#include "../playlist/BaseAdaptationSet.h"
#include "../playlist/BaseRepresentation.h"
#include "../playlist/BasePeriod.h"
#include "../http/Chunk.h"
#include "../tools/Debug.hpp"

#include <cmath>

using namespace adaptive::logic;
using namespace adaptive;

/*
 * Multi stream version of BOLA: Near-Optimal Bitrate Adaptation for Online Videos
 * http://arxiv.org/abs/1601.06748
 */

#define minimumBufferS (CLOCK_FREQ * 6)  /* Qmin */
#define bufferTargetS  (CLOCK_FREQ * 30) /* Qmax */

NearOptimalContext::NearOptimalContext()
    : buffering_min( minimumBufferS )
    , buffering_level( 0 )
    , buffering_target( bufferTargetS )
    , last_download_rate( 0 )
{ }

NearOptimalAdaptationLogic::NearOptimalAdaptationLogic( vlc_object_t *obj )
    : AbstractAdaptationLogic(obj)
    , currentBps( 0 )
    , usedBps( 0 )
    , p_obj( obj )
{
    vlc_mutex_init(&lock);
}

NearOptimalAdaptationLogic::~NearOptimalAdaptationLogic()
{
    vlc_mutex_destroy(&lock);
}

BaseRepresentation *
NearOptimalAdaptationLogic::getNextQualityIndex( BaseAdaptationSet *adaptSet, RepresentationSelector &selector,
                                                 float gammaP, mtime_t VD, mtime_t Q )
{
    BaseRepresentation *ret = nullptr;
    BaseRepresentation *prev = nullptr;
    float argmax;
    for(BaseRepresentation *rep = selector.lowest(adaptSet);
                            rep && rep != prev; rep = selector.higher(adaptSet, rep))
    {
        float arg = ( VD * (getUtility(rep) + gammaP) - Q ) / rep->getBandwidth();
        if(ret == nullptr || argmax <= arg)
        {
            ret = rep;
            argmax = arg;
        }
        prev = rep;
    }
    return ret;
}

BaseRepresentation *NearOptimalAdaptationLogic::getNextRepresentation(BaseAdaptationSet *adaptSet, BaseRepresentation *prevRep)
{
    RepresentationSelector selector(maxwidth, maxheight);

    BaseRepresentation *lowest = selector.lowest(adaptSet);
    BaseRepresentation *highest = selector.highest(adaptSet);
    if(lowest == nullptr || highest == nullptr)
        return nullptr;

    if(lowest == highest)
        return lowest;

    const float umin = getUtility(lowest);
    const float umax = getUtility(highest);

    vlc_mutex_lock(&lock);

    std::map<ID, NearOptimalContext>::iterator it = streams.find(adaptSet->getID());
    if(it == streams.end())
    {
        vlc_mutex_unlock(&lock);
        return selector.lowest(adaptSet);
    }
    NearOptimalContext ctxcopy = (*it).second;

    const unsigned bps = getAvailableBw(currentBps, prevRep);

    vlc_mutex_unlock(&lock);

    const float gammaP = 1.0 + (umax - umin) / ((float)ctxcopy.buffering_target / ctxcopy.buffering_min - 1.0);
    const float Vd = ((float)ctxcopy.buffering_min / CLOCK_FREQ - 1.0) / (umin + gammaP);

    BaseRepresentation *m;
    if(prevRep == nullptr) /* Starting */
    {
        m = selector.select(adaptSet, bps);
        if(m == lowest)
        {
            /* Handle HLS specific cases where the lowest is audio only. Try to pick first A+V */
            BaseRepresentation *n = selector.higher(adaptSet, m);
            if(m != n  && m->getCodecs().size() == 1 && n->getCodecs().size() > 1)
                m = n;
        }
    }
    else
    {
        /* noted m* */
        m = getNextQualityIndex(adaptSet, selector, gammaP - umin /* umin == Sm, utility = std::log(S/Sm) */,
                                Vd, (float)ctxcopy.buffering_level / CLOCK_FREQ);
        if(m->getBandwidth() < prevRep->getBandwidth()) /* m*[n] < m*[n-1] */
        {
            BaseRepresentation *mp = selector.select(adaptSet, bps); /* m' */
            if(mp->getBandwidth() <= m->getBandwidth())
            {
                mp = m;
            }
            else if(mp->getBandwidth() > prevRep->getBandwidth())
            {
                mp = prevRep;
            }
            else
            {
                mp = selector.lower(adaptSet, mp);
            }
            m = mp;
        }
    }

    BwDebug( msg_Info(p_obj, "buffering level %.2f% rep %ld kBps %zu kBps",
             (float) 100 * ctxcopy.buffering_level / ctxcopy.buffering_target, m->getBandwidth()/8000, bps / 8000); );

    return m;
}

float NearOptimalAdaptationLogic::getUtility(const BaseRepresentation *rep)
{
    float ret;
    std::map<uint64_t, float>::iterator it = utilities.find(rep->getBandwidth());
    if(it == utilities.end())
    {
        ret = std::log((float)rep->getBandwidth());
        utilities.insert(std::pair<uint64_t, float>(rep->getBandwidth(), ret));
    }
    else ret = (*it).second;
    return ret;
}

unsigned NearOptimalAdaptationLogic::getAvailableBw(unsigned i_bw, const BaseRepresentation *curRep) const
{
    unsigned i_remain = i_bw;
    if(i_remain > usedBps)
        i_remain -= usedBps;
    else
        i_remain = 0;
    if(curRep)
        i_remain += curRep->getBandwidth();
    return i_remain > i_bw ? i_remain : i_bw;
}

unsigned NearOptimalAdaptationLogic::getMaxCurrentBw() const
{
    unsigned i_max_bitrate = 0;
    for(std::map<ID, NearOptimalContext>::const_iterator it = streams.begin();
                                                         it != streams.end(); ++it)
        i_max_bitrate = std::max(i_max_bitrate, ((*it).second).last_download_rate);
    return i_max_bitrate;
}

void NearOptimalAdaptationLogic::updateDownloadRate(const ID &id, size_t dlsize,
                                                    mtime_t time, mtime_t)
{
    vlc_mutex_lock(&lock);
    std::map<ID, NearOptimalContext>::iterator it = streams.find(id);
    if(it != streams.end())
    {
        NearOptimalContext &ctx = (*it).second;
        ctx.last_download_rate = ctx.average.push(CLOCK_FREQ * dlsize * 8 / time);
    }
    currentBps = getMaxCurrentBw();
    vlc_mutex_unlock(&lock);
}

void NearOptimalAdaptationLogic::trackerEvent(const TrackerEvent &ev)
{
    switch(ev.getType())
    {
    case TrackerEvent::Type::RepresentationSwitch:
        {
            const RepresentationSwitchEvent &event =
                    static_cast<const RepresentationSwitchEvent &>(ev);
            vlc_mutex_lock(&lock);
            if(event.prev)
                usedBps -= event.prev->getBandwidth();
            if(event.next)
                usedBps += event.next->getBandwidth();
                 BwDebug(msg_Info(p_obj, "New total bandwidth usage %zu kBps", (usedBps / 8000)));
            vlc_mutex_unlock(&lock);
        }
        break;

    case TrackerEvent::Type::BufferingStateUpdate:
        {
            const BufferingStateUpdatedEvent &event =
                    static_cast<const BufferingStateUpdatedEvent &>(ev);
            const ID &id = *event.id;
            vlc_mutex_lock(&lock);
            if(event.enabled)
            {
                if(streams.find(id) == streams.end())
                {
                    NearOptimalContext ctx;
                    streams.insert(std::pair<ID, NearOptimalContext>(id, ctx));
                }
            }
            else
            {
                std::map<ID, NearOptimalContext>::iterator it = streams.find(id);
                if(it != streams.end())
                    streams.erase(it);
            }
            vlc_mutex_unlock(&lock);
            BwDebug(msg_Info(p_obj, "Stream %s is now known %sactive", id.str().c_str(),
                         (event.enabled) ? "" : "in"));
        }
        break;

    case TrackerEvent::Type::BufferingLevelChange:
        {
            const BufferingLevelChangedEvent &event =
                    static_cast<const BufferingLevelChangedEvent &>(ev);
            const ID &id = *event.id;
            vlc_mutex_lock(&lock);
            NearOptimalContext &ctx = streams[id];
            ctx.buffering_level = event.current;
            ctx.buffering_target = event.target;
            vlc_mutex_unlock(&lock);
        }
        break;

    default:
            break;
    }
}
