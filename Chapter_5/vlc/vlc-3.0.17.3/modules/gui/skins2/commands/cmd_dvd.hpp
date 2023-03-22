/*****************************************************************************
 * cmd_dvd.hpp
 *****************************************************************************
 * Copyright (C) 2003 the VideoLAN team
 * $Id: ee905984339bb5c2902d98775726fa170cfaf9fe $
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

#ifndef CMD_DVD_HPP
#define CMD_DVD_HPP

#include "cmd_generic.hpp"

/// Commands to control the input
DEFINE_COMMAND( DvdNextTitle, "next title" )
DEFINE_COMMAND( DvdPreviousTitle, "previous title" )
DEFINE_COMMAND( DvdNextChapter, "next chapter" )
DEFINE_COMMAND( DvdPreviousChapter, "previous chapter" )
DEFINE_COMMAND( DvdRootMenu, "root menu" )


#endif
