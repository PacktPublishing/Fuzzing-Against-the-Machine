/*****************************************************************************
 * aout_internal.h : internal defines for audio output
 *****************************************************************************
 * Copyright (C) 2002 VLC authors and VideoLAN
 * $Id: 77070e093baa59e9e9df43a2fef44dc6a4a2573d $
 *
 * Authors: Christophe Massiot <massiot@via.ecp.fr>
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

#ifndef LIBVLC_AOUT_INTERNAL_H
# define LIBVLC_AOUT_INTERNAL_H 1

# include <vlc_atomic.h>
# include <vlc_viewpoint.h>

/* Max input rate factor (1/4 -> 4) */
# define AOUT_MAX_INPUT_RATE (4)

enum {
    AOUT_RESAMPLING_NONE=0,
    AOUT_RESAMPLING_UP,
    AOUT_RESAMPLING_DOWN
};

struct aout_request_vout
{
    struct vout_thread_t  *(*pf_request_vout)( void *, struct vout_thread_t *,
                                               const video_format_t *, bool );
    void *p_private;
};

typedef struct aout_volume aout_volume_t;
typedef struct aout_dev aout_dev_t;

typedef struct
{
    vlc_mutex_t lock;
    module_t *module; /**< Output plugin (or NULL if inactive) */
    aout_filters_t *filters;
    aout_volume_t *volume;

    struct
    {
        vlc_mutex_t lock;
        char *device;
        float volume;
        signed char mute;
    } req;

    struct
    {
        vlc_mutex_t lock;
        aout_dev_t *list;
        unsigned count;
    } dev;

    struct
    {
        atomic_bool update;
        vlc_mutex_t lock;
        vlc_viewpoint_t value;
    } vp;

    struct
    {
        mtime_t end; /**< Last seen PTS */
        unsigned resamp_start_drift; /**< Resampler drift absolute value */
        int resamp_type; /**< Resampler mode (FIXME: redundant / resampling) */
        bool discontinuity;
    } sync;

    int initial_stereo_mode; /**< Initial stereo mode set by options */

    audio_sample_format_t input_format;
    audio_sample_format_t mixer_format;

    aout_request_vout_t request_vout;
    aout_filters_cfg_t filters_cfg;

    atomic_uint buffers_lost;
    atomic_uint buffers_played;
    atomic_uchar restart;
} aout_owner_t;

typedef struct
{
    audio_output_t output;
    aout_owner_t   owner;
} aout_instance_t;

static inline aout_owner_t *aout_owner (audio_output_t *aout)
{
    return &((aout_instance_t *)aout)->owner;
}

/****************************************************************************
 * Prototypes
 *****************************************************************************/

/* From mixer.c : */
aout_volume_t *aout_volume_New(vlc_object_t *, const audio_replay_gain_t *);
#define aout_volume_New(o, g) aout_volume_New(VLC_OBJECT(o), g)
int aout_volume_SetFormat(aout_volume_t *, vlc_fourcc_t);
void aout_volume_SetVolume(aout_volume_t *, float);
int aout_volume_Amplify(aout_volume_t *, block_t *);
void aout_volume_Delete(aout_volume_t *);


/* From output.c : */
audio_output_t *aout_New (vlc_object_t *);
#define aout_New(a) aout_New(VLC_OBJECT(a))
void aout_Destroy (audio_output_t *);

int aout_OutputNew(audio_output_t *, audio_sample_format_t *,
                   aout_filters_cfg_t *filters_cfg);
int aout_OutputTimeGet(audio_output_t *, mtime_t *);
void aout_OutputPlay(audio_output_t *, block_t *);
void aout_OutputPause( audio_output_t * p_aout, bool, mtime_t );
void aout_OutputFlush( audio_output_t * p_aout, bool );
void aout_OutputDelete( audio_output_t * p_aout );
void aout_OutputLock(audio_output_t *);
void aout_OutputUnlock(audio_output_t *);


/* From common.c : */
void aout_FormatsPrint(vlc_object_t *, const char *,
                       const audio_sample_format_t *,
                       const audio_sample_format_t *);
#define aout_FormatsPrint(o, t, a, b) \
        aout_FormatsPrint(VLC_OBJECT(o), t, a, b)
bool aout_ChangeFilterString( vlc_object_t *manager, vlc_object_t *aout,
                              const char *var, const char *name, bool b_add );

/* From dec.c */
#define AOUT_DEC_SUCCESS 0
#define AOUT_DEC_CHANGED 1
#define AOUT_DEC_FAILED VLC_EGENERIC

int aout_DecNew(audio_output_t *, const audio_sample_format_t *,
                const audio_replay_gain_t *, const aout_request_vout_t *);
void aout_DecDelete(audio_output_t *);
int aout_DecPlay(audio_output_t *, block_t *, int i_input_rate);
void aout_DecGetResetStats(audio_output_t *, unsigned *, unsigned *);
void aout_DecChangePause(audio_output_t *, bool b_paused, mtime_t i_date);
void aout_DecFlush(audio_output_t *, bool wait);
void aout_RequestRestart (audio_output_t *, unsigned);

static inline void aout_InputRequestRestart(audio_output_t *aout)
{
    aout_RequestRestart(aout, AOUT_RESTART_FILTERS);
}

static inline void aout_SetWavePhysicalChannels(audio_sample_format_t *fmt)
{
    static const uint32_t wave_channels[] = {
        AOUT_CHAN_LEFT, AOUT_CHAN_RIGHT, AOUT_CHAN_CENTER,
        AOUT_CHAN_LFE, AOUT_CHAN_REARLEFT, AOUT_CHAN_REARRIGHT,
        AOUT_CHAN_MIDDLELEFT, AOUT_CHAN_MIDDLERIGHT, AOUT_CHAN_REARCENTER };

    fmt->i_physical_channels = 0;
    for (int i = 0; i < fmt->i_channels && i < AOUT_CHAN_MAX; ++i)
        fmt->i_physical_channels |= wave_channels[i];
    aout_FormatPrepare(fmt);
}

/* From filters.c */
bool aout_FiltersCanResample (aout_filters_t *filters);

void aout_ChangeViewpoint(audio_output_t *aout,
                          const vlc_viewpoint_t *p_viewpoint);

#endif /* !LIBVLC_AOUT_INTERNAL_H */
