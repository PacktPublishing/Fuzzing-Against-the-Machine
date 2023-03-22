/*****************************************************************************
 * mtime.c: high resolution time management functions
 * Functions are prototyped in vlc_mtime.h.
 *****************************************************************************
 * Copyright (C) 1998-2007 VLC authors and VideoLAN
 * Copyright © 2006-2007 Rémi Denis-Courmont
 * $Id: 2d579005e7a44845cba99871abc5452d4c3212a6 $
 *
 * Authors: Vincent Seguin <seguin@via.ecp.fr>
 *          Rémi Denis-Courmont
 *          Gisle Vanem
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

#include <time.h>

/**
 * Convert seconds to a time in the format h:mm:ss.
 *
 * This function is provided for any interface function which need to print a
 * time string in the format h:mm:ss
 * date.
 * \param secs  the date to be converted
 * \param psz_buffer should be a buffer at least MSTRTIME_MAX_SIZE characters
 * \return psz_buffer is returned so this can be used as printf parameter.
 */
char *secstotimestr( char *psz_buffer, int32_t i_seconds )
{
    if( unlikely(i_seconds < 0) )
    {
        secstotimestr( psz_buffer + 1, -i_seconds );
        *psz_buffer = '-';
        return psz_buffer;
    }

    div_t d;

    d = div( i_seconds, 60 );
    i_seconds = d.rem;
    d = div( d.quot, 60 );

    if( d.quot )
        snprintf( psz_buffer, MSTRTIME_MAX_SIZE, "%u:%02u:%02u",
                 d.quot, d.rem, i_seconds );
    else
        snprintf( psz_buffer, MSTRTIME_MAX_SIZE, "%02u:%02u",
                  d.rem, i_seconds );
    return psz_buffer;
}

/*
 * Date management (internal and external)
 */

/**
 * Initialize a date_t.
 *
 * \param date to initialize
 * \param divider (sample rate) numerator
 * \param divider (sample rate) denominator
 */

void date_Init( date_t *p_date, uint32_t i_divider_n, uint32_t i_divider_d )
{
    p_date->date = 0;
    p_date->i_divider_num = i_divider_n;
    p_date->i_divider_den = i_divider_d;
    p_date->i_remainder = 0;
}

/**
 * Change a date_t.
 *
 * \param date to change
 * \param divider (sample rate) numerator
 * \param divider (sample rate) denominator
 */

void date_Change( date_t *p_date, uint32_t i_divider_n, uint32_t i_divider_d )
{
    /* change time scale of remainder */
    p_date->i_remainder = p_date->i_remainder * i_divider_n / p_date->i_divider_num;
    p_date->i_divider_num = i_divider_n;
    p_date->i_divider_den = i_divider_d;
}

/**
 * Set the date value of a date_t.
 *
 * \param date to set
 * \param date value
 */
void date_Set( date_t *p_date, mtime_t i_new_date )
{
    p_date->date = i_new_date;
    p_date->i_remainder = 0;
}

/**
 * Get the date of a date_t
 *
 * \param date to get
 * \return date value
 */
mtime_t date_Get( const date_t *p_date )
{
    return p_date->date;
}

/**
 * Move forwards or backwards the date of a date_t.
 *
 * \param date to move
 * \param difference value
 */
void date_Move( date_t *p_date, mtime_t i_difference )
{
    p_date->date += i_difference;
}

/**
 * Increment the date and return the result, taking into account
 * rounding errors.
 *
 * \param date to increment
 * \param incrementation in number of samples
 * \return date value
 */
mtime_t date_Increment( date_t *p_date, uint32_t i_nb_samples )
{
    assert( p_date->i_divider_num != 0 );
    mtime_t i_dividend = i_nb_samples * CLOCK_FREQ * p_date->i_divider_den;
    lldiv_t d = lldiv( i_dividend, p_date->i_divider_num );

    p_date->date += d.quot;
    p_date->i_remainder += (int)d.rem;

    if( p_date->i_remainder >= p_date->i_divider_num )
    {
        /* This is Bresenham algorithm. */
        assert( p_date->i_remainder < 2*p_date->i_divider_num);
        p_date->date += 1;
        p_date->i_remainder -= p_date->i_divider_num;
    }

    return p_date->date;
}

/**
 * Decrement the date and return the result, taking into account
 * rounding errors.
 *
 * \param date to decrement
 * \param decrementation in number of samples
 * \return date value
 */
mtime_t date_Decrement( date_t *p_date, uint32_t i_nb_samples )
{
    mtime_t i_dividend = (mtime_t)i_nb_samples * CLOCK_FREQ * p_date->i_divider_den;
    p_date->date -= i_dividend / p_date->i_divider_num;
    unsigned i_rem_adjust = i_dividend % p_date->i_divider_num;

    if( p_date->i_remainder < i_rem_adjust )
    {
        /* This is Bresenham algorithm. */
        assert( p_date->i_remainder < p_date->i_divider_num);
        p_date->date -= 1;
        p_date->i_remainder += p_date->i_divider_num;
    }

    p_date->i_remainder -= i_rem_adjust;

    return p_date->date;
}

/**
 * @return NTP 64-bits timestamp in host byte order.
 */
uint64_t NTPtime64(void)
{
    struct timespec ts;

    timespec_get(&ts, TIME_UTC);

    /* Convert nanoseconds to 32-bits fraction (232 picosecond units) */
    uint64_t t = (uint64_t)(ts.tv_nsec) << 32;
    t /= 1000000000;

    /* The offset to Unix epoch is 70 years (incl. 17 leap ones). There were
     * no leap seconds during that period since they had not been invented yet.
     */
    t |= ((UINT64_C(70) * 365 + 17) * 24 * 60 * 60 + ts.tv_sec) << 32;
    return t;
}
