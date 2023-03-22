/*****************************************************************************
 * VLCMainWindow.m: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2002-2018 VLC authors and VideoLAN
 * $Id: 36f0748516924207296cce5f7268d0bbc068fb48 $
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

#import "VLCMainWindow.h"

#import "VLCMain.h"
#import "CompatibilityFixes.h"
#import "VLCCoreInteraction.h"
#import "VLCAudioEffectsWindowController.h"
#import "VLCMainMenu.h"
#import "VLCOpenWindowController.h"
#import "VLCPlaylist.h"
#import "VLCSidebarDataSource.h"
#import "VLCSourceListItem.h"

#import <math.h>
#import <vlc_playlist.h>
#import <vlc_url.h>
#import <vlc_strings.h>
#import "VLCPLModel.h"

#import "PXSourceList/PXSourceList.h"

#import "VLCMainWindowControlsBar.h"
#import "VLCVoutView.h"
#import "VLCVoutWindowController.h"


@interface VLCMainWindow() <NSWindowDelegate, NSAnimationDelegate, NSSplitViewDelegate>
{
    BOOL videoPlaybackEnabled;
    BOOL dropzoneActive;
    BOOL splitViewRemoved;
    BOOL minimizedView;

    BOOL b_video_playback_enabled;
    BOOL b_dropzone_active;
    BOOL b_splitview_removed;
    BOOL b_minimized_view;

    CGFloat f_lastSplitViewHeight;
    CGFloat f_lastLeftSplitViewWidth;

    /* this is only true, when we have NO video playing inside the main window */

    BOOL b_podcastView_displayed;

    NSRect frameBeforePlayback;
}
- (void)makeSplitViewVisible;
- (void)makeSplitViewHidden;
- (void)showPodcastControls;
- (void)hidePodcastControls;
@end

static const float f_min_window_height = 307.;

@implementation VLCMainWindow

#pragma mark -
#pragma mark Initialization

- (BOOL)isEvent:(NSEvent *)o_event forKey:(const char *)keyString
{
    char *key;
    NSString *o_key;

    key = config_GetPsz(getIntf(), keyString);
    o_key = [NSString stringWithFormat:@"%s", key];
    FREENULL(key);

    unsigned int i_keyModifiers = [[VLCStringUtility sharedInstance] VLCModifiersToCocoa:o_key];

    NSString * characters = [o_event charactersIgnoringModifiers];
    if ([characters length] > 0) {
        return [[characters lowercaseString] isEqualToString: [[VLCStringUtility sharedInstance] VLCKeyToString: o_key]] &&
                (i_keyModifiers & NSShiftKeyMask)     == ([o_event modifierFlags] & NSShiftKeyMask) &&
                (i_keyModifiers & NSControlKeyMask)   == ([o_event modifierFlags] & NSControlKeyMask) &&
                (i_keyModifiers & NSAlternateKeyMask) == ([o_event modifierFlags] & NSAlternateKeyMask) &&
                (i_keyModifiers & NSCommandKeyMask)   == ([o_event modifierFlags] & NSCommandKeyMask);
    }
    return NO;
}

- (BOOL)performKeyEquivalent:(NSEvent *)o_event
{
    BOOL b_force = NO;
    // these are key events which should be handled by vlc core, but are attached to a main menu item
    if (![self isEvent: o_event forKey: "key-vol-up"] &&
        ![self isEvent: o_event forKey: "key-vol-down"] &&
        ![self isEvent: o_event forKey: "key-vol-mute"] &&
        ![self isEvent: o_event forKey: "key-prev"] &&
        ![self isEvent: o_event forKey: "key-next"] &&
        ![self isEvent: o_event forKey: "key-jump+short"] &&
        ![self isEvent: o_event forKey: "key-jump-short"]) {
        /* We indeed want to prioritize some Cocoa key equivalent against libvlc,
         so we perform the menu equivalent now. */
        if ([[NSApp mainMenu] performKeyEquivalent:o_event])
            return TRUE;
    }
    else
        b_force = YES;

    VLCCoreInteraction *coreInteraction = [VLCCoreInteraction sharedInstance];
    return [coreInteraction hasDefinedShortcutKey:o_event force:b_force] ||
           [coreInteraction keyEvent:o_event];
}

- (void)dealloc
{
    msg_Dbg(getIntf(), "Deinitializing VLCMainWindow object");

    [[NSNotificationCenter defaultCenter] removeObserver: self];
    if (@available(macOS 10_14, *)) {
        [[NSApplication sharedApplication] removeObserver:self forKeyPath:@"effectiveAppearance"];
    }
}

