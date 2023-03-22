/*****************************************************************************
 * output.c : internal management of output streams for the audio output
 *****************************************************************************
 * Copyright (C) 2002-2004 VLC authors and VideoLAN
 * $Id: a1294110be2127cd0f43c2244c33c1453fbaeb75 $
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <assert.h>

#include <vlc_common.h>
#include <vlc_aout.h>
#include <vlc_modules.h>

#include "libvlc.h"
#include "aout_internal.h"

static const char unset_str[1] = ""; /* Non-NULL constant string pointer */

struct aout_dev
{
    aout_dev_t *next;
    char *name;
    char id[1];
};


/* Local functions */
static void aout_OutputAssertLocked (audio_output_t *aout)
{
    aout_owner_t *owner = aout_owner (aout);

    vlc_assert_locked (&owner->lock);
}

static void aout_Destructor( vlc_object_t * p_this );

static int var_Copy (vlc_object_t *src, const char *name, vlc_value_t prev,
                     vlc_value_t value, void *data)
{
    vlc_object_t *dst = data;

    (void) src; (void) prev;
    return var_Set (dst, name, value);
}

static int var_CopyDevice (vlc_object_t *src, const char *name,
                           vlc_value_t prev, vlc_value_t value, void *data)
{
    vlc_object_t *dst = data;

    (void) src; (void) name; (void) prev;
    return var_Set (dst, "audio-device", value);
}

/**
 * Supply or update the current custom ("hardware") volume.
 * @note This only makes sense after calling aout_VolumeHardInit().
 * @param volume current custom volume
 *
 * @warning The caller (i.e. the audio output plug-in) is responsible for
 * interlocking and synchronizing call to this function and to the
 * audio_output_t.volume_set callback. This ensures that VLC gets correct
 * volume information (possibly with a latency).
 */
static void aout_VolumeNotify (audio_output_t *aout, float volume)
{
    var_SetFloat (aout, "volume", volume);
}

static void aout_MuteNotify (audio_output_t *aout, bool mute)
{
    var_SetBool (aout, "mute", mute);
}

static void aout_PolicyNotify (audio_output_t *aout, bool cork)
{
    (cork ? var_IncInteger : var_DecInteger) (aout->obj.parent, "corks");
}

static void aout_DeviceNotify (audio_output_t *aout, const char *id)
{
    var_SetString (aout, "device", (id != NULL) ? id : "");
}

static void aout_HotplugNotify (audio_output_t *aout,
                                const char *id, const char *name)
{
    aout_owner_t *owner = aout_owner (aout);
    aout_dev_t *dev, **pp = &owner->dev.list;

    vlc_mutex_lock (&owner->dev.lock);
    while ((dev = *pp) != NULL)
    {
        if (!strcmp (id, dev->id))
            break;
        pp = &dev->next;
    }

    if (name != NULL)
    {
        if (dev == NULL) /* Added device */
        {
            dev = malloc (sizeof (*dev) + strlen (id));
            if (unlikely(dev == NULL))
                goto out;
            dev->next = NULL;
            strcpy (dev->id, id);
            *pp = dev;
            owner->dev.count++;
        }
        else /* Modified device */
            free (dev->name);
        dev->name = strdup (name);
    }
    else
    {
        if (dev != NULL) /* Removed device */
        {
            owner->dev.count--;
            *pp = dev->next;
            free (dev->name);
            free (dev);
        }
    }
out:
    vlc_mutex_unlock (&owner->dev.lock);
}

static void aout_RestartNotify (audio_output_t *aout, unsigned mode)
{
    aout_RequestRestart (aout, mode);
}

static int aout_GainNotify (audio_output_t *aout, float gain)
{
    aout_owner_t *owner = aout_owner (aout);

    aout_OutputAssertLocked (aout);
    aout_volume_SetVolume (owner->volume, gain);
    /* XXX: ideally, return -1 if format cannot be amplified */
    return 0;
}

