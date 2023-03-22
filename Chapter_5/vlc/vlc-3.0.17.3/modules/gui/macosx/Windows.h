/*****************************************************************************
 * Windows.h: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2012-2014 VLC authors and VideoLAN
 * $Id: e228188765f48ac414a51b9d4e002a2a98832cd8 $
 *
 * Authors: Felix Paul Kühne <fkuehne -at- videolan -dot- org>
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

#import <Cocoa/Cocoa.h>

/*****************************************************************************
 * VLCWindow
 *
 *  Missing extension to NSWindow
 *****************************************************************************/

@class VLCVoutView;

@interface VLCWindow : NSWindow <NSAnimationDelegate>

@property (readwrite) BOOL canBecomeKeyWindow;
@property (readwrite) BOOL canBecomeMainWindow;

@property (nonatomic, readwrite) BOOL hasActiveVideo;
@property (nonatomic, readwrite) BOOL fullscreen;

- (void)closeAndAnimate:(BOOL)animate;
- (void)orderFront:(id)sender animate:(BOOL)animate;
- (void)orderOut:(id)sender animate:(BOOL)animate;

- (VLCVoutView *)videoView;

@end


static const float f_min_video_height = 70.0;

@class VLCControlsBarCommon;
@class VLCMainWindowTitleView;

/*****************************************************************************
 * VLCVideoWindowCommon
 *
 *  Common code for main window, detached window and extra video window
 *****************************************************************************/

@interface VLCVideoWindowCommon : VLCWindow <NSWindowDelegate, NSAnimationDelegate>

@property (nonatomic, weak) IBOutlet VLCMainWindowTitleView *titlebarView; // only set in main or detached window
@property (weak) IBOutlet NSLayoutConstraint *videoViewTopConstraint;
@property (weak) IBOutlet NSLayoutConstraint *videoViewBottomConstraint;

@property (nonatomic, weak) IBOutlet VLCVoutView* videoView;
@property (nonatomic, weak) IBOutlet VLCControlsBarCommon* controlsBar;
@property (readonly) BOOL inFullscreenTransition;
@property (readonly) BOOL darkInterface;
@property (readonly) BOOL windowShouldExitFullscreenWhenFinished;
@property (readwrite, assign) NSRect previousSavedFrame;
@property (nonatomic, readwrite, assign) NSSize nativeVideoSize;

- (void)setWindowLevel:(NSInteger)i_state;
- (void)resizeWindow;

- (NSRect)getWindowRectForProposedVideoViewSize:(NSSize)size;

- (void)setTitle:(NSString *)title;

/* fullscreen handling */
- (void)enterFullscreenWithAnimation:(BOOL)b_animation;
- (void)leaveFullscreenWithAnimation:(BOOL)b_animation;

/* lion fullscreen handling */
- (void)hideControlsBar;
- (void)showControlsBar;

- (void)windowWillEnterFullScreen:(NSNotification *)notification;
- (void)windowDidEnterFullScreen:(NSNotification *)notification;
- (void)windowWillExitFullScreen:(NSNotification *)notification;

@end