- (void)awakeFromNib
{
    [super awakeFromNib];

    /*
     * General setup
     */

    NSNotificationCenter *defaultCenter = [NSNotificationCenter defaultCenter];
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];

    BOOL splitViewShouldBeHidden = NO;

    [self setDelegate:self];
    [self setRestorable:NO];
    // TODO: useOptimizedDrawing deprecated since 10.10, but no
    // documentation provided what do use instead.
    // see radar://23047516
    [self useOptimizedDrawing:YES];
    [self setExcludedFromWindowsMenu:YES];
    [self setAcceptsMouseMovedEvents:YES];
    [self setFrameAutosaveName:@"mainwindow"];

    _nativeFullscreenMode = var_InheritBool(getIntf(), "macosx-nativefullscreenmode");
    b_dropzone_active = YES;

    // Playlist setup
    VLCPlaylist *playlist = [[VLCMain sharedInstance] playlist];
    [playlist setOutlineView:(VLCPlaylistView *)_outlineView];
    [playlist setPlaylistHeaderView:_outlineView.headerView];
    [self setNextResponder:playlist];

    // (Re)load sidebar for the first time and select first item
    self.sidebarDataSource = [[VLCSidebarDataSource alloc] init];
    self.sidebarDataSource.sidebarView = self.sidebarView;

    [self.sidebarDataSource reloadSidebar];
    [_sidebarView setAutosaveName:@"mainwindow-sidebar"];
    [_sidebarView setAutosaveExpandedItems:YES];

    [_sidebarView selectRowIndexes:[NSIndexSet indexSetWithIndex:1] byExtendingSelection:NO];

    /*
     * Set up translatable strings for the UI elements
     */

    // Window title
    [self setTitle:_NS("VLC media player")];

    // Search Field
    [_searchField setToolTip:_NS("Search in Playlist")];
    [_searchField.cell setPlaceholderString:_NS("Search")];
    [_searchField.cell accessibilitySetOverrideValue:_NS("Search the playlist. Results will be selected in the table.")
                                        forAttribute:NSAccessibilityDescriptionAttribute];

    // Dropzone
    [_dropzoneLabel setStringValue:_NS("Drop media here")];
    if (@available(macOS 10.14, *)) {
        NSApplication *app = [NSApplication sharedApplication];
        if ([app.effectiveAppearance.name isEqualToString:NSAppearanceNameDarkAqua]) {
            [_dropzoneImageView setImage:[NSImage imageNamed:@"mj-dropzone-dark"]];
        } else {
            [_dropzoneImageView setImage:imageFromRes(@"dropzone")];
        }
        [app addObserver:self
              forKeyPath:@"effectiveAppearance"
                 options:0
                 context:nil];
        self.dropzoneBackgroundImageView.hidden = YES;
    } else {
        [_dropzoneImageView setImage:imageFromRes(@"dropzone")];
    }
    [_dropzoneButton setTitle:_NS("Open media...")];
    [_dropzoneButton.cell accessibilitySetOverrideValue:_NS("Open a dialog to select the media to play")
                                           forAttribute:NSAccessibilityDescriptionAttribute];

    // Podcast view
    [_podcastAddButton setTitle:_NS("Subscribe")];
    [_podcastRemoveButton setTitle:_NS("Unsubscribe")];

    // Podcast subscribe window
    [_podcastSubscribeTitle setStringValue:_NS("Subscribe to a podcast")];
    [_podcastSubscribeSubtitle setStringValue:_NS("Enter URL of the podcast to subscribe to:")];
    [_podcastSubscribeOkButton setTitle:_NS("Subscribe")];
    [_podcastSubscribeCancelButton setTitle:_NS("Cancel")];

    // Podcast unsubscribe window
    [_podcastUnsubscirbeTitle setStringValue:_NS("Unsubscribe from a podcast")];
    [_podcastUnsubscribeSubtitle setStringValue:_NS("Select the podcast you would like to unsubscribe from:")];
    [_podcastUnsubscribeOkButton setTitle:_NS("Unsubscribe")];
    [_podcastUnsubscribeCancelButton setTitle:_NS("Cancel")];

    /* interface builder action */
    CGFloat f_threshold_height = f_min_video_height + [self.controlsBar height];
    if (self.darkInterface)
        f_threshold_height += [self.titlebarView frame].size.height;
    if ([[self contentView] frame].size.height < f_threshold_height)
        splitViewShouldBeHidden = YES;

    // Set that here as IB seems to be buggy
    if (self.darkInterface)
        [self setContentMinSize:NSMakeSize(604., f_min_window_height + [self.titlebarView frame].size.height)];
    else
        [self setContentMinSize:NSMakeSize(604., f_min_window_height)];

    _fspanel = [[VLCFSPanelController alloc] init];
    [_fspanel showWindow:self];

    /* make sure we display the desired default appearance when VLC launches for the first time */
    if (![defaults objectForKey:@"VLCFirstRun"]) {
        [defaults setObject:[NSDate date] forKey:@"VLCFirstRun"];

        [_sidebarView expandItem:nil expandChildren:YES];

        NSAlert *albumArtAlert = [NSAlert alertWithMessageText:_NS("Check for album art and metadata?") defaultButton:_NS("Enable Metadata Retrieval") alternateButton:_NS("No, Thanks") otherButton:nil informativeTextWithFormat:@"%@",_NS("VLC can check online for album art and metadata to enrich your playback experience, e.g. by providing track information when playing Audio CDs. To provide this functionality, VLC will send information about your contents to trusted services in an anonymized form.")];
        NSInteger returnValue = [albumArtAlert runModal];
        config_PutInt(getIntf(), "metadata-network-access", returnValue == NSAlertDefaultReturn);
    }

    if (self.darkInterface) {
        [defaultCenter addObserver: self selector: @selector(windowResizedOrMoved:) name: NSWindowDidResizeNotification object: nil];
        [defaultCenter addObserver: self selector: @selector(windowResizedOrMoved:) name: NSWindowDidMoveNotification object: nil];

        [self setBackgroundColor: [NSColor clearColor]];
        [self setOpaque: NO];
        [self display];
        [self setHasShadow:NO];
        [self setHasShadow:YES];

        self.previousSavedFrame = [self frame];
    } else {
        [_playlistScrollView setBorderType:NSNoBorder];
        [_sidebarScrollView setBorderType:NSNoBorder];
    }

    [defaultCenter addObserver: self selector: @selector(someWindowWillClose:) name: NSWindowWillCloseNotification object: nil];
    [defaultCenter addObserver: self selector: @selector(someWindowWillMiniaturize:) name: NSWindowWillMiniaturizeNotification object:nil];
    [defaultCenter addObserver: self selector: @selector(applicationWillTerminate:) name: NSApplicationWillTerminateNotification object: nil];
    [defaultCenter addObserver: self selector: @selector(mainSplitViewDidResizeSubviews:) name: NSSplitViewDidResizeSubviewsNotification object:_splitView];

    if (splitViewShouldBeHidden) {
        [self hideSplitView:YES];
        f_lastSplitViewHeight = 300;
    }

    /* sanity check for the window size */
    NSRect frame = [self frame];
    NSSize screenSize = [[self screen] frame].size;
    if (screenSize.width <= frame.size.width || screenSize.height <= frame.size.height) {
        self.nativeVideoSize = screenSize;
        [self resizeWindow];
    }

    /* update fs button to reflect state for next startup */
    if (var_InheritBool(pl_Get(getIntf()), "fullscreen"))
        [self.controlsBar setFullscreenState:YES];

    /* restore split view */
    f_lastLeftSplitViewWidth = 200;
    [[[VLCMain sharedInstance] mainMenu] updateSidebarMenuItem: ![_splitView isSubviewCollapsed:_splitViewLeft]];
}

