/*****************************************************************************
 * interface.c: interface access for other threads
 * This library provides basic functions for threads to interact with user
 * interface, such as command line.
 *****************************************************************************
 * Copyright (C) 1998-2007 VLC authors and VideoLAN
 * $Id: 6155d5ab7c403055e5c01c5973a6a0192a22e816 $
 *
 * Authors: Vincent Seguin <seguin@via.ecp.fr>
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

/**
 *   \file
 *   This file contains functions related to interface management
 */


/*****************************************************************************
 * Preamble
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <unistd.h>

#include <vlc_common.h>
#include <vlc_modules.h>
#include <vlc_interface.h>
#include <vlc_playlist.h>
#include "libvlc.h"
#include "playlist/playlist_internal.h"
#include "../lib/libvlc_internal.h"

static int AddIntfCallback( vlc_object_t *, char const *,
                            vlc_value_t , vlc_value_t , void * );

/* This lock ensures that the playlist is created only once (per instance). It
 * also protects the list of running interfaces against concurrent access,
 * either to add or remove an interface.
 *
 * However, it does NOT protect from destruction of the playlist by
 * intf_DestroyAll(). Instead, care must be taken that intf_Create() and any
 * other function that depends on the playlist is only called BEFORE
 * intf_DestroyAll() has the possibility to destroy all interfaces.
 */
static vlc_mutex_t lock = VLC_STATIC_MUTEX;

/**
 * Create and start an interface.
 *
 * @param playlist playlist and parent object for the interface
 * @param chain configuration chain string
 * @return VLC_SUCCESS or an error code
 */
int intf_Create( playlist_t *playlist, const char *chain )
{
    /* Allocate structure */
    intf_thread_t *p_intf = vlc_custom_create( playlist, sizeof( *p_intf ),
                                               "interface" );
    if( unlikely(p_intf == NULL) )
        return VLC_ENOMEM;

    /* Variable used for interface spawning */
    vlc_value_t val, text;
    var_Create( p_intf, "intf-add", VLC_VAR_STRING | VLC_VAR_ISCOMMAND );
    text.psz_string = _("Add Interface");
    var_Change( p_intf, "intf-add", VLC_VAR_SETTEXT, &text, NULL );
#if !defined(_WIN32) && defined(HAVE_ISATTY)
    if( isatty( 0 ) )
#endif
    {
        val.psz_string = (char *)"rc,none";
        text.psz_string = (char *)_("Console");
        var_Change( p_intf, "intf-add", VLC_VAR_ADDCHOICE, &val, &text );
    }
    val.psz_string = (char *)"telnet,none";
    text.psz_string = (char *)_("Telnet");
    var_Change( p_intf, "intf-add", VLC_VAR_ADDCHOICE, &val, &text );
    val.psz_string = (char *)"http,none";
    text.psz_string = (char *)_("Web");
    var_Change( p_intf, "intf-add", VLC_VAR_ADDCHOICE, &val, &text );
    val.psz_string = (char *)"gestures,none";
    text.psz_string = (char *)_("Mouse Gestures");
    var_Change( p_intf, "intf-add", VLC_VAR_ADDCHOICE, &val, &text );

    var_AddCallback( p_intf, "intf-add", AddIntfCallback, playlist );

    /* Choose the best module */
    char *module;

    p_intf->p_cfg = NULL;
    free( config_ChainCreate( &module, &p_intf->p_cfg, chain ) );
    p_intf->p_module = module_need( p_intf, "interface", module, true );
    free(module);
    if( p_intf->p_module == NULL )
    {
        msg_Err( p_intf, "no suitable interface module" );
        goto error;
    }

    vlc_mutex_lock( &lock );
    p_intf->p_next = pl_priv( playlist )->interface;
    pl_priv( playlist )->interface = p_intf;
    vlc_mutex_unlock( &lock );

    return VLC_SUCCESS;

error:
    if( p_intf->p_module )
        module_unneed( p_intf, p_intf->p_module );
    config_ChainDestroy( p_intf->p_cfg );
    vlc_object_release( p_intf );
    return VLC_EGENERIC;
}

/**
 * Creates the playlist if necessary, and return a pointer to it.
 * @note The playlist is not reference-counted. So the pointer is only valid
 * until intf_DestroyAll() destroys interfaces.
 */
