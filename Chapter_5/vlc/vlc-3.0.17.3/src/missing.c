/*****************************************************************************
 * missing.c: missing libvlccore symbols
 *****************************************************************************
 * Copyright (C) 2008-2011 Rémi Denis-Courmont
 * Copyright (C) 2009-2014 VLC authors and VideoLAN
 * $Id: eb359b9f5269c1fbe6db5f9a8ee5e4440c4cf3a5 $
 *
 * Authors: Rémi Denis-Courmont
 *          Pierre Ynard <linkfanel # yahoo fr>
 *          Toralf Niebuhr <gmthor85 # aim com>
 *          Felix Paul Kühne <fkuehne # videolan org>
 *          Jean-Paul Saman <jpsaman # videolan org>
 *          Antoine Cellerier <dionoea # videolan org>
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

/** \file
 * This file contains dummy replacement API for disabled features
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include <assert.h>

#ifndef ENABLE_SOUT
# include <vlc_sout.h>

void sdp_AddMedia (struct vlc_memstream *sdp, const char *type, const char *protocol,
                    int dport, unsigned pt, bool bw_indep, unsigned bw,
                    const char *ptname, unsigned clockrate, unsigned channels,
                    const char *fmtp)
{
    VLC_UNUSED (sdp); VLC_UNUSED (type); VLC_UNUSED (protocol);
    VLC_UNUSED (dport); VLC_UNUSED (pt); VLC_UNUSED (bw_indep);
    VLC_UNUSED (bw); VLC_UNUSED (ptname); VLC_UNUSED (clockrate);
    VLC_UNUSED (channels); VLC_UNUSED (fmtp);
    assert (sdp == NULL);
}

void sdp_AddAttribute (struct vlc_memstream *sdp, const char *name, const char *fmt, ...)
{
    VLC_UNUSED (sdp); VLC_UNUSED (name); VLC_UNUSED (fmt);
    assert (sdp == NULL);
}

int sout_AccessOutControl (sout_access_out_t *out, int query, ...)
{
    VLC_UNUSED (out); VLC_UNUSED (query);
    vlc_assert_unreachable ();
}

void sout_AccessOutDelete (sout_access_out_t *out)
{
    VLC_UNUSED (out);
    vlc_assert_unreachable ();
}

#undef sout_AccessOutNew
sout_access_out_t *sout_AccessOutNew (vlc_object_t *obj,
                                      const char *access, const char *name)
{
    VLC_UNUSED (access); VLC_UNUSED (name);
    msg_Err (obj, "Output support not compiled-in!");
    return NULL;
}

ssize_t sout_AccessOutRead (sout_access_out_t *out, block_t *block)
{
    VLC_UNUSED (out); VLC_UNUSED (block);
    vlc_assert_unreachable ();
}

int sout_AccessOutSeek (sout_access_out_t *out, off_t offset)
{
    VLC_UNUSED (out); VLC_UNUSED (offset);
    vlc_assert_unreachable ();
}

ssize_t sout_AccessOutWrite (sout_access_out_t *out, block_t *block)
{
    VLC_UNUSED (out); VLC_UNUSED (block);
    vlc_assert_unreachable ();
}

#undef sout_AnnounceRegisterSDP
session_descriptor_t *sout_AnnounceRegisterSDP (vlc_object_t *obj,
                                                const char *sdp,
                                                const char *dst)
{
    VLC_UNUSED (sdp); VLC_UNUSED (dst);
    msg_Err (obj, "SDP export not compiled-in!");
    return NULL;
}

#undef sout_AnnounceUnRegister
void sout_AnnounceUnRegister (vlc_object_t *obj, session_descriptor_t *d)
{
    VLC_UNUSED (obj); VLC_UNUSED (d);
    vlc_assert_unreachable ();
}

#undef sout_EncoderCreate
encoder_t *sout_EncoderCreate( vlc_object_t *p_this )
{
    msg_Err (p_this, "Encoding support not compiled-in!");
    return NULL;
}

sout_input_t *sout_MuxAddStream( sout_mux_t *mux, const es_format_t *fmt )
{
    VLC_UNUSED (mux); VLC_UNUSED (fmt);
    vlc_assert_unreachable ();
}

void sout_MuxDelete (sout_mux_t *mux)
{
    VLC_UNUSED (mux);
    vlc_assert_unreachable ();
}

void sout_MuxDeleteStream (sout_mux_t *mux, sout_input_t *input)
{
    VLC_UNUSED (mux); VLC_UNUSED (input);
    vlc_assert_unreachable ();
}

int sout_MuxGetStream (sout_mux_t *p_mux, unsigned int i_blocks, mtime_t *pi_dts)
{
    VLC_UNUSED (p_mux); VLC_UNUSED (i_blocks); VLC_UNUSED (pi_dts);
    vlc_assert_unreachable ();
}

sout_mux_t *sout_MuxNew (sout_instance_t *instance, const char *mux,
                         sout_access_out_t *out)
{
    VLC_UNUSED (instance); VLC_UNUSED (mux); VLC_UNUSED (out);
    vlc_assert_unreachable ();
}

int sout_MuxSendBuffer (sout_mux_t *mux, sout_input_t *input, block_t *block)
{
    VLC_UNUSED (mux); VLC_UNUSED (input); VLC_UNUSED (block);
    vlc_assert_unreachable ();
}

void sout_MuxFlush( sout_mux_t *mux, sout_input_t *input )
{
    VLC_UNUSED (mux); VLC_UNUSED (input);
    vlc_assert_unreachable ();
}

void sout_StreamChainDelete (sout_stream_t *p_first, sout_stream_t *p_last)
{
    VLC_UNUSED (p_first); VLC_UNUSED (p_last);
    vlc_assert_unreachable ();
}

sout_stream_t *sout_StreamChainNew (sout_instance_t *p_sout, const char *psz_chain,
                                    sout_stream_t *p_next,
                                    sout_stream_t **pp_last)
{
    VLC_UNUSED (p_sout); VLC_UNUSED (psz_chain); VLC_UNUSED (p_next);
    VLC_UNUSED (pp_last);
    vlc_assert_unreachable ();
}

int vlc_sdp_Start (struct vlc_memstream *sdp, vlc_object_t *obj, const char *cfg,
                     const struct sockaddr *src, size_t srclen,
                     const struct sockaddr *addr, size_t addrlen)
{
    VLC_UNUSED (obj); VLC_UNUSED (cfg); VLC_UNUSED (src); VLC_UNUSED (srclen);
    VLC_UNUSED (addr); VLC_UNUSED (addrlen);
    return 0;
}
#endif /* !ENABLE_SOUT */