static int FilterCallback (vlc_object_t *obj, const char *var,
                           vlc_value_t prev, vlc_value_t cur, void *data)
{
    if (strcmp(prev.psz_string, cur.psz_string))
        aout_InputRequestRestart ((audio_output_t *)obj);
    (void) var; (void) data;
    return VLC_SUCCESS;
}

static int StereoModeCallback (vlc_object_t *obj, const char *varname,
                               vlc_value_t oldval, vlc_value_t newval, void *data)
{
    audio_output_t *aout = (audio_output_t *)obj;
    (void)varname; (void)oldval; (void)newval; (void)data;

    aout_RestartRequest (aout, AOUT_RESTART_STEREOMODE);
    return 0;
}

static int ViewpointCallback (vlc_object_t *obj, const char *var,
                              vlc_value_t prev, vlc_value_t cur, void *data)
{
    if( cur.p_address != NULL )
        aout_ChangeViewpoint((audio_output_t *)obj, cur.p_address );
    (void) var; (void) data; (void) prev;
    return VLC_SUCCESS;
}

#undef aout_New
/**
 * Creates an audio output object and initializes an output module.
 */
audio_output_t *aout_New (vlc_object_t *parent)
{
    vlc_value_t val, text;

    audio_output_t *aout = vlc_custom_create (parent, sizeof (aout_instance_t),
                                              "audio output");
    if (unlikely(aout == NULL))
        return NULL;

    aout_owner_t *owner = aout_owner (aout);

    vlc_mutex_init (&owner->lock);
    vlc_mutex_init (&owner->req.lock);
    vlc_mutex_init (&owner->dev.lock);
    vlc_mutex_init (&owner->vp.lock);
    vlc_viewpoint_init (&owner->vp.value);
    atomic_init (&owner->vp.update, false);
    owner->req.device = (char *)unset_str;
    owner->req.volume = -1.f;
    owner->req.mute = -1;

    vlc_object_set_destructor (aout, aout_Destructor);

    /* Audio output module callbacks */
    var_Create (aout, "volume", VLC_VAR_FLOAT);
    var_AddCallback (aout, "volume", var_Copy, parent);
    var_Create (aout, "mute", VLC_VAR_BOOL | VLC_VAR_DOINHERIT);
    var_AddCallback (aout, "mute", var_Copy, parent);
    var_Create (aout, "device", VLC_VAR_STRING);
    var_AddCallback (aout, "device", var_CopyDevice, parent);
    /* TODO: 3.0 HACK: only way to signal DTS_HD to aout modules. */
    var_Create (aout, "dtshd", VLC_VAR_BOOL);

    aout->event.volume_report = aout_VolumeNotify;
    aout->event.mute_report = aout_MuteNotify;
    aout->event.policy_report = aout_PolicyNotify;
    aout->event.device_report = aout_DeviceNotify;
    aout->event.hotplug_report = aout_HotplugNotify;
    aout->event.gain_request = aout_GainNotify;
    aout->event.restart_request = aout_RestartNotify;

    /* Audio output module initialization */
    aout->start = NULL;
    aout->stop = NULL;
    aout->volume_set = NULL;
    aout->mute_set = NULL;
    aout->device_select = NULL;
    owner->module = module_need (aout, "audio output", "$aout", false);
    if (owner->module == NULL)
    {
        msg_Err (aout, "no suitable audio output module");
        vlc_object_release (aout);
        return NULL;
    }

    /*
     * Persistent audio output variables
     */
    module_config_t *cfg;
    char *str;

    /* Visualizations */
    var_Create (aout, "visual", VLC_VAR_STRING);
    text.psz_string = _("Visualizations");
    var_Change (aout, "visual", VLC_VAR_SETTEXT, &text, NULL);
    val.psz_string = (char *)"";
    text.psz_string = _("Disable");
    var_Change (aout, "visual", VLC_VAR_ADDCHOICE, &val, &text);
    val.psz_string = (char *)"spectrometer";
    text.psz_string = _("Spectrometer");
    var_Change (aout, "visual", VLC_VAR_ADDCHOICE, &val, &text);
    val.psz_string = (char *)"scope";
    text.psz_string = _("Scope");
    var_Change (aout, "visual", VLC_VAR_ADDCHOICE, &val, &text);
    val.psz_string = (char *)"spectrum";
    text.psz_string = _("Spectrum");
    var_Change (aout, "visual", VLC_VAR_ADDCHOICE, &val, &text);
    val.psz_string = (char *)"vuMeter";
    text.psz_string = _("VU meter");
    var_Change (aout, "visual", VLC_VAR_ADDCHOICE, &val, &text);
    /* Look for goom plugin */
    if (module_exists ("goom"))
    {
        val.psz_string = (char *)"goom";
        text.psz_string = (char *)"Goom";
        var_Change (aout, "visual", VLC_VAR_ADDCHOICE, &val, &text);
    }
    /* Look for libprojectM plugin */
    if (module_exists ("projectm"))
    {
        val.psz_string = (char *)"projectm";
        text.psz_string = (char*)"projectM";
        var_Change (aout, "visual", VLC_VAR_ADDCHOICE, &val, &text);
    }
    /* Look for VSXu plugin */
    if (module_exists ("vsxu"))
    {
        val.psz_string = (char *)"vsxu";
        text.psz_string = (char*)"Vovoid VSXu";
        var_Change (aout, "visual", VLC_VAR_ADDCHOICE, &val, &text);
    }
    /* Look for glspectrum plugin */
    if (module_exists ("glspectrum"))
    {
        val.psz_string = (char *)"glspectrum";
        text.psz_string = (char*)"3D spectrum";
        var_Change (aout, "visual", VLC_VAR_ADDCHOICE, &val, &text);
    }
    str = var_GetNonEmptyString (aout, "effect-list");
    if (str != NULL)
    {
        var_SetString (aout, "visual", str);
        free (str);
    }

    var_Create (aout, "audio-filter", VLC_VAR_STRING | VLC_VAR_DOINHERIT);
    var_AddCallback (aout, "audio-filter", FilterCallback, NULL);
    text.psz_string = _("Audio filters");
    var_Change (aout, "audio-filter", VLC_VAR_SETTEXT, &text, NULL);

    var_Create (aout, "viewpoint", VLC_VAR_ADDRESS );
    var_AddCallback (aout, "viewpoint", ViewpointCallback, NULL);

    var_Create (aout, "audio-visual", VLC_VAR_STRING | VLC_VAR_DOINHERIT);
    text.psz_string = _("Audio visualizations");
    var_Change (aout, "audio-visual", VLC_VAR_SETTEXT, &text, NULL);

    /* Replay gain */
    var_Create (aout, "audio-replay-gain-mode",
                VLC_VAR_STRING | VLC_VAR_DOINHERIT );
    text.psz_string = _("Replay gain");
    var_Change (aout, "audio-replay-gain-mode", VLC_VAR_SETTEXT, &text, NULL);
    cfg = config_FindConfig("audio-replay-gain-mode");
    if (likely(cfg != NULL))
        for (unsigned i = 0; i < cfg->list_count; i++)
        {
            val.psz_string = (char *)cfg->list.psz[i];
            text.psz_string = vlc_gettext(cfg->list_text[i]);
            var_Change (aout, "audio-replay-gain-mode", VLC_VAR_ADDCHOICE,
                            &val, &text);
        }

    /* Stereo mode */
    var_Create (aout, "stereo-mode", VLC_VAR_INTEGER | VLC_VAR_DOINHERIT);
    owner->initial_stereo_mode = var_GetInteger (aout, "stereo-mode");

    var_AddCallback (aout, "stereo-mode", StereoModeCallback, NULL);
    vlc_value_t txt;
    txt.psz_string = _("Stereo audio mode");
    var_Change (aout, "stereo-mode", VLC_VAR_SETTEXT, &txt, NULL);

    /* Equalizer */
    var_Create (aout, "equalizer-preamp", VLC_VAR_FLOAT | VLC_VAR_DOINHERIT);
    var_Create (aout, "equalizer-bands", VLC_VAR_STRING | VLC_VAR_DOINHERIT);
    var_Create (aout, "equalizer-preset", VLC_VAR_STRING | VLC_VAR_DOINHERIT);

    return aout;
}

