/*****************************************************************************
 * Spotify.h
 *****************************************************************************
 * Copyright (C) 2014 VLC authors and VideoLAN
 * $Id: dc7e7ecd9ac921c5007d27229b8909731338b224 $
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

#import <AppKit/AppKit.h>
#import <ScriptingBridge/ScriptingBridge.h>

OSType const kSpotifyPlayerStateStopped = 'kPSS';
OSType const kSpotifyPlayerStatePlaying = 'kPSP';
OSType const kSpotifyPlayerStatePaused  = 'kPSp';

@interface SpotifyApplication : SBApplication
@property (readonly) OSType playerState;  // is Spotify stopped, paused, or playing?
- (void)play;
- (void)pause;
@end
