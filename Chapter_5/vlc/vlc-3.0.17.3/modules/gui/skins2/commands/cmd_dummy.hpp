/*****************************************************************************
 * cmd_dummy.hpp
 *****************************************************************************
 * Copyright (C) 2003 the VideoLAN team
 * $Id: d908eb87fa7efb93e51e4bf0b68482735ca0245b $
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

#ifndef CMD_DUMMY_HPP
#define CMD_DUMMY_HPP

#include "cmd_generic.hpp"


/// Dummy command
DEFINE_COMMAND( Dummy, "dummy" )

void CmdDummy::execute() {}

#endif