/**
 * Deinitializes an audio output module and destroys an audio output object.
 */
void aout_Destroy (audio_output_t *aout)
{
    aout_owner_t *owner = aout_owner (aout);

    aout_OutputLock (aout);
    module_unneed (aout, owner->module);
    /* Protect against late call from intf.c */
    aout->volume_set = NULL;
    aout->mute_set = NULL;
    aout->device_select = NULL;
    aout_OutputUnlock (aout);

    var_DelCallback (aout, "viewpoint", ViewpointCallback, NULL);
    var_DelCallback (aout, "audio-filter", FilterCallback, NULL);
    var_DelCallback (aout, "device", var_CopyDevice, aout->obj.parent);
    var_DelCallback (aout, "mute", var_Copy, aout->obj.parent);
    var_SetFloat (aout, "volume", -1.f);
    var_DelCallback (aout, "volume", var_Copy, aout->obj.parent);
    var_DelCallback (aout, "stereo-mode", StereoModeCallback, NULL);
    vlc_object_release (aout);
}

/**
 * Destroys the audio output lock used (asynchronously) by interface functions.
 */
static void aout_Destructor (vlc_object_t *obj)
{
    audio_output_t *aout = (audio_output_t *)obj;
    aout_owner_t *owner = aout_owner (aout);

    vlc_mutex_destroy (&owner->dev.lock);
    for (aout_dev_t *dev = owner->dev.list, *next; dev != NULL; dev = next)
    {
        next = dev->next;
        free (dev->name);
        free (dev);
    }

    assert (owner->req.device == unset_str);
    vlc_mutex_destroy (&owner->vp.lock);
    vlc_mutex_destroy (&owner->req.lock);
    vlc_mutex_destroy (&owner->lock);
}

