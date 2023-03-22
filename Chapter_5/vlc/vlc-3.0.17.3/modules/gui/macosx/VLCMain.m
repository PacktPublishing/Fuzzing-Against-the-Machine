/*****************************************************************************
 * VLCMain.m: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2002-2020 VLC authors and VideoLAN
 * $Id: 7f10c3978a3b3b2fa9ff2926577734bf137d93e1 $
 *
 * Authors: Derk-Jan Hartman <hartman at videolan.org>
 *          Felix Paul Kühne <fkuehne at videolan dot org>
 *          Pierre d'Herbemont <pdherbemont # videolan org>
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

/*****************************************************************************
 * Preamble
 *****************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#import "VLCMain.h"

#include <stdlib.h>                                      /* malloc(), free() */
#include <string.h>
#include <sys/sysctl.h>
#include <vlc_common.h>
#include <vlc_atomic.h>
#include <vlc_actions.h>
#include <vlc_dialog.h>
#include <vlc_url.h>
#include <vlc_variables.h>

#import "CompatibilityFixes.h"
#import "VLCInputManager.h"
#import "VLCMainMenu.h"
#import "VLCVoutView.h"
#import "prefs.h"
#import "VLCPlaylist.h"
#import "VLCPlaylistInfo.h"
#import "VLCPlaylistInfo.h"
#import "VLCOpenWindowController.h"
#import "VLCBookmarksWindowController.h"
#import "VLCCoreDialogProvider.h"
#import "VLCSimplePrefsController.h"
#import "VLCCoreInteraction.h"
#import "VLCTrackSynchronizationWindowController.h"
#import "VLCExtensionsManager.h"
#import "VLCResumeDialogController.h"
#import "VLCLogWindowController.h"
#import "VLCConvertAndSaveWindowController.h"

#import "VLCVideoEffectsWindowController.h"
#import "VLCAudioEffectsWindowController.h"
#import "VLCMain+OldPrefs.h"
#import "VLCApplication.h"

#ifdef HAVE_SPARKLE
#import <Sparkle/Sparkle.h>                 /* we're the update delegate */
NSString *const kIntel64UpdateURLString = @"https://update.videolan.org/vlc/sparkle/vlc-intel64.xml";
NSString *const kARM64UpdateURLString = @"https://update.videolan.org/vlc/sparkle/vlc-arm64.xml";
#endif

#pragma mark -
#pragma mark VLC Interface Object Callbacks

/*****************************************************************************
 * OpenIntf: initialize interface
 *****************************************************************************/

static intf_thread_t *p_interface_thread;

intf_thread_t *getIntf()
{
    return p_interface_thread;
}

int OpenIntf (vlc_object_t *p_this)
{
    @autoreleasepool {
        intf_thread_t *p_intf = (intf_thread_t*) p_this;
        p_interface_thread = p_intf;
        msg_Dbg(p_intf, "Starting macosx interface");

        @try {
            [VLCApplication sharedApplication];
            [VLCMain sharedInstance];

            if (@available(macOS 10.14, *)) {
                if (var_InheritBool(getIntf(), "macosx-interfacestyle")) {

                    // Use the native dark appearance style on Mojave
                    // Automatic switching between both styles does not work yet, see commit msg
                    NSApplication *app = [NSApplication sharedApplication];
                    app.appearance = [NSAppearance appearanceNamed: NSAppearanceNameDarkAqua];
                }
            }

            [NSBundle loadNibNamed:@"MainMenu" owner:[[VLCMain sharedInstance] mainMenu]];
            [[[VLCMain sharedInstance] mainWindow] makeKeyAndOrderFront:nil];

            msg_Dbg(p_intf, "Finished loading macosx interface");
            return VLC_SUCCESS;
        } @catch (NSException *exception) {
            msg_Err(p_intf, "Loading the macosx interface failed. Do you have a valid window server?");
            return VLC_EGENERIC;
        }
    }
}

void CloseIntf (vlc_object_t *p_this)
{
    @autoreleasepool {
        msg_Dbg(p_this, "Closing macosx interface");
        [[VLCMain sharedInstance] applicationWillTerminate:nil];
        [VLCMain killInstance];
    }

    p_interface_thread = nil;
}

