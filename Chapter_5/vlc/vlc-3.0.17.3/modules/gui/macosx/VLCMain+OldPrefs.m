/*****************************************************************************
 * intf-prefs.m
 *****************************************************************************
 * Copyright (C) 2001-2015 VLC authors and VideoLAN
 * $Id: a99ca3d8796040d9bb37265cfbfcd884e1314344 $
 *
 * Authors: Pierre d'Herbemont <pdherbemont # videolan org>
 *          Felix Paul Kühne <fkuehne at videolan dot org>
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

#import "VLCMain+OldPrefs.h"
#import "VLCCoreInteraction.h"
#import "VLCSimplePrefsController.h"

#include <unistd.h> /* execl() */

#import <vlc_interface.h>

@implementation VLCMain(OldPrefs)

static NSString * kVLCPreferencesVersion = @"VLCPreferencesVersion";
static const int kCurrentPreferencesVersion = 4;

+ (void)initialize
{
    NSDictionary *appDefaults = [NSDictionary dictionaryWithObject:[NSNumber numberWithInt:kCurrentPreferencesVersion]
                                                            forKey:kVLCPreferencesVersion];

    [[NSUserDefaults standardUserDefaults] registerDefaults:appDefaults];
}

- (void)resetAndReinitializeUserDefaults
{
    // note that [NSUserDefaults resetStandardUserDefaults] will NOT correctly reset to the defaults

    NSString *appDomain = [[NSBundle mainBundle] bundleIdentifier];
    [[NSUserDefaults standardUserDefaults] removePersistentDomainForName:appDomain];

    // set correct version to avoid question about outdated config
    [[NSUserDefaults standardUserDefaults] setInteger:kCurrentPreferencesVersion forKey:kVLCPreferencesVersion];
    [[NSUserDefaults standardUserDefaults] synchronize];
}

- (void)migrateOldPreferences
{
    NSUserDefaults * defaults = [NSUserDefaults standardUserDefaults];
    int version = [defaults integerForKey:kVLCPreferencesVersion];

    // This was set in user defaults in VLC 2.0.x (preferences version 2), overriding any
    // value in the Info.plist. Make sure to delete it here, always,
    // as it could be set if an old version of VLC is launched again.
    [defaults removeObjectForKey:@"SUFeedURL"];

    /*
     * Store version explicitely in file, for ease of debugging.
     * Otherwise, the value will be just defined at app startup,
     * as initialized above.
     */
    [defaults setInteger:version forKey:kVLCPreferencesVersion];
    if (version >= kCurrentPreferencesVersion)
        return;

    if (version == 1) {
        [defaults setInteger:kCurrentPreferencesVersion forKey:kVLCPreferencesVersion];
        [defaults synchronize];

        if (![[VLCCoreInteraction sharedInstance] fixIntfSettings])
            return;
        else
            config_SaveConfigFile(getIntf()); // we need to do manually, since we won't quit libvlc cleanly
    } else if (version == 2) {
        /* version 2 (used by VLC 2.0.x and early versions of 2.1) can lead to exceptions within 2.1 or later
         * so we reset the OS X specific prefs here - in practice, no user will notice */
        [self resetAndReinitializeUserDefaults];

    } else if (version == 3) {
        /* version 4 (introduced in 3.0.0) adds RTL settings depending on stored language */
        [defaults setInteger:kCurrentPreferencesVersion forKey:kVLCPreferencesVersion];
        BOOL hasUpdated = [VLCSimplePrefsController updateRightToLeftSettings];
        [defaults synchronize];

        // In VLC 2.2.x, config for filters was fully controlled by audio and video effects panel.
        // In VLC 3.0, this is no longer the case and VLCs config is not touched anymore. Therefore,
        // disable filter in VLCs config in this transition.
        playlist_t *p_playlist = pl_Get(getIntf());
        var_SetString(p_playlist, "audio-filter", "");
        var_SetString(p_playlist, "video-filter", "");

        config_PutPsz(getIntf(), "audio-filter", "");
        config_PutPsz(getIntf(), "video-filter", "");
        config_SaveConfigFile(getIntf());

        // This migration only has effect rarely, therefore only restart then
        if (!hasUpdated)
            return;

    } else {
        NSArray *libraries = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory,
                                                                 NSUserDomainMask, YES);
        if (!libraries || [libraries count] == 0) return;
        NSString * preferences = [[libraries firstObject] stringByAppendingPathComponent:@"Preferences"];

        int res = NSRunInformationalAlertPanel(_NS("Remove old preferences?"),
                                               _NS("We just found an older version of VLC's preferences files."),
                                               _NS("Move To Trash and Relaunch VLC"), _NS("Ignore"), nil, nil);
        if (res != NSOKButton) {
            [defaults setInteger:kCurrentPreferencesVersion forKey:kVLCPreferencesVersion];
            return;
        }

        // Do NOT add the current plist file here as this would conflict with caching.
        // Instead, just reset below.
        NSArray * ourPreferences = [NSArray arrayWithObjects:@"org.videolan.vlc", @"VLC", nil];

        /* Move the file to trash one by one. Using above array the method would stop after first file
         not found. */
        for (NSString *file in ourPreferences) {
            [[NSWorkspace sharedWorkspace] performFileOperation:NSWorkspaceRecycleOperation source:preferences destination:@"" files:[NSArray arrayWithObject:file] tag:nil];
        }

        [self resetAndReinitializeUserDefaults];
    }

    /* Relaunch now */
    const char * path = [[[NSBundle mainBundle] executablePath] UTF8String];

    /* For some reason we need to fork(), not just execl(), which reports a ENOTSUP then. */
    if (fork() != 0) {
        exit(0);
    }
    execl(path, path, NULL);
}

@end