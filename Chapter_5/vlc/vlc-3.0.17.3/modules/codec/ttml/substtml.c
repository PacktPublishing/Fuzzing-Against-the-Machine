/*****************************************************************************
 * substtml.c : TTML subtitles decoder
 *****************************************************************************
 * Copyright (C) 2015-2017 VLC authors and VideoLAN
 *
 * Authors: Hugo Beauzée-Luyssen <hugo@beauzee.fr>
 *          Sushma Reddy <sushma.reddy@research.iiit.ac.in>
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
#include <vlc_codec.h>
#include <vlc_xml.h>
#include <vlc_stream.h>
#include <vlc_text_style.h>
#include <vlc_charset.h>

#include <ctype.h>
#include <assert.h>

#include "substext.h"
#include "ttml.h"

//#define TTML_DEBUG

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/
typedef struct
{
    float       i_value;
    enum
    {
        TTML_UNIT_UNKNOWN = 0,
        TTML_UNIT_PERCENT,
        TTML_UNIT_CELL,
        TTML_UNIT_PIXELS,
    } unit;
} ttml_length_t;

#define TTML_DEFAULT_CELL_RESOLUTION_H 32
#define TTML_DEFAULT_CELL_RESOLUTION_V 15
#define TTML_LINE_TO_HEIGHT_RATIO      1.06

typedef struct
{
    text_style_t*   font_style;
    ttml_length_t   font_size;
    /* sizes override */
    ttml_length_t   extent_h, extent_v;
    ttml_length_t   origin_h, origin_v;
    int             i_text_align;
    bool            b_text_align_set;
    int             i_direction;
    bool            b_direction_set;
    bool            b_preserve_space;
    enum
    {
        TTML_DISPLAY_UNKNOWN = 0,
        TTML_DISPLAY_AUTO,
        TTML_DISPLAY_NONE,
    } display;
}  ttml_style_t;

typedef struct
{
    vlc_dictionary_t regions;
    tt_node_t *      p_rootnode; /* for now. FIXME: split header */
    ttml_length_t    root_extent_h, root_extent_v;
    unsigned         i_cell_resolution_v;
    unsigned         i_cell_resolution_h;
} ttml_context_t;

typedef struct
{
    subpicture_updater_sys_region_t updt;
    text_segment_t **pp_last_segment;
} ttml_region_t;

struct decoder_sys_t
{
    int                     i_align;
};

enum
{
    UNICODE_BIDI_LTR = 0,
    UNICODE_BIDI_RTL = 1,
    UNICODE_BIDI_EMBEDDED = 2,
    UNICODE_BIDI_OVERRIDE = 4,
};

/*
 * TTML Parsing and inheritance order:
 * Each time a text node is found and belongs to out time interval,
 * we backward merge attributes dictionnary up to root.
 * Then we convert attributes, merging with style by id or region
 * style, and sets from parent node.
 */
static tt_node_t *ParseTTML( decoder_t *, const uint8_t *, size_t );

static void ttml_style_Delete( ttml_style_t* p_ttml_style )
{
    text_style_Delete( p_ttml_style->font_style );
    free( p_ttml_style );
}

static ttml_style_t * ttml_style_New( )
{
    ttml_style_t *p_ttml_style = calloc( 1, sizeof( ttml_style_t ) );
    if( unlikely( !p_ttml_style ) )
        return NULL;

    p_ttml_style->extent_h.unit = TTML_UNIT_UNKNOWN;
    p_ttml_style->extent_v.unit = TTML_UNIT_UNKNOWN;
    p_ttml_style->origin_h.unit = TTML_UNIT_UNKNOWN;
    p_ttml_style->origin_v.unit = TTML_UNIT_UNKNOWN;
    p_ttml_style->font_size.i_value = 1.0;
    p_ttml_style->font_size.unit = TTML_UNIT_CELL;
    p_ttml_style->font_style = text_style_Create( STYLE_NO_DEFAULTS );
    if( unlikely( !p_ttml_style->font_style ) )
    {
        free( p_ttml_style );
        return NULL;
    }
    return p_ttml_style;
}

static void ttml_region_Delete( ttml_region_t *p_region )
{
    SubpictureUpdaterSysRegionClean( &p_region->updt );
    free( p_region );
}

static ttml_style_t * ttml_style_Duplicate( const ttml_style_t *p_src )
{
    ttml_style_t *p_dup = ttml_style_New( );
    if( p_dup )
    {
        *p_dup = *p_src;
        p_dup->font_style = text_style_Duplicate( p_src->font_style );
    }
    return p_dup;
}

static void ttml_style_Merge( const ttml_style_t *p_src, ttml_style_t *p_dst )
{
    if( p_src && p_dst )
    {
        if( p_src->font_style )
        {
            if( p_dst->font_style )
                text_style_Merge( p_dst->font_style, p_src->font_style, true );
            else
                p_dst->font_style = text_style_Duplicate( p_src->font_style );
        }

        if( p_src->b_direction_set )
        {
            p_dst->b_direction_set = true;
            p_dst->i_direction = p_src->i_direction;
        }

        if( p_src->display != TTML_DISPLAY_UNKNOWN )
            p_dst->display = p_src->display;
    }
}

