/*****************************************************************************
 * CompatibilityFixes.h: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2011-2020 VLC authors and VideoLAN
 * $Id: 479646abb60a0f5f892e1e11e2c445264bdc574a $
 *
 * Authors: Felix Paul Kühne <fkuehne -at- videolan -dot- org>
 *          Marvin Scholz <epirat07 -at- gmail -dot- com>
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

#import <AvailabilityMacros.h>
#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

#pragma mark -
#pragma OS detection code
#define OSX_LION_AND_HIGHER (NSAppKitVersionNumber >= 1115.2)
#define OSX_MOUNTAIN_LION_AND_HIGHER (NSAppKitVersionNumber >= 1162)
#define OSX_MAVERICKS_AND_HIGHER (NSAppKitVersionNumber >= 1244)
#define OSX_YOSEMITE_AND_HIGHER (NSAppKitVersionNumber >= 1334)
#define OSX_EL_CAPITAN_AND_HIGHER (NSAppKitVersionNumber >= 1404)
#define OSX_SIERRA_AND_HIGHER (NSAppKitVersionNumber >= 1485)
#define OSX_HIGH_SIERRA_AND_HIGHER (NSAppKitVersionNumber >= 1560)
#define OSX_MOJAVE_AND_HIGHER (NSAppKitVersionNumber >= 1639.10)
#define OSX_CATALINA_AND_HIGHER (NSAppKitVersionNumber >= 1894.00)
#define OSX_BIGSUR_AND_HIGHER (NSAppKitVersionNumber >= 2022.00)

// Sierra only APIs
#ifndef MAC_OS_X_VERSION_10_12

typedef NS_OPTIONS(NSUInteger, NSStatusItemBehavior) {

    NSStatusItemBehaviorRemovalAllowed = (1 << 1),
    NSStatusItemBehaviorTerminationOnRemoval = (1 << 2),
};

@interface NSStatusItem(IntroducedInSierra)

@property (assign) NSStatusItemBehavior behavior;
@property (assign, getter=isVisible) BOOL visible;
@property (null_resettable, copy) NSString *autosaveName;

@end

typedef NSUInteger NSWindowStyleMask;

#endif

void swapoutOverride(Class _Nonnull cls, SEL _Nonnull selector);

@interface NSColor(VLCAdditions)

@property(class, strong, readonly) NSColor * _Nonnull VLCSecondaryLabelColor;

@end

#ifndef MAC_OS_X_VERSION_10_14

extern NSString *const NSAppearanceNameDarkAqua;

@interface NSApplication (NSAppearanceCustomization)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunguarded-availability"
@property (nullable, strong) NSAppearance *appearance;
@property (readonly, strong) NSAppearance *effectiveAppearance;
#pragma clang diagnostic pop
@end

#endif

#ifndef MAC_OS_VERSION_12_0
@interface NSScreen (IntroducedInMonterey)
@property (readonly) NSEdgeInsets safeAreaInsets;
@end
#endif

NS_ASSUME_NONNULL_END
