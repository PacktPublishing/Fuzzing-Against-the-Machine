/*****************************************************************************
 * strings.c: String related functions
 *****************************************************************************
 * Copyright (C) 2006 VLC authors and VideoLAN
 * Copyright (C) 2008-2009 Rémi Denis-Courmont
 * $Id: a15541afbab7213a1528e57eb309b89f5bdcfc58 $
 *
 * Authors: Antoine Cellerier <dionoea at videolan dot org>
 *          Daniel Stranger <vlc at schmaller dot de>
 *          Rémi Denis-Courmont
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

/*****************************************************************************
 * Preamble
 *****************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include <assert.h>

/* Needed by vlc_strftime */
#include <time.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#ifndef HAVE_STRCOLL
# define strcoll strcasecmp
#endif

/* Needed by vlc_strfinput */
#include <vlc_input.h>
#include <vlc_meta.h>
#include <vlc_aout.h>
#include <vlc_memstream.h>

#include <vlc_strings.h>
#include <vlc_charset.h>
#include <vlc_arrays.h>
#include <libvlc.h>
#include <errno.h>

static const struct xml_entity_s
{
    char    psz_entity[8];
    char    psz_char[4];
} xml_entities[] = {
    /* Important: this list has to be in alphabetical order (psz_entity-wise) */
    { "AElig;",  "Æ" },
    { "Aacute;", "Á" },
    { "Acirc;",  "Â" },
    { "Agrave;", "À" },
    { "Aring;",  "Å" },
    { "Atilde;", "Ã" },
    { "Auml;",   "Ä" },
    { "Ccedil;", "Ç" },
    { "Dagger;", "‡" },
    { "ETH;",    "Ð" },
    { "Eacute;", "É" },
    { "Ecirc;",  "Ê" },
    { "Egrave;", "È" },
    { "Euml;",   "Ë" },
    { "Iacute;", "Í" },
    { "Icirc;",  "Î" },
    { "Igrave;", "Ì" },
    { "Iuml;",   "Ï" },
    { "Ntilde;", "Ñ" },
    { "OElig;",  "Œ" },
    { "Oacute;", "Ó" },
    { "Ocirc;",  "Ô" },
    { "Ograve;", "Ò" },
    { "Oslash;", "Ø" },
    { "Otilde;", "Õ" },
    { "Ouml;",   "Ö" },
    { "Scaron;", "Š" },
    { "THORN;",  "Þ" },
    { "Uacute;", "Ú" },
    { "Ucirc;",  "Û" },
    { "Ugrave;", "Ù" },
    { "Uuml;",   "Ü" },
    { "Yacute;", "Ý" },
    { "Yuml;",   "Ÿ" },
    { "aacute;", "á" },
    { "acirc;",  "â" },
    { "acute;",  "´" },
    { "aelig;",  "æ" },
    { "agrave;", "à" },
    { "amp;",    "&" },
    { "apos;",   "'" },
    { "aring;",  "å" },
    { "atilde;", "ã" },
    { "auml;",   "ä" },
    { "bdquo;",  "„" },
    { "brvbar;", "¦" },
    { "ccedil;", "ç" },
    { "cedil;",  "¸" },
    { "cent;",   "¢" },
    { "circ;",   "ˆ" },
    { "copy;",   "©" },
    { "curren;", "¤" },
    { "dagger;", "†" },
    { "deg;",    "°" },
    { "divide;", "÷" },
    { "eacute;", "é" },
    { "ecirc;",  "ê" },
    { "egrave;", "è" },
    { "eth;",    "ð" },
    { "euml;",   "ë" },
    { "euro;",   "€" },
    { "frac12;", "½" },
    { "frac14;", "¼" },
    { "frac34;", "¾" },
    { "gt;",     ">" },
    { "hellip;", "…" },
    { "iacute;", "í" },
    { "icirc;",  "î" },
    { "iexcl;",  "¡" },
    { "igrave;", "ì" },
    { "iquest;", "¿" },
    { "iuml;",   "ï" },
    { "laquo;",  "«" },
    { "ldquo;",  "“" },
    { "lsaquo;", "‹" },
    { "lsquo;",  "‘" },
    { "lt;",     "<" },
    { "macr;",   "¯" },
    { "mdash;",  "—" },
    { "micro;",  "µ" },
    { "middot;", "·" },
    { "nbsp;",   "\xc2\xa0" },
    { "ndash;",  "–" },
    { "not;",    "¬" },
    { "ntilde;", "ñ" },
    { "oacute;", "ó" },
    { "ocirc;",  "ô" },
    { "oelig;",  "œ" },
    { "ograve;", "ò" },
    { "ordf;",   "ª" },
    { "ordm;",   "º" },
    { "oslash;", "ø" },
    { "otilde;", "õ" },
    { "ouml;",   "ö" },
    { "para;",   "¶" },
    { "permil;", "‰" },
    { "plusmn;", "±" },
    { "pound;",  "£" },
    { "quot;",   "\"" },
    { "raquo;",  "»" },
    { "rdquo;",  "”" },
    { "reg;",    "®" },
    { "rsaquo;", "›" },
    { "rsquo;",  "’" },
    { "sbquo;",  "‚" },
    { "scaron;", "š" },
    { "sect;",   "§" },
    { "shy;",    "­" },
    { "sup1;",   "¹" },
    { "sup2;",   "²" },
    { "sup3;",   "³" },
    { "szlig;",  "ß" },
    { "thorn;",  "þ" },
    { "tilde;",  "˜" },
    { "times;",  "×" },
    { "trade;",  "™" },
    { "uacute;", "ú" },
    { "ucirc;",  "û" },
    { "ugrave;", "ù" },
    { "uml;",    "¨" },
    { "uuml;",   "ü" },
    { "yacute;", "ý" },
    { "yen;",    "¥" },
    { "yuml;",   "ÿ" },
};

