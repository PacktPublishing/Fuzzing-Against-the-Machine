/*****************************************************************************
 * ctrl_flat.hpp
 *****************************************************************************
 * Copyright (C) 2003 the VideoLAN team
 * $Id: f21482d25f66f3825ce0b837deae6226ee687abc $
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

#ifndef CTRL_FLAT_HPP
#define CTRL_FLAT_HPP

#include "ctrl_generic.hpp"


/// Base class for "mover controls" and images
class CtrlFlat: public CtrlGeneric
{
protected:
    CtrlFlat( intf_thread_t *pIntf, const UString &rHelp, VarBool *pVisible )
            : CtrlGeneric( pIntf, rHelp, pVisible ) { }

    virtual ~CtrlFlat() { }
};

#endif
