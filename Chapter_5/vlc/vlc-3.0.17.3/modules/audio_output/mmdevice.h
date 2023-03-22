/*****************************************************************************
 * mmdevice.h : Windows Multimedia Device API audio output plugin for VLC
 *****************************************************************************
 * Copyright (C) 2012 Rémi Denis-Courmont
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

#ifndef VLC_AOUT_MMDEVICE_H
# define VLC_AOUT_MMDEVICE_H 1

#define MM_PASSTHROUGH_DISABLED 0
#define MM_PASSTHROUGH_ENABLED 1
#define MM_PASSTHROUGH_ENABLED_HD 2
#define MM_PASSTHROUGH_DEFAULT MM_PASSTHROUGH_DISABLED

typedef struct aout_stream aout_stream_t;

/**
 * Audio output simplified API for Windows
 */
struct aout_stream
{
    VLC_COMMON_MEMBERS
    void *sys;

    HRESULT (*time_get)(aout_stream_t *, mtime_t *);
    HRESULT (*play)(aout_stream_t *, block_t *);
    HRESULT (*pause)(aout_stream_t *, bool);
    HRESULT (*flush)(aout_stream_t *);

    struct
    {
        void *device;
        HRESULT (*activate)(void *device, REFIID, PROPVARIANT *, void **);
    } owner;
};

/**
 * Creates an audio output stream on a given Windows multimedia device.
 * \param s audio output stream object to be initialized
 * \param fmt audio output sample format [IN/OUT]
 * \param sid audio output session GUID [IN]
 */
typedef HRESULT (*aout_stream_start_t)(aout_stream_t *s,
    audio_sample_format_t *fmt, const GUID *sid);

/**
 * Destroys an audio output stream.
 */
typedef HRESULT (*aout_stream_stop_t)(aout_stream_t *);

static inline HRESULT aout_stream_TimeGet(aout_stream_t *s, mtime_t *delay)
{
    return (s->time_get)(s, delay);
}

static inline HRESULT aout_stream_Play(aout_stream_t *s, block_t *block)
{
    return (s->play)(s, block);
}

static inline HRESULT aout_stream_Pause(aout_stream_t *s, bool paused)
{
    return (s->pause)(s, paused);
}

static inline HRESULT aout_stream_Flush(aout_stream_t *s, bool wait)
{
    if (wait)
    {   /* Loosy drain emulation */
        mtime_t delay;

        if (SUCCEEDED(aout_stream_TimeGet(s, &delay))
         && delay <= INT64_C(5000000))
            Sleep((delay / (CLOCK_FREQ / 1000)) + 1);
        return S_OK;
    }
    else
        return (s->flush)(s);
}

static inline
HRESULT aout_stream_Activate(aout_stream_t *s, REFIID iid,
                             PROPVARIANT *actparms, void **pv)
{
    return s->owner.activate(s->owner.device, iid, actparms, pv);
}
#endif