#ifndef ENABLE_VLM
# include <vlc_vlm.h>

int vlm_Control (vlm_t *vlm, int query, ...)
{
    VLC_UNUSED (query);
    VLC_UNUSED (vlm);
    vlc_assert_unreachable ();
}

void vlm_Delete (vlm_t *vlm)
{
    VLC_UNUSED (vlm);
    vlc_assert_unreachable ();
}

int vlm_ExecuteCommand (vlm_t *vlm, const char *cmd, vlm_message_t **pm)
{
    VLC_UNUSED (vlm);
    VLC_UNUSED (cmd);
    VLC_UNUSED (pm);
    vlc_assert_unreachable ();
}

vlm_message_t *vlm_MessageAdd (vlm_message_t *a, vlm_message_t *b)
{
    VLC_UNUSED (a);
    VLC_UNUSED (b);
    vlc_assert_unreachable ();
}

void vlm_MessageDelete (vlm_message_t *m)
{
    VLC_UNUSED (m);
    vlc_assert_unreachable ();
}

vlm_message_t *vlm_MessageSimpleNew (const char *a)
{
    VLC_UNUSED (a);
    return NULL;
}

vlm_message_t *vlm_MessageNew (const char *a, const char *fmt, ...)
{
    VLC_UNUSED (a);
    VLC_UNUSED (fmt);
    return vlm_MessageSimpleNew (a);
}

#undef vlm_New
vlm_t *vlm_New (vlc_object_t *obj)
{
     msg_Err (obj, "VLM not compiled-in!");
     return NULL;
}
#endif /* !ENABLE_VLM */

#ifndef UPDATE_CHECK
# include <vlc_update.h>

update_t *(update_New)(vlc_object_t *obj)
{
    (void) obj;
    return NULL;
}

void update_Delete(update_t *u)
{
    (void) u;
    vlc_assert_unreachable();
}

void update_Check(update_t *u, void (*cb)(void *, bool), void *opaque)
{
    (void) u; (void) cb; (void) opaque;
    vlc_assert_unreachable();
}

bool update_NeedUpgrade(update_t *u)
{
    (void) u;
    vlc_assert_unreachable();
}

void update_Download(update_t *u, const char *dir)
{
    (void) u; (void) dir;
    vlc_assert_unreachable();
}

update_release_t *update_GetRelease(update_t *u)
{
    (void) u;
    vlc_assert_unreachable();
}
#endif /* !UPDATE_CHECK */