static int cmp_entity (const void *key, const void *elem)
{
    const struct xml_entity_s *ent = elem;
    const char *name = key;

    return strncmp (name, ent->psz_entity, strlen (ent->psz_entity));
}

void vlc_xml_decode( char *psz_value )
{
    char *p_pos = psz_value;

    while ( *psz_value )
    {
        if( *psz_value == '&' )
        {
            if( psz_value[1] == '#' )
            {   /* &#DDD; or &#xHHHH; Unicode code point */
                char *psz_end;
                unsigned long cp;

                if( psz_value[2] == 'x' ) /* The x must be lower-case. */
                    cp = strtoul( psz_value + 3, &psz_end, 16 );
                else
                    cp = strtoul( psz_value + 2, &psz_end, 10 );

                if( *psz_end == ';' )
                {
                    psz_value = psz_end + 1;
                    if( cp == 0 )
                        (void)0; /* skip nulls */
                    else
                    if( cp <= 0x7F )
                    {
                        *p_pos =            cp;
                    }
                    else
                    /* Unicode code point outside ASCII.
                     * &#xxx; representation is longer than UTF-8 :) */
                    if( cp <= 0x7FF )
                    {
                        *p_pos++ = 0xC0 |  (cp >>  6);
                        *p_pos   = 0x80 |  (cp        & 0x3F);
                    }
                    else
                    if( cp <= 0xFFFF )
                    {
                        *p_pos++ = 0xE0 |  (cp >> 12);
                        *p_pos++ = 0x80 | ((cp >>  6) & 0x3F);
                        *p_pos   = 0x80 |  (cp        & 0x3F);
                    }
                    else
                    if( cp <= 0x1FFFFF ) /* Outside the BMP */
                    {   /* Unicode stops at 10FFFF, but who cares? */
                        *p_pos++ = 0xF0 |  (cp >> 18);
                        *p_pos++ = 0x80 | ((cp >> 12) & 0x3F);
                        *p_pos++ = 0x80 | ((cp >>  6) & 0x3F);
                        *p_pos   = 0x80 |  (cp        & 0x3F);
                    }
                }
                else
                {
                    /* Invalid entity number */
                    *p_pos = *psz_value;
                    psz_value++;
                }
            }
            else
            {   /* Well-known XML entity */
                const struct xml_entity_s *ent;

                ent = bsearch (psz_value + 1, xml_entities,
                               ARRAY_SIZE (xml_entities),
                               sizeof (*ent), cmp_entity);
                if (ent != NULL)
                {
                    size_t olen = strlen (ent->psz_char);
                    memcpy (p_pos, ent->psz_char, olen);
                    p_pos += olen - 1;
                    psz_value += strlen (ent->psz_entity) + 1;
                }
                else
                {   /* No match */
                    *p_pos = *psz_value;
                    psz_value++;
                }
            }
        }
        else
        {
            *p_pos = *psz_value;
            psz_value++;
        }

        p_pos++;
    }

    *p_pos = '\0';
}