#pragma mark -
#pragma mark Variables Callback

/*****************************************************************************
 * ShowController: Callback triggered by the show-intf playlist variable
 * through the ShowIntf-control-intf, to let us show the controller-win;
 * usually when in fullscreen-mode
 *****************************************************************************/
static int ShowController(vlc_object_t *p_this, const char *psz_variable,
                     vlc_value_t old_val, vlc_value_t new_val, void *param)
{
    @autoreleasepool {
        dispatch_async(dispatch_get_main_queue(), ^{

            intf_thread_t * p_intf = getIntf();
            if (p_intf) {
                playlist_t * p_playlist = pl_Get(p_intf);
                BOOL b_fullscreen = var_GetBool(p_playlist, "fullscreen");
                if (b_fullscreen)
                    [[VLCMain sharedInstance] showFullscreenController];

                else if (!strcmp(psz_variable, "intf-show"))
                    [[[VLCMain sharedInstance] mainWindow] makeKeyAndOrderFront:nil];
            }

        });

        return VLC_SUCCESS;
    }
}

#pragma mark -
#pragma mark Private

@interface VLCMain ()
#ifdef HAVE_SPARKLE
    <SUUpdaterDelegate>
#endif
{
    intf_thread_t *p_intf;
    BOOL launched;

    BOOL b_active_videoplayback;

    NSWindowController *_mainWindowController;
    VLCMainMenu *_mainmenu;
    VLCPrefs *_prefs;
    VLCSimplePrefsController *_sprefs;
    VLCOpenWindowController *_open;
    VLCCoreDialogProvider *_coredialogs;
    VLCBookmarksWindowController *_bookmarks;
    VLCCoreInteraction *_coreinteraction;
    VLCResumeDialogController *_resume_dialog;
    VLCInputManager *_input_manager;
    VLCPlaylist *_playlist;
    VLCLogWindowController *_messagePanelController;
    VLCStatusBarIcon *_statusBarIcon;
    VLCTrackSynchronizationWindowController *_trackSyncPanel;
    VLCAudioEffectsWindowController *_audioEffectsPanel;
    VLCVideoEffectsWindowController *_videoEffectsPanel;
    VLCConvertAndSaveWindowController *_convertAndSaveWindow;
    VLCExtensionsManager *_extensionsManager;
    VLCInfo *_currentMediaInfoPanel;

    bool b_intf_terminating; /* Makes sure applicationWillTerminate will be called only once */
}

@end

/*****************************************************************************
 * VLCMain implementation
 *****************************************************************************/
@implementation VLCMain

#pragma mark -
#pragma mark Initialization

static VLCMain *sharedInstance = nil;

+ (VLCMain *)sharedInstance;
{
    static dispatch_once_t pred;
    dispatch_once(&pred, ^{
        sharedInstance = [[VLCMain alloc] init];
    });

    return sharedInstance;
}

+ (void)killInstance
{
    sharedInstance = nil;
}

- (id)init
{
    self = [super init];
    if (self) {
        p_intf = getIntf();

        [VLCApplication sharedApplication].delegate = self;

        _input_manager = [[VLCInputManager alloc] initWithMain:self];

        // first initalize extensions dialog provider, then core dialog
        // provider which will register both at the core
        _extensionsManager = [[VLCExtensionsManager alloc] init];
        _coredialogs = [[VLCCoreDialogProvider alloc] init];

        _mainmenu = [[VLCMainMenu alloc] init];
        _statusBarIcon = [[VLCStatusBarIcon  alloc] init];

        _voutController = [[VLCVoutWindowController alloc] init];
        _playlist = [[VLCPlaylist alloc] init];

        _mainWindowController = [[NSWindowController alloc] initWithWindowNibName:@"MainWindow"];

        var_AddCallback(p_intf->obj.libvlc, "intf-toggle-fscontrol", ShowController, (__bridge void *)self);
        var_AddCallback(p_intf->obj.libvlc, "intf-show", ShowController, (__bridge void *)self);

        // Load them here already to apply stored profiles
        _videoEffectsPanel = [[VLCVideoEffectsWindowController alloc] init];
        _audioEffectsPanel = [[VLCAudioEffectsWindowController alloc] init];

        playlist_t *p_playlist = pl_Get(p_intf);
        if ([NSApp currentSystemPresentationOptions] & NSApplicationPresentationFullScreen)
            var_SetBool(p_playlist, "fullscreen", YES);

        _nativeFullscreenMode = var_InheritBool(p_intf, "macosx-nativefullscreenmode");

        if (var_InheritInteger(p_intf, "macosx-icon-change")) {
            /* After day 354 of the year, the usual VLC cone is replaced by another cone
             * wearing a Father Xmas hat.
             * Note: this icon doesn't represent an endorsement of The Coca-Cola Company.
             */
            NSCalendar *gregorian =
            [[NSCalendar alloc] initWithCalendarIdentifier:NSGregorianCalendar];
            NSUInteger dayOfYear = [gregorian ordinalityOfUnit:NSDayCalendarUnit inUnit:NSYearCalendarUnit forDate:[NSDate date]];

            if (dayOfYear >= 354)
                [[VLCApplication sharedApplication] setApplicationIconImage: [NSImage imageNamed:@"VLC-Xmas"]];
            else
                [[VLCApplication sharedApplication] setApplicationIconImage: [NSImage imageNamed:@"VLC"]];
        }
    }

    return self;
}