static void aout_PrepareStereoMode (audio_output_t *aout,
                                    audio_sample_format_t *restrict fmt,
                                    aout_filters_cfg_t *filters_cfg,
                                    audio_channel_type_t input_chan_type,
                                    unsigned i_nb_input_channels,
                                    int i_forced_stereo_mode)
{
    /* Fill Stereo mode choices */
    var_Change (aout, "stereo-mode", VLC_VAR_CLEARCHOICES, NULL, NULL);
    vlc_value_t val, txt, default_val = { .i_int = AOUT_VAR_CHAN_UNSET };
    val.i_int = 0;

    if (!AOUT_FMT_LINEAR(fmt) || i_nb_input_channels == 1)
        return;

    val.i_int = AOUT_VAR_CHAN_MONO;
    txt.psz_string = _("Mono");
    var_Change (aout, "stereo-mode", VLC_VAR_ADDCHOICE, &val, &txt);

    if (i_nb_input_channels != 2)
    {
        val.i_int = AOUT_VAR_CHAN_UNSET;
        txt.psz_string = _("Original");
        var_Change (aout, "stereo-mode", VLC_VAR_ADDCHOICE, &val, &txt);
    }
    if (fmt->i_chan_mode & AOUT_CHANMODE_DOLBYSTEREO)
    {
        val.i_int = AOUT_VAR_CHAN_DOLBYS;
        txt.psz_string = _("Dolby Surround");
    }
    else
    {
        val.i_int = AOUT_VAR_CHAN_STEREO;
        txt.psz_string = _("Stereo");
    }
    var_Change (aout, "stereo-mode", VLC_VAR_ADDCHOICE, &val, &txt);

    if (i_nb_input_channels == 2)
    {
        default_val.i_int = val.i_int; /* Stereo or Dolby Surround */

        val.i_int = AOUT_VAR_CHAN_LEFT;
        txt.psz_string = _("Left");
        var_Change (aout, "stereo-mode", VLC_VAR_ADDCHOICE, &val, &txt);
        val.i_int = AOUT_VAR_CHAN_RIGHT;
        txt.psz_string = _("Right");
        var_Change (aout, "stereo-mode", VLC_VAR_ADDCHOICE, &val, &txt);

        val.i_int = AOUT_VAR_CHAN_RSTEREO;
        txt.psz_string = _("Reverse stereo");
        var_Change (aout, "stereo-mode", VLC_VAR_ADDCHOICE, &val, &txt);
    }

    if (input_chan_type == AUDIO_CHANNEL_TYPE_AMBISONICS
     || i_nb_input_channels > 2)
    {
        val.i_int = AOUT_VAR_CHAN_HEADPHONES;
        txt.psz_string = _("Headphones");
        var_Change (aout, "stereo-mode", VLC_VAR_ADDCHOICE, &val, &txt);

        if (i_forced_stereo_mode == AOUT_VAR_CHAN_UNSET
         && aout->current_sink_info.headphones)
        {
            i_forced_stereo_mode = AOUT_VAR_CHAN_HEADPHONES;
            default_val.i_int = val.i_int;
            var_Change (aout, "stereo-mode", VLC_VAR_SETVALUE, &default_val,
                        NULL);
        }
    }

    /* The user may have selected a different channels configuration. */
    switch (i_forced_stereo_mode)
    {
        case AOUT_VAR_CHAN_RSTEREO:
            filters_cfg->remap[AOUT_CHANIDX_LEFT] = AOUT_CHANIDX_RIGHT;
            filters_cfg->remap[AOUT_CHANIDX_RIGHT] = AOUT_CHANIDX_LEFT;
            break;
        case AOUT_VAR_CHAN_STEREO:
            break;
        case AOUT_VAR_CHAN_LEFT:
            filters_cfg->remap[AOUT_CHANIDX_RIGHT] = AOUT_CHANIDX_DISABLE;
            break;
        case AOUT_VAR_CHAN_RIGHT:
            filters_cfg->remap[AOUT_CHANIDX_LEFT] = AOUT_CHANIDX_DISABLE;
            break;
        case AOUT_VAR_CHAN_DOLBYS:
            fmt->i_chan_mode = AOUT_CHANMODE_DOLBYSTEREO;
            break;
        case AOUT_VAR_CHAN_HEADPHONES:
            filters_cfg->headphones = true;
            break;
        case AOUT_VAR_CHAN_MONO:
            /* Remix all channels into one */
            for (size_t i = 0; i < AOUT_CHANIDX_MAX; ++ i)
                filters_cfg->remap[i] = AOUT_CHANIDX_LEFT;
            break;
        default:
            if (i_nb_input_channels == 2
             && fmt->i_chan_mode & AOUT_CHANMODE_DUALMONO)
            {   /* Go directly to the left channel. */
                filters_cfg->remap[AOUT_CHANIDX_RIGHT] = AOUT_CHANIDX_DISABLE;
                default_val.i_int = val.i_int = AOUT_VAR_CHAN_LEFT;
            }
            var_Change (aout, "stereo-mode", VLC_VAR_SETVALUE, &default_val,
                        NULL);
            break;
    }
}

