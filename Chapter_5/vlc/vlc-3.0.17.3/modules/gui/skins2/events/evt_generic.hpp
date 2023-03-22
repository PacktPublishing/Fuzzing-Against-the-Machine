/*****************************************************************************
 * evt_generic.hpp
 *****************************************************************************
 * Copyright (C) 2003 the VideoLAN team
 * $Id: beefb8ba0a452fdf6b74b1801ac8acee703808bd $
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifndef EVT_GENERIC_HPP
#define EVT_GENERIC_HPP

#include "../src/skin_common.hpp"
#include <string>


/// Base class for OS events
class EvtGeneric: public SkinObject
{
public:
    virtual ~EvtGeneric() { }

    /// Return the type of the event
    virtual const std::string getAsString() const = 0;

protected:
    EvtGeneric( intf_thread_t *pIntf ): SkinObject( pIntf ) { }
};


#endif
