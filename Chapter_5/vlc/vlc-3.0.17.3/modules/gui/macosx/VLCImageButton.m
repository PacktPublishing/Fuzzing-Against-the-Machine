/*****************************************************************************
 * VLCImageButton.m
 *****************************************************************************
 * Copyright (C) 2017 VLC authors and VideoLAN
 * $Id: d891b25ad53d41d99d155888d067bf0d89938ff5 $
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

#import "VLCImageButton.h"

@implementation VLCImageButton

@synthesize toggle = _toggle;

- (instancetype)initWithCoder:(NSCoder *)coder
{
    self =  [super initWithCoder:coder];
    if (self) {
        _toggle = NO;
    }
    return self;
}

- (void)awakeFromNib
{
    [(NSButtonCell*)[self cell] setHighlightsBy:NSPushInCellMask];
    [(NSButtonCell*)[self cell] setShowsStateBy:(_toggle) ? NSContentsCellMask : NSNoCellMask];
}

- (void)setToggle:(BOOL)toggle
{
    _toggle = toggle;
    [(NSButtonCell*)[self cell] setShowsStateBy:(_toggle) ? NSContentsCellMask : NSNoCellMask];
}

@end
