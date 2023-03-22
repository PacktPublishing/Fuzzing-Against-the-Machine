/*****************************************************************************
 * VLCImageButton.h
 *****************************************************************************
 * Copyright (C) 2017 VLC authors and VideoLAN
 * $Id: 8bfa7a9b2c268d511542d6136ac22de7c860c379 $
 *
 * Authors: Cameron Mozie <camsw0rld14@gmail.com>
 *          Marvin Scholz <epirat07 at gmail dot com>
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

@interface VLCImageButton : NSButton

@property (nonatomic) IBInspectable BOOL toggle;

@end