- (void)dealloc
{
    msg_Dbg(getIntf(), "Deinitializing VLCMain object");
}

- (void)applicationWillFinishLaunching:(NSNotification *)aNotification
{
    _coreinteraction = [VLCCoreInteraction sharedInstance];

#ifdef HAVE_SPARKLE
    [[SUUpdater sharedUpdater] setDelegate:self];
#endif
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    launched = YES;

    if (!p_intf)
        return;

    [_coreinteraction updateCurrentlyUsedHotkeys];

    [self migrateOldPreferences];

    /* Handle sleep notification */
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(computerWillSleep:)
           name:NSWorkspaceWillSleepNotification object:nil];

    /* update the main window */
    [[self mainWindow] updateWindow];
    [[self mainWindow] updateTimeSlider];
    [[self mainWindow] updateVolumeSlider];

    // respect playlist-autostart
    // note that PLAYLIST_PLAY will not stop any playback if already started
    playlist_t * p_playlist = pl_Get(getIntf());
    PL_LOCK;
    BOOL kidsAround = p_playlist->p_playing->i_children != 0;
    if (kidsAround && var_GetBool(p_playlist, "playlist-autostart"))
        playlist_Control(p_playlist, PLAYLIST_PLAY, true);
    PL_UNLOCK;

    /* on macOS 11 and later, check whether the user attempts to deploy
     * the x86_64 binary on ARM-64 - if yes, log it */
    if (OSX_BIGSUR_AND_HIGHER) {
        if ([self processIsTranslated] > 0) {
            msg_Warn(p_intf, "Process is translated!");
        }
    }
}

- (int)processIsTranslated
{
   int ret = 0;
   size_t size = sizeof(ret);
   if (sysctlbyname("sysctl.proc_translated", &ret, &size, NULL, 0) == -1) {
      if (errno == ENOENT)
         return 0;
      return -1;
   }
   return ret;
}

#pragma mark -
#pragma mark Termination

- (BOOL)isTerminating
{
    return b_intf_terminating;
}

- (void)applicationWillTerminate:(NSNotification *)notification
{
    msg_Dbg(getIntf(), "applicationWillTerminate called");
    if (b_intf_terminating)
        return;
    b_intf_terminating = true;

    [_input_manager onPlaybackHasEnded:nil];
    [_input_manager deinit];

    if (notification == nil)
        [[NSNotificationCenter defaultCenter] postNotificationName: NSApplicationWillTerminateNotification object: nil];

    playlist_t * p_playlist = pl_Get(p_intf);

    /* save current video and audio profiles */
    [[self videoEffectsPanel] saveCurrentProfileAtTerminate];
    [[self audioEffectsPanel] saveCurrentProfileAtTerminate];

    /* Save some interface state in configuration, at module quit */
    config_PutInt(p_intf, "random", var_GetBool(p_playlist, "random"));
    config_PutInt(p_intf, "loop", var_GetBool(p_playlist, "loop"));
    config_PutInt(p_intf, "repeat", var_GetBool(p_playlist, "repeat"));

    var_DelCallback(p_intf->obj.libvlc, "intf-toggle-fscontrol", ShowController, (__bridge void *)self);
    var_DelCallback(p_intf->obj.libvlc, "intf-show", ShowController, (__bridge void *)self);

    [[NSNotificationCenter defaultCenter] removeObserver: self];

    // closes all open vouts
    _voutController = nil;

    /* write cached user defaults to disk */
    [[NSUserDefaults standardUserDefaults] synchronize];
}