char *vlc_xml_encode (const char *str)
{
    struct vlc_memstream stream;
    size_t n;
    uint32_t cp;

    assert(str != NULL);
    vlc_memstream_open(&stream);

    while ((n = vlc_towc (str, &cp)) != 0)
    {
        if (unlikely(n == (size_t)-1))
        {
            if (vlc_memstream_close(&stream) == 0)
                free(stream.ptr);
            errno = EILSEQ;
            return NULL;
        }

        switch (cp)
        {
            case '\"':
                vlc_memstream_puts(&stream, "&quot;");
                break;
            case '&':
                vlc_memstream_puts(&stream, "&amp;");
                break;
            case '\'':
                vlc_memstream_puts(&stream, "&#39;");
                break;
            case '<':
                vlc_memstream_puts(&stream, "&lt;");
                break;
            case '>':
                vlc_memstream_puts(&stream, "&gt;");
                break;
            default:
                if (cp < 32) /* C0 code not allowed (except 9, 10 and 13) */
                    break;
                if (cp >= 128 && cp < 160) /* C1 code encoded (except 133) */
                {
                    vlc_memstream_printf(&stream, "&#%"PRIu32";", cp);
                    break;
                }
                /* fall through */
            case 9:
            case 10:
            case 13:
            case 133:
                vlc_memstream_write(&stream, str, n);
                break;
        }
        str += n;
    }

    if (vlc_memstream_close(&stream))
        return NULL;
    return stream.ptr;
}

/* Base64 encoding */
char *vlc_b64_encode_binary( const uint8_t *src, size_t i_src )
{
    static const char b64[] =
           "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    char *ret = malloc( ( i_src + 4 ) * 4 / 3 );
    char *dst = ret;

    if( dst == NULL )
        return NULL;

    while( i_src > 0 )
    {
        /* pops (up to) 3 bytes of input, push 4 bytes */
        uint32_t v;

        /* 1/3 -> 1/4 */
        v = ((unsigned)*src++) << 24;
        *dst++ = b64[v >> 26];
        v = v << 6;

        /* 2/3 -> 2/4 */
        if( i_src >= 2 )
            v |= *src++ << 22;
        *dst++ = b64[v >> 26];
        v = v << 6;

        /* 3/3 -> 3/4 */
        if( i_src >= 3 )
            v |= *src++ << 20; // 3/3
        *dst++ = ( i_src >= 2 ) ? b64[v >> 26] : '='; // 3/4
        v = v << 6;

        /* -> 4/4 */
        *dst++ = ( i_src >= 3 ) ? b64[v >> 26] : '='; // 4/4

        if( i_src <= 3 )
            break;
        i_src -= 3;
    }

    *dst = '\0';

    return ret;
}

char *vlc_b64_encode( const char *src )
{
    if( src )
        return vlc_b64_encode_binary( (const uint8_t*)src, strlen(src) );
    else
        return vlc_b64_encode_binary( (const uint8_t*)"", 0 );
}

/* Base64 decoding */
size_t vlc_b64_decode_binary_to_buffer( uint8_t *p_dst, size_t i_dst, const char *p_src )
{
    static const int b64[256] = {
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 00-0F */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 10-1F */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,  /* 20-2F */
        52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,  /* 30-3F */
        -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,  /* 40-4F */
        15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,  /* 50-5F */
        -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,  /* 60-6F */
        41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,  /* 70-7F */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 80-8F */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 90-9F */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* A0-AF */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* B0-BF */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* C0-CF */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* D0-DF */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* E0-EF */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1   /* F0-FF */
    };
    uint8_t *p_start = p_dst;
    uint8_t *p = (uint8_t *)p_src;

    int i_level;
    int i_last;

    for( i_level = 0, i_last = 0; (size_t)( p_dst - p_start ) < i_dst && *p != '\0'; p++ )
    {
        const int c = b64[(unsigned int)*p];
        if( c == -1 )
            break;

        switch( i_level )
        {
            case 0:
                i_level++;
                break;
            case 1:
                *p_dst++ = ( i_last << 2 ) | ( ( c >> 4)&0x03 );
                i_level++;
                break;
            case 2:
                *p_dst++ = ( ( i_last << 4 )&0xf0 ) | ( ( c >> 2 )&0x0f );
                i_level++;
                break;
            case 3:
                *p_dst++ = ( ( i_last &0x03 ) << 6 ) | c;
                i_level = 0;
        }
        i_last = c;
    }

    return p_dst - p_start;
}
size_t vlc_b64_decode_binary( uint8_t **pp_dst, const char *psz_src )
{
    const int i_src = strlen( psz_src );
    uint8_t   *p_dst;

    *pp_dst = p_dst = malloc( i_src );
    if( !p_dst )
        return 0;
    return  vlc_b64_decode_binary_to_buffer( p_dst, i_src, psz_src );
}
char *vlc_b64_decode( const char *psz_src )
{
    const int i_src = strlen( psz_src );
    char *p_dst = malloc( i_src + 1 );
    size_t i_dst;
    if( !p_dst )
        return NULL;

    i_dst = vlc_b64_decode_binary_to_buffer( (uint8_t*)p_dst, i_src, psz_src );
    p_dst[i_dst] = '\0';

    return p_dst;
}

