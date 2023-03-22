/*****************************************************************************
 * VLCAboutWindowController.h
 *****************************************************************************
 * Copyright (C) 2001-2013 VLC authors and VideoLAN
 * $Id: ff0ed5f6a116911da4b53a0bfd3901ee41deeb74 $
 *
 * Authors: Derk-Jan Hartman <thedj@users.sourceforge.net>
 *          Felix Paul Kühne <fkuehne -at- videolan.org>
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

@interface VLCAboutWindowController : NSWindowController<NSWindowDelegate>
{
    /* main about panel and stuff related to its views */
    IBOutlet id o_name_version_field;
    IBOutlet id o_revision_field;
    IBOutlet id o_copyright_field;
    IBOutlet id o_credits_textview;
    IBOutlet id o_credits_scrollview;
    IBOutlet id o_gpl_btn;
    IBOutlet id o_credits_btn;
    IBOutlet id o_authors_btn;
    IBOutlet id o_name_field;
    IBOutlet id o_icon_view;
    IBOutlet id o_joinus_txt;
    IBOutlet id o_trademarks_txt;
}

- (void)showAbout;
- (void)showGPL;
- (IBAction)buttonAction:(id)sender;

@end
