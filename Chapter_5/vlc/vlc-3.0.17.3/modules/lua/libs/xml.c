/*****************************************************************************
 * xml.c: XML related functions
 *****************************************************************************
 * Copyright (C) 2010 Antoine Cellerier
 * $Id: fe88fff1b03cd57752d29e61f2e57381d48ea72c $
 *
 * Authors: Antoine Cellerier <dionoea at videolan tod org>
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

/*****************************************************************************
 * Preamble
 *****************************************************************************/
#ifndef  _GNU_SOURCE
#   define  _GNU_SOURCE
#endif

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_xml.h>

#include "../vlc.h"
#include "../libs.h"

/*****************************************************************************
 * XML
 *****************************************************************************/
static int vlclua_xml_create_reader( lua_State * );

static const luaL_Reg vlclua_xml_reg[] = {
    { "create_reader", vlclua_xml_create_reader },
    { NULL, NULL }
};

static int vlclua_xml_create( lua_State *L )
{
    lua_newuserdata( L, 0 );

    if( luaL_newmetatable( L, "xml" ) )
    {
        lua_newtable( L );
        luaL_register( L, NULL, vlclua_xml_reg );
        lua_setfield( L, -2, "__index" );
    }

    lua_setmetatable( L, -2 );
    return 1;
}

/*****************************************************************************
 * XML Reader
 *****************************************************************************/
static int vlclua_xml_reader_next_node( lua_State * );
static int vlclua_xml_reader_next_attr( lua_State * );
static int vlclua_xml_reader_node_empty( lua_State * );

static const luaL_Reg vlclua_xml_reader_reg[] = {
    { "next_node", vlclua_xml_reader_next_node },
    { "next_attr", vlclua_xml_reader_next_attr },
    { "node_empty", vlclua_xml_reader_node_empty },
    { NULL, NULL }
};

static int vlclua_xml_reader_delete( lua_State *L )
{
    xml_reader_t *p_reader = *(xml_reader_t**)luaL_checkudata( L, 1, "xml_reader" );
    xml_ReaderDelete( p_reader );
    return 0;
}

static int vlclua_xml_create_reader( lua_State *L )
{
    vlc_object_t *obj = vlclua_get_this( L );
    stream_t *p_stream = *(stream_t **)luaL_checkudata( L, 2, "stream" );

    xml_reader_t *p_reader = xml_ReaderCreate( obj, p_stream );
    if( !p_reader )
        return luaL_error( L, "XML reader creation failed." );

    xml_reader_t **pp_reader = lua_newuserdata( L, sizeof( xml_reader_t * ) );
    *pp_reader = p_reader;

    if( luaL_newmetatable( L, "xml_reader" ) )
    {
        lua_newtable( L );
        luaL_register( L, NULL, vlclua_xml_reader_reg );
        lua_setfield( L, -2, "__index" );
        lua_pushcfunction( L, vlclua_xml_reader_delete );
        lua_setfield( L, -2, "__gc" );
    }

    lua_setmetatable( L, -2 );
    return 1;
}

static int vlclua_xml_reader_next_node( lua_State *L )
{
    xml_reader_t *p_reader = *(xml_reader_t**)luaL_checkudata( L, 1, "xml_reader" );
    const char *psz_name;
    int i_type = xml_ReaderNextNode( p_reader, &psz_name );
    if( i_type <= 0 )
    {
        lua_pushinteger( L, 0 );
        return 1;
    }

    lua_pushinteger( L, i_type );
    lua_pushstring( L, psz_name );
    return 2;
}

static int vlclua_xml_reader_next_attr( lua_State *L )
{
    xml_reader_t *p_reader = *(xml_reader_t**)luaL_checkudata( L, 1, "xml_reader" );
    const char *psz_value;
    const char *psz_name = xml_ReaderNextAttr( p_reader, &psz_value );
    if( !psz_name )
        return 0;

    lua_pushstring( L, psz_name );
    lua_pushstring( L, psz_value );
    return 2;
}

static int vlclua_xml_reader_node_empty( lua_State* L )
{
    xml_reader_t *p_reader = *(xml_reader_t**)luaL_checkudata( L, 1, "xml_reader" );

    lua_pushinteger( L, xml_ReaderIsEmptyElement( p_reader ) );
    return 1;
}

void luaopen_xml( lua_State *L )
{
    lua_pushcfunction( L, vlclua_xml_create );
    lua_setfield( L, -2, "xml" );
}
