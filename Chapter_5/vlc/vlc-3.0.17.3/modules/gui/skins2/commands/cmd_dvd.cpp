/*****************************************************************************
 * cmd_dvd.cpp
 *****************************************************************************
 * Copyright (C) 2003 the VideoLAN team
 * $Id: 9e9c5f80c506a48b380ac41a66e323e5ca0303a0 $
 *
 * Authors: Olivier Teulière <ipkiss@via.ecp.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#include "cmd_dvd.hpp"
#include <vlc_input.h>
#include <vlc_playlist.h>

void CmdDvdNextTitle::execute()
{
    input_thread_t *p_input = playlist_CurrentInput( getPL() );

    if( p_input )
    {
        var_TriggerCallback( p_input, "next-title" );
        vlc_object_release( p_input );
    }
}


void CmdDvdPreviousTitle::execute()
{
    input_thread_t *p_input = playlist_CurrentInput( getPL() );

    if( p_input )
    {
        var_TriggerCallback( p_input, "prev-title" );
        vlc_object_release( p_input );
    }
}


void CmdDvdNextChapter::execute()
{
    input_thread_t *p_input = playlist_CurrentInput( getPL() );

    if( p_input )
    {
        var_TriggerCallback( p_input, "next-chapter" );
        vlc_object_release( p_input );
    }
}


void CmdDvdPreviousChapter::execute()
{
    input_thread_t *p_input = playlist_CurrentInput( getPL() );

    if( p_input )
    {
        var_TriggerCallback( p_input, "prev-chapter" );
        vlc_object_release( p_input );
    }
}


void CmdDvdRootMenu::execute()
{
    input_thread_t *p_input = playlist_CurrentInput( getPL() );

    if( p_input )
    {
        var_SetInteger( p_input, "title  0", 2);
        vlc_object_release( p_input );
    }
}

