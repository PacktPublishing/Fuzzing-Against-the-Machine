/*****************************************************************************
 * test.cpp
 *****************************************************************************
 * Copyright (C) 2018-2020 VideoLabs, VideoLAN and VLC Authors
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

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_demux.h>

#include "test.hpp"

#include <iostream>

extern const char vlc_module_name[] = "foobar";

#define TEST(func) []() { std::cerr << "Testing "#func << std::endl;\
                          return func##_test(); }()

int main()
{
    return
    TEST(Inheritables) ||
    TEST(SegmentBase) ||
    TEST(SegmentList) ||
    TEST(SegmentTemplate) ||
    TEST(Timeline) ||
    TEST(Conversions) ||
    TEST(TemplatedUri) ||
    TEST(BufferingLogic) ||
    TEST(CommandsQueue) ||
    TEST(M3U8MasterPlaylist) ||
    TEST(M3U8Playlist);
}