char *vlc_strftime( const char *tformat )
{
    time_t curtime;
    struct tm loctime;

    if (strcmp (tformat, "") == 0)
        return strdup (""); /* corner case w.r.t. strftime() return value */

    /* Get the current time.  */
    time( &curtime );

    /* Convert it to local time representation.  */
    localtime_r( &curtime, &loctime );
    for (size_t buflen = strlen (tformat) + 32;; buflen += 32)
    {
        char *str = malloc (buflen);
        if (str == NULL)
            return NULL;

        size_t len = strftime (str, buflen, tformat, &loctime);
        if (len > 0)
        {
            char *ret = realloc (str, len + 1);
            return ret ? ret : str; /* <- this cannot fail */
        }
        free (str);
    }
    vlc_assert_unreachable ();
}

static void write_duration(struct vlc_memstream *stream, int64_t duration)
{
    lldiv_t d;
    long long sec;

    duration /= CLOCK_FREQ;
    d = lldiv(duration, 60);
    sec = d.rem;
    d = lldiv(d.quot, 60);
    vlc_memstream_printf(stream, "%02lld:%02lld:%02lld", d.quot, d.rem, sec);
}

static int write_meta(struct vlc_memstream *stream, input_item_t *item,
                      vlc_meta_type_t type)
{
    if (item == NULL)
        return EOF;

    char *value = input_item_GetMeta(item, type);
    if (value == NULL)
        return EOF;

    vlc_memstream_puts(stream, value);
    free(value);
    return 0;
}