static ttml_region_t *ttml_region_New( bool b_root )
{
    ttml_region_t *p_ttml_region = calloc( 1, sizeof( ttml_region_t ) );
    if( unlikely( !p_ttml_region ) )
        return NULL;

    SubpictureUpdaterSysRegionInit( &p_ttml_region->updt );
    p_ttml_region->pp_last_segment = &p_ttml_region->updt.p_segments;
    /* Align to top by default. !Warn: center align is obtained with NO flags */
    p_ttml_region->updt.align = SUBPICTURE_ALIGN_TOP|SUBPICTURE_ALIGN_LEFT;
    if( b_root )
    {
        p_ttml_region->updt.inner_align = SUBPICTURE_ALIGN_BOTTOM;
        p_ttml_region->updt.extent.x = 1.0;
        p_ttml_region->updt.extent.y = 1.0;
        p_ttml_region->updt.flags = UPDT_REGION_EXTENT_X_IS_RATIO|UPDT_REGION_EXTENT_Y_IS_RATIO;
    }
    else
    {
        p_ttml_region->updt.inner_align = SUBPICTURE_ALIGN_TOP|SUBPICTURE_ALIGN_LEFT;
    }

    return p_ttml_region;
}

static ttml_length_t ttml_read_length( const char *psz )
{
    ttml_length_t len = { 0.0, TTML_UNIT_UNKNOWN };

    char* psz_end = NULL;
    float size = us_strtof( psz, &psz_end );
    len.i_value = size;
    if( psz_end )
    {
        if( *psz_end == 'c' || *psz_end == 'r' )
            len.unit = TTML_UNIT_CELL;
        else if( *psz_end == '%' )
            len.unit = TTML_UNIT_PERCENT;
        else if( *psz_end == 'p' && *(psz_end + 1) == 'x' )
            len.unit = TTML_UNIT_PIXELS;
    }
    return len;
}

static ttml_length_t ttml_rebase_length( unsigned i_cell_resolution,
                                         ttml_length_t value,
                                         ttml_length_t reference )
{
    if( value.unit == TTML_UNIT_PERCENT )
    {
        value.i_value *= reference.i_value / 100.0;
        value.unit = reference.unit;
    }
    else if( value.unit == TTML_UNIT_CELL )
    {
        value.i_value *= reference.i_value / i_cell_resolution;
        value.unit = reference.unit;
    }
    // pixels as-is
    return value;
}

static bool ttml_read_coords( const char *value, ttml_length_t *h, ttml_length_t *v )
{
    ttml_length_t vals[2] = { { 0.0, TTML_UNIT_UNKNOWN },
                              { 0.0, TTML_UNIT_UNKNOWN } };
    char *dup = strdup( value );
    char* psz_saveptr = NULL;
    char* token = (dup) ? strtok_r( dup, " ", &psz_saveptr ) : NULL;
    for(int i=0; i<2 && token != NULL; i++)
    {
        vals[i] = ttml_read_length( token );
        token = strtok_r( NULL, " ", &psz_saveptr );
    }
    free( dup );

    if( vals[0].unit != TTML_UNIT_UNKNOWN &&
        vals[1].unit != TTML_UNIT_UNKNOWN )
    {
        *h = vals[0];
        *v = vals[1];
        return true;
    }
    return false;
}

static tt_node_t * FindNode( tt_node_t *p_node, const char *psz_nodename,
                             size_t i_maxdepth, const char *psz_id )
{
    if( !tt_node_NameCompare( p_node->psz_node_name, psz_nodename ) )
    {
        if( psz_id != NULL )
        {
            char *psz = vlc_dictionary_value_for_key( &p_node->attr_dict, "xml:id" );
            if( !psz ) /* People can't do xml properly */
                psz = vlc_dictionary_value_for_key( &p_node->attr_dict, "id" );
            if( psz && !strcmp( psz, psz_id ) )
                return p_node;
        }
        else return p_node;
    }

    if( i_maxdepth == 0 )
        return NULL;

    for( tt_basenode_t *p_child = p_node->p_child;
                        p_child; p_child = p_child->p_next )
    {
        if( p_child->i_type == TT_NODE_TYPE_TEXT )
            continue;

        p_node = FindNode( (tt_node_t *) p_child, psz_nodename, i_maxdepth - 1, psz_id );
        if( p_node )
            return p_node;
    }

    return NULL;
}