#pragma mark -
#pragma mark appearance management

// Show split view and hide the video view
- (void)makeSplitViewVisible
{
    if (self.darkInterface)
        [self setContentMinSize: NSMakeSize(604., f_min_window_height + [self.titlebarView frame].size.height)];
    else
        [self setContentMinSize: NSMakeSize(604., f_min_window_height)];

    NSRect old_frame = [self frame];
    CGFloat newHeight = [self minSize].height;
    if (old_frame.size.height < newHeight) {
        NSRect new_frame = old_frame;
        new_frame.origin.y = old_frame.origin.y + old_frame.size.height - newHeight;
        new_frame.size.height = newHeight;

        [[self animator] setFrame:new_frame display:YES animate:YES];
    }

    [self.videoView setHidden:YES];
    [_splitView setHidden:NO];
    if (self.nativeFullscreenMode && [self fullscreen]) {
        [self showControlsBar];
        [self.fspanel setNonActive];
    }

    [self makeFirstResponder:_playlistScrollView];
}

// Hides the split view and makes the vout view in foreground
- (void)makeSplitViewHidden
{
    if (self.darkInterface)
        [self setContentMinSize: NSMakeSize(604., f_min_video_height + [self.titlebarView frame].size.height)];
    else
        [self setContentMinSize: NSMakeSize(604., f_min_video_height)];

    [_splitView setHidden:YES];
    [self.videoView setHidden:NO];
    if (self.nativeFullscreenMode && [self fullscreen]) {
        [self hideControlsBar];
        [self.fspanel setActive];
    }

    if ([[self.videoView subviews] count] > 0)
        [self makeFirstResponder: [[self.videoView subviews] firstObject]];
}