char *vlc_strfinput(input_thread_t *input, const char *s)
{
    struct vlc_memstream stream[1];

    input_item_t *item = (input != NULL) ? input_GetItem(input) : NULL;

    char c;
    bool b_is_format = false;
    bool b_empty_if_na = false;

    assert(s != NULL);

    vlc_memstream_open(stream);

    while ((c = *s) != '\0')
    {
        s++;

        if (!b_is_format)
        {
            if (c == '$')
            {
                b_is_format = true;
                b_empty_if_na = false;
                continue;
            }

            vlc_memstream_putc(stream, c);
            continue;
        }

        b_is_format = false;

        switch (c)
        {
            case 'a':
                write_meta(stream, item, vlc_meta_Artist);
                break;
            case 'b':
                write_meta(stream, item, vlc_meta_Album);
                break;
            case 'c':
                write_meta(stream, item, vlc_meta_Copyright);
                break;
            case 'd':
                write_meta(stream, item, vlc_meta_Description);
                break;
            case 'e':
                write_meta(stream, item, vlc_meta_EncodedBy);
                break;
            case 'f':
                if (item != NULL)
                {
                    vlc_mutex_lock(&item->lock);
                    if (item->p_stats != NULL)
                    {
                        vlc_mutex_lock(&item->p_stats->lock);
                        vlc_memstream_printf(stream, "%"PRIi64,
                            item->p_stats->i_displayed_pictures);
                        vlc_mutex_unlock(&item->p_stats->lock);
                    }
                    else if (!b_empty_if_na)
                        vlc_memstream_putc(stream, '-');
                    vlc_mutex_unlock(&item->lock);
                }
                else if (!b_empty_if_na)
                    vlc_memstream_putc(stream, '-');
                break;
            case 'g':
                write_meta(stream, item, vlc_meta_Genre);
                break;
            case 'l':
                write_meta(stream, item, vlc_meta_Language);
                break;
            case 'n':
                write_meta(stream, item, vlc_meta_TrackNumber);
                break;
            case 'o':
                write_meta(stream, item, vlc_meta_TrackTotal);
                break;
            case 'p':
                if (item == NULL)
                    break;
                {
                    char *value = input_item_GetNowPlayingFb(item);
                    if (value == NULL)
                        break;

                    vlc_memstream_puts(stream, value);
                    free(value);
                }
                break;
            case 'r':
                write_meta(stream, item, vlc_meta_Rating);
                break;
            case 's':
            {
                char *lang = NULL;

                if (input != NULL)
                    lang = var_GetNonEmptyString(input, "sub-language");
                if (lang != NULL)
                {
                    vlc_memstream_puts(stream, lang);
                    free(lang);
                }
                else if (!b_empty_if_na)
                    vlc_memstream_putc(stream, '-');
                break;
            }
            case 't':
                write_meta(stream, item, vlc_meta_Title);
                break;
            case 'u':
                write_meta(stream, item, vlc_meta_URL);
                break;
            case 'A':
                write_meta(stream, item, vlc_meta_Date);
                break;
            case 'B':
                if (input != NULL)
                    vlc_memstream_printf(stream, "%"PRId64,
                            var_GetInteger(input, "bit-rate") / 1000);
                else if (!b_empty_if_na)
                    vlc_memstream_putc(stream, '-');
                break;
            case 'C':
                if (input != NULL)
                    vlc_memstream_printf(stream, "%"PRId64,
                            var_GetInteger(input, "chapter"));
                else if (!b_empty_if_na)
                    vlc_memstream_putc(stream, '-');
                break;
            case 'D':
                if (item != NULL)
                    write_duration(stream, input_item_GetDuration(item));
                else if (!b_empty_if_na)
                    vlc_memstream_puts(stream, "--:--:--");
                break;
            case 'F':
                if (item != NULL)
                {
                    char *uri = input_item_GetURI(item);
                    if (uri != NULL)
                    {
                        vlc_memstream_puts(stream, uri);
                        free(uri);
                    }
                }
                break;
            case 'I':
                if (input != NULL)
                    vlc_memstream_printf(stream, "%"PRId64,
                                         var_GetInteger(input, "title"));
                else if (!b_empty_if_na)
                    vlc_memstream_putc(stream, '-');
                break;
            case 'L':
                if (item != NULL)
                {
                    assert(input != NULL);
                    write_duration(stream, input_item_GetDuration(item)
                                   - var_GetInteger(input, "time"));
                }
                else if (!b_empty_if_na)
                    vlc_memstream_puts(stream, "--:--:--");
                break;
            case 'N':
                if (item != NULL)
                {
                    char *name = input_item_GetName(item);
                    if (name != NULL)
                    {
                        vlc_memstream_puts(stream, name);
                        free(name);
                    }
                }
                break;
            case 'O':
            {
                char *lang = NULL;

                if (input != NULL)
                    lang = var_GetNonEmptyString(input, "audio-language");
                if (lang != NULL)
                {
                    vlc_memstream_puts(stream, lang);
                    free(lang);
                }
                else if (!b_empty_if_na)
                    vlc_memstream_putc(stream, '-');
                break;
            }
            case 'P':
                if (input != NULL)
                    vlc_memstream_printf(stream, "%2.1f",
                            var_GetFloat(input, "position") * 100.f);
                else if (!b_empty_if_na)
                    vlc_memstream_puts(stream, "--.-%");
                break;
            case 'R':
                if (input != NULL)
                    vlc_memstream_printf(stream, "%.3f",
                                         var_GetFloat(input, "rate"));
                else if (!b_empty_if_na)
                    vlc_memstream_putc(stream, '-');
                break;
            case 'S':
                if (input != NULL)
                {
                    int rate = var_GetInteger(input, "sample-rate");
                    div_t dr = div((rate + 50) / 100, 10);

                    vlc_memstream_printf(stream, "%d.%01d", dr.quot, dr.rem);
                }
                else if (!b_empty_if_na)
                    vlc_memstream_putc(stream, '-');
                break;
            case 'T':
                if (input != NULL)
                    write_duration(stream, var_GetInteger(input, "time"));
                else if (!b_empty_if_na)
                    vlc_memstream_puts(stream, "--:--:--");
                break;
            case 'U':
                write_meta(stream, item, vlc_meta_Publisher);
                break;
            case 'V':
            {
                float vol = 0.f;

                if (input != NULL)
                {
                    audio_output_t *aout = input_GetAout(input);
                    if (aout != NULL)
                    {
                        vol = aout_VolumeGet(aout);
                        vlc_object_release(aout);
                    }
                }
                if (vol >= 0.f)
                    vlc_memstream_printf(stream, "%ld", lroundf(vol * 256.f));
                else if (!b_empty_if_na)
                    vlc_memstream_puts(stream, "---");
                break;
            }
            case '_':
                vlc_memstream_putc(stream, '\n');
                break;
            case 'Z':
                if (item == NULL)
                    break;
                {
                    char *value = input_item_GetNowPlayingFb(item);
                    if (value != NULL)
                    {
                        vlc_memstream_puts(stream, value);
                        free(value);
                    }
                    else
                    {
                        char *title = input_item_GetTitleFbName(item);

                        if (write_meta(stream, item, vlc_meta_Artist) >= 0
                            && title != NULL)
                            vlc_memstream_puts(stream, " - ");

                        if (title != NULL)
                        {
                            vlc_memstream_puts(stream, title);
                            free(title);
                        }
                    }
                }
                break;
            case ' ':
                b_empty_if_na = true;
                b_is_format = true;
                break;
            default:
                vlc_memstream_putc(stream, c);
                break;
        }
    }

    if (vlc_memstream_close(stream))
        return NULL;
    return stream->ptr;
}

