/*****************************************************************************
 * cmd_on_top.hpp
 *****************************************************************************
 * Copyright (C) 2003 the VideoLAN team
 * $Id: c6b84f8a5ab85b92c13faa0c56088a41054c7126 $
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

#ifndef CMD_ON_TOP_HPP
#define CMD_ON_TOP_HPP

#include "cmd_generic.hpp"


/// "Always on top" command
DEFINE_COMMAND( OnTop, "always on top" )

class CmdSetOnTop: public CmdGeneric
{
public:
    CmdSetOnTop( intf_thread_t *pIntf, bool b_ontop )
        : CmdGeneric( pIntf ), m_ontop( b_ontop ) { }
    virtual ~CmdSetOnTop() { }
    virtual void execute();
    virtual std::string getType() const { return "set on top"; }

private:
    bool m_ontop;
};


#endif
