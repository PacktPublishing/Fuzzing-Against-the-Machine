/*****************************************************************************
 * cmd_vars.hpp
 *****************************************************************************
 * Copyright (C) 2004 the VideoLAN team
 * $Id: 73aab301053b616c3225ad0d23a1e51a1a343f87 $
 *
 * Authors: Cyril Deguet     <asmax@via.ecp.fr>
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

#ifndef CMD_VARS_HPP
#define CMD_VARS_HPP

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_playlist.h>
#include <vlc_input_item.h>

#include "cmd_generic.hpp"
#include "../utils/ustring.hpp"

class EqualizerBands;
class EqualizerPreamp;
class VarText;

/// Command to notify the playtree of an item update
class CmdItemUpdate: public CmdGeneric
{
public:
    CmdItemUpdate( intf_thread_t *pIntf, input_item_t* pItem ):
        CmdGeneric( pIntf ), m_pItem( pItem )
    {
        if( pItem )
            input_item_Hold( pItem );
    }
    virtual ~CmdItemUpdate()
    {
        if( m_pItem )
            input_item_Release( m_pItem );
    }
    virtual void execute();
    virtual std::string getType() const { return "playtree update"; }

    /// Only accept removal of command if they concern the same item
    virtual bool checkRemove( CmdGeneric * ) const;

private:
    /// input item changed
    input_item_t* m_pItem;
};

/// Command to notify the playtree of an item append
class CmdPlaytreeAppend: public CmdGeneric
{
public:
    CmdPlaytreeAppend( intf_thread_t *pIntf, int i_id ):
        CmdGeneric( pIntf ), m_id( i_id )
    { }
    virtual ~CmdPlaytreeAppend() { }
    virtual void execute();
    virtual std::string getType() const { return "playtree append"; }

private:
    int m_id;
};

/// Command to notify the playtree of an item deletion
class CmdPlaytreeDelete: public CmdGeneric
{
public:
    CmdPlaytreeDelete( intf_thread_t *pIntf, int i_id ):
        CmdGeneric( pIntf ), m_id( i_id ) { }
    virtual ~CmdPlaytreeDelete() { }
    virtual void execute();
    virtual std::string getType() const { return "playtree append"; }

private:
    int m_id;
};


/// Command to set a text variable
class CmdSetText: public CmdGeneric
{
public:
    CmdSetText( intf_thread_t *pIntf, VarText &rText, const UString &rValue ):
        CmdGeneric( pIntf ), m_rText( rText ), m_value( rValue ) { }
    virtual ~CmdSetText() { }
    virtual void execute();
    virtual std::string getType() const { return "set text"; }

private:
    /// Text variable to set
    VarText &m_rText;
    /// Value to set
    const UString m_value;
};


/// Command to set the equalizer preamp
class CmdSetEqPreamp: public CmdGeneric
{
public:
    CmdSetEqPreamp( intf_thread_t *I, EqualizerPreamp &P, float v )
                  : CmdGeneric( I ), m_rPreamp( P ), m_value( v ) { }
    virtual ~CmdSetEqPreamp() { }
    virtual void execute();
    virtual std::string getType() const { return "set equalizer preamp"; }

private:
    /// Preamp variable to set
    EqualizerPreamp &m_rPreamp;
    /// Value to set
    float m_value;
};


/// Command to set the equalizerbands
class CmdSetEqBands: public CmdGeneric
{
public:
    CmdSetEqBands( intf_thread_t *I, EqualizerBands &B, const std::string &V )
                 : CmdGeneric( I ), m_rEqBands( B ), m_value( V ) { }
    virtual ~CmdSetEqBands() { }
    virtual void execute();
    virtual std::string getType() const { return "set equalizer bands"; }

private:
    /// Equalizer variable to set
    EqualizerBands &m_rEqBands;
    /// Value to set
    const std::string m_value;
};


#endif
