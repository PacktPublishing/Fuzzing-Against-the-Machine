/*****************************************************************************
 * cmd_fullscreen.cpp
 *****************************************************************************
 * Copyright (C) 2003-2009 the VideoLAN team
 * $Id: 7dcfe7923c70ab72edb8f9154802109a71137350 $
 *
 * Authors: Cyril Deguet     <asmax@via.ecp.fr>
 *          Olivier Teulière <ipkiss@via.ecp.fr>
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

#include "cmd_fullscreen.hpp"
#include <vlc_input.h>
#include <vlc_vout.h>
#include <vlc_playlist.h>


void CmdFullscreen::execute()
{
    bool fs;
    bool hasVout = false;
    if( getIntf()->p_sys->p_input != NULL )
    {
        vout_thread_t *pVout = input_GetVout( getIntf()->p_sys->p_input );
        if( pVout )
        {
            // Toggle fullscreen
            fs = var_ToggleBool( pVout, "fullscreen" );
            vlc_object_release( pVout );
            hasVout = true;
        }
    }

    if( hasVout )
        var_SetBool( pl_Get( getIntf() ), "fullscreen", fs );
    else
        var_ToggleBool( pl_Get( getIntf() ), "fullscreen" );
}