static void FillTextStyle( const char *psz_attr, const char *psz_val,
                           text_style_t *p_text_style )
{
    if( !strcasecmp ( "tts:fontFamily", psz_attr ) )
    {
        free( p_text_style->psz_fontname );
        p_text_style->psz_fontname = strdup( psz_val );
    }
    else if( !strcasecmp( "tts:opacity", psz_attr ) )
    {
        p_text_style->i_background_alpha = atoi( psz_val );
        p_text_style->i_font_alpha = atoi( psz_val );
        p_text_style->i_features |= STYLE_HAS_BACKGROUND_ALPHA | STYLE_HAS_FONT_ALPHA;
    }
    else if( !strcasecmp( "tts:color", psz_attr ) )
    {
        unsigned int i_color = vlc_html_color( psz_val, NULL );
        p_text_style->i_font_color = (i_color & 0xffffff);
        p_text_style->i_font_alpha = (i_color & 0xFF000000) >> 24;
        p_text_style->i_features |= STYLE_HAS_FONT_COLOR | STYLE_HAS_FONT_ALPHA;
    }
    else if( !strcasecmp( "tts:backgroundColor", psz_attr ) )
    {
        unsigned int i_color = vlc_html_color( psz_val, NULL );
        p_text_style->i_background_color = i_color & 0xFFFFFF;
        p_text_style->i_background_alpha = (i_color & 0xFF000000) >> 24;
        p_text_style->i_features |= STYLE_HAS_BACKGROUND_COLOR
                                                  | STYLE_HAS_BACKGROUND_ALPHA;
        p_text_style->i_style_flags |= STYLE_BACKGROUND;
    }
    else if( !strcasecmp( "tts:fontStyle", psz_attr ) )
    {
        if( !strcasecmp ( "italic", psz_val ) || !strcasecmp ( "oblique", psz_val ) )
            p_text_style->i_style_flags |= STYLE_ITALIC;
        else
            p_text_style->i_style_flags &= ~STYLE_ITALIC;
        p_text_style->i_features |= STYLE_HAS_FLAGS;
    }
    else if( !strcasecmp ( "tts:fontWeight", psz_attr ) )
    {
        if( !strcasecmp ( "bold", psz_val ) )
            p_text_style->i_style_flags |= STYLE_BOLD;
        else
            p_text_style->i_style_flags &= ~STYLE_BOLD;
        p_text_style->i_features |= STYLE_HAS_FLAGS;
    }
    else if( !strcasecmp ( "tts:textDecoration", psz_attr ) )
    {
        if( !strcasecmp ( "underline", psz_val ) )
            p_text_style->i_style_flags |= STYLE_UNDERLINE;
        else if( !strcasecmp ( "noUnderline", psz_val ) )
            p_text_style->i_style_flags &= ~STYLE_UNDERLINE;
        if( !strcasecmp ( "lineThrough", psz_val ) )
            p_text_style->i_style_flags |= STYLE_STRIKEOUT;
        else if( !strcasecmp ( "noLineThrough", psz_val ) )
            p_text_style->i_style_flags &= ~STYLE_STRIKEOUT;
        p_text_style->i_features |= STYLE_HAS_FLAGS;
    }
    else if( !strcasecmp( "tts:textOutline", psz_attr ) )
    {
        char *value = strdup( psz_val );
        char* psz_saveptr = NULL;
        char* token = (value) ? strtok_r( value, " ", &psz_saveptr ) : NULL;
        // <color>? <length> <length>?
        if( token != NULL )
        {
            bool b_ok = false;
            unsigned int color = vlc_html_color( token, &b_ok );
            if( b_ok )
            {
                p_text_style->i_outline_color = color & 0xFFFFFF;
                p_text_style->i_outline_alpha = (color & 0xFF000000) >> 24;
                token = strtok_r( NULL, " ", &psz_saveptr );
                if( token != NULL )
                {
                    char* psz_end = NULL;
                    int i_outline_width = strtol( token, &psz_end, 10 );
                    if( psz_end != token )
                    {
                        // Assume unit is pixel, and ignore border radius
                        p_text_style->i_outline_width = i_outline_width;
                    }
                }
            }
        }
        free( value );
    }
}

static void FillCoord( ttml_length_t v, int i_flag, float *p_val, int *pi_flags )
{
    *p_val = v.i_value;
    if( v.unit == TTML_UNIT_PERCENT )
    {
        *p_val /= 100.0;
        *pi_flags |= i_flag;
    }
    else *pi_flags &= ~i_flag;
}

static void FillUpdaterCoords( ttml_context_t *p_ctx, ttml_length_t h, ttml_length_t v,
                               bool b_origin, subpicture_updater_sys_region_t *p_updt )
{
    ttml_length_t base = { 100.0, TTML_UNIT_PERCENT };
    ttml_length_t x = ttml_rebase_length( p_ctx->i_cell_resolution_h, h, base );
    ttml_length_t y = ttml_rebase_length( p_ctx->i_cell_resolution_v, v, base );
    if( b_origin )
    {
        FillCoord( x, UPDT_REGION_ORIGIN_X_IS_RATIO, &p_updt->origin.x, &p_updt->flags );
        FillCoord( y, UPDT_REGION_ORIGIN_Y_IS_RATIO, &p_updt->origin.y, &p_updt->flags );
        p_updt->align = SUBPICTURE_ALIGN_TOP|SUBPICTURE_ALIGN_LEFT;
    }
    else
    {
        FillCoord( x, UPDT_REGION_EXTENT_X_IS_RATIO, &p_updt->extent.x, &p_updt->flags );
        FillCoord( y, UPDT_REGION_EXTENT_Y_IS_RATIO, &p_updt->extent.y, &p_updt->flags );
    }
}

static void FillRegionStyle( ttml_context_t *p_ctx,
                             const char *psz_attr, const char *psz_val,
                             ttml_region_t *p_region )
{
    if( !strcasecmp( "tts:displayAlign", psz_attr ) )
    {
        p_region->updt.inner_align &= ~(SUBPICTURE_ALIGN_TOP|SUBPICTURE_ALIGN_BOTTOM);
        if( !strcasecmp( "after", psz_val ) )
            p_region->updt.inner_align |= SUBPICTURE_ALIGN_BOTTOM;
        else if( strcasecmp( "center", psz_val ) )
            /* "before" */
            p_region->updt.inner_align |= SUBPICTURE_ALIGN_TOP;
    }
    else if( !strcasecmp ( "tts:origin", psz_attr ) ||
             !strcasecmp ( "tts:extent", psz_attr ) )
    {
        ttml_length_t x, y;
        if( ttml_read_coords( psz_val, &x, &y ) )
            FillUpdaterCoords( p_ctx, x, y, (psz_attr[4] == 'o'), &p_region->updt );
    }
}

