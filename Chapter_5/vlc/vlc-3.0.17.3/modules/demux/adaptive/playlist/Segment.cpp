/*
 * Segment.cpp
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

#include "Segment.h"
#include "BaseAdaptationSet.h"
#include "BaseRepresentation.h"
#include "BasePlaylist.hpp"
#include "SegmentChunk.hpp"
#include "../http/BytesRange.hpp"
#include "../http/HTTPConnectionManager.h"
#include "../http/Downloader.hpp"
#include <cassert>

using namespace adaptive::http;
using namespace adaptive::playlist;

ISegment::ISegment(const ICanonicalUrl *parent):
    ICanonicalUrl( parent ),
    startByte  (0),
    endByte    (0)
{
    debugName = "Segment";
    startTime.Set(0);
    duration.Set(0);
    sequence = 0;
    templated = false;
    discontinuity = false;
    displayTime = VLC_TS_INVALID;
}

ISegment::~ISegment()
{
}

bool ISegment::prepareChunk(SharedResources *res, SegmentChunk *chunk, BaseRepresentation *rep)
{
    CommonEncryption enc = encryption;
    enc.mergeWith(rep->intheritEncryption());

    if(enc.method != CommonEncryption::Method::None)
    {
        CommonEncryptionSession *encryptionSession = new CommonEncryptionSession();
        if(!encryptionSession->start(res, enc))
        {
            delete encryptionSession;
            return false;
        }
        chunk->setEncryptionSession(encryptionSession);
    }
    return true;
}

SegmentChunk* ISegment::toChunk(SharedResources *res, AbstractConnectionManager *connManager,
                                size_t index, BaseRepresentation *rep)
{
    const std::string url = getUrlSegment().toString(index, rep);
    BytesRange range;
    if(startByte != endByte)
        range = BytesRange(startByte, endByte);
    ChunkType chunkType;
    if(dynamic_cast<InitSegment *>(this))
        chunkType = ChunkType::Init;
    else if(dynamic_cast<IndexSegment *>(this))
        chunkType = ChunkType::Index;
    else
        chunkType = ChunkType::Segment;
    AbstractChunkSource *source = connManager->makeSource(url,
                                                          rep->getAdaptationSet()->getID(),
                                                          chunkType,
                                                          range);
    if(source)
    {
        SegmentChunk *chunk = createChunk(source, rep);
        if(chunk)
        {
            chunk->sequence = index;
            chunk->discontinuity = discontinuity;
            if(!prepareChunk(res, chunk, rep))
            {
                delete chunk;
                return nullptr;
            }
            connManager->start(source);
            return chunk;
        }
        else
        {
            connManager->recycleSource(source);
        }
    }
    return nullptr;
}

bool ISegment::isTemplate() const
{
    return templated;
}

void ISegment::setByteRange(size_t start, size_t end)
{
    startByte = start;
    endByte   = end;
}

void ISegment::setSequenceNumber(uint64_t seq)
{
    sequence = seq;
}

uint64_t ISegment::getSequenceNumber() const
{
    return sequence;
}

size_t ISegment::getOffset() const
{
    return startByte;
}

void ISegment::debug(vlc_object_t *obj, int indent) const
{
    std::stringstream ss;
    ss.imbue(std::locale("C"));
    ss << std::string(indent, ' ') << debugName << " #" << getSequenceNumber();
    ss << " url=" << getUrlSegment().toString();
    if(startByte!=endByte)
        ss << " @" << startByte << ".." << endByte;
    if(startTime.Get() > 0)
    	 ss << " stime " << startTime.Get();
    ss << " duration " << duration.Get();
    msg_Dbg(obj, "%s", ss.str().c_str());
}

bool ISegment::contains(size_t byte) const
{
    if (startByte == endByte)
        return false;
    return (byte >= startByte &&
            (!endByte || byte <= endByte) );
}

int ISegment::compare(ISegment *other) const
{
    if(duration.Get())
    {
        if(startTime.Get() > other->startTime.Get())
            return 1;
        else if(startTime.Get() < other->startTime.Get())
            return -1;
    }

    if(startByte > other->startByte)
        return 1;
    else if(startByte < other->startByte)
        return -1;

    if(endByte > other->endByte)
        return 1;
    else if(endByte < other->endByte)
        return -1;

    return 0;
}

void ISegment::setEncryption(CommonEncryption &e)
{
    encryption = e;
}

void ISegment::setDisplayTime(mtime_t t)
{
    displayTime = t;
}

mtime_t ISegment::getDisplayTime() const
{
    return displayTime;
}

Segment::Segment(ICanonicalUrl *parent) :
        ISegment(parent)
{
}

SegmentChunk* Segment::createChunk(AbstractChunkSource *source, BaseRepresentation *rep)
{
     /* act as factory */
    return new (std::nothrow) SegmentChunk(source, rep);
}

void Segment::addSubSegment(SubSegment *subsegment)
{
    if(!subsegments.empty())
    {
        /* Use our own sequence number, and since it it now
           uneffective, also for next subsegments numbering */
        subsegment->setSequenceNumber(subsegments.size());
    }
    subsegments.push_back(subsegment);
}

Segment::~Segment()
{
    std::vector<Segment*>::iterator it;
    for(it=subsegments.begin();it!=subsegments.end();++it)
        delete *it;
}

void                    Segment::setSourceUrl   ( const std::string &url )
{
    if ( url.empty() == false )
        this->sourceUrl = Url(url);
}

void Segment::debug(vlc_object_t *obj, int indent) const
{
    if (subsegments.empty())
    {
        ISegment::debug(obj, indent);
    }
    else
    {
        std::string text(indent, ' ');
        text.append("Segment");
        msg_Dbg(obj, "%s", text.c_str());
        std::vector<Segment *>::const_iterator l;
        for(l = subsegments.begin(); l != subsegments.end(); ++l)
            (*l)->debug(obj, indent + 1);
    }
}

Url Segment::getUrlSegment() const
{
    if(sourceUrl.hasScheme())
    {
        return sourceUrl;
    }
    else
    {
        Url ret = getParentUrlSegment();
        if (!sourceUrl.empty())
            ret.append(sourceUrl);
        return ret;
    }
}

const std::vector<Segment*> & Segment::subSegments() const
{
    return subsegments;
}

InitSegment::InitSegment(ICanonicalUrl *parent) :
    Segment(parent)
{
    debugName = "InitSegment";
}

IndexSegment::IndexSegment(ICanonicalUrl *parent) :
    Segment(parent)
{
    debugName = "IndexSegment";
}

SubSegment::SubSegment(Segment *main, size_t start, size_t end) :
    Segment(main)
{
    setByteRange(start, end);
    debugName = "SubSegment";
}


