/*****************************************************************************
 * playlist_fetcher.h:
 *****************************************************************************
 * Copyright (C) 1999-2008 VLC authors and VideoLAN
 * $Id: 06718acf77542c69d2f881ad5ed8662d693fe2fb $
 *
 * Authors: Samuel Hocevar <sam@zoy.org>
 *          Clément Stenac <zorglub@videolan.org>
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

#ifndef _PLAYLIST_FETCHER_H
#define _PLAYLIST_FETCHER_H 1

#include <vlc_input_item.h>

/**
 * Fetcher opaque structure.
 *
 * The fetcher object will retrieve the art album data for any given input
 * item in an asynchronous way.
 */
typedef struct playlist_fetcher_t playlist_fetcher_t;

/**
 * This function creates the fetcher object and thread.
 */
playlist_fetcher_t *playlist_fetcher_New( vlc_object_t * );

/**
 * This function enqueues the provided item to be art fetched.
 *
 * The input item is retained until the art fetching is done or until the
 * fetcher object is destroyed.
 */
int playlist_fetcher_Push( playlist_fetcher_t *, input_item_t *,
                           input_item_meta_request_option_t, int );

/**
 * This function destroys the fetcher object and thread.
 *
 * All pending input items will be released.
 */
void playlist_fetcher_Delete( playlist_fetcher_t * );

#endif