static void ComputeTTMLStyles( ttml_context_t *p_ctx, const vlc_dictionary_t *p_dict,
                               ttml_style_t *p_ttml_style )
{
    VLC_UNUSED(p_dict);
    /* Values depending on multiple others are converted last
     * Default value conversion must also not depend on attribute presence */
    text_style_t *p_text_style = p_ttml_style->font_style;
    ttml_length_t len = p_ttml_style->font_size;

    /* font size is pixels, cells or, % of cell */
    if( len.unit == TTML_UNIT_PERCENT )
    {
        len.i_value /= 100.0;
        len.unit = TTML_UNIT_CELL;
    }

    /* font size is now pixels or cells */
    /* if cell (and indirectly cell %), rebase as line height depending on resolution */
    if( len.unit == TTML_UNIT_CELL )
        len = ttml_rebase_length( p_ctx->i_cell_resolution_v, len, p_ctx->root_extent_v );

    /* font size is root_extent height % or pixels */
    if( len.unit == TTML_UNIT_PERCENT )
        p_text_style->f_font_relsize = len.i_value / TTML_LINE_TO_HEIGHT_RATIO;
    else
    if( len.unit == TTML_UNIT_PIXELS )
        p_text_style->i_font_size = len.i_value;
}

static void FillTTMLStyle( const char *psz_attr, const char *psz_val,
                           ttml_style_t *p_ttml_style )
{
    if( !strcasecmp( "tts:extent", psz_attr ) )
    {
        ttml_read_coords( psz_val, &p_ttml_style->extent_h,
                                   &p_ttml_style->extent_v );
    }
    else if( !strcasecmp( "tts:origin", psz_attr ) )
    {
        ttml_read_coords( psz_val, &p_ttml_style->origin_h,
                                   &p_ttml_style->origin_v );
    }
    else if( !strcasecmp( "tts:textAlign", psz_attr ) )
    {
        p_ttml_style->i_text_align &= ~(SUBPICTURE_ALIGN_LEFT|SUBPICTURE_ALIGN_RIGHT);
        if( !strcasecmp ( "left", psz_val ) )
            p_ttml_style->i_text_align |= SUBPICTURE_ALIGN_LEFT;
        else if( !strcasecmp ( "right", psz_val ) )
            p_ttml_style->i_text_align |= SUBPICTURE_ALIGN_RIGHT;
        else if( !strcasecmp ( "end", psz_val ) )  /* FIXME: should be BIDI based */
            p_ttml_style->i_text_align |= SUBPICTURE_ALIGN_RIGHT;
        else if( strcasecmp ( "center", psz_val ) )
            /* == "start" FIXME: should be BIDI based */
            p_ttml_style->i_text_align |= SUBPICTURE_ALIGN_LEFT;
        p_ttml_style->b_text_align_set = true;
#ifdef TTML_DEBUG
        printf("**%s %x\n", psz_val, p_ttml_style->i_text_align);
#endif
    }
    else if( !strcasecmp( "tts:fontSize", psz_attr ) )
    {
        ttml_length_t len = ttml_read_length( psz_val );
        if( len.unit != TTML_UNIT_UNKNOWN && len.i_value > 0.0 )
            p_ttml_style->font_size = len;
    }
    else if( !strcasecmp( "tts:direction", psz_attr ) )
    {
        if( !strcasecmp( "rtl", psz_val ) )
        {
            p_ttml_style->i_direction |= UNICODE_BIDI_RTL;
            p_ttml_style->b_direction_set = true;
        }
        else if( !strcasecmp( "ltr", psz_val ) )
        {
            p_ttml_style->i_direction |= UNICODE_BIDI_LTR;
            p_ttml_style->b_direction_set = true;
        }
    }
    else if( !strcasecmp( "tts:unicodeBidi", psz_attr ) )
    {
            if( !strcasecmp( "bidiOverride", psz_val ) )
                p_ttml_style->i_direction |= UNICODE_BIDI_OVERRIDE & ~UNICODE_BIDI_EMBEDDED;
            else if( !strcasecmp( "embed", psz_val ) )
                p_ttml_style->i_direction |= UNICODE_BIDI_EMBEDDED & ~UNICODE_BIDI_OVERRIDE;
    }
    else if( !strcasecmp( "tts:writingMode", psz_attr ) )
    {
        if( !strcasecmp( "rl", psz_val ) || !strcasecmp( "rltb", psz_val ) )
        {
            p_ttml_style->i_direction = UNICODE_BIDI_RTL | UNICODE_BIDI_OVERRIDE;
            //p_ttml_style->i_align = SUBPICTURE_ALIGN_BOTTOM | SUBPICTURE_ALIGN_RIGHT;
            p_ttml_style->b_direction_set = true;
        }
        else if( !strcasecmp( "lr", psz_val ) || !strcasecmp( "lrtb", psz_val ) )
        {
            p_ttml_style->i_direction = UNICODE_BIDI_LTR | UNICODE_BIDI_OVERRIDE;
            //p_ttml_style->i_align = SUBPICTURE_ALIGN_BOTTOM | SUBPICTURE_ALIGN_LEFT;
            p_ttml_style->b_direction_set = true;
        }
    }
    else if( !strcmp( "tts:display", psz_attr ) )
    {
        if( !strcmp( "none", psz_val ) )
            p_ttml_style->display = TTML_DISPLAY_NONE;
        else
            p_ttml_style->display = TTML_DISPLAY_AUTO;
    }
    else if( !strcasecmp( "xml:space", psz_attr ) )
    {
        p_ttml_style->b_preserve_space = !strcmp( "preserve", psz_val );
    }
    else FillTextStyle( psz_attr, psz_val, p_ttml_style->font_style );
}

static void DictionaryMerge( const vlc_dictionary_t *p_src, vlc_dictionary_t *p_dst )
{
    for( int i = 0; i < p_src->i_size; ++i )
    {
        for ( const vlc_dictionary_entry_t* p_entry = p_src->p_entries[i];
                                            p_entry != NULL; p_entry = p_entry->p_next )
        {
            if( ( !strncmp( "tts:", p_entry->psz_key, 4 ) ||
                  !strncmp( "ttp:", p_entry->psz_key, 4 ) ||
                  !strcmp( "xml:space", p_entry->psz_key ) ) &&
                !vlc_dictionary_has_key( p_dst, p_entry->psz_key ) )
                vlc_dictionary_insert( p_dst, p_entry->psz_key, p_entry->p_value );
        }
    }
}