/**
 * Starts an audio output stream.
 * \param fmt audio output stream format [IN/OUT]
 * \warning The caller must hold the audio output lock.
 */
int aout_OutputNew (audio_output_t *aout, audio_sample_format_t *restrict fmt,
                    aout_filters_cfg_t *filters_cfg)
{
    aout_OutputAssertLocked (aout);

    audio_channel_type_t input_chan_type = fmt->channel_type;
    int i_forced_stereo_mode = AOUT_VAR_CHAN_UNSET;
    unsigned i_nb_input_channels = fmt->i_channels;

    /* Ideally, the audio filters would be created before the audio output,
     * and the ideal audio format would be the output of the filters chain.
     * But that scheme would not really play well with digital pass-through. */
    if (AOUT_FMT_LINEAR(fmt))
    {
        if (fmt->channel_type == AUDIO_CHANNEL_TYPE_BITMAP
         && aout_FormatNbChannels(fmt) == 0)
        {
            /* The output channel map is unknown, use the WAVE one. */
            assert(fmt->i_channels > 0);
            aout_SetWavePhysicalChannels(fmt);
        }

        if (fmt->channel_type == AUDIO_CHANNEL_TYPE_AMBISONICS)
        {
            /* Set the maximum of channels to render ambisonics contents. The
             * aout module will still be free to select less channels in order
             * to respect the sink setup. */
            fmt->i_physical_channels = AOUT_CHANS_7_1;
        }

        /* Try to stay in integer domain if possible for no/slow FPU. */
        fmt->i_format = (fmt->i_bitspersample > 16) ? VLC_CODEC_FL32
                                                    : VLC_CODEC_S16N;

        i_forced_stereo_mode = var_GetInteger (aout, "stereo-mode");
        if (i_forced_stereo_mode != AOUT_VAR_CHAN_UNSET)
        {
            if (i_forced_stereo_mode == AOUT_VAR_CHAN_LEFT
             || i_forced_stereo_mode == AOUT_VAR_CHAN_RIGHT)
                fmt->i_physical_channels = AOUT_CHAN_CENTER;
            else
                fmt->i_physical_channels = AOUT_CHANS_STEREO;
        }

        aout_FormatPrepare (fmt);
        assert (aout_FormatNbChannels(fmt) > 0);
    }

    aout->current_sink_info.headphones = false;

    if (aout->start (aout, fmt))
    {
        msg_Err (aout, "module not functional");
        return -1;
    }

    aout_PrepareStereoMode (aout, fmt, filters_cfg, input_chan_type,
                            i_nb_input_channels, i_forced_stereo_mode);

    aout_FormatPrepare (fmt);
    assert (fmt->i_bytes_per_frame > 0 && fmt->i_frame_length > 0);
    aout_FormatPrint (aout, "output", fmt);
    return 0;
}