- (void)changePlaylistState:(VLCPlaylistStateEvent)event
{
    // Beware, this code is really ugly

    msg_Dbg(getIntf(), "toggle playlist from state: removed splitview %i, minimized view %i. Event %i", b_splitview_removed, b_minimized_view, event);
    if (![self isVisible] && event == psUserMenuEvent) {
        [self makeKeyAndOrderFront: nil];
        return;
    }

    BOOL b_activeVideo = [[VLCMain sharedInstance] activeVideoPlayback];
    BOOL b_restored = NO;

    // ignore alt if triggered through main menu shortcut
    BOOL b_have_alt_key = ([[NSApp currentEvent] modifierFlags] & NSAlternateKeyMask) != 0;
    if (event == psUserMenuEvent)
        b_have_alt_key = NO;

    // eUserMenuEvent is now handled same as eUserEvent
    if(event == psUserMenuEvent)
        event = psUserEvent;

    if (b_dropzone_active && b_have_alt_key) {
        [self hideDropZone];
        return;
    }

    if (!(self.nativeFullscreenMode && self.fullscreen) && !b_splitview_removed && ((b_have_alt_key && b_activeVideo)
                                                                              || (self.nonembedded && event == psUserEvent)
                                                                              || (!b_activeVideo && event == psUserEvent)
                                                                              || (b_minimized_view && event == psVideoStartedOrStoppedEvent))) {
        // for starting playback, window is resized through resized events
        // for stopping playback, resize through reset to previous frame
        [self hideSplitView: event != psVideoStartedOrStoppedEvent];
        b_minimized_view = NO;
    } else {
        if (b_splitview_removed) {
            if (!self.nonembedded || (event == psUserEvent && self.nonembedded))
                [self showSplitView: event != psVideoStartedOrStoppedEvent];

            if (event != psUserEvent)
                b_minimized_view = YES;
            else
                b_minimized_view = NO;

            if (b_activeVideo)
                b_restored = YES;
        }

        if (!self.nonembedded) {
            if (([self.videoView isHidden] && b_activeVideo) || b_restored || (b_activeVideo && event != psUserEvent))
                [self makeSplitViewHidden];
            else
                [self makeSplitViewVisible];
        } else {
            [_splitView setHidden: NO];
            [_playlistScrollView setHidden: NO];
            [self.videoView setHidden: YES];
            [self showControlsBar];
        }
    }

    msg_Dbg(getIntf(), "toggle playlist to state: removed splitview %i, minimized view %i", b_splitview_removed, b_minimized_view);
}

- (IBAction)dropzoneButtonAction:(id)sender
{
    [[[VLCMain sharedInstance] open] openFileGeneric];
}

#pragma mark -
#pragma mark overwritten default functionality

- (void)windowResizedOrMoved:(NSNotification *)notification
{
    [self saveFrameUsingName:[self frameAutosaveName]];
}

- (void)applicationWillTerminate:(NSNotification *)notification
{
    [self saveFrameUsingName:[self frameAutosaveName]];
}


- (void)someWindowWillClose:(NSNotification *)notification
{
    id obj = [notification object];

    // hasActiveVideo is defined for VLCVideoWindowCommon and subclasses
    if ([obj respondsToSelector:@selector(hasActiveVideo)] && [obj hasActiveVideo]) {
        if ([[VLCMain sharedInstance] activeVideoPlayback])
            [[VLCCoreInteraction sharedInstance] stop];
    }
}

- (void)someWindowWillMiniaturize:(NSNotification *)notification
{
    if (config_GetInt(getIntf(), "macosx-pause-minimized")) {
        id obj = [notification object];

        if ([obj class] == [VLCVideoWindowCommon class] || [obj class] == [VLCDetachedVideoWindow class] || ([obj class] == [VLCMainWindow class] && !self.nonembedded)) {
            if ([[VLCMain sharedInstance] activeVideoPlayback])
                [[VLCCoreInteraction sharedInstance] pause];
        }
    }
}

#pragma mark -
#pragma mark Update interface and respond to foreign events
- (void)showDropZone
{
    b_dropzone_active = YES;
    [_dropzoneView setHidden:NO];
    [_playlistScrollView setHidden:YES];
}

- (void)hideDropZone
{
    b_dropzone_active = NO;
    [_dropzoneView setHidden:YES];
    [_playlistScrollView setHidden:NO];
}

- (void)hideSplitView:(BOOL)resize
{
    if (resize) {
        NSRect winrect = [self frame];
        f_lastSplitViewHeight = [_splitView frame].size.height;
        winrect.size.height = winrect.size.height - f_lastSplitViewHeight;
        winrect.origin.y = winrect.origin.y + f_lastSplitViewHeight;
        [self setFrame:winrect display:YES animate:YES];
    }

    if (self.darkInterface) {
        [self setContentMinSize: NSMakeSize(604., [self.controlsBar height] + [self.titlebarView frame].size.height)];
        [self setContentMaxSize: NSMakeSize(FLT_MAX, [self.controlsBar height] + [self.titlebarView frame].size.height)];
    } else {
        [self setContentMinSize: NSMakeSize(604., [self.controlsBar height])];
        [self setContentMaxSize: NSMakeSize(FLT_MAX, [self.controlsBar height])];
    }

    b_splitview_removed = YES;
}

