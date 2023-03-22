/*****************************************************************************
 * cmd_snapshot.hpp
 *****************************************************************************
 * Copyright (C) 2003 the VideoLAN team
 * $Id: c8c5cb43d5c1a73d0586b5a145ddd5fd4cfd01be $
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

#ifndef CMD_SNAPSHOT_HPP
#define CMD_SNAPSHOT_HPP

#include "cmd_generic.hpp"


/// Command to snapshot VLC
DEFINE_COMMAND(Snapshot, "snapshot" )
DEFINE_COMMAND(ToggleRecord, "togglerecord" )
DEFINE_COMMAND(NextFrame, "nextframe" )

#endif