static void DictMergeWithStyleID( ttml_context_t *p_ctx, const char *psz_id,
                                  vlc_dictionary_t *p_dst )
{
    assert(p_ctx->p_rootnode);
    if( psz_id && p_ctx->p_rootnode )
    {
        /* Lookup referenced style ID */
        const tt_node_t *p_node = FindNode( p_ctx->p_rootnode,
                                            "style", -1, psz_id );
        if( p_node )
            DictionaryMerge( &p_node->attr_dict, p_dst );
    }
}

static void DictMergeWithRegionID( ttml_context_t *p_ctx, const char *psz_id,
                                   vlc_dictionary_t *p_dst )
{
    assert(p_ctx->p_rootnode);
    if( psz_id && p_ctx->p_rootnode )
    {
        const tt_node_t *p_regionnode = FindNode( p_ctx->p_rootnode,
                                                 "region", -1, psz_id );
        if( !p_regionnode )
            return;

        DictionaryMerge( &p_regionnode->attr_dict, p_dst );

        const char *psz_styleid = (const char *)
                vlc_dictionary_value_for_key( &p_regionnode->attr_dict, "style" );
        if( psz_styleid )
            DictMergeWithStyleID( p_ctx, psz_styleid, p_dst );

        for( const tt_basenode_t *p_child = p_regionnode->p_child;
                                  p_child; p_child = p_child->p_next )
        {
            if( unlikely( p_child->i_type == TT_NODE_TYPE_TEXT ) )
                continue;

            const tt_node_t *p_node = (const tt_node_t *) p_child;
            if( !tt_node_NameCompare( p_node->psz_node_name, "style" ) )
            {
                DictionaryMerge( &p_node->attr_dict, p_dst );
            }
        }
    }
}

static void DictToTTMLStyle( ttml_context_t *p_ctx, const vlc_dictionary_t *p_dict,
                             ttml_style_t *p_ttml_style )
{
    for( int i = 0; i < p_dict->i_size; ++i )
    {
        for ( vlc_dictionary_entry_t* p_entry = p_dict->p_entries[i];
              p_entry != NULL; p_entry = p_entry->p_next )
        {
            FillTTMLStyle( p_entry->psz_key, p_entry->p_value, p_ttml_style );
        }
    }
    ComputeTTMLStyles( p_ctx, p_dict, p_ttml_style );
}

static ttml_style_t * InheritTTMLStyles( ttml_context_t *p_ctx, tt_node_t *p_node )
{
    assert( p_node );
    ttml_style_t *p_ttml_style = NULL;
    vlc_dictionary_t merged;
    vlc_dictionary_init( &merged, 0 );

    /* Merge dics backwards without overwriting */
    for( ; p_node; p_node = p_node->p_parent )
    {
        DictionaryMerge( &p_node->attr_dict, &merged );

        const char *psz_styleid = (const char *)
                vlc_dictionary_value_for_key( &p_node->attr_dict, "style" );
        if( psz_styleid )
            DictMergeWithStyleID( p_ctx, psz_styleid, &merged );

        const char *psz_regionid = (const char *)
                vlc_dictionary_value_for_key( &p_node->attr_dict, "region" );
        if( psz_regionid )
            DictMergeWithRegionID( p_ctx, psz_regionid, &merged );
    }

    if( !vlc_dictionary_is_empty( &merged ) && (p_ttml_style = ttml_style_New()) )
    {
        DictToTTMLStyle( p_ctx, &merged, p_ttml_style );
    }

    vlc_dictionary_clear( &merged, NULL, NULL );

    return p_ttml_style;
}

static int ParseTTMLChunk( xml_reader_t *p_reader, tt_node_t **pp_rootnode )
{
    const char* psz_node_name;

    do
    {
        int i_type = xml_ReaderNextNode( p_reader, &psz_node_name );

        if( i_type <= XML_READER_NONE )
            break;

        switch(i_type)
        {
            default:
                break;

            case XML_READER_STARTELEM:
                if( tt_node_NameCompare( psz_node_name, "tt" ) ||
                    *pp_rootnode != NULL )
                    return VLC_EGENERIC;

                *pp_rootnode = tt_node_New( p_reader, NULL, psz_node_name );
                if( !*pp_rootnode ||
                    tt_nodes_Read( p_reader, *pp_rootnode ) != VLC_SUCCESS )
                    return VLC_EGENERIC;
                break;

            case XML_READER_ENDELEM:
                if( !*pp_rootnode ||
                    tt_node_NameCompare( psz_node_name, (*pp_rootnode)->psz_node_name ) )
                    return VLC_EGENERIC;
                break;
        }

    } while( 1 );

    if( *pp_rootnode == NULL )
        return VLC_EGENERIC;

    return VLC_SUCCESS;
}

static void BIDIConvert( text_segment_t *p_segment, int i_direction )
{
    /*
    * For bidirectionnal support, we use different enum
    * to recognize different cases, en then we add the
    * corresponding unicode character to the text of
    * the text_segment.
    */
    static const struct
    {
        const char* psz_uni_start;
        const char* psz_uni_end;
    } p_bidi[] = {
        { "\u2066", "\u2069" },
        { "\u2067", "\u2069" },
        { "\u202A", "\u202C" },
        { "\u202B", "\u202C" },
        { "\u202D", "\u202C" },
        { "\u202E", "\u202C" },
    };

    if( unlikely((size_t)i_direction >= ARRAY_SIZE(p_bidi)) )
        return;

    char *psz_text = NULL;
    if( asprintf( &psz_text, "%s%s%s", p_bidi[i_direction].psz_uni_start,
                  p_segment->psz_text, p_bidi[i_direction].psz_uni_end ) < 0 )
    {
        free( p_segment->psz_text );
        p_segment->psz_text = psz_text;
    }
}