- (void)showSplitView:(BOOL)resize
{
    [self updateWindow];
    if (self.darkInterface)
        [self setContentMinSize:NSMakeSize(604., f_min_window_height + [self.titlebarView frame].size.height)];
    else
        [self setContentMinSize:NSMakeSize(604., f_min_window_height)];
    [self setContentMaxSize: NSMakeSize(FLT_MAX, FLT_MAX)];

    if (resize) {
        NSRect winrect;
        winrect = [self frame];
        winrect.size.height = winrect.size.height + f_lastSplitViewHeight;
        winrect.origin.y = winrect.origin.y - f_lastSplitViewHeight;
        [self setFrame:winrect display:YES animate:YES];
    }

    b_splitview_removed = NO;
}

- (void)updateTimeSlider
{
    [self.controlsBar updateTimeSlider];
    [self.fspanel updatePositionAndTime];

    [[[VLCMain sharedInstance] voutController] updateControlsBarsUsingBlock:^(VLCControlsBarCommon *controlsBar) {
        [controlsBar updateTimeSlider];
    }];

    [[VLCCoreInteraction sharedInstance] updateAtoB];
}

- (void)updateName
{
    input_thread_t *p_input;
    p_input = pl_CurrentInput(getIntf());
    if (p_input) {
        NSString *aString = @"";

        if (!config_GetPsz(getIntf(), "video-title")) {
            char *format = var_InheritString(getIntf(), "input-title-format");
            if (format) {
                char *formated = vlc_strfinput(p_input, format);
                free(format);
                aString = toNSStr(formated);
                free(formated);
            }
        } else
            aString = toNSStr(config_GetPsz(getIntf(), "video-title"));

        char *uri = input_item_GetURI(input_GetItem(p_input));

        NSURL * o_url = [NSURL URLWithString:toNSStr(uri)];
        if ([o_url isFileURL]) {
            [self setRepresentedURL: o_url];
            [[[VLCMain sharedInstance] voutController] updateWindowsUsingBlock:^(VLCVideoWindowCommon *o_window) {
                [o_window setRepresentedURL:o_url];
            }];
        } else {
            [self setRepresentedURL: nil];
            [[[VLCMain sharedInstance] voutController] updateWindowsUsingBlock:^(VLCVideoWindowCommon *o_window) {
                [o_window setRepresentedURL:nil];
            }];
        }
        free(uri);

        if ([aString isEqualToString:@""]) {
            if ([o_url isFileURL])
                aString = [[NSFileManager defaultManager] displayNameAtPath: [o_url path]];
            else
                aString = [o_url absoluteString];
        }

        if ([aString length] > 0) {
            [self setTitle: aString];
            [[[VLCMain sharedInstance] voutController] updateWindowsUsingBlock:^(VLCVideoWindowCommon *o_window) {
                [o_window setTitle:aString];
            }];

            [self.fspanel setStreamTitle: aString];
        } else {
            [self setTitle: _NS("VLC media player")];
            [self setRepresentedURL: nil];
        }

        vlc_object_release(p_input);
    } else {
        [self setTitle: _NS("VLC media player")];
        [self setRepresentedURL: nil];
    }
}

- (void)updateWindow
{
    [self.controlsBar updateControls];
    [[[VLCMain sharedInstance] voutController] updateControlsBarsUsingBlock:^(VLCControlsBarCommon *controlsBar) {
        [controlsBar updateControls];
    }];

    bool b_seekable = false;

    playlist_t *p_playlist = pl_Get(getIntf());
    input_thread_t *p_input = playlist_CurrentInput(p_playlist);
    if (p_input) {
        /* seekable streams */
        b_seekable = var_GetBool(p_input, "can-seek");

        vlc_object_release(p_input);
    }

    [self updateTimeSlider];
    if ([self.fspanel respondsToSelector:@selector(setSeekable:)])
        [self.fspanel setSeekable: b_seekable];

    PL_LOCK;
    if ([[[[VLCMain sharedInstance] playlist] model] currentRootType] != ROOT_TYPE_PLAYLIST ||
        [[[[VLCMain sharedInstance] playlist] model] hasChildren])
        [self hideDropZone];
    else
        [self showDropZone];
    PL_UNLOCK;
    [_sidebarView setNeedsDisplay:YES];

    // Update badge in the sidebar for the first two items (Playlist and Media library)
    [_sidebarView reloadDataForRowIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(1,2)]
                            columnIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, [[_sidebarView tableColumns] count])]];

    [self _updatePlaylistTitle];
}

- (void)setPause
{
    [self.controlsBar setPause];
    [self.fspanel setPause];

    [[[VLCMain sharedInstance] voutController] updateControlsBarsUsingBlock:^(VLCControlsBarCommon *controlsBar) {
        [controlsBar setPause];
    }];
}

- (void)setPlay
{
    [self.controlsBar setPlay];
    [self.fspanel setPlay];

    [[[VLCMain sharedInstance] voutController] updateControlsBarsUsingBlock:^(VLCControlsBarCommon *controlsBar) {
        [controlsBar setPlay];
    }];
}

