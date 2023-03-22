/*****************************************************************************
 * stream_filter.c
 *****************************************************************************
 * Copyright (C) 2008 Laurent Aimar
 * $Id: eeae19faee91f1eed50b3a47ffe2f705d917627e $
 *
 * Author: Laurent Aimar <fenrir _AT_ videolan _DOT_ org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_stream.h>
#include <vlc_modules.h>
#include <libvlc.h>

#include <assert.h>

#include "stream.h"

static void StreamDelete( stream_t * );

stream_t *vlc_stream_FilterNew( stream_t *p_source,
                                const char *psz_stream_filter )
{
    stream_t *s;
    assert( p_source != NULL );

    s = vlc_stream_CommonNew( p_source->obj.parent, StreamDelete );
    if( s == NULL )
        return NULL;

    s->p_input = p_source->p_input;

    if( p_source->psz_url != NULL )
    {
        s->psz_url = strdup( p_source->psz_url );
        if( unlikely(s->psz_url == NULL) )
            goto error;
    }
    s->p_source = p_source;

    /* */
    s->p_module = module_need( s, "stream_filter", psz_stream_filter, true );
    if( s->p_module == NULL )
        goto error;

    return s;
error:
    stream_CommonDelete( s );
    return NULL;
}

/* Add automatic stream filter */
stream_t *stream_FilterAutoNew( stream_t *p_source )
{
    /* Limit number of entries to avoid infinite recursion. */
    for( unsigned i = 0; i < 16; i++ )
    {
        stream_t *p_filter = vlc_stream_FilterNew( p_source, NULL );
        if( p_filter == NULL )
            break;

        msg_Dbg( p_filter, "stream filter added to %p", (void *)p_source );
        p_source = p_filter;
    }
    return p_source;
}

/* Add specified stream filter(s) */
stream_t *stream_FilterChainNew( stream_t *p_source, const char *psz_chain )
{
    /* Add user stream filter */
    char *chain = strdup( psz_chain );
    if( unlikely(chain == NULL) )
        return p_source;

    char *buf;
    for( const char *name = strtok_r( chain, ":", &buf );
         name != NULL;
         name = strtok_r( NULL, ":", &buf ) )
    {
        stream_t *p_filter = vlc_stream_FilterNew( p_source, name );
        if( p_filter != NULL )
            p_source = p_filter;
        else
            msg_Warn( p_source, "cannot insert stream filter %s", name );
    }
    free( chain );

    return p_source;
}

static void StreamDelete( stream_t *s )
{
    module_unneed( s, s->p_module );

    if( s->p_source )
        vlc_stream_Delete( s->p_source );
}

int vlc_stream_FilterDefaultReadDir( stream_t *s, input_item_node_t *p_node )
{
    assert( s->p_source != NULL );
    return vlc_stream_ReadDir( s->p_source, p_node );
}