static void StripSpacing( text_segment_t *p_segment )
{
    /* Newlines must be replaced */
    char *p = p_segment->psz_text;
    while( (p = strchr( p, '\n' )) )
        *p = ' ';
}

static ttml_region_t *GetTTMLRegion( ttml_context_t *p_ctx, const char *psz_region_id )
{
    ttml_region_t *p_region = ( ttml_region_t * )
            vlc_dictionary_value_for_key( &p_ctx->regions, psz_region_id ? psz_region_id : "" );
    if( p_region == NULL )
    {
        if( psz_region_id && strcmp( psz_region_id, "" ) ) /* not default region */
        {
            /* Create region if if missing */

            vlc_dictionary_t merged;
            vlc_dictionary_init( &merged, 0 );
            /* Get all attributes, including region > style */
            DictMergeWithRegionID( p_ctx, psz_region_id, &merged );
            if( (p_region = ttml_region_New( false )) )
            {
                /* Fill from its own attributes */
                for( int i = 0; i < merged.i_size; ++i )
                {
                    for ( vlc_dictionary_entry_t* p_entry = merged.p_entries[i];
                          p_entry != NULL; p_entry = p_entry->p_next )
                    {
                        FillRegionStyle( p_ctx, p_entry->psz_key, p_entry->p_value,
                                         p_region );
                    }
                }
            }
            vlc_dictionary_clear( &merged, NULL, NULL );

            vlc_dictionary_insert( &p_ctx->regions, psz_region_id, p_region );
        }
        else if( (p_region = ttml_region_New( true )) ) /* create default */
        {
            vlc_dictionary_insert( &p_ctx->regions, "", p_region );
        }
    }
    return p_region;
}

static void AppendLineBreakToRegion( ttml_region_t *p_region )
{
    text_segment_t *p_segment = text_segment_New( "\n" );
    if( p_segment )
    {
        *p_region->pp_last_segment = p_segment;
        p_region->pp_last_segment = &p_segment->p_next;
    }
}

static void AppendTextToRegion( ttml_context_t *p_ctx, const tt_textnode_t *p_ttnode,
                                const ttml_style_t *p_set_styles, ttml_region_t *p_region )
{
    text_segment_t *p_segment;

    if( p_region == NULL )
        return;

    p_segment = text_segment_New( p_ttnode->psz_text );
    if( p_segment )
    {
        bool b_preserve_space = false;
        ttml_style_t *s = InheritTTMLStyles( p_ctx, p_ttnode->p_parent );
        if( s )
        {
            if( p_set_styles )
                ttml_style_Merge( p_set_styles, s );

            p_segment->style = s->font_style;
            s->font_style = NULL;

            b_preserve_space = s->b_preserve_space;
            if( s->b_direction_set )
                BIDIConvert( p_segment, s->i_direction );

            if( s->display == TTML_DISPLAY_NONE )
            {
                /* Must not display, but still occupies space */
                p_segment->style->i_features &= ~(STYLE_BACKGROUND|STYLE_OUTLINE|STYLE_STRIKEOUT|STYLE_SHADOW);
                p_segment->style->i_font_alpha = STYLE_ALPHA_TRANSPARENT;
                p_segment->style->i_features |= STYLE_HAS_FONT_ALPHA;
            }

            /* we don't have paragraph, so no per text line alignment.
             * Text style brings horizontal textAlign to region.
             * Region itself is styled with vertical displayAlign */
            if( s->b_text_align_set )
            {
                p_region->updt.inner_align &= ~(SUBPICTURE_ALIGN_LEFT|SUBPICTURE_ALIGN_RIGHT);
                p_region->updt.inner_align |= s->i_text_align;
            }

            if( s->extent_h.unit != TTML_UNIT_UNKNOWN )
                FillUpdaterCoords( p_ctx, s->extent_h, s->extent_v, false, &p_region->updt );

            if( s->origin_h.unit != TTML_UNIT_UNKNOWN )
                FillUpdaterCoords( p_ctx, s->origin_h, s->origin_v, true, &p_region->updt );

            ttml_style_Delete( s );
        }

        if( !b_preserve_space )
            StripSpacing( p_segment );
    }

    *p_region->pp_last_segment = p_segment;
    p_region->pp_last_segment = &p_segment->p_next;
}

