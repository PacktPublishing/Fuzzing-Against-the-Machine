/*****************************************************************************
* VLCSidebarDataSource.h: MacOS X interface module
*****************************************************************************
* Copyright (C) 2021 VLC authors and VideoLAN
* $Id: 4b451a36ceb640a5b81cd26f929317ad3da36afd $
*
* Authors: Felix Paul Kühne <fkuehne -at- videolan -dot- org>
*          Jon Lech Johansen <jon-vl@nanocrew.net>
*          Christophe Massiot <massiot@via.ecp.fr>
*          Derk-Jan Hartman <hartman at videolan.org>
*          David Fuhrmann <david dot fuhrmann at googlemail dot com>
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

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class PXSourceList;

@interface VLCSidebarDataSource : NSObject

@property (readwrite, weak) PXSourceList *sidebarView;

- (void)reloadSidebar;

@end

NS_ASSUME_NONNULL_END