- (void)updateVolumeSlider
{
    [(VLCMainWindowControlsBar *)[self controlsBar] updateVolumeSlider];
    [self.fspanel setVolumeLevel:[[VLCCoreInteraction sharedInstance] volume]];
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary<NSKeyValueChangeKey,id> *)change
                       context:(void *)context
{
    if (@available(macOS 10_14, *)) {
        if ([[[NSApplication sharedApplication] effectiveAppearance].name isEqualToString:NSAppearanceNameDarkAqua]) {
            [_dropzoneImageView setImage:[NSImage imageNamed:@"mj-dropzone-dark"]];
        } else {
            [_dropzoneImageView setImage:imageFromRes(@"dropzone")];
        }
        [self.sidebarDataSource reloadSidebar];
    }
}

#pragma mark -
#pragma mark Video Output handling

- (void)videoplayWillBeStarted
{
    if (!self.fullscreen)
        frameBeforePlayback = [self frame];
}

- (void)setVideoplayEnabled
{
    BOOL b_videoPlayback = [[VLCMain sharedInstance] activeVideoPlayback];

    if (!b_videoPlayback) {
        if (!self.nonembedded && (!self.nativeFullscreenMode || (self.nativeFullscreenMode && !self.fullscreen)) && frameBeforePlayback.size.width > 0 && frameBeforePlayback.size.height > 0) {

            // only resize back to minimum view of this is still desired final state
            CGFloat f_threshold_height = f_min_video_height + [self.controlsBar height];
            if(frameBeforePlayback.size.height > f_threshold_height || b_minimized_view) {

                if ([[VLCMain sharedInstance] isTerminating])
                    [self setFrame:frameBeforePlayback display:YES];
                else
                    [[self animator] setFrame:frameBeforePlayback display:YES];

            }
        }

        frameBeforePlayback = NSMakeRect(0, 0, 0, 0);

        // update fs button to reflect state for next startup
        if (var_InheritBool(getIntf(), "fullscreen") || var_GetBool(pl_Get(getIntf()), "fullscreen")) {
            [self.controlsBar setFullscreenState:YES];
        }

        [self makeFirstResponder: _playlistScrollView];
        [[[VLCMain sharedInstance] voutController] updateWindowLevelForHelperWindows: NSNormalWindowLevel];

        // restore alpha value to 1 for the case that macosx-opaqueness is set to < 1
        [self setAlphaValue:1.0];
    }

    if (self.nativeFullscreenMode) {
        if ([self hasActiveVideo] && [self fullscreen] && b_videoPlayback) {
            [self hideControlsBar];
            [self.fspanel setActive];
        } else {
            [self showControlsBar];
            [self.fspanel setNonActive];
        }
    }
}

#pragma mark -
#pragma mark Fullscreen support

- (void)showFullscreenController
{
    id currentWindow = [NSApp keyWindow];
    if ([currentWindow respondsToSelector:@selector(hasActiveVideo)] && [currentWindow hasActiveVideo]) {
        if ([currentWindow respondsToSelector:@selector(fullscreen)] && [currentWindow fullscreen] && ![[currentWindow videoView] isHidden]) {

            if ([[VLCMain sharedInstance] activeVideoPlayback])
                [self.fspanel fadeIn];
        }
    }

}

#pragma mark -
#pragma mark split view delegate
- (CGFloat)splitView:(NSSplitView *)splitView constrainMaxCoordinate:(CGFloat)proposedMax ofSubviewAt:(NSInteger)dividerIndex
{
    if (dividerIndex == 0)
        return 300.;
    else
        return proposedMax;
}

- (CGFloat)splitView:(NSSplitView *)splitView constrainMinCoordinate:(CGFloat)proposedMin ofSubviewAt:(NSInteger)dividerIndex
{
    if (dividerIndex == 0)
        return 100.;
    else
        return proposedMin;
}

- (BOOL)splitView:(NSSplitView *)splitView canCollapseSubview:(NSView *)subview
{
    return ([subview isEqual:_splitViewLeft]);
}

- (BOOL)splitView:(NSSplitView *)splitView shouldAdjustSizeOfSubview:(NSView *)subview
{
    return (![subview isEqual:_splitViewLeft]);
}

- (void)mainSplitViewDidResizeSubviews:(id)object
{
    f_lastLeftSplitViewWidth = [_splitViewLeft frame].size.width;
    [[[VLCMain sharedInstance] mainMenu] updateSidebarMenuItem: ![_splitView isSubviewCollapsed:_splitViewLeft]];
}

- (void)toggleLeftSubSplitView
{
    [_splitView adjustSubviews];
    if ([_splitView isSubviewCollapsed:_splitViewLeft])
        [_splitView setPosition:f_lastLeftSplitViewWidth ofDividerAtIndex:0];
    else
        [_splitView setPosition:[_splitView minPossiblePositionOfDividerAtIndex:0] ofDividerAtIndex:0];

    [[[VLCMain sharedInstance] mainMenu] updateSidebarMenuItem: ![_splitView isSubviewCollapsed:_splitViewLeft]];
}