static void ConvertNodesToRegionContent( ttml_context_t *p_ctx, const tt_node_t *p_node,
                                         ttml_region_t *p_region,
                                         const ttml_style_t *p_upper_set_styles,
                                         tt_time_t playbacktime )
{
    if( tt_time_Valid( &playbacktime ) &&
       !tt_timings_Contains( &p_node->timings, &playbacktime ) )
        return;

    const char *psz_regionid = (const char *)
        vlc_dictionary_value_for_key( &p_node->attr_dict, "region" );

    /* Region isn't set or is changing */
    if( psz_regionid || p_region == NULL )
        p_region = GetTTMLRegion( p_ctx, psz_regionid );

    /* awkward paragraph handling */
    if( !tt_node_NameCompare( p_node->psz_node_name, "p" ) &&
        p_region->updt.p_segments )
    {
        AppendLineBreakToRegion( p_region );
    }

    /* Styles from <set> element */
    ttml_style_t *p_set_styles = (p_upper_set_styles)
                               ? ttml_style_Duplicate( p_upper_set_styles )
                               : NULL;

    for( const tt_basenode_t *p_child = p_node->p_child;
                              p_child; p_child = p_child->p_next )
    {
        if( p_child->i_type == TT_NODE_TYPE_TEXT )
        {
            AppendTextToRegion( p_ctx, (const tt_textnode_t *) p_child,
                                p_set_styles, p_region );
        }
        else if( !tt_node_NameCompare( ((const tt_node_t *)p_child)->psz_node_name, "set" ) )
        {
            const tt_node_t *p_set = (const tt_node_t *)p_child;
            if( !tt_time_Valid( &playbacktime ) ||
                tt_timings_Contains( &p_set->timings, &playbacktime ) )
            {
                if( p_set_styles != NULL || (p_set_styles = ttml_style_New()) )
                {
                    /* Merge with or create a local set of styles to apply to following childs */
                    DictToTTMLStyle( p_ctx, &p_set->attr_dict, p_set_styles );
                }
            }
        }
        else if( !tt_node_NameCompare( ((const tt_node_t *)p_child)->psz_node_name, "br" ) )
        {
            AppendLineBreakToRegion( p_region );
        }
        else
        {
            ConvertNodesToRegionContent( p_ctx, (const tt_node_t *) p_child,
                                         p_region, p_set_styles, playbacktime );
        }
    }

    if( p_set_styles )
        ttml_style_Delete( p_set_styles );
}

static tt_node_t *ParseTTML( decoder_t *p_dec, const uint8_t *p_buffer, size_t i_buffer )
{
    stream_t*       p_sub;
    xml_reader_t*   p_xml_reader;

    p_sub = vlc_stream_MemoryNew( p_dec, (uint8_t*) p_buffer, i_buffer, true );
    if( unlikely( p_sub == NULL ) )
        return NULL;

    p_xml_reader = xml_ReaderCreate( p_dec, p_sub );
    if( unlikely( p_xml_reader == NULL ) )
    {
        vlc_stream_Delete( p_sub );
        return NULL;
    }

    tt_node_t *p_rootnode = NULL;
    if( ParseTTMLChunk( p_xml_reader, &p_rootnode ) != VLC_SUCCESS )
    {
        if( p_rootnode )
            tt_node_RecursiveDelete( p_rootnode );
        p_rootnode = NULL;
    }

    xml_ReaderDelete( p_xml_reader );
    vlc_stream_Delete( p_sub );

    return p_rootnode;
}

static void InitTTMLContext( tt_node_t *p_rootnode, ttml_context_t *p_ctx )
{
    p_ctx->p_rootnode = p_rootnode;
    /* set defaults required for size/cells computation */
    p_ctx->root_extent_h.i_value = 100;
    p_ctx->root_extent_h.unit = TTML_UNIT_PERCENT;
    p_ctx->root_extent_v.i_value = 100;
    p_ctx->root_extent_v.unit = TTML_UNIT_PERCENT;
    p_ctx->i_cell_resolution_v = TTML_DEFAULT_CELL_RESOLUTION_V;
    p_ctx->i_cell_resolution_h = TTML_DEFAULT_CELL_RESOLUTION_H;
    /* and override them */
    const char *value = vlc_dictionary_value_for_key( &p_rootnode->attr_dict,
                                                      "tts:extent" );
    if( value != kVLCDictionaryNotFound )
    {
        ttml_read_coords( value, &p_ctx->root_extent_h,
                               &p_ctx->root_extent_v );
    }
    value = vlc_dictionary_value_for_key( &p_rootnode->attr_dict,
                                          "ttp:cellResolution" );
    if( value != kVLCDictionaryNotFound )
    {
        unsigned w, h;
        if( sscanf( value, "%u %u", &w, &h) == 2 && w && h )
        {
            p_ctx->i_cell_resolution_h = w;
            p_ctx->i_cell_resolution_v = h;
        }
    }
}

static ttml_region_t *GenerateRegions( tt_node_t *p_rootnode, tt_time_t playbacktime )
{
    ttml_region_t*  p_regions = NULL;
    ttml_region_t** pp_region_last = &p_regions;

    if( !tt_node_NameCompare( p_rootnode->psz_node_name, "tt" ) )
    {
        const tt_node_t *p_bodynode = FindNode( p_rootnode, "body", 1, NULL );
        if( p_bodynode )
        {
            ttml_context_t context;
            InitTTMLContext( p_rootnode, &context );
            context.p_rootnode = p_rootnode;

            vlc_dictionary_init( &context.regions, 1 );
            ConvertNodesToRegionContent( &context, p_bodynode, NULL, NULL, playbacktime );

            for( int i = 0; i < context.regions.i_size; ++i )
            {
                for ( const vlc_dictionary_entry_t* p_entry = context.regions.p_entries[i];
                                                    p_entry != NULL; p_entry = p_entry->p_next )
                {
                    *pp_region_last = (ttml_region_t *) p_entry->p_value;
                    pp_region_last = (ttml_region_t **) &(*pp_region_last)->updt.p_next;
                }
            }

            vlc_dictionary_clear( &context.regions, NULL, NULL );
        }
    }
    else if ( !tt_node_NameCompare( p_rootnode->psz_node_name, "div" ) ||
              !tt_node_NameCompare( p_rootnode->psz_node_name, "p" ) )
    {
        /* TODO */
    }

    return p_regions;
}

