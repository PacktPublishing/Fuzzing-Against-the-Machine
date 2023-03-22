/*****************************************************************************
 * evt_special.cpp
 *****************************************************************************
 * Copyright (C) 2003 the VideoLAN team
 * $Id: afa1d1f87334adf30bdb4ef194974f71b54e40e5 $
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

#include "evt_special.hpp"


const std::string EvtSpecial::getAsString() const
{
    std::string event = "special";

    // Add the action
    if( m_action == kShow )
        event += ":show";
    else if( m_action == kHide )
        event += ":hide";
    else if( m_action == kEnable )
        event += ":enable";
    else if( m_action == kDisable )
        event += ":disable";
    else
        msg_Warn( getIntf(), "unknown action type" );

    return event;
}

