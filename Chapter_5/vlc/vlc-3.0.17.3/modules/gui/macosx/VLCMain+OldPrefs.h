/*****************************************************************************
 * intf-prefs.h
 *****************************************************************************
 * Copyright (C) 2001-2015 VLC authors and VideoLAN
 * $Id: 281e2af453de03db0d7aad0efe4a9709d64db8c1 $
 *
 * Authors: Felix Paul Kühne <fkuehne at videolan dot org>
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

#import "VLCMain.h"

@interface VLCMain (OldPrefs)

- (void)resetAndReinitializeUserDefaults;
- (void)migrateOldPreferences;

@end