#pragma mark -
#pragma mark Sparkle delegate

#ifdef HAVE_SPARKLE
/* received directly before the update gets installed, so let's shut down a bit */
- (void)updater:(SUUpdater *)updater willInstallUpdate:(SUAppcastItem *)update
{
    [NSApp activateIgnoringOtherApps:YES];
    [_coreinteraction stopListeningWithAppleRemote];
    [_coreinteraction stop];
}

/* don't be enthusiastic about an update if we currently play a video */
- (BOOL)updaterMayCheckForUpdates:(SUUpdater *)bundle
{
    if ([self activeVideoPlayback])
        return NO;

    return YES;
}

/* use the correct feed depending on the hardware architecture */
- (nullable NSString *)feedURLStringForUpdater:(SUUpdater *)updater
{
#ifdef __x86_64__
    if (OSX_BIGSUR_AND_HIGHER) {
        if ([self processIsTranslated] > 0) {
            msg_Dbg(p_intf, "Process is translated. On update, VLC will install the native ARM-64 binary.");
            return kARM64UpdateURLString;
        }
    }
    return kIntel64UpdateURLString;
#elif __arm64__
    return kARM64UpdateURLString;
#else
    #error unsupported architecture
#endif
}

- (void)updaterDidNotFindUpdate:(SUUpdater *)updater
{
    msg_Dbg(p_intf, "No update found");
}

- (void)updater:(SUUpdater *)updater failedToDownloadUpdate:(SUAppcastItem *)item error:(NSError *)error
{
    msg_Warn(p_intf, "Failed to download update with error %li", error.code);
}

- (void)updater:(SUUpdater *)updater didAbortWithError:(NSError *)error
{
    msg_Err(p_intf, "Updater aborted with error %li", error.code);
}
#endif

#pragma mark -
#pragma mark Other notification

/* Listen to the remote in exclusive mode, only when VLC is the active
   application */
- (void)applicationDidBecomeActive:(NSNotification *)aNotification
{
    if (!p_intf)
        return;
    if (var_InheritBool(p_intf, "macosx-appleremote") == YES)
        [_coreinteraction startListeningWithAppleRemote];
}
- (void)applicationDidResignActive:(NSNotification *)aNotification
{
    if (!p_intf)
        return;
    [_coreinteraction stopListeningWithAppleRemote];
}

/* Triggered when the computer goes to sleep */
- (void)computerWillSleep: (NSNotification *)notification
{
    [_coreinteraction pause];
}

#pragma mark -
#pragma mark File opening over dock icon

- (void)application:(NSApplication *)o_app openFiles:(NSArray *)o_names
{
    // Only add items here which are getting dropped to to the application icon
    // or are given at startup. If a file is passed via command line, libvlccore
    // will add the item, but cocoa also calls this function. In this case, the
    // invocation is ignored here.
    NSArray *resultItems = o_names;
    if (launched == NO) {
        NSArray *launchArgs = [[NSProcessInfo processInfo] arguments];

        if (launchArgs) {
            NSSet *launchArgsSet = [NSSet setWithArray:launchArgs];
            NSMutableSet *itemSet = [NSMutableSet setWithArray:o_names];
            [itemSet minusSet:launchArgsSet];
            resultItems = [itemSet allObjects];
        }
    }

    NSArray *o_sorted_names = [resultItems sortedArrayUsingSelector: @selector(caseInsensitiveCompare:)];
    NSMutableArray *o_result = [NSMutableArray arrayWithCapacity: [o_sorted_names count]];
    for (NSUInteger i = 0; i < [o_sorted_names count]; i++) {
        char *psz_uri = vlc_path2uri([[o_sorted_names objectAtIndex:i] UTF8String], "file");
        if (!psz_uri)
            continue;

        NSDictionary *o_dic = [NSDictionary dictionaryWithObject:toNSStr(psz_uri) forKey:@"ITEM_URL"];
        [o_result addObject: o_dic];
        free(psz_uri);
    }

    [[[VLCMain sharedInstance] playlist] addPlaylistItems:o_result tryAsSubtitle:YES];
}