static int ParseBlock( decoder_t *p_dec, const block_t *p_block )
{
    tt_time_t *p_timings_array = NULL;
    size_t   i_timings_count = 0;

    /* We Only support absolute timings */
    tt_timings_t temporal_extent;
    temporal_extent.i_type = TT_TIMINGS_PARALLEL;
    tt_time_Init( &temporal_extent.begin );
    tt_time_Init( &temporal_extent.end );
    tt_time_Init( &temporal_extent.dur );
    temporal_extent.begin.base = 0;

    if( p_block->i_flags & BLOCK_FLAG_CORRUPTED )
        return VLCDEC_SUCCESS;

    /* We cannot display a subpicture with no date */
    if( p_block->i_pts <= VLC_TS_INVALID )
    {
        msg_Warn( p_dec, "subtitle without a date" );
        return VLCDEC_SUCCESS;
    }

    tt_node_t *p_rootnode = ParseTTML( p_dec, p_block->p_buffer, p_block->i_buffer );
    if( !p_rootnode )
        return VLCDEC_SUCCESS;

    tt_timings_Resolve( (tt_basenode_t *) p_rootnode, &temporal_extent,
                        &p_timings_array, &i_timings_count );

#ifdef TTML_DEBUG
    for( size_t i=0; i<i_timings_count; i++ )
        printf("%ld ", tt_time_Convert( &p_timings_array[i] ) );
    printf("\n");
#endif

    for( size_t i=0; i+1 < i_timings_count; i++ )
    {
        /* We Only support absolute timings (2) */
        if( tt_time_Convert( &p_timings_array[i] ) + VLC_TS_0 < p_block->i_dts )
            continue;

        if( tt_time_Convert( &p_timings_array[i] ) + VLC_TS_0 > p_block->i_dts + p_block->i_length )
            break;

        subpicture_t *p_spu = NULL;
        ttml_region_t *p_regions = GenerateRegions( p_rootnode, p_timings_array[i] );
        if( p_regions && ( p_spu = decoder_NewSubpictureText( p_dec ) ) )
        {
            p_spu->i_start    = VLC_TS_0 + tt_time_Convert( &p_timings_array[i] );
            p_spu->i_stop     = VLC_TS_0 + tt_time_Convert( &p_timings_array[i+1] ) - 1;
            p_spu->b_ephemer  = true;
            p_spu->b_absolute = true;

            subpicture_updater_sys_t *p_spu_sys = p_spu->updater.p_sys;
            subpicture_updater_sys_region_t *p_updtregion = NULL;

            /* Create region update info from each ttml region */
            for( ttml_region_t *p_region = p_regions;
                 p_region; p_region = (ttml_region_t *) p_region->updt.p_next )
            {
                if( p_updtregion == NULL )
                {
                    p_updtregion = &p_spu_sys->region;
                }
                else
                {
                    p_updtregion = SubpictureUpdaterSysRegionNew();
                    if( p_updtregion == NULL )
                        break;
                    SubpictureUpdaterSysRegionAdd( &p_spu_sys->region, p_updtregion );
                }

                /* broken legacy align var (can't handle center...). Will change only regions content. */
                if( p_dec->p_sys->i_align & SUBPICTURE_ALIGN_MASK )
                    p_spu_sys->region.inner_align = p_dec->p_sys->i_align;

                p_spu_sys->margin_ratio = 0.0;

                /* copy and take ownership of pointeds */
                *p_updtregion = p_region->updt;
                p_updtregion->p_next = NULL;
                p_region->updt.p_region_style = NULL;
                p_region->updt.p_segments = NULL;
            }

        }

        /* cleanup */
        while( p_regions )
        {
            ttml_region_t *p_nextregion = (ttml_region_t *) p_regions->updt.p_next;
            ttml_region_Delete( p_regions );
            p_regions = p_nextregion;
        }

        if( p_spu )
            decoder_QueueSub( p_dec, p_spu );
    }

    tt_node_RecursiveDelete( p_rootnode );

    free( p_timings_array );

    return VLCDEC_SUCCESS;
}



/****************************************************************************
 * DecodeBlock: the whole thing
 ****************************************************************************/
static int DecodeBlock( decoder_t *p_dec, block_t *p_block )
{
    if( p_block == NULL ) /* No Drain */
        return VLCDEC_SUCCESS;

    int ret = ParseBlock( p_dec, p_block );
#ifdef TTML_DEBUG
    if( p_block->i_buffer )
    {
        p_block->p_buffer[p_block->i_buffer - 1] = 0;
        msg_Dbg(p_dec,"time %ld %s", p_block->i_dts, p_block->p_buffer);
    }
#endif
    block_Release( p_block );
    return ret;
}

/*****************************************************************************
 * tt_OpenDecoder: probe the decoder and return score
 *****************************************************************************/
int tt_OpenDecoder( vlc_object_t *p_this )
{
    decoder_t *p_dec = (decoder_t*)p_this;
    decoder_sys_t *p_sys;

    if( p_dec->fmt_in.i_codec != VLC_CODEC_TTML )
        return VLC_EGENERIC;

    /* Allocate the memory needed to store the decoder's structure */
    p_dec->p_sys = p_sys = calloc( 1, sizeof( *p_sys ) );
    if( unlikely( p_sys == NULL ) )
        return VLC_ENOMEM;

    p_dec->pf_decode = DecodeBlock;
    p_sys->i_align = var_InheritInteger( p_dec, "ttml-align" );

    return VLC_SUCCESS;
}

/*****************************************************************************
 * tt_CloseDecoder: clean up the decoder
 *****************************************************************************/
void tt_CloseDecoder( vlc_object_t *p_this )
{
    decoder_t *p_dec = (decoder_t *)p_this;
    decoder_sys_t *p_sys = p_dec->p_sys;

    free( p_sys );
}