int vlc_filenamecmp(const char *a, const char *b)
{
    size_t i;
    char ca, cb;

    /* Attempt to guess if the sorting algorithm should be alphabetic
     * (i.e. collation) or numeric:
     * - If the first mismatching characters are not both digits,
     *   then collation is the only option.
     * - If one of the first mismatching characters is 0 and the other is also
     *   a digit, the comparands are probably left-padded numerical values.
     *   It does not matter which algorithm is used: the zero will be smaller
     *   than non-zero either way.
     * - Otherwise, the comparands are numerical values, and might not be
     *   aligned (i.e. not same order of magnitude). If so, collation would
     *   fail. So numerical comparison is performed. */
    for (i = 0; (ca = a[i]) == (cb = b[i]); i++)
        if (ca == '\0')
            return 0; /* strings are exactly identical */

    if ((unsigned)(ca - '0') > 9 || (unsigned)(cb - '0') > 9)
        return strcoll(a, b);

    unsigned long long ua = strtoull(a + i, NULL, 10);
    unsigned long long ub = strtoull(b + i, NULL, 10);

    /* The number may be identical in two cases:
     * - leading zero (e.g. "012" and "12")
     * - overflow on both sides (#ULLONG_MAX) */
    if (ua == ub)
        return strcoll(a, b);

    return (ua > ub) ? +1 : -1;
}

/**
 * Sanitize a file name.
 *
 * Remove forbidden, potentially forbidden and otherwise evil characters from
 * file names. That includes slashes, and popular characters like colon
 * (on Unix anyway).
 *
 * \warning This function should only be used for automatically generated
 * file names. Do not use this on full paths, only single file names without
 * any directory separator!
 */
void filename_sanitize( char *str )
{
    unsigned char c;

    /* Special file names, not allowed */
    if( !strcmp( str, "." ) || !strcmp( str, ".." ) )
    {
        while( *str )
            *(str++) = '_';
        return;
    }

    /* On platforms not using UTF-8, VLC cannot access non-Unicode paths.
     * Also, some file systems require Unicode file names.
     * NOTE: This may inserts '?' thus is done replacing '?' with '_'. */
    EnsureUTF8( str );

    /* Avoid leading spaces to please Windows. */
    while( (c = *str) != '\0' )
    {
        if( c != ' ' )
            break;
        *(str++) = '_';
    }

    char *start = str;

    while( (c = *str) != '\0' )
    {
        /* Non-printable characters are not a good idea */
        if( c < 32 )
            *str = '_';
        /* This is the list of characters not allowed by Microsoft.
         * We also black-list them on Unix as they may be confusing, and are
         * not supported by some file system types (notably CIFS). */
        else if( strchr( "/:\\*\"?|<>", c ) != NULL )
            *str = '_';
        str++;
    }

    /* Avoid trailing spaces also to please Windows. */
    while( str > start )
    {
        if( *(--str) != ' ' )
            break;
        *str = '_';
    }
}