#pragma mark -
#pragma mark private playlist magic
- (void)_updatePlaylistTitle
{
    PLRootType root = [[[[VLCMain sharedInstance] playlist] model] currentRootType];
    playlist_t *p_playlist = pl_Get(getIntf());

    PL_LOCK;
    if (root == ROOT_TYPE_PLAYLIST)
        [_categoryLabel setStringValue: [_NS("Playlist") stringByAppendingString:[self _playbackDurationOfNode:p_playlist->p_playing]]];
    else if (root == ROOT_TYPE_MEDIALIBRARY)
        [_categoryLabel setStringValue: [_NS("Media Library") stringByAppendingString:[self _playbackDurationOfNode:p_playlist->p_media_library]]];

    PL_UNLOCK;
}

- (NSString *)_playbackDurationOfNode:(playlist_item_t*)node
{
    if (!node)
        return @"";

    playlist_t * p_playlist = pl_Get(getIntf());
    PL_ASSERT_LOCKED;

    mtime_t mt_duration = playlist_GetNodeDuration( node );

    if (mt_duration < 1)
        return @"";

    mt_duration = mt_duration / 1000000;

    int sec = (mt_duration % 60);
    int min = (mt_duration % 3600) / 60;
    int hours = (mt_duration % 86400) / 3600;
    int days = (int)(mt_duration / 86400);

    NSString *result;
    if (days > 0) {
        result = [NSString stringWithFormat:@"%i:%i:%02i:%02i", days, hours, min, sec];
    } else {
        result = [NSString stringWithFormat:@"%i:%02i:%02i", hours, min, sec];
    }

    return [NSString stringWithFormat:@" — %@", result];
}

- (IBAction)searchItem:(id)sender
{
    [[[[VLCMain sharedInstance] playlist] model] searchUpdate:[_searchField stringValue]];
}

- (IBAction)highlightSearchField:(id)sender
{
    [_searchField selectText:sender];
}

#pragma mark -
#pragma mark Sidebar handling

- (IBAction)sdmenuhandler:(id)sender
{
    NSString * identifier = [sender representedObject];
    msg_Dbg(getIntf(), "Change state for service discovery item '%s'", [identifier UTF8String]);

    if ([identifier length] > 0 && ![identifier isEqualToString:@"lua{sd='freebox',longname='Freebox TV'}"]) {
        playlist_t * p_playlist = pl_Get(getIntf());
        BOOL sd_loaded = playlist_IsServicesDiscoveryLoaded(p_playlist, [identifier UTF8String]);

        if (!sd_loaded)
            playlist_ServicesDiscoveryAdd(p_playlist, [identifier UTF8String]);
        else
            playlist_ServicesDiscoveryRemove(p_playlist, [identifier UTF8String]);
    }
}

- (void)sourceListSelectionDidChange:(NSNotification *)notification
{
    playlist_t * p_playlist = pl_Get(getIntf());

    NSIndexSet *selectedIndexes = [_sidebarView selectedRowIndexes];
    if (selectedIndexes.count == 0)
        return;

    id item = [_sidebarView itemAtRow:[selectedIndexes firstIndex]];

    //Set the label text to represent the new selection
    if ([item sdtype] > -1 && [[item identifier] length] > 0) {
        BOOL sd_loaded = playlist_IsServicesDiscoveryLoaded(p_playlist, [[item identifier] UTF8String]);
        if (!sd_loaded)
            playlist_ServicesDiscoveryAdd(p_playlist, [[item identifier] UTF8String]);
    }

    [_categoryLabel setStringValue:[item title]];

    if ([[item identifier] isEqualToString:@"playlist"]) {
        PL_LOCK;
        [[[[VLCMain sharedInstance] playlist] model] changeRootItem:p_playlist->p_playing];
        PL_UNLOCK;

        [self _updatePlaylistTitle];

    } else if ([[item identifier] isEqualToString:@"medialibrary"]) {
        if (p_playlist->p_media_library) {

            PL_LOCK;
            [[[[VLCMain sharedInstance] playlist] model] changeRootItem:p_playlist->p_media_library];

            PL_UNLOCK;

            [self _updatePlaylistTitle];
        }
    } else {
        PL_LOCK;
        const char *title = [[item title] UTF8String];
        playlist_item_t *pl_item = playlist_ChildSearchName(&p_playlist->root, title);
        if (pl_item)
            [[[[VLCMain sharedInstance] playlist] model] changeRootItem:pl_item];
        else
            msg_Err(getIntf(), "Could not find playlist entry with name %s", title);

        PL_UNLOCK;
    }

    // Note the order: first hide the podcast controls, then show the drop zone
    if ([[item identifier] isEqualToString:@"podcast"])
        [self showPodcastControls];
    else
        [self hidePodcastControls];

    PL_LOCK;
    if ([[[[VLCMain sharedInstance] playlist] model] currentRootType] != ROOT_TYPE_PLAYLIST ||
        [[[[VLCMain sharedInstance] playlist] model] hasChildren])
        [self hideDropZone];
    else
        [self showDropZone];
    PL_UNLOCK;

    [[NSNotificationCenter defaultCenter] postNotificationName: VLCMediaKeySupportSettingChangedNotification
                                                        object: nil
                                                      userInfo: nil];
}

