/*****************************************************************************
 * VLCRemoteControlService.h: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2017, 2018, 2021 VLC authors and VideoLAN
 *
 * Authors: Carola Nitz <nitz.carola # gmail.com>
 *          Felix Paul Kühne <fkuehne # videolan.org>
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

#import <Cocoa/Cocoa.h>
#include <vlc_common.h>

@interface VLCRemoteControlService : NSObject

- (void)subscribeToRemoteCommands;
- (void)unsubscribeFromRemoteCommands;

- (void)playbackPositionUpdated;
- (void)playbackStateChangedTo:(int)state;
- (void)metaDataChangedForCurrentMediaItem:(input_item_t *)p_input_item;

@end