/* When user click in the Dock icon our double click in the finder */
- (BOOL)applicationShouldHandleReopen:(NSApplication *)theApplication hasVisibleWindows:(BOOL)hasVisibleWindows
{
    if (!hasVisibleWindows)
        [[self mainWindow] makeKeyAndOrderFront:self];

    return YES;
}

- (void)showFullscreenController
{
    // defer selector here (possibly another time) to ensure that keyWindow is set properly
    // (needed for NSApplicationDidBecomeActiveNotification)
    [[self mainWindow] performSelectorOnMainThread:@selector(showFullscreenController) withObject:nil waitUntilDone:NO];
}

- (void)setActiveVideoPlayback:(BOOL)b_value
{
    assert([NSThread isMainThread]);

    b_active_videoplayback = b_value;
    if ([self mainWindow]) {
        [[self mainWindow] setVideoplayEnabled];
    }

    // update sleep blockers
    [_input_manager playbackStatusUpdated];
}

#pragma mark -
#pragma mark Other objects getters

- (VLCMainMenu *)mainMenu
{
    return _mainmenu;
}

- (VLCStatusBarIcon *)statusBarIcon
{
    return _statusBarIcon;
}

- (VLCMainWindow *)mainWindow
{
    return (VLCMainWindow *)[_mainWindowController window];
}

- (VLCInputManager *)inputManager
{
    return _input_manager;
}

- (VLCExtensionsManager *)extensionsManager
{
    return _extensionsManager;
}

- (VLCLogWindowController *)debugMsgPanel
{
    if (!_messagePanelController)
        _messagePanelController = [[VLCLogWindowController alloc] init];

    return _messagePanelController;
}

- (VLCTrackSynchronizationWindowController *)trackSyncPanel
{
    if (!_trackSyncPanel)
        _trackSyncPanel = [[VLCTrackSynchronizationWindowController alloc] init];

    return _trackSyncPanel;
}

- (VLCAudioEffectsWindowController *)audioEffectsPanel
{
    return _audioEffectsPanel;
}

- (VLCVideoEffectsWindowController *)videoEffectsPanel
{
    return _videoEffectsPanel;
}

- (VLCInfo *)currentMediaInfoPanel;
{
    if (!_currentMediaInfoPanel)
        _currentMediaInfoPanel = [[VLCInfo alloc] init];

    return _currentMediaInfoPanel;
}

- (VLCBookmarksWindowController *)bookmarks
{
    if (!_bookmarks)
        _bookmarks = [[VLCBookmarksWindowController alloc] init];

    return _bookmarks;
}

- (VLCOpenWindowController *)open
{
    if (!_open)
        _open = [[VLCOpenWindowController alloc] init];

    return _open;
}

- (VLCConvertAndSaveWindowController *)convertAndSaveWindow
{
    if (_convertAndSaveWindow == nil)
        _convertAndSaveWindow = [[VLCConvertAndSaveWindowController alloc] init];

    return _convertAndSaveWindow;
}

- (VLCSimplePrefsController *)simplePreferences
{
    if (!_sprefs)
        _sprefs = [[VLCSimplePrefsController alloc] init];

    return _sprefs;
}

- (VLCPrefs *)preferences
{
    if (!_prefs)
        _prefs = [[VLCPrefs alloc] init];

    return _prefs;
}

- (VLCPlaylist *)playlist
{
    return _playlist;
}

- (VLCCoreDialogProvider *)coreDialogProvider
{
    return _coredialogs;
}

- (VLCResumeDialogController *)resumeDialog
{
    if (!_resume_dialog)
        _resume_dialog = [[VLCResumeDialogController alloc] init];

    return _resume_dialog;
}

- (BOOL)activeVideoPlayback
{
    return b_active_videoplayback;
}

@end