#pragma mark -
#pragma mark Podcast

- (IBAction)addPodcast:(id)sender
{
    [NSApp beginSheet:_podcastSubscribeWindow modalForWindow:self modalDelegate:self didEndSelector:NULL contextInfo:nil];
}

- (IBAction)addPodcastWindowAction:(id)sender
{
    [_podcastSubscribeWindow orderOut:sender];
    [NSApp endSheet:_podcastSubscribeWindow];

    if (sender == _podcastSubscribeOkButton && [[_podcastSubscribeUrlField stringValue] length] > 0) {
        NSMutableString *podcastConf = [[NSMutableString alloc] init];
        if (config_GetPsz(getIntf(), "podcast-urls") != NULL)
            [podcastConf appendFormat:@"%s|", config_GetPsz(getIntf(), "podcast-urls")];

        [podcastConf appendString: [_podcastSubscribeUrlField stringValue]];
        config_PutPsz(getIntf(), "podcast-urls", [podcastConf UTF8String]);
        var_SetString(pl_Get(getIntf()), "podcast-urls", [podcastConf UTF8String]);
    }
}

- (IBAction)removePodcast:(id)sender
{
    char *psz_urls = var_InheritString(pl_Get(getIntf()), "podcast-urls");
    if (psz_urls != NULL) {
        [_podcastUnsubscribePopUpButton removeAllItems];
        [_podcastUnsubscribePopUpButton addItemsWithTitles:[toNSStr(psz_urls) componentsSeparatedByString:@"|"]];
        [NSApp beginSheet:_podcastUnsubscribeWindow modalForWindow:self modalDelegate:self didEndSelector:NULL contextInfo:nil];
    }
    free(psz_urls);
}

- (IBAction)removePodcastWindowAction:(id)sender
{
    [_podcastUnsubscribeWindow orderOut:sender];
    [NSApp endSheet:_podcastUnsubscribeWindow];

    if (sender == _podcastUnsubscribeOkButton) {
        playlist_t * p_playlist = pl_Get(getIntf());
        char *psz_urls = var_InheritString(p_playlist, "podcast-urls");

        NSMutableArray * urls = [[NSMutableArray alloc] initWithArray:[toNSStr(config_GetPsz(getIntf(), "podcast-urls")) componentsSeparatedByString:@"|"]];
        [urls removeObjectAtIndex: [_podcastUnsubscribePopUpButton indexOfSelectedItem]];
        const char *psz_new_urls = [[urls componentsJoinedByString:@"|"] UTF8String];
        var_SetString(pl_Get(getIntf()), "podcast-urls", psz_new_urls);
        config_PutPsz(getIntf(), "podcast-urls", psz_new_urls);

        free(psz_urls);

        /* update playlist table */
        if (playlist_IsServicesDiscoveryLoaded(p_playlist, "podcast")) {
            [[[VLCMain sharedInstance] playlist] playlistUpdated];
        }
    }
}

- (void)showPodcastControls
{
    _tableViewToPodcastConstraint.priority = 999;
    _podcastView.hidden = NO;

    b_podcastView_displayed = YES;
}

- (void)hidePodcastControls
{
    if (b_podcastView_displayed) {
        _tableViewToPodcastConstraint.priority = 1;
        _podcastView.hidden = YES;

        b_podcastView_displayed = NO;
    }
}

@end

@interface VLCDetachedVideoWindow ()
@end

@implementation VLCDetachedVideoWindow

- (void)awakeFromNib
{
    // sets lion fullscreen behaviour
    [super awakeFromNib];
    [self setAcceptsMouseMovedEvents: YES];

    if (@available(macOS 10.14, *)) {
        [self setContentMinSize: NSMakeSize(363., f_min_video_height + [[self controlsBar] height])];
    } else {
        BOOL darkInterface = config_GetInt(getIntf(), "macosx-interfacestyle");

        if (darkInterface) {
            [self setBackgroundColor: [NSColor clearColor]];

            [self setOpaque: NO];
            [self display];
            [self setHasShadow:NO];
            [self setHasShadow:YES];

            [self setTitle: _NS("VLC media player")];

            [self setContentMinSize: NSMakeSize(363., f_min_video_height + [[self controlsBar] height] + [self.titlebarView frame].size.height)];
        } else {
            [self setContentMinSize: NSMakeSize(363., f_min_video_height + [[self controlsBar] height])];
        }
    }
}

@end
