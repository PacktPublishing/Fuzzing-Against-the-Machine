/*
 * FakeESOutID.cpp
 *****************************************************************************
 * Copyright © 2015 VideoLAN and VLC Authors
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

#include "FakeESOutID.hpp"
#include "FakeESOut.hpp"

using namespace adaptive;

FakeESOutID::FakeESOutID( FakeESOut *fakeesout, const es_format_t *p_fmt )
    : fakeesout( fakeesout )
    , p_real_es_id( nullptr )
    , pending_delete( false )
{
    es_format_Copy( &fmt, p_fmt );
}

FakeESOutID::~FakeESOutID()
{
    es_format_Clean( &fmt );
}

void FakeESOutID::setRealESID( es_out_id_t *real_es_id )
{
   p_real_es_id = real_es_id;
}

void FakeESOutID::sendData( block_t *p_block )
{
    fakeesout->sendData( this, p_block );
}

EsType FakeESOutID::esType() const
{
    if(fmt.i_cat == VIDEO_ES)
        return EsType::Video;
    else if(fmt.i_cat == AUDIO_ES)
        return EsType::Audio;
    else
        return EsType::Other;
}

void FakeESOutID::create()
{
    fakeesout->createOrRecycleRealEsID( this );
}

void FakeESOutID::release()
{
    fakeesout->recycle( this );
}

es_out_id_t * FakeESOutID::realESID()
{
    return p_real_es_id;
}

const es_format_t *FakeESOutID::getFmt() const
{
    return &fmt;
}

bool FakeESOutID::isCompatible( const FakeESOutID *p_other ) const
{
    if( p_other->fmt.i_cat != fmt.i_cat ||
        fmt.i_codec != p_other->fmt.i_codec ||
        fmt.i_original_fourcc != p_other->fmt.i_original_fourcc )
        return false;

    if((fmt.i_extra > 0) ^ (p_other->fmt.i_extra > 0))
        return false;

    if(fmt.i_profile != p_other->fmt.i_profile ||
       fmt.i_level != p_other->fmt.i_level)
        return false;

    switch(fmt.i_codec)
    {
        case VLC_CODEC_H264:
        case VLC_CODEC_HEVC:
        case VLC_CODEC_VC1:
        case VLC_CODEC_AV1:
        {
            if(fmt.i_extra && p_other->fmt.i_extra &&
               fmt.i_extra == p_other->fmt.i_extra)
            {
               return !!memcmp(fmt.p_extra, p_other->fmt.p_extra, fmt.i_extra);
            }
            else return false; /* no extra, can't tell anything */
        }

        default:
            if(fmt.i_cat == AUDIO_ES)
            {
                /* Reject audio streams with different or unknown rates */
                if(fmt.audio.i_rate != p_other->fmt.audio.i_rate || !fmt.audio.i_rate)
                    return false;
                if(fmt.i_extra &&
                   (fmt.i_extra != p_other->fmt.i_extra ||
                    memcmp(fmt.p_extra, p_other->fmt.p_extra, fmt.i_extra)))
                    return false;
            }

            return es_format_IsSimilar( &p_other->fmt, &fmt );
    }
}

void FakeESOutID::setScheduledForDeletion()
{
    pending_delete = true;
}

bool FakeESOutID::scheduledForDeletion() const
{
    return pending_delete;
}