/**
 * Stops the audio output stream (undoes aout_OutputNew()).
 * \note This can only be called after a successful aout_OutputNew().
 * \warning The caller must hold the audio output lock.
 */
void aout_OutputDelete (audio_output_t *aout)
{
    aout_OutputAssertLocked (aout);

    if (aout->stop != NULL)
        aout->stop (aout);
}

int aout_OutputTimeGet (audio_output_t *aout, mtime_t *delay)
{
    aout_OutputAssertLocked (aout);

    if (aout->time_get == NULL)
        return -1;
    return aout->time_get (aout, delay);
}

/**
 * Plays a decoded audio buffer.
 * \note This can only be called after a successful aout_OutputNew().
 * \warning The caller must hold the audio output lock.
 */
void aout_OutputPlay (audio_output_t *aout, block_t *block)
{
    aout_OutputAssertLocked (aout);
#ifndef NDEBUG
    aout_owner_t *owner = aout_owner (aout);
    assert (owner->mixer_format.i_frame_length > 0);
    assert (block->i_buffer == 0 || block->i_buffer / block->i_nb_samples ==
            owner->mixer_format.i_bytes_per_frame /
            owner->mixer_format.i_frame_length);
#endif
    aout->play (aout, block);
}

static void PauseDefault (audio_output_t *aout, bool pause, mtime_t date)
{
    if (pause)
        aout_OutputFlush (aout, false);
    (void) date;
}

/**
 * Notifies the audio output (if any) of pause/resume events.
 * This enables the output to expedite pause, instead of waiting for its
 * buffers to drain.
 * \note This can only be called after a successful aout_OutputNew().
 * \warning The caller must hold the audio output lock.
 */
