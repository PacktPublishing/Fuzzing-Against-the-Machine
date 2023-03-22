/*****************************************************************************
 * interlacing.c
 *****************************************************************************
 * Copyright (C) 2010 Laurent Aimar
 * $Id: f07ef8ef5a016521bd4732f90185027cb5dda9ed $
 *
 * Authors: Laurent Aimar <fenrir _AT_ videolan _DOT_ org>
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
#include <assert.h>

#include <vlc_common.h>
#include <vlc_vout.h>

#include "interlacing.h"
#include "vout_internal.h"

/*****************************************************************************
 * Deinterlacing
 *****************************************************************************/
/* XXX
 * You can use the non vout filter if and only if the video properties stay the
 * same (width/height/chroma/fps), at least for now.
 */
static const char deinterlace_modes[][9]= {
    "auto",
    "discard",
    "blend",
    "mean",
    "bob",
    "linear",
    "x",
    "yadif",
    "yadif2x",
    "phosphor",
    "ivtc",
};

static bool DeinterlaceIsModeValid(const char *mode)
{
    for (unsigned i = 0; i < ARRAY_SIZE(deinterlace_modes); i++) {
        if (!strcmp(deinterlace_modes[i], mode))
            return true;
    }
    return false;
}

static int DeinterlaceCallback(vlc_object_t *object, char const *cmd,
                               vlc_value_t oldval, vlc_value_t newval, void *data)
{
    VLC_UNUSED(cmd); VLC_UNUSED(oldval); VLC_UNUSED(newval); VLC_UNUSED(data);
    vout_thread_t *vout = (vout_thread_t *)object;

    /* */
    const int  deinterlace_state = var_GetInteger(vout, "deinterlace");
    char       *mode             = var_GetString(vout,  "deinterlace-mode");
    const bool is_needed         = var_GetBool(vout,    "deinterlace-needed");
    if (!mode || !DeinterlaceIsModeValid(mode))
    {
        free(mode);
        return VLC_EGENERIC;
    }

    /* */
    char *old = var_CreateGetString(vout, "sout-deinterlace-mode");
    var_SetString(vout, "sout-deinterlace-mode", mode);

    msg_Dbg(vout, "deinterlace %d, mode %s, is_needed %d", deinterlace_state, mode, is_needed);
    if (deinterlace_state == 0 || (deinterlace_state < 0 && !is_needed))
        vout_control_PushBool(&vout->p->control,
                              VOUT_CONTROL_CHANGE_INTERLACE, false);
    else
        vout_control_PushBool(&vout->p->control,
                              VOUT_CONTROL_CHANGE_INTERLACE, true);

    /* */
    free(old);
    free(mode);
    return VLC_SUCCESS;
}

void vout_InitInterlacingSupport(vout_thread_t *vout, bool is_interlaced)
{
    vlc_value_t val, text;

    msg_Dbg(vout, "Deinterlacing available");

    vout->p->filter.has_deint = false;

    /* Create the configuration variables */
    /* */
    var_Create(vout, "deinterlace", VLC_VAR_INTEGER | VLC_VAR_DOINHERIT );
    int deinterlace_state = var_GetInteger(vout, "deinterlace");

    text.psz_string = _("Deinterlace");
    var_Change(vout, "deinterlace", VLC_VAR_SETTEXT, &text, NULL);

    const module_config_t *optd = config_FindConfig("deinterlace");
    var_Change(vout, "deinterlace", VLC_VAR_CLEARCHOICES, NULL, NULL);
    if (likely(optd != NULL))
        for (unsigned i = 0; i < optd->list_count; i++) {
            val.i_int = optd->list.i[i];
            text.psz_string = vlc_gettext(optd->list_text[i]);
            var_Change(vout, "deinterlace", VLC_VAR_ADDCHOICE, &val, &text);
        }
    var_AddCallback(vout, "deinterlace", DeinterlaceCallback, NULL);
    /* */
    var_Create(vout, "deinterlace-mode", VLC_VAR_STRING | VLC_VAR_DOINHERIT );
    char *deinterlace_mode = var_GetNonEmptyString(vout, "deinterlace-mode");

    text.psz_string = _("Deinterlace mode");
    var_Change(vout, "deinterlace-mode", VLC_VAR_SETTEXT, &text, NULL);

    const module_config_t *optm = config_FindConfig("deinterlace-mode");
    var_Change(vout, "deinterlace-mode", VLC_VAR_CLEARCHOICES, NULL, NULL);
    if (likely(optm != NULL))
        for (unsigned i = 0; i < optm->list_count; i++) {
             if (!DeinterlaceIsModeValid(optm->list.psz[i]))
                 continue;

             val.psz_string  = (char *)optm->list.psz[i];
             text.psz_string = vlc_gettext(optm->list_text[i]);
             var_Change(vout, "deinterlace-mode", VLC_VAR_ADDCHOICE,
                        &val, &text);
         }
    var_AddCallback(vout, "deinterlace-mode", DeinterlaceCallback, NULL);
    /* */
    var_Create(vout, "deinterlace-needed", VLC_VAR_BOOL);
    var_AddCallback(vout, "deinterlace-needed", DeinterlaceCallback, NULL);

    /* Override the initial value from filters if present */
    char *filter_mode = NULL;
    if (vout->p->filter.has_deint)
        filter_mode = var_CreateGetNonEmptyString(vout, "sout-deinterlace-mode");
    if (filter_mode) {
        deinterlace_state = 1;
        free(deinterlace_mode);
        deinterlace_mode = filter_mode;
    }

    /* */
    val.psz_string = deinterlace_mode ? deinterlace_mode : optm->orig.psz;
    var_Change(vout, "deinterlace-mode", VLC_VAR_SETVALUE, &val, NULL);
    val.b_bool = is_interlaced;
    var_Change(vout, "deinterlace-needed", VLC_VAR_SETVALUE, &val, NULL);

    var_SetInteger(vout, "deinterlace", deinterlace_state);
    free(deinterlace_mode);

    vout->p->interlacing.is_interlaced = is_interlaced;
    if (is_interlaced)
        vout->p->interlacing.date = mdate();
}

void vout_ReinitInterlacingSupport(vout_thread_t *vout)
{
    vout->p->interlacing.is_interlaced = false;
    var_SetBool(vout, "deinterlace-needed", false);
}

void vout_SetInterlacingState(vout_thread_t *vout, bool is_interlaced)
{
     /* Wait 30s before quiting interlacing mode */
    const int interlacing_change = (!!is_interlaced)
                                 - (!!vout->p->interlacing.is_interlaced);
    if (interlacing_change == 1 ||
        (interlacing_change == -1 &&
        vout->p->interlacing.date + 30000000 < mdate()))
    {
        msg_Dbg(vout, "Detected %s video",
                 is_interlaced ? "interlaced" : "progressive");
        var_SetBool(vout, "deinterlace-needed", is_interlaced);
        vout->p->interlacing.is_interlaced = is_interlaced;
    }
    if (is_interlaced)
        vout->p->interlacing.date = mdate();
}