static playlist_t *intf_GetPlaylist(libvlc_int_t *libvlc)
{
    playlist_t *playlist;

    vlc_mutex_lock(&lock);
    playlist = libvlc_priv(libvlc)->playlist;
    if (playlist == NULL)
    {
        playlist = playlist_Create(VLC_OBJECT(libvlc));
        libvlc_priv(libvlc)->playlist = playlist;
    }
    vlc_mutex_unlock(&lock);

    return playlist;
}

/**
 * Inserts an item in the playlist.
 *
 * This function is used during initialization. Unlike playlist_Add() and
 * variants, it inserts an item to the beginning of the playlist. That is
 * meant to compensate for the reverse parsing order of the command line.
 *
 * @note This function may <b>not</b> be called at the same time as
 * intf_DestroyAll().
 */
int intf_InsertItem(libvlc_int_t *libvlc, const char *mrl, unsigned optc,
                    const char *const *optv, unsigned flags)
{
    playlist_t *playlist = intf_GetPlaylist(libvlc);
    input_item_t *item = input_item_New(mrl, NULL);

    if (unlikely(item == NULL))
        return -1;

    int ret = -1;

    if (input_item_AddOptions(item, optc, optv, flags) == VLC_SUCCESS)
    {
        playlist_Lock(playlist);
        if (playlist_NodeAddInput(playlist, item, playlist->p_playing,
                                  0) != NULL)
            ret = 0;
        playlist_Unlock(playlist);
    }
    input_item_Release(item);
    return ret;
}

void libvlc_InternalPlay(libvlc_int_t *libvlc)
{
    playlist_t *pl;

    vlc_mutex_lock(&lock);
    pl = libvlc_priv(libvlc)->playlist;
    vlc_mutex_unlock(&lock);

    if (pl != NULL && var_GetBool(pl, "playlist-autostart"))
        playlist_Control(pl, PLAYLIST_PLAY, false);
}

/**
 * Starts an interface plugin.
 */
int libvlc_InternalAddIntf(libvlc_int_t *libvlc, const char *name)
{
    playlist_t *playlist = intf_GetPlaylist(libvlc);
    int ret;

    if (unlikely(playlist == NULL))
        ret = VLC_ENOMEM;
    else
    if (name != NULL)
        ret = intf_Create(playlist, name);
    else
    {   /* Default interface */
        char *intf = var_InheritString(libvlc, "intf");
        if (intf == NULL) /* "intf" has not been set */
        {
#if !defined(_WIN32) && !defined(__OS2__)
            char *pidfile = var_InheritString(libvlc, "pidfile");
            if (pidfile != NULL)
                free(pidfile);
            else
#endif
                msg_Info(libvlc, _("Running vlc with the default interface. "
                         "Use 'cvlc' to use vlc without interface."));
        }
        ret = intf_Create(playlist, intf);
        free(intf);
        name = "default";
    }
    if (ret != VLC_SUCCESS)
        msg_Err(libvlc, "interface \"%s\" initialization failed", name);
    return ret;
}

/**
 * Stops and destroys all interfaces, then the playlist.
 * @warning FIXME
 * @param libvlc the LibVLC instance
 */
void intf_DestroyAll(libvlc_int_t *libvlc)
{
    playlist_t *playlist;

    vlc_mutex_lock(&lock);
    playlist = libvlc_priv(libvlc)->playlist;
    if (playlist != NULL)
    {
        intf_thread_t *intf, **pp = &(pl_priv(playlist)->interface);

        while ((intf = *pp) != NULL)
        {
            *pp = intf->p_next;
            vlc_mutex_unlock(&lock);

            module_unneed(intf, intf->p_module);
            config_ChainDestroy(intf->p_cfg);
            var_DelCallback(intf, "intf-add", AddIntfCallback, playlist);
            vlc_object_release(intf);

            vlc_mutex_lock(&lock);
        }

        libvlc_priv(libvlc)->playlist = NULL;
    }
    vlc_mutex_unlock(&lock);

    if (playlist != NULL)
        playlist_Destroy(playlist);
}

/* Following functions are local */

static int AddIntfCallback( vlc_object_t *obj, char const *var,
                            vlc_value_t old, vlc_value_t cur, void *data )
{
    playlist_t *playlist = data;

    int ret = intf_Create( playlist, cur.psz_string );
    if( ret )
        msg_Err( obj, "interface \"%s\" initialization failed",
                 cur.psz_string );

    (void) var; (void) old;
    return ret;
}