void aout_OutputPause( audio_output_t *aout, bool pause, mtime_t date )
{
    aout_OutputAssertLocked (aout);
    ((aout->pause != NULL) ? aout->pause : PauseDefault) (aout, pause, date);
}

/**
 * Flushes or drains the audio output buffers.
 * This enables the output to expedite seek and stop.
 * \param wait if true, wait for buffer playback (i.e. drain),
 *             if false, discard the buffers immediately (i.e. flush)
 * \note This can only be called after a successful aout_OutputNew().
 * \warning The caller must hold the audio output lock.
 */
void aout_OutputFlush( audio_output_t *aout, bool wait )
{
    aout_OutputAssertLocked( aout );
    aout->flush (aout, wait);
}

static int aout_OutputVolumeSet (audio_output_t *aout, float vol)
{
    aout_OutputAssertLocked (aout);
    return (aout->volume_set != NULL) ? aout->volume_set (aout, vol) : -1;
}

static int aout_OutputMuteSet (audio_output_t *aout, bool mute)
{
    aout_OutputAssertLocked (aout);
    return (aout->mute_set != NULL) ? aout->mute_set (aout, mute) : -1;
}

static int aout_OutputDeviceSet (audio_output_t *aout, const char *id)
{
    aout_OutputAssertLocked (aout);
    return (aout->device_select != NULL) ? aout->device_select (aout, id) : -1;
}

void aout_OutputLock (audio_output_t *aout)
{
    aout_owner_t *owner = aout_owner (aout);

    vlc_mutex_lock (&owner->lock);
}

static int aout_OutputTryLock (audio_output_t *aout)
{
    aout_owner_t *owner = aout_owner (aout);

    return vlc_mutex_trylock (&owner->lock);
}

void aout_OutputUnlock (audio_output_t *aout)
{
    aout_owner_t *owner = aout_owner (aout);

    vlc_assert_locked (&owner->lock);
    vlc_mutex_lock (&owner->req.lock);

    if (owner->req.device != unset_str)
    {
        aout_OutputDeviceSet (aout, owner->req.device);
        free (owner->req.device);
        owner->req.device = (char *)unset_str;
    }

    if (owner->req.volume >= 0.f)
    {
        aout_OutputVolumeSet (aout, owner->req.volume);
        owner->req.volume = -1.f;
    }

    if (owner->req.mute >= 0)
    {
        aout_OutputMuteSet (aout, owner->req.mute);
        owner->req.mute = -1;
    }

    vlc_mutex_unlock (&owner->lock);
    /* If another thread is blocked waiting for owner->req.lock at this point,
     * this aout_OutputUnlock() call will not see and apply its change request.
     * The other thread will need to apply the change request itself, which
     * implies it is able to (try-)lock owner->lock. Therefore this thread must
     * release owner->lock _before_ owner->req.lock. Do not reorder!!! */
    vlc_mutex_unlock (&owner->req.lock);
}

/**
 * Gets the volume of the audio output stream (independent of mute).
 * \return Current audio volume (0. = silent, 1. = nominal),
 * or a strictly negative value if undefined.
 */
float aout_VolumeGet (audio_output_t *aout)
{
    return var_GetFloat (aout, "volume");
}

/**
 * Sets the volume of the audio output stream.
 * \note The mute status is not changed.
 * \return 0 on success, -1 on failure (TODO).
 */
int aout_VolumeSet (audio_output_t *aout, float vol)
{
    aout_owner_t *owner = aout_owner (aout);

    assert (vol >= 0.f);
    vlc_mutex_lock (&owner->req.lock);
    owner->req.volume = vol;
    vlc_mutex_unlock (&owner->req.lock);

    if (aout_OutputTryLock (aout) == 0)
        aout_OutputUnlock (aout);
    return 0;
}

/**
 * Raises the volume.
 * \param value how much to increase (> 0) or decrease (< 0) the volume
 * \param volp if non-NULL, will contain contain the resulting volume
 */
