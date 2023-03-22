/*****************************************************************************
 * VLCStringUtility.h: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2002-2014 VLC authors and VideoLAN
 * $Id: 4163d97c4b7f344a7ab7c097be4b61712025b7e0 $
 *
 * Authors: Jon Lech Johansen <jon-vl@nanocrew.net>
 *          Christophe Massiot <massiot@via.ecp.fr>
 *          Derk-Jan Hartman <hartman at videolan dot org>
 *          Felix Paul Kühne <fkuehne at videolan dot org>
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

#import <vlc_common.h>
#import <vlc_input.h>


#define _NS(s) ((s) ? toNSStr(vlc_gettext(s)) : @"")

/* Get an alternate version of the string.
 * This string is stored as '1:string' but when displayed it only displays
 * the translated string. the translation should be '1:translatedstring' though */
#define _ANS(s) [((s) ? toNSStr(vlc_gettext(s)) : @"") substringFromIndex:2]

#define B64DecNSStr(s) [[VLCStringUtility sharedInstance] b64Decode: s]
#define B64EncAndFree(s) [[VLCStringUtility sharedInstance] b64EncodeAndFree: s]

extern NSString *const kVLCMediaAudioCD;
extern NSString *const kVLCMediaDVD;
extern NSString *const kVLCMediaVCD;
extern NSString *const kVLCMediaSVCD;
extern NSString *const kVLCMediaBD;
extern NSString *const kVLCMediaVideoTSFolder;
extern NSString *const kVLCMediaBDMVFolder;
extern NSString *const kVLCMediaUnknown;

NSString *toNSStr(const char *str);
unsigned int CocoaKeyToVLC(unichar i_key);

/**
 * Gets the proper variant for an image resource,
 * depending on the os version.
 */
NSImage *imageFromRes(NSString *o_id);
NSImage *sidebarImageFromRes(NSString *o_id, BOOL darkMode);

@interface VLCStringUtility : NSObject

+ (VLCStringUtility *)sharedInstance;

- (NSString *)wrapString: (NSString *)o_in_string toWidth: (int)i_width;
- (NSString *)getCurrentTimeAsString:(input_thread_t *)p_input negative:(BOOL)b_negative;
- (NSString *)stringForTime:(long long int)time;

- (NSString *)OSXStringKeyToString:(NSString *)theString;
- (NSString *)VLCKeyToString:(NSString *)theString;
- (unsigned int)VLCModifiersToCocoa:(NSString *)theString;

- (NSString *)b64Decode:(NSString *)string;
- (NSString *)b64EncodeAndFree:(char *)psz_string;

- (NSString *)getVolumeTypeFromMountPath:(NSString *)mountPath;
- (NSString *)getBSDNodeFromMountPath:(NSString *)mountPath;

@end
