/*
 * Conversions.cpp
 *****************************************************************************
 * Copyright © 2015 - VideoLAN and VLC Authors
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
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

#include "Conversions.hpp"

#include <vlc_charset.h>
#include <sstream>

/*
  Decodes a duration as defined by ISO 8601
  http://en.wikipedia.org/wiki/ISO_8601#Durations
  @param str A nullptr-terminated string to convert
  @return: The duration in seconds. -1 if an error occurred.

  Exemple input string: "PT0H9M56.46S"
 */
static mtime_t str_duration( const char *psz_duration )
{
    bool        timeDesignatorReached = false;
    mtime_t  res = 0;
    char*       end_ptr;

    if ( psz_duration == nullptr )
        return -1;
    if ( ( *(psz_duration++) ) != 'P' )
        return -1;
    do
    {
        double number = us_strtod( psz_duration, &end_ptr );
        double      mul = 0;
        if ( psz_duration != end_ptr )
            psz_duration = end_ptr;
        switch( *psz_duration )
        {
            case 'M':
            {
                //M can mean month or minutes, if the 'T' flag has been reached.
                //We don't handle months though.
                if ( timeDesignatorReached == true )
                    mul = 60.0;
                break ;
            }
            case 'Y':
            case 'W':
                break ; //Don't handle this duration.
            case 'D':
                mul = 86400.0;
                break ;
            case 'T':
                timeDesignatorReached = true;
                break ;
            case 'H':
                mul = 3600.0;
                break ;
            case 'S':
                mul = 1.0;
                break ;
            default:
                break ;
        }
        res += mul * number * CLOCK_FREQ;
        if ( *psz_duration )
            psz_duration++;
    } while ( *psz_duration );
    return res;
}

IsoTime::IsoTime(const std::string &str)
{
    time = str_duration(str.c_str());
}

IsoTime::operator mtime_t () const
{
    return time;
}

UTCTime::UTCTime(const std::string &str)
{
    enum { UTCTIME_YEAR = 0, UTCTIME_MON, UTCTIME_DAY, UTCTIME_HOUR, UTCTIME_MIN,
           UTCTIME_SEC, UTCTIME_FRAC_NUM, UTCTIME_FRAC_DEN, UTCTIME_TZ };
    int values[9] = {0};
    std::istringstream in(str);
    in.imbue(std::locale("C"));

    try
    {
        /* Date */
        for(int i=UTCTIME_YEAR;i<=UTCTIME_DAY && !in.eof();i++)
        {
            if(i!=UTCTIME_YEAR)
                in.ignore(1);
            in >> values[i];
        }
        /* Time */
        if (!in.eof() && in.peek() == 'T')
        {
            for(int i=UTCTIME_HOUR;i<=UTCTIME_SEC && !in.eof();i++)
            {
                in.ignore(1);
                in >> values[i];
            }
        }
        if(!in.eof() && in.peek() == '.')
        {
            in.ignore(1);
            values[UTCTIME_FRAC_NUM] = 0;
            values[UTCTIME_FRAC_DEN] = 1;
            int c = in.peek();
            while(c >= '0' && c <= '9')
            {
                values[UTCTIME_FRAC_NUM] = values[UTCTIME_FRAC_NUM] * 10 + (c - '0');
                values[UTCTIME_FRAC_DEN] *= 10;
                in.ignore(1);
                c = in.peek();
            }
        }
        /* Timezone */
        if(!in.eof() && in.peek() == 'Z')
        {
            in.ignore(1);
        }
        else if (!in.eof() && (in.peek() == '+' || in.peek() == '-'))
        {
            int sign = (in.peek() == '+') ? 1 : -1;
            int tz = 0;
            in.ignore(1);

            if(!in.eof())
            {
                std::string tzspec;
                in >> tzspec;

                if(tzspec.length() >= 4)
                {
                    tz = sign * std::stoul(tzspec.substr(0, 2)) * 60;
                    if(tzspec.length() == 5 && tzspec.find(':') == 2)
                        tz += sign * std::stoul(tzspec.substr(3, 2));
                    else
                        tz += sign * std::stoul(tzspec.substr(2, 2));
                }
                else
                {
                    tz = sign * std::stoul(tzspec) * 60;
                }
                values[UTCTIME_TZ] = tz;
            }
        }

        if (!in.fail() && !in.bad()) {
            struct tm tm;

            tm.tm_year = values[UTCTIME_YEAR] - 1900;
            tm.tm_mon = values[UTCTIME_MON] - 1;
            tm.tm_mday = values[UTCTIME_DAY];
            tm.tm_hour = values[UTCTIME_HOUR];
            tm.tm_min = values[UTCTIME_MIN];
            tm.tm_sec = values[UTCTIME_SEC];
            tm.tm_isdst = 0;

            int64_t st = timegm( &tm );
            st += values[UTCTIME_TZ] * -60;
            t = st * CLOCK_FREQ;
            if(values[UTCTIME_FRAC_DEN] > 0)
                t += CLOCK_FREQ * values[UTCTIME_FRAC_NUM] / values[UTCTIME_FRAC_DEN];
        } else {
            // Failure parsing time string
            t = 0;
        }
    } catch(...) {
        t = 0;
    }
}

mtime_t UTCTime::mtime() const
{
    return t;
}
