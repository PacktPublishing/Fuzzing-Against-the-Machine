/*****************************************************************************
 * VLCFSPanelController.h: macOS fullscreen controls window controller
 *****************************************************************************
 * Copyright (C) 2006-2016 VLC authors and VideoLAN
 * $Id: 5b86759b0bdf0cafd6770c018432577d966f426b $
 *
 * Authors: Jérôme Decoodt <djc at videolan dot org>
 *          Felix Paul Kühne <fkuehne at videolan dot org>
 *          David Fuhrmann <david dot fuhrmann at googlemail dot com>
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

#import "Windows.h"
#import "VLCDefaultValueSlider.h"
#import "VLCTimeField.h"

@interface VLCFSPanelController : NSWindowController

@property (readwrite, weak) NSTimer   *hideTimer;

@property IBOutlet NSView       *controlsView;
@property IBOutlet NSButton     *playPauseButton;
@property IBOutlet NSButton     *forwardButton;
@property IBOutlet NSButton     *backwardButton;
@property IBOutlet NSButton     *nextButton;
@property IBOutlet NSButton     *previousButton;
@property IBOutlet NSButton     *fullscreenButton;
@property IBOutlet NSTextField  *mediaTitle;
@property IBOutlet VLCTimeField *elapsedTime;
@property IBOutlet VLCTimeField *remainingOrTotalTime;
@property IBOutlet NSSlider     *timeSlider;
@property IBOutlet VLCDefaultValueSlider *volumeSlider;

@property (assign) IBOutlet NSLayoutConstraint *heightMaxConstraint;

- (IBAction)togglePlayPause:(id)sender;
- (IBAction)jumpForward:(id)sender;
- (IBAction)jumpBackward:(id)sender;
- (IBAction)gotoPrevious:(id)sender;
- (IBAction)gotoNext:(id)sender;
- (IBAction)toggleFullscreen:(id)sender;
- (IBAction)timeSliderUpdate:(id)sender;
- (IBAction)volumeSliderUpdate:(id)sender;

- (void)fadeIn;
- (void)fadeOut;
- (void)setActive;
- (void)setNonActive;
- (void)setVoutWasUpdated:(VLCWindow *)voutWindow;

- (void)setSeekable:(BOOL)seekable;
- (void)setVolumeLevel:(int)value;
- (void)updatePositionAndTime;
- (void)setStreamTitle:(NSString *)title;
- (void)setPlay;
- (void)setPause;

// Constrain frame to window. Used by VLCFSPanelDraggableView.
- (NSRect)contrainFrameToAssociatedVoutWindow:(NSRect)frame;

@end