int aout_VolumeUpdate (audio_output_t *aout, int value, float *volp)
{
    int ret = -1;
    float stepSize = var_InheritFloat (aout, "volume-step") / (float)AOUT_VOLUME_DEFAULT;
    float delta = value * stepSize;
    float vol = aout_VolumeGet (aout);

    if (vol >= 0.f)
    {
        vol += delta;
        if (vol < 0.f)
            vol = 0.f;
        if (vol > 2.f)
            vol = 2.f;
        vol = (roundf (vol / stepSize)) * stepSize;
        if (volp != NULL)
            *volp = vol;
        ret = aout_VolumeSet (aout, vol);
    }
    return ret;
}

/**
 * Gets the audio output stream mute flag.
 * \return 0 if not muted, 1 if muted, -1 if undefined.
 */
int aout_MuteGet (audio_output_t *aout)
{
    return var_InheritBool (aout, "mute");
}

/**
 * Sets the audio output stream mute flag.
 * \return 0 on success, -1 on failure (TODO).
 */
int aout_MuteSet (audio_output_t *aout, bool mute)
{
    aout_owner_t *owner = aout_owner (aout);

    vlc_mutex_lock (&owner->req.lock);
    owner->req.mute = mute;
    vlc_mutex_unlock (&owner->req.lock);

    if (aout_OutputTryLock (aout) == 0)
        aout_OutputUnlock (aout);
    return 0;
}

/**
 * Gets the currently selected device.
 * \return the selected device ID (caller must free() it)
 *         NULL if no device is selected or in case of error.
 */
char *aout_DeviceGet (audio_output_t *aout)
{
    return var_GetNonEmptyString (aout, "device");
}

/**
 * Selects an audio output device.
 * \param id device ID to select, or NULL for the default device
 * \return zero on success, non-zero on error (TODO).
 */
int aout_DeviceSet (audio_output_t *aout, const char *id)
{
    aout_owner_t *owner = aout_owner (aout);

    char *dev = NULL;
    if (id != NULL)
    {
        dev = strdup (id);
        if (unlikely(dev == NULL))
            return -1;
    }

    vlc_mutex_lock (&owner->req.lock);
    if (owner->req.device != unset_str)
        free (owner->req.device);
    owner->req.device = dev;
    vlc_mutex_unlock (&owner->req.lock);

    if (aout_OutputTryLock (aout) == 0)
        aout_OutputUnlock (aout);
    return 0;
}

/**
 * Enumerates possible audio output devices.
 *
 * The function will heap-allocate two tables of heap-allocated strings;
 * the caller is responsible for freeing all strings and both tables.
 *
 * \param ids pointer to a table of device identifiers [OUT]
 * \param names pointer to a table of device human-readable descriptions [OUT]
 * \return the number of devices, or negative on error.
 * \note In case of error, *ids and *names are undefined.
 */
int aout_DevicesList (audio_output_t *aout, char ***ids, char ***names)
{
    aout_owner_t *owner = aout_owner (aout);
    char **tabid, **tabname;
    unsigned i = 0;

    vlc_mutex_lock (&owner->dev.lock);
    tabid = vlc_alloc (owner->dev.count, sizeof (*tabid));
    tabname = vlc_alloc (owner->dev.count, sizeof (*tabname));

    if (unlikely(tabid == NULL || tabname == NULL))
        goto error;

    *ids = tabid;
    *names = tabname;

    for (aout_dev_t *dev = owner->dev.list; dev != NULL; dev = dev->next)
    {
        tabid[i] = strdup(dev->id);
        if (unlikely(tabid[i] == NULL))
            goto error;

        tabname[i] = strdup(dev->name);
        if (unlikely(tabname[i] == NULL))
        {
            free(tabid[i]);
            goto error;
        }

        i++;
    }
    vlc_mutex_unlock (&owner->dev.lock);

    return i;

error:
    vlc_mutex_unlock(&owner->dev.lock);
    while (i > 0)
    {
        i--;
        free(tabname[i]);
        free(tabid[i]);
    }
    free(tabname);
    free(tabid);
    return -1;
}
