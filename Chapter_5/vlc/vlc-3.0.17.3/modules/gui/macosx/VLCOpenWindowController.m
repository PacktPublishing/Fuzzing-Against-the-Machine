/*****************************************************************************
 * VLCOpenWindowController.m: Open dialogues for VLC's MacOS X port
 *****************************************************************************
 * Copyright (C) 2002-2015 VLC authors and VideoLAN
 * $Id: 28b2377c5b74b3eab1a5ddfaaa7ed0070e3514c8 $
 *
 * Authors: Jon Lech Johansen <jon-vl@nanocrew.net>
 *          Christophe Massiot <massiot@via.ecp.fr>
 *          Derk-Jan Hartman <thedj@users.sourceforge.net>
 *          Benjamin Pracht <bigben at videolan dot org>
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

#import <stdlib.h>                                      /* malloc(), free() */
#import <sys/param.h>                                    /* for MAXPATHLEN */

#import "CompatibilityFixes.h"

#import <paths.h>
#import <IOKit/IOBSD.h>
#import <Cocoa/Cocoa.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreMedia/CoreMedia.h>

#import "VLCMain.h"
#import "VLCPlaylist.h"
#import "VLCOpenWindowController.h"
#import "VLCOutput.h"
#import "misc.h"

#import <vlc_url.h>

NSString *const VLCOpenTextFieldWasClicked = @"VLCOpenTextFieldWasClicked";

@interface VLCOpenTextField : NSTextField
- (void)mouseDown:(NSEvent *)theEvent;
@end

@implementation VLCOpenTextField

- (void)mouseDown:(NSEvent *)theEvent
{
    [[NSNotificationCenter defaultCenter] postNotificationName: VLCOpenTextFieldWasClicked
                                                        object: self];
    [super mouseDown: theEvent];
}

@end

struct display_info_t
{
    CGRect rect;
    CGDirectDisplayID id;
};

@interface VLCOpenWindowController()
{
    VLCOutput *_output;
    BOOL b_outputNibLoaded;
    NSArray *_avvideoDevices;
    NSArray *_avaudioDevices;
    NSString *_avCurrentDeviceUID;
    NSString *_avCurrentAudioDeviceUID;

    BOOL b_autoplay;
    BOOL b_nodvdmenus;
    NSView *_currentOpticalMediaView;
    NSImageView *_currentOpticalMediaIconView;
    NSMutableArray *_allMediaDevices;
    NSArray *_opticalDevices;
    NSMutableArray *_specialMediaFolders;
    NSString *_filePath;
    NSString *_fileSlavePath;
    NSString *_subPath;
    NSString *_MRL;
    NSMutableArray *_displayInfos;
}

@property (readwrite, assign) NSString *MRL;

@end

@implementation VLCOpenWindowController


static NSString *kFileTabViewId     = @"file";
static NSString *kDiscTabViewId     = @"disc";
static NSString *kNetworkTabViewId  = @"network";
static NSString *kCaptureTabViewId  = @"capture";

#pragma mark -
#pragma mark Init

- (id)init
{
    self = [super initWithWindowNibName:@"Open"];

    return self;
}


- (void)dealloc
{
    for (int i = 0; i < [_displayInfos count]; i ++) {
        NSValue *v = [_displayInfos objectAtIndex:i];
        free([v pointerValue]);
    }
}

- (void)windowDidLoad
{
    _output = [VLCOutput new];

    [self.window setCollectionBehavior: NSWindowCollectionBehaviorFullScreenAuxiliary];

    [self.window setTitle: _NS("Open Source")];
    [_mrlButtonLabel setTitle: _NS("Media Resource Locator (MRL)")];

    [_okButton setTitle: _NS("Open")];
    [_cancelButton setTitle: _NS("Cancel")];

    [_outputCheckbox setTitle:_NS("Stream output:")];
    [_outputSettingsButton setTitle:_NS("Settings...")];

    [_tabView accessibilitySetOverrideValue:_NS("Choose media input type") forAttribute:NSAccessibilityDescriptionAttribute];
    [[_tabView tabViewItemAtIndex: 0] setLabel: _NS("File")];
    [[_tabView tabViewItemAtIndex: 1] setLabel: _NS("Disc")];
    [[_tabView tabViewItemAtIndex: 2] setLabel: _NS("Network")];
    [[_tabView tabViewItemAtIndex: 3] setLabel: _NS("Capture")];
    [_fileNameLabel setStringValue: @""];
    [_fileNameStubLabel setStringValue: _NS("Choose a file")];
    [_fileIconWell setImage: [NSImage imageNamed:@"generic"]];
    [_fileBrowseButton setTitle: _NS("Browse...")];
    [[_fileBrowseButton cell] accessibilitySetOverrideValue:_NS("Select a file for playback") forAttribute:NSAccessibilityDescriptionAttribute];
    [_fileTreatAsPipeButton setTitle: _NS("Treat as a pipe rather than as a file")];
    [_fileTreatAsPipeButton setHidden: NO];
    [_fileSlaveCheckbox setTitle: _NS("Play another media synchronously")];
    [_fileSelectSlaveButton setTitle: _NS("Choose...")];
    [[_fileBrowseButton cell] accessibilitySetOverrideValue:_NS("Select another file to play in sync with the previously selected file") forAttribute:NSAccessibilityDescriptionAttribute];
    [_fileSlaveFilenameLabel setStringValue: @""];
    [_fileSlaveIconWell setImage: NULL];
    [_fileSubtitlesFilenameLabel setStringValue: @""];
    [_fileSubtitlesIconWell setImage: NULL];
    [_fileCustomTimingCheckbox setTitle: _NS("Custom playback")];
    [_fileStartTimeLabel setStringValue: _NS("Start time")];
    [_fileStartTimeTextField setStringValue: @""];
    [_fileStopTimeLabel setStringValue: _NS("Stop time")];
    [_fileStopTimeTextField setStringValue: @""];

    [_discSelectorPopup removeAllItems];
    [_discSelectorPopup setHidden: NO];
    NSString *oVideoTS = _NS("Open VIDEO_TS / BDMV folder");
    [_discNoDiscLabel setStringValue: _NS("Insert Disc")];
    [_discNoDiscVideoTSButton setTitle: oVideoTS];
    [_discAudioCDLabel setStringValue: _NS("Audio CD")];
    [_discAudioCDTrackCountLabel setStringValue: @""];
    [_discAudioCDVideoTSButton setTitle: oVideoTS];
    [_discDVDLabel setStringValue: @""];
    [_discDVDDisableMenusButton setTitle: _NS("Disable DVD menus")];
    [_discDVDVideoTSButton setTitle: oVideoTS];
    [_discDVDwomenusLabel setStringValue: @""];
    [_discDVDwomenusEnableMenusButton setTitle: _NS("Enable DVD menus")];
    [_discDVDwomenusVideoTSButton setTitle: oVideoTS];
    [_discDVDwomenusTitleLabel setStringValue: _NS("Title")];
    [_discDVDwomenusChapterLabel setStringValue: _NS("Chapter")];
    [_discVCDTitleLabel setStringValue: _NS("Title")];
    [_discVCDChapterLabel setStringValue: _NS("Chapter")];
    [_discVCDVideoTSButton setTitle: oVideoTS];
    [_discBDVideoTSButton setTitle: oVideoTS];

    [_netUDPPortLabel setStringValue: _NS("Port")];
    [_netUDPMAddressLabel setStringValue: _NS("IP Address")];
    [_netUDPMPortLabel setStringValue: _NS("Port")];
    [_netHTTPURLLabel setStringValue: _NS("URL")];
    [_netHelpLabel setStringValue: _NS("To Open a usual network stream (HTTP, RTSP, RTMP, MMS, FTP, etc.), just enter the URL in the field above. If you want to open a RTP or UDP stream, press the button below.")];
    [_netHelpUDPLabel setStringValue: _NS("If you want to open a multicast stream, enter the respective IP address given by the stream provider. In unicast mode, VLC will use your machine's IP automatically.\n\nTo open a stream using a different protocol, just press Cancel to close this sheet.")];
    [_netHTTPURLTextField accessibilitySetOverrideValue:_NS("Enter a stream URL here. To open RTP or UDP streams, use the respective button below.") forAttribute:NSAccessibilityDescriptionAttribute];
    [_netUDPCancelButton setTitle: _NS("Cancel")];
    [_netUDPOKButton setTitle: _NS("Open")];
    [_netOpenUDPButton setTitle: _NS("Open RTP/UDP Stream")];
    [_netUDPModeLabel setStringValue: _NS("Mode")];
    [_netUDPProtocolLabel setStringValue: _NS("Protocol")];
    [_netUDPAddressLabel setStringValue: _NS("Address")];

    [[_netModeMatrix cellAtRow:0 column:0] setTitle: _NS("Unicast")];
    [[_netModeMatrix cellAtRow:1 column:0] setTitle: _NS("Multicast")];

    [_netUDPPortTextField setIntValue: config_GetInt(getIntf(), "server-port")];
    [_netUDPPortStepper setIntValue: config_GetInt(getIntf(), "server-port")];

    [_captureModePopup removeAllItems];
    [_captureModePopup addItemWithTitle: _NS("Input Devices")];
    [_captureModePopup addItemWithTitle: _NS("Screen")];
    [_screenFPSLabel setStringValue: [NSString stringWithFormat:@"%@:",_NS("Frames per Second")]];
    [_screenLabel setStringValue: [NSString stringWithFormat:@"%@:",_NS("Screen")]];
    [_screenLeftLabel setStringValue: [NSString stringWithFormat:@"%@:",_NS("Subscreen left")]];
    [_screenTopLabel setStringValue: [NSString stringWithFormat:@"%@:",_NS("Subscreen top")]];
    [_screenWidthLabel setStringValue: [NSString stringWithFormat:@"%@:",_NS("Subscreen Width")]];
    [_screenHeightLabel setStringValue: [NSString stringWithFormat:@"%@:",_NS("Subscreen Height")]];
    [_screenFollowMouseCheckbox setTitle: _NS("Follow the mouse")];
    [_screenqtkAudioCheckbox setTitle: _NS("Capture Audio")];

    // setup start / stop time fields
    [_fileStartTimeTextField setFormatter:[[PositionFormatter alloc] init]];
    [_fileStopTimeTextField setFormatter:[[PositionFormatter alloc] init]];

    // Auto collapse MRL field
    self.mrlViewHeightConstraint.constant = 0;

    [self updateVideoDevices];
    [_qtkVideoDevicePopup removeAllItems];
    msg_Dbg(getIntf(), "Found %lu video capture devices", _avvideoDevices.count);

    if (_avvideoDevices.count >= 1) {
        if (!_avCurrentDeviceUID)
            _avCurrentDeviceUID = [[[AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo] uniqueID]
                                    stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];

        NSUInteger deviceCount = _avvideoDevices.count;
        for (int ivideo = 0; ivideo < deviceCount; ivideo++) {
            AVCaptureDevice *avDevice = [_avvideoDevices objectAtIndex:ivideo];
            // allow same name for multiple times
            [[_qtkVideoDevicePopup menu] addItemWithTitle:[avDevice localizedName] action:nil keyEquivalent:@""];

            if ([[[avDevice uniqueID] stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]] isEqualToString:_avCurrentDeviceUID])
                [_qtkVideoDevicePopup selectItemAtIndex:ivideo];
        }
    } else {
        [_qtkVideoDevicePopup addItemWithTitle: _NS("None")];
    }

    [_qtkAudioDevicePopup removeAllItems];
    [_screenqtkAudioPopup removeAllItems];

    [self updateAudioDevices];
    msg_Dbg(getIntf(), "Found %lu audio capture devices", _avaudioDevices.count);

    if (_avaudioDevices.count >= 1) {
        if (!_avCurrentAudioDeviceUID)
            _avCurrentAudioDeviceUID = [[[AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeAudio] uniqueID]
                                         stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];

        NSUInteger deviceCount = _avaudioDevices.count;
        for (int iaudio = 0; iaudio < deviceCount; iaudio++) {
            AVCaptureDevice *avAudioDevice = [_avaudioDevices objectAtIndex:iaudio];

            // allow same name for multiple times
            NSString *localizedName = [avAudioDevice localizedName];
            [[_qtkAudioDevicePopup menu] addItemWithTitle:localizedName action:nil keyEquivalent:@""];
            [[_screenqtkAudioPopup menu] addItemWithTitle:localizedName action:nil keyEquivalent:@""];

            if ([[[avAudioDevice uniqueID] stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]] isEqualToString:_avCurrentAudioDeviceUID]) {
                [_qtkAudioDevicePopup selectItemAtIndex:iaudio];
                [_screenqtkAudioPopup selectItemAtIndex:iaudio];
            }
        }
    } else {
        [_qtkAudioDevicePopup addItemWithTitle: _NS("None")];
        [_screenqtkAudioPopup addItemWithTitle: _NS("None")];
    }

    [self setSubPanel];

    [[NSNotificationCenter defaultCenter] addObserver: self
                                             selector: @selector(openNetInfoChanged:)
                                                 name: NSControlTextDidChangeNotification
                                               object: _netUDPPortTextField];
    [[NSNotificationCenter defaultCenter] addObserver: self
                                             selector: @selector(openNetInfoChanged:)
                                                 name: NSControlTextDidChangeNotification
                                               object: _netUDPMAddressTextField];
    [[NSNotificationCenter defaultCenter] addObserver: self
                                             selector: @selector(openNetInfoChanged:)
                                                 name: NSControlTextDidChangeNotification
                                               object: _netUDPMPortTextField];
    [[NSNotificationCenter defaultCenter] addObserver: self
                                             selector: @selector(openNetInfoChanged:)
                                                 name: NSControlTextDidChangeNotification
                                               object: _netHTTPURLTextField];

    [[NSNotificationCenter defaultCenter] addObserver: self
                                             selector: @selector(screenFPSfieldChanged:)
                                                 name: NSControlTextDidChangeNotification
                                               object: _screenFPSTextField];

    /* register clicks on text fields */
    [[NSNotificationCenter defaultCenter] addObserver: self
                                             selector: @selector(textFieldWasClicked:)
                                                 name: VLCOpenTextFieldWasClicked
                                               object: nil];

    /* we want to be notified about removed or added media */
    _allMediaDevices = [[NSMutableArray alloc] init];
    _specialMediaFolders = [[NSMutableArray alloc] init];
    _displayInfos = [[NSMutableArray alloc] init];
    NSWorkspace *sharedWorkspace = [NSWorkspace sharedWorkspace];
    [[sharedWorkspace notificationCenter] addObserver:self selector:@selector(scanOpticalMedia:) name:NSWorkspaceDidMountNotification object:nil];
    [[sharedWorkspace notificationCenter] addObserver:self selector:@selector(scanOpticalMedia:) name:NSWorkspaceDidUnmountNotification object:nil];

    [self qtkToggleUIElements:nil];
    [self updateMediaSelector:nil];
    [self scanOpticalMedia:nil];

    [self setMRL: @""];
}

- (void)setMRL:(NSString *)newMRL
{
    if (!newMRL)
        newMRL = @"";

    _MRL = newMRL;
    [self.mrlTextField performSelectorOnMainThread:@selector(setStringValue:) withObject:_MRL waitUntilDone:NO];
    if ([_MRL length] > 0)
        [_okButton setEnabled: YES];
    else
        [_okButton setEnabled: NO];
}

- (NSString *)MRL
{
    return _MRL;
}

- (void)setSubPanel
{
    int i_index;
    module_config_t * p_item;

    [_fileSubCheckbox setTitle: _NS("Add Subtitle File:")];
    [_fileSubPathLabel setStringValue: _NS("Choose a file")];
    [_fileSubPathLabel setHidden: NO];
    [_fileSubPathTextField setStringValue: @""];
    [_fileSubSettingsButton setTitle: _NS("Choose...")];
    [[_fileSubSettingsButton cell] accessibilitySetOverrideValue:_NS("Setup subtitle playback details") forAttribute:NSAccessibilityDescriptionAttribute];
    [[_fileBrowseButton cell] accessibilitySetOverrideValue:_NS("Select a file for playback") forAttribute:NSAccessibilityDescriptionAttribute];
    [_fileSubBrowseButton setTitle: _NS("Browse...")];
    [[_fileSubBrowseButton cell] accessibilitySetOverrideValue:_NS("Select a subtitle file") forAttribute:NSAccessibilityDescriptionAttribute];
    [_fileSubOverrideCheckbox setTitle: _NS("Override parameters")];
    [_fileSubDelayLabel setStringValue: _NS("Delay")];
    [_fileSubDelayStepper setEnabled: NO];
    [_fileSubFPSLabel setStringValue: _NS("FPS")];
    [_fileSubFPSStepper setEnabled: NO];
    [_fileSubEncodingLabel setStringValue: _NS("Subtitle encoding")];
    [_fileSubEncodingPopup removeAllItems];
    [_fileSubSizeLabel setStringValue: _NS("Font size")];
    [_fileSubSizePopup removeAllItems];
    [_fileSubAlignLabel setStringValue: _NS("Subtitle alignment")];
    [_fileSubAlignPopup removeAllItems];
    [_fileSubOKButton setStringValue: _NS("OK")];
    [[_fileSubOKButton cell] accessibilitySetOverrideValue:_NS("Dismiss the subtitle setup dialog") forAttribute:NSAccessibilityDescriptionAttribute];
    [_fileSubFontBox setTitle: _NS("Font Properties")];
    [_fileSubFileBox setTitle: _NS("Subtitle File")];

    [[_fileSubDelayTextField formatter] setFormat:[NSString stringWithFormat:@"#,##0.000 %@", _NS("s")]];
    [[_fileSubFPSTextField formatter] setFormat:[NSString stringWithFormat:@"#,##0.000 %@", _NS("fps")]];
    self.fileSubFps = 1.0;

    p_item = config_FindConfig("subsdec-encoding");

    if (p_item) {
        for (int i = 0; i < p_item->list_count; i++) {
            [_fileSubEncodingPopup addItemWithTitle: _NS(p_item->list_text[i])];
            [[_fileSubEncodingPopup lastItem] setRepresentedObject:[NSString stringWithFormat:@"%s", p_item->list.psz[i]]];
            if (p_item->value.psz && !strcmp(p_item->value.psz, p_item->list.psz[i]))
                [_fileSubEncodingPopup selectItem: [_fileSubEncodingPopup lastItem]];
        }

        if ([_fileSubEncodingPopup indexOfSelectedItem] < 0)
            [_fileSubEncodingPopup selectItemAtIndex:0];
    }

    p_item = config_FindConfig("subsdec-align");

    if (p_item) {
        for (i_index = 0; i_index < p_item->list_count; i_index++)
            [_fileSubAlignPopup addItemWithTitle: _NS(p_item->list_text[i_index])];

        [_fileSubAlignPopup selectItemAtIndex: p_item->value.i];
    }

    p_item = config_FindConfig("freetype-rel-fontsize");

    if (p_item) {
        for (i_index = 0; i_index < p_item->list_count; i_index++) {
            [_fileSubSizePopup addItemWithTitle: _NS(p_item->list_text[i_index])];

            if (p_item->value.i == p_item->list.i[i_index])
                [_fileSubSizePopup selectItemAtIndex: i_index];
        }
    }
}

- (void)openTarget:(NSString *)identifier
{
    /* check whether we already run a modal dialog */
    if ([NSApp modalWindow] != nil)
        return;

    // load window
    [self window];

    // Delay this to allow the full animation to run inside the modal event loop
    dispatch_async(dispatch_get_main_queue(), ^{
        [_tabView selectTabViewItemWithIdentifier:identifier];
        [_fileSubCheckbox setState: NSOffState];
    });

    int i_result = [NSApp runModalForWindow: self.window];
    [self.window close];

    // Check if dialog was canceled or stopped (NSModalResponseStop)
    if (i_result <= 0)
        return;


    NSMutableDictionary *itemOptionsDictionary;
    NSMutableArray *options = [NSMutableArray array];

    itemOptionsDictionary = [NSMutableDictionary dictionaryWithObject: [self MRL] forKey: @"ITEM_URL"];
    if ([_fileSubCheckbox state] == NSOnState) {
        module_config_t * p_item;

        [options addObject: [NSString stringWithFormat: @"sub-file=%@", _subPath]];
        if ([_fileSubOverrideCheckbox state] == NSOnState) {
            [options addObject: [NSString stringWithFormat: @"sub-delay=%f", ([self fileSubDelay] * 10)]];
            [options addObject: [NSString stringWithFormat: @"sub-fps=%f", [self fileSubFps]]];
        }
        [options addObject: [NSString stringWithFormat:
                             @"subsdec-encoding=%@", [[_fileSubEncodingPopup selectedItem] representedObject]]];
        [options addObject: [NSString stringWithFormat:
                             @"subsdec-align=%li", [_fileSubAlignPopup indexOfSelectedItem]]];

        p_item = config_FindConfig("freetype-rel-fontsize");

        if (p_item) {
            [options addObject: [NSString stringWithFormat:
                                 @"freetype-rel-fontsize=%i",
                                 p_item->list.i[[_fileSubSizePopup indexOfSelectedItem]]]];
        }
    }
    if ([_fileCustomTimingCheckbox state] == NSOnState) {
        NSArray *components = [[_fileStartTimeTextField stringValue] componentsSeparatedByString:@":"];
        NSUInteger componentCount = [components count];
        NSInteger tempValue = 0;
        if (componentCount == 1)
            tempValue = [[components firstObject] intValue];
        else if (componentCount == 2)
            tempValue = [[components firstObject] intValue] * 60 + [[components objectAtIndex:1] intValue];
        else if (componentCount == 3)
            tempValue = [[components firstObject] intValue] * 3600 + [[components objectAtIndex:1] intValue] * 60 + [[components objectAtIndex:2] intValue];
        if (tempValue > 0)
            [options addObject: [NSString stringWithFormat:@"start-time=%li", tempValue]];
        components = [[_fileStopTimeTextField stringValue] componentsSeparatedByString:@":"];
        componentCount = [components count];
        if (componentCount == 1)
            tempValue = [[components firstObject] intValue];
        else if (componentCount == 2)
            tempValue = [[components firstObject] intValue] * 60 + [[components objectAtIndex:1] intValue];
        else if (componentCount == 3)
            tempValue = [[components firstObject] intValue] * 3600 + [[components objectAtIndex:1] intValue] * 60 + [[components objectAtIndex:2] intValue];
        if (tempValue != 0)
            [options addObject: [NSString stringWithFormat:@"stop-time=%li", tempValue]];
    }
    if ([_outputCheckbox state] == NSOnState) {
        NSArray *soutMRL = [_output soutMRL];
        NSUInteger count = [soutMRL count];
        for (NSUInteger i = 0 ; i < count ; i++)
            [options addObject: [NSString stringWithString: [soutMRL objectAtIndex:i]]];
    }
    if ([_fileSlaveCheckbox state] && _fileSlavePath)
        [options addObject: [NSString stringWithFormat: @"input-slave=%@", _fileSlavePath]];
    if ([[[_tabView selectedTabViewItem] identifier] isEqualToString: kCaptureTabViewId]) {
        if ([[[_captureModePopup selectedItem] title] isEqualToString: _NS("Screen")]) {
            int selected_index = [_screenPopup indexOfSelectedItem];
            NSValue *v = [_displayInfos objectAtIndex:selected_index];
            struct display_info_t *item = (struct display_info_t *)[v pointerValue];

            [options addObject: [NSString stringWithFormat: @"screen-fps=%f", [_screenFPSTextField floatValue]]];
            [options addObject: [NSString stringWithFormat: @"screen-display-id=%i", item->id]];
            [options addObject: [NSString stringWithFormat: @"screen-left=%i", [_screenLeftTextField intValue]]];
            [options addObject: [NSString stringWithFormat: @"screen-top=%i", [_screenTopTextField intValue]]];
            [options addObject: [NSString stringWithFormat: @"screen-width=%i", [_screenWidthTextField intValue]]];
            [options addObject: [NSString stringWithFormat: @"screen-height=%i", [_screenHeightTextField intValue]]];
            if ([_screenFollowMouseCheckbox intValue] == YES)
                [options addObject: @"screen-follow-mouse"];
            else
                [options addObject: @"no-screen-follow-mouse"];
            if ([_screenqtkAudioCheckbox state] && _avCurrentAudioDeviceUID)
                [options addObject: [NSString stringWithFormat: @"input-slave=avaudiocapture://%@", _avCurrentAudioDeviceUID]];
        }
        else if ([[[_captureModePopup selectedItem] title] isEqualToString: _NS("Input Devices")]) {
            if ([_qtkVideoCheckbox state]) {
                if ([_qtkAudioCheckbox state] && _avCurrentAudioDeviceUID)
                    [options addObject: [NSString stringWithFormat: @"input-slave=avaudiocapture://%@", _avCurrentAudioDeviceUID]];
            }
        }
    }

    /* apply the options to our item(s) */
    [itemOptionsDictionary setObject: (NSArray *)[options copy] forKey: @"ITEM_OPTIONS"];

    [[[VLCMain sharedInstance] playlist] addPlaylistItems:[NSArray arrayWithObject:itemOptionsDictionary]];
}

#pragma mark -
#pragma mark Main Actions


- (void)tabView:(NSTabView *)o_tv didSelectTabViewItem:(NSTabViewItem *)tabViewItem
{
    NSString *identifier = [tabViewItem identifier];

    if ([identifier isEqualToString: kFileTabViewId])
        [self openFilePathChanged: nil];
    else if ([identifier isEqualToString: kDiscTabViewId])
        [self scanOpticalMedia: nil];
    else if ([identifier isEqualToString: kNetworkTabViewId]) {
        [self openNetInfoChanged: nil];
        [_netHTTPURLTextField selectText:nil];
    } else if ([identifier isEqualToString: kCaptureTabViewId])
        [self openCaptureModeChanged: nil];
}

- (IBAction)expandMRLfieldAction:(id)sender
{
    if ([_mrlButton state] == NSOffState) {
        self.mrlViewHeightConstraint.animator.constant = 0;
    } else {
        self.mrlViewHeightConstraint.animator.constant = 39;
    }
}

- (void)openFileGeneric
{
    [self openFilePathChanged: nil];
    [self openTarget: kFileTabViewId];
}

- (void)openDisc
{
    @synchronized (self) {
        [_specialMediaFolders removeAllObjects];
    }

    [self scanOpticalMedia: nil];
    [self openTarget: kDiscTabViewId];
}

- (void)openNet
{
    [self openNetInfoChanged: nil];
    [self openTarget: kNetworkTabViewId];
    [_netHTTPURLTextField selectText:nil];
}

- (void)openCapture
{
    [self openCaptureModeChanged: nil];
    [self openTarget: kCaptureTabViewId];
}

- (void)openFileWithAction:(void (^)(NSArray *files))action;
{
    NSOpenPanel *openPanel = [NSOpenPanel openPanel];
    [openPanel setAllowsMultipleSelection: YES];
    [openPanel setCanChooseDirectories: YES];
    [openPanel setTitle: _NS("Open File")];
    [openPanel setPrompt: _NS("Open")];

    if ([openPanel runModal] == NSOKButton) {
        NSArray *URLs = [openPanel URLs];
        NSUInteger count = [URLs count];
        NSMutableArray *values = [NSMutableArray arrayWithCapacity:count];
        NSMutableArray *array = [NSMutableArray arrayWithCapacity:count];
        for (NSUInteger i = 0; i < count; i++)
            [values addObject: [[URLs objectAtIndex:i] path]];
        [values sortUsingSelector:@selector(caseInsensitiveCompare:)];

        for (NSUInteger i = 0; i < count; i++) {
            NSDictionary *dictionary;
            char *psz_uri = vlc_path2uri([[values objectAtIndex:i] UTF8String], "file");
            if (!psz_uri)
                continue;
            dictionary = [NSDictionary dictionaryWithObject:toNSStr(psz_uri) forKey:@"ITEM_URL"];
            free(psz_uri);
            [array addObject: dictionary];
        }

        action(array);
    }
}

- (IBAction)outputSettings:(id)sender
{
    if (sender == self.outputCheckbox) {
        self.outputSettingsButton.enabled = self.outputCheckbox.state;
        return;
    }

    if (!b_outputNibLoaded)
        b_outputNibLoaded = [NSBundle loadNibNamed:@"StreamOutput" owner:_output];

    [NSApp beginSheet:_output.outputSheet
       modalForWindow:self.window
        modalDelegate:self
       didEndSelector:NULL
          contextInfo:nil];
}

#pragma mark -
#pragma mark File Panel

- (void)openFilePathChanged:(NSNotification *)o_notification
{
    if (_filePath && [_filePath length] > 0) {
        bool b_stream = [_fileTreatAsPipeButton state];
        BOOL b_dir = NO;

        [[NSFileManager defaultManager] fileExistsAtPath:_filePath isDirectory:&b_dir];

        char *psz_uri = vlc_path2uri([_filePath UTF8String], "file");
        if (!psz_uri) return;

        NSMutableString *mrlString = [NSMutableString stringWithUTF8String:psz_uri];
        NSRange offile = [mrlString rangeOfString:@"file"];
        free(psz_uri);

        if (b_dir)
            [mrlString replaceCharactersInRange:offile withString: @"directory"];
        else if (b_stream)
            [mrlString replaceCharactersInRange:offile withString: @"stream"];

        [_fileNameLabel setStringValue: [[NSFileManager defaultManager] displayNameAtPath:_filePath]];
        [_fileNameStubLabel setHidden: YES];
        [_fileTreatAsPipeButton setHidden: NO];
        [_fileIconWell setImage: [[NSWorkspace sharedWorkspace] iconForFile: _filePath]];
        [_fileIconWell setHidden: NO];
        [self setMRL: mrlString];
    } else {
        [_fileNameLabel setStringValue: @""];
        [_fileNameStubLabel setHidden: NO];
        [_fileTreatAsPipeButton setHidden: YES];
        [_fileIconWell setImage: [NSImage imageNamed:@"generic"]];
        [self setMRL: @""];
    }
}

- (IBAction)openFileBrowse:(id)sender
{
    NSOpenPanel *openPanel = [NSOpenPanel openPanel];
    [openPanel setAllowsMultipleSelection: NO];
    [openPanel setCanChooseDirectories: YES];
    [openPanel setTitle: _NS("Open File")];
    [openPanel setPrompt: _NS("Open")];
    [openPanel beginSheetModalForWindow:[sender window] completionHandler:^(NSInteger returnCode) {
        if (returnCode == NSFileHandlingPanelOKButton) {
            _filePath = [[[openPanel URLs] firstObject] path];
            [self openFilePathChanged: nil];
        }
    }];
}

- (IBAction)openFileStreamChanged:(id)sender
{
    [self openFilePathChanged: nil];
}

- (IBAction)inputSlaveAction:(id)sender
{
    if (sender == _fileSlaveCheckbox)
        [_fileSelectSlaveButton setEnabled: [_fileSlaveCheckbox state]];
    else {
        NSOpenPanel *openPanel;
        openPanel = [NSOpenPanel openPanel];
        [openPanel setCanChooseFiles: YES];
        [openPanel setCanChooseDirectories: NO];
        if ([openPanel runModal] == NSOKButton) {
            _fileSlavePath = [[[openPanel URLs] firstObject] path];
        }
    }
    if (_fileSlavePath && [_fileSlaveCheckbox state] == NSOnState) {
        [_fileSlaveFilenameLabel setStringValue: [[NSFileManager defaultManager] displayNameAtPath:_fileSlavePath]];
        [_fileSlaveIconWell setImage: [[NSWorkspace sharedWorkspace] iconForFile: _fileSlavePath]];
    } else {
        [_fileSlaveFilenameLabel setStringValue: @""];
        [_fileSlaveIconWell setImage: NULL];
    }
}

- (IBAction)fileTimeCustomization:(id)sender
{
    BOOL b_value = [_fileCustomTimingCheckbox state];
    [_fileStartTimeTextField setEnabled: b_value];
    [_fileStartTimeLabel setEnabled: b_value];
    [_fileStopTimeTextField setEnabled: b_value];
    [_fileStopTimeLabel setEnabled: b_value];
}

#pragma mark -
#pragma mark Optical Media Panel

- (void)showOpticalMediaView:(NSView *)theView withIcon:(NSImage *)icon
{
    NSRect viewRect = [theView frame];
    [theView setFrame: NSMakeRect(233, 0, viewRect.size.width, viewRect.size.height)];
    [theView setAutoresizesSubviews: YES];

    NSView *opticalTabView = [[_tabView tabViewItemAtIndex: [_tabView indexOfTabViewItemWithIdentifier:kDiscTabViewId]] view];
    if (_currentOpticalMediaView) {
        [[opticalTabView animator] replaceSubview: _currentOpticalMediaView with: theView];
    }
    else
        [[opticalTabView animator] addSubview: theView];
    _currentOpticalMediaView = theView;

    NSImageView *imageView = [[NSImageView alloc] init];
    [imageView setFrame: NSMakeRect(53, 61, 128, 128)];
    [icon setSize: NSMakeSize(128,128)];
    [imageView setImage: icon];
    if (_currentOpticalMediaIconView) {
        [[opticalTabView animator] replaceSubview: _currentOpticalMediaIconView with: imageView];
    }
    else
        [[opticalTabView animator] addSubview: imageView];
    _currentOpticalMediaIconView = imageView;
    [_currentOpticalMediaView setNeedsDisplay: YES];
    [_currentOpticalMediaIconView setNeedsDisplay: YES];
    [opticalTabView setNeedsDisplay: YES];
    [opticalTabView displayIfNeeded];
}

- (void)showOpticalAtPath: (NSDictionary *)valueDictionary
{
    NSString *diskType = [valueDictionary objectForKey:@"mediaType"];
    NSString *opticalDevicePath = [valueDictionary objectForKey:@"path"];
    NSString *devicePath = [valueDictionary objectForKey:@"devicePath"];
    NSImage *image = [valueDictionary objectForKey:@"image"];

    if ([diskType isEqualToString: kVLCMediaDVD] || [diskType isEqualToString: kVLCMediaVideoTSFolder]) {
        [_discDVDLabel setStringValue: [[NSFileManager defaultManager] displayNameAtPath:opticalDevicePath]];
        [_discDVDwomenusLabel setStringValue: [_discDVDLabel stringValue]];

        if (!b_nodvdmenus) {
            [self setMRL: [NSString stringWithFormat: @"dvdnav://%@", devicePath]];
            [self showOpticalMediaView: _discDVDView withIcon:image];
        } else {
            [self setMRL: [NSString stringWithFormat: @"dvdread://%@#%i:%i-", devicePath, [_discDVDwomenusTitleTextField intValue], [_discDVDwomenusChapterTextField intValue]]];
            [self showOpticalMediaView: _discDVDwomenusView withIcon:image];
        }
    } else if ([diskType isEqualToString: kVLCMediaAudioCD]) {
        [_discAudioCDLabel setStringValue: [[NSFileManager defaultManager] displayNameAtPath: opticalDevicePath]];
        [_discAudioCDTrackCountLabel setStringValue: [NSString stringWithFormat:_NS("%i tracks"), [[[NSFileManager defaultManager] subpathsOfDirectoryAtPath: opticalDevicePath error:NULL] count] - 1]]; // minus .TOC.plist
        [self showOpticalMediaView: _discAudioCDView withIcon: image];
        [self setMRL: [NSString stringWithFormat: @"cdda://%@", devicePath]];
    } else if ([diskType isEqualToString: kVLCMediaVCD]) {
        [_discVCDLabel setStringValue: [[NSFileManager defaultManager] displayNameAtPath: opticalDevicePath]];
        [self showOpticalMediaView: _discVCDView withIcon: image];
        [self setMRL: [NSString stringWithFormat: @"vcd://%@#%i:%i", devicePath, [_discVCDTitleTextField intValue], [_discVCDChapterTextField intValue]]];
    } else if ([diskType isEqualToString: kVLCMediaSVCD]) {
        [_discVCDLabel setStringValue: [[NSFileManager defaultManager] displayNameAtPath: opticalDevicePath]];
        [self showOpticalMediaView: _discVCDView withIcon: image];
        [self setMRL: [NSString stringWithFormat: @"vcd://%@@%i:%i", devicePath, [_discVCDTitleTextField intValue], [_discVCDChapterTextField intValue]]];
    } else if ([diskType isEqualToString: kVLCMediaBD] || [diskType isEqualToString: kVLCMediaBDMVFolder]) {
        [_discBDLabel setStringValue: [[NSFileManager defaultManager] displayNameAtPath: opticalDevicePath]];
        [self showOpticalMediaView: _discBDView withIcon: image];
        [self setMRL: [NSString stringWithFormat: @"bluray://%@", opticalDevicePath]];
    } else {
        if (getIntf())
            msg_Warn(getIntf(), "unknown disk type, no idea what to display");

        [self showOpticalMediaView: _discNoDiscView withIcon: [NSImage imageNamed:@"NSApplicationIcon"]];
    }
}

- (NSDictionary *)scanPath:(NSString *)path
{
    NSString *type = [[VLCStringUtility sharedInstance] getVolumeTypeFromMountPath:path];
    NSImage *image = [[NSWorkspace sharedWorkspace] iconForFile: path];
    NSString *devicePath;

    // BDMV path must not end with BDMV directory
    if([type isEqualToString: kVLCMediaBDMVFolder]) {
        if([[path lastPathComponent] isEqualToString: @"BDMV"]) {
            path = [path stringByDeletingLastPathComponent];
        }
    }

    if ([type isEqualToString: kVLCMediaVideoTSFolder] ||
        [type isEqualToString: kVLCMediaBD] ||
        [type isEqualToString: kVLCMediaBDMVFolder] ||
        [type isEqualToString: kVLCMediaUnknown])
        devicePath = path;
    else
        devicePath = [[VLCStringUtility sharedInstance] getBSDNodeFromMountPath:path];

    return [NSDictionary dictionaryWithObjectsAndKeys: path, @"path",
            devicePath, @"devicePath",
            type, @"mediaType",
            image, @"image", nil];
}

- (void)scanDevices
{
    @autoreleasepool {
        NSArray *paths = [NSArray arrayWithArray:[[NSWorkspace sharedWorkspace] mountedRemovableMedia]];
        NSUInteger count = [paths count];
        NSMutableArray *o_result = [NSMutableArray array];
        for (NSUInteger i = 0; i < count; i++)
            [o_result addObject: [self scanPath:[paths objectAtIndex:i]]];

        @synchronized (self) {
            _opticalDevices = [[NSArray alloc] initWithArray: o_result];
        }

        [self performSelectorOnMainThread:@selector(updateMediaSelector:) withObject:nil waitUntilDone:NO];
    }
}

- (void)scanSpecialPath:(NSString *)oPath
{
    @autoreleasepool {
        NSDictionary *o_dict = [self scanPath:oPath];

        @synchronized (self) {
            [_specialMediaFolders addObject:o_dict];
        }

        [self performSelectorOnMainThread:@selector(updateMediaSelector:) withObject:[NSNumber numberWithBool:YES] waitUntilDone:NO];
    }
}

- (void)scanOpticalMedia:(NSNotification *)o_notification
{
    [NSThread detachNewThreadSelector:@selector(scanDevices) toTarget:self withObject:nil];
}

- (void)updateMediaSelector:(NSNumber *)selection
{
    [_allMediaDevices removeAllObjects];
    [_discSelectorPopup removeAllItems];

    @synchronized (self) {
        [_allMediaDevices addObjectsFromArray:_opticalDevices];
        [_allMediaDevices addObjectsFromArray:_specialMediaFolders];
    }

    NSUInteger count = [_allMediaDevices count];
    if (count > 0) {
        for (NSUInteger i = 0; i < count ; i++) {
            NSDictionary *o_dict = [_allMediaDevices objectAtIndex:i];
            [_discSelectorPopup addItemWithTitle: [[NSFileManager defaultManager] displayNameAtPath:[o_dict objectForKey:@"path"]]];
        }

        if ([_discSelectorPopup numberOfItems] <= 1)
            [_discSelectorPopup setHidden: YES];
        else
            [_discSelectorPopup setHidden: NO];

        // select newly added media folder
        if (selection && [selection boolValue])
            [_discSelectorPopup selectItemAtIndex: [[_discSelectorPopup itemArray] count] - 1];

        // only trigger MRL update if the tab view is active
        if ([[[_tabView selectedTabViewItem] identifier] isEqualToString:kDiscTabViewId])
            [self discSelectorChanged:nil];
    } else {
        msg_Dbg(getIntf(), "no optical media found");
        [_discSelectorPopup setHidden: YES];
        [self setMRL:@""];
        [self showOpticalMediaView: _discNoDiscView withIcon: [NSImage imageNamed: @"NSApplicationIcon"]];
    }
}

- (IBAction)discSelectorChanged:(id)sender
{
    [self showOpticalAtPath:[_allMediaDevices objectAtIndex:[_discSelectorPopup indexOfSelectedItem]]];
}

- (IBAction)openSpecialMediaFolder:(id)sender
{
    /* this is currently for VIDEO_TS and BDMV folders */
    NSOpenPanel *openPanel = [NSOpenPanel openPanel];
    [openPanel setAllowsMultipleSelection: NO];
    [openPanel setCanChooseDirectories: YES];
    [openPanel setTitle: [sender title]];
    [openPanel setPrompt: _NS("Open")];

    /* work-around for Mountain Lion, which treats folders called "BDMV" including an item named "INDEX.BDM"
     * as a _FILE_. Don't ask, move on. There is nothing to see here */
    [openPanel setCanChooseFiles: YES];
    [openPanel setAllowedFileTypes:[NSArray arrayWithObject:@"public.directory"]];

    if ([openPanel runModal] == NSOKButton) {
        NSString *oPath = [[[openPanel URLs] firstObject] path];
        if ([oPath length] > 0) {
            [NSThread detachNewThreadSelector:@selector(scanSpecialPath:) toTarget:self withObject:oPath];
        }
    }
}

- (IBAction)dvdreadOptionChanged:(id)sender
{
    NSString *devicePath = [[_allMediaDevices objectAtIndex:[_discSelectorPopup indexOfSelectedItem]] objectForKey:@"devicePath"];

    if (sender == _discDVDwomenusEnableMenusButton) {
        b_nodvdmenus = NO;
        [self setMRL: [NSString stringWithFormat: @"dvdnav://%@", devicePath]];
        [self showOpticalMediaView:_discDVDView withIcon:[_currentOpticalMediaIconView image]];
        return;
    }
    if (sender == _discDVDDisableMenusButton) {
        b_nodvdmenus = YES;
        [self showOpticalMediaView:_discDVDwomenusView withIcon:[_currentOpticalMediaIconView image]];
    }

    if (sender == _discDVDwomenusTitleTextField)
        [_discDVDwomenusTitleStepper setIntValue: [_discDVDwomenusTitleTextField intValue]];
    if (sender == _discDVDwomenusTitleStepper)
        [_discDVDwomenusTitleTextField setIntValue: [_discDVDwomenusTitleStepper intValue]];
    if (sender == _discDVDwomenusChapterTextField)
        [_discDVDwomenusChapterStepper setIntValue: [_discDVDwomenusChapterTextField intValue]];
    if (sender == _discDVDwomenusChapterStepper)
        [_discDVDwomenusChapterTextField setIntValue: [_discDVDwomenusChapterStepper intValue]];

    [self setMRL: [NSString stringWithFormat: @"dvdread://%@#%i:%i-", devicePath, [_discDVDwomenusTitleTextField intValue], [_discDVDwomenusChapterTextField intValue]]];
}

- (IBAction)vcdOptionChanged:(id)sender
{
    if (sender == _discVCDTitleTextField)
        [_discVCDTitleStepper setIntValue: [_discVCDTitleTextField intValue]];
    if (sender == _discVCDTitleStepper)
        [_discVCDTitleTextField setIntValue: [_discVCDTitleStepper intValue]];
    if (sender == _discVCDChapterTextField)
        [_discVCDChapterStepper setIntValue: [_discVCDChapterTextField intValue]];
    if (sender == _discVCDChapterStepper)
        [_discVCDChapterTextField setIntValue: [_discVCDChapterStepper intValue]];

    NSString *devicePath = [[_allMediaDevices objectAtIndex:[_discSelectorPopup indexOfSelectedItem]] objectForKey:@"devicePath"];
    [self setMRL: [NSString stringWithFormat: @"vcd://%@@%i:%i", devicePath, [_discVCDTitleTextField intValue], [_discVCDChapterTextField intValue]]];
}

#pragma mark -
#pragma mark Network Panel

- (void)textFieldWasClicked:(NSNotification *)notification
{
    if ([notification object] == _netUDPPortTextField)
        [_netModeMatrix selectCellAtRow: 0 column: 0];
    else if ([notification object] == _netUDPMAddressTextField ||
             [notification object] == _netUDPMPortTextField)
        [_netModeMatrix selectCellAtRow: 1 column: 0];
    else
        [_netModeMatrix selectCellAtRow: 2 column: 0];

    [self openNetInfoChanged: nil];
}

- (IBAction)openNetModeChanged:(id)sender
{
    if (sender == _netModeMatrix) {
        if ([[sender selectedCell] tag] == 0)
            [self.window makeFirstResponder: _netUDPPortTextField];
        else if ([[sender selectedCell] tag] == 1)
            [self.window makeFirstResponder: _netUDPMAddressTextField];
        else
            msg_Warn(getIntf(), "Unknown sender tried to change UDP/RTP mode");
    }

    [self openNetInfoChanged: nil];
}

- (IBAction)openNetStepperChanged:(id)sender
{
    int i_tag = [sender tag];

    if (i_tag == 0) {
        [_netUDPPortTextField setIntValue: [_netUDPPortStepper intValue]];
        [[NSNotificationCenter defaultCenter] postNotificationName: VLCOpenTextFieldWasClicked
                                                            object: _netUDPPortTextField];
        [self.window makeFirstResponder: _netUDPPortTextField];
    }
    else if (i_tag == 1) {
        [_netUDPMPortTextField setIntValue: [_netUDPMPortStepper intValue]];
        [[NSNotificationCenter defaultCenter] postNotificationName: VLCOpenTextFieldWasClicked
                                                            object: _netUDPMPortTextField];
        [self.window makeFirstResponder: _netUDPMPortTextField];
    }

    [self openNetInfoChanged: nil];
}

- (void)openNetInfoChanged:(NSNotification *)o_notification
{
    NSString *mrlString;

    if ([_netUDPPanel isVisible]) {
        NSString *mode;
        mode = [[_netModeMatrix selectedCell] title];

        if ([mode isEqualToString: _NS("Unicast")]) {
            int port = [_netUDPPortTextField intValue];

            if ([[_netUDPProtocolMatrix selectedCell] tag] == 0)
                mrlString = @"udp://";
            else
                mrlString = @"rtp://";

            if (port != config_GetInt(getIntf(), "server-port")) {
                mrlString =
                [mrlString stringByAppendingFormat: @"@:%i", port];
            }
        }
        else if ([mode isEqualToString: _NS("Multicast")]) {
            NSString *oAddress = [_netUDPMAddressTextField stringValue];
            int iPort = [_netUDPMPortTextField intValue];

            if ([[_netUDPProtocolMatrix selectedCell] tag] == 0)
                mrlString = [NSString stringWithFormat: @"udp://@%@", oAddress];
            else
                mrlString = [NSString stringWithFormat: @"rtp://@%@", oAddress];

            if (iPort != config_GetInt(getIntf(), "server-port")) {
                mrlString =
                [mrlString stringByAppendingFormat: @":%i", iPort];
            }
        }
    } else {
        mrlString = [_netHTTPURLTextField stringValue];

        // Fixup the user-provided URI
        const char *orig_uri = [mrlString UTF8String];
        if (orig_uri == NULL)
            return;
        char *fixed_uri = vlc_uri_fixup(orig_uri);
        if (fixed_uri) {
            mrlString = [[NSString alloc] initWithUTF8String:fixed_uri];
            free(fixed_uri);
        }
    }

    [self setMRL: mrlString];
}

- (IBAction)openNetUDPButtonAction:(id)sender
{
    if (sender == _netOpenUDPButton) {
        [NSApp beginSheet: self.netUDPPanel
           modalForWindow: self.window
            modalDelegate: self
           didEndSelector: NULL
              contextInfo: nil];
        [self openNetInfoChanged:nil];
    }
    else if (sender == _netUDPCancelButton) {
        [_netUDPPanel orderOut: sender];
        [NSApp endSheet: _netUDPPanel];
    }
    else if (sender == _netUDPOKButton) {
        NSString *mrlString;
        if ([[[_netModeMatrix selectedCell] title] isEqualToString: _NS("Unicast")]) {
            int port = [_netUDPPortTextField intValue];

            if ([[_netUDPProtocolMatrix selectedCell] tag] == 0)
                mrlString = @"udp://";
            else
                mrlString = @"rtp://";

            if (port != config_GetInt(getIntf(), "server-port")) {
                mrlString =
                [mrlString stringByAppendingFormat: @"@:%i", port];
            }
        }
        else if ([[[_netModeMatrix selectedCell] title] isEqualToString: _NS("Multicast")]) {
            NSString *oAddress = [_netUDPMAddressTextField stringValue];
            int iPort = [_netUDPMPortTextField intValue];

            if ([[_netUDPProtocolMatrix selectedCell] tag] == 0)
                mrlString = [NSString stringWithFormat: @"udp://@%@", oAddress];
            else
                mrlString = [NSString stringWithFormat: @"rtp://@%@", oAddress];

            if (iPort != config_GetInt(getIntf(), "server-port")) {
                mrlString =
                [mrlString stringByAppendingFormat: @":%i", iPort];
            }
        }
        [self setMRL: mrlString];
        [_netHTTPURLTextField setStringValue: mrlString];
        [_netUDPPanel orderOut: sender];
        [NSApp endSheet: _netUDPPanel];
    }
}

#pragma mark -
#pragma mark Capture Panel

- (IBAction)openCaptureModeChanged:(id)sender
{
    intf_thread_t * p_intf = getIntf();

    if ([[[_captureModePopup selectedItem] title] isEqualToString: _NS("Screen")]) {
        [_captureTabView selectTabViewItemAtIndex:1];

        [self setMRL: @"screen://"];
        [_screenHeightTextField setIntValue: config_GetInt(p_intf, "screen-height")];
        [_screenWidthTextField setIntValue: config_GetInt(p_intf, "screen-width")];
        [_screenFPSTextField setFloatValue: config_GetFloat(p_intf, "screen-fps")];
        [_screenLeftTextField setIntValue: config_GetInt(p_intf, "screen-left")];
        [_screenTopTextField setIntValue: config_GetInt(p_intf, "screen-top")];
        [_screenFollowMouseCheckbox setIntValue: config_GetInt(p_intf, "screen-follow-mouse")];

        int screenIindex = config_GetInt(p_intf, "screen-index");
        int displayID = config_GetInt(p_intf, "screen-display-id");
        unsigned int displayCount = 0;
        CGError returnedError;
        struct display_info_t *item;
        NSValue *v;

        returnedError = CGGetOnlineDisplayList(0, NULL, &displayCount);
        if (!returnedError) {
            CGDirectDisplayID *ids;
            ids = (CGDirectDisplayID *)vlc_alloc(displayCount, sizeof(CGDirectDisplayID));
            returnedError = CGGetOnlineDisplayList(displayCount, ids, &displayCount);
            if (!returnedError) {
                NSUInteger displayInfoCount = [_displayInfos count];
                for (NSUInteger i = 0; i < displayInfoCount; i ++) {
                    v = [_displayInfos objectAtIndex:i];
                    free([v pointerValue]);
                }
                [_displayInfos removeAllObjects];
                [_screenPopup removeAllItems];
                for (unsigned int i = 0; i < displayCount; i ++) {
                    item = (struct display_info_t *)malloc(sizeof(struct display_info_t));
                    item->id = ids[i];
                    item->rect = CGDisplayBounds(item->id);
                    [_screenPopup addItemWithTitle: [NSString stringWithFormat:@"Screen %d (%dx%d)", i + 1, (int)item->rect.size.width, (int)item->rect.size.height]];
                    v = [NSValue valueWithPointer:item];
                    [_displayInfos addObject:v];
                    if (i == 0 || displayID == item->id || screenIindex - 1 == i) {
                        [_screenPopup selectItemAtIndex: i];
                        [_screenLeftStepper setMaxValue: item->rect.size.width];
                        [_screenTopStepper setMaxValue: item->rect.size.height];
                        [_screenWidthStepper setMaxValue: item->rect.size.width];
                        [_screenHeightStepper setMaxValue: item->rect.size.height];
                    }
                }
            }
            free(ids);
        }
    }
    else if ([[[_captureModePopup selectedItem] title] isEqualToString: _NS("Input Devices")]) {
        [_captureTabView selectTabViewItemAtIndex:0];

        [self setMRL: @""];
        [self qtkToggleUIElements: nil];
    }
}

// Screen actions
- (void)screenFPSfieldChanged:(NSNotification *)o_notification
{
    [_screenFPSStepper setFloatValue: [_screenFPSTextField floatValue]];
    if ([[_screenFPSTextField stringValue] isEqualToString: @""])
        [_screenFPSTextField setFloatValue: 1.0];
    [self setMRL: @"screen://"];
}

- (IBAction)screenChanged:(id)sender
{
    int selected_index = [_screenPopup indexOfSelectedItem];
    if (selected_index >= [_displayInfos count]) return;

    NSValue *v = [_displayInfos objectAtIndex:selected_index];
    struct display_info_t *item = (struct display_info_t *)[v pointerValue];

    [_screenLeftStepper setMaxValue: item->rect.size.width];
    [_screenTopStepper setMaxValue: item->rect.size.height];
    [_screenWidthStepper setMaxValue: item->rect.size.width];
    [_screenHeightStepper setMaxValue: item->rect.size.height];

    [_screenqtkAudioPopup setEnabled: [_screenqtkAudioCheckbox state]];
}

- (IBAction)screenAudioChanged:(id)sender
{
    [_screenqtkAudioPopup setEnabled:_screenqtkAudioCheckbox.state];
}

// QTKit Recording actions

- (void)updateInputDevicesUrl
{
    if ([_qtkVideoCheckbox state] && _avCurrentDeviceUID)
        [self setMRL:[NSString stringWithFormat:@"avcapture://%@", _avCurrentDeviceUID]];
    else if ([_qtkAudioCheckbox state] && _avCurrentAudioDeviceUID)
        [self setMRL:[NSString stringWithFormat:@"avaudiocapture://%@", _avCurrentAudioDeviceUID]];
    else
        [self setMRL:@""];
}

- (IBAction)qtkChanged:(id)sender
{
    NSInteger selectedDevice = [_qtkVideoDevicePopup indexOfSelectedItem];
    if (selectedDevice >= _avvideoDevices.count)
        return;

    _avCurrentDeviceUID = [[(AVCaptureDevice *)[_avvideoDevices objectAtIndex:selectedDevice] uniqueID] stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
    [self updateInputDevicesUrl];
}

- (IBAction)qtkAudioChanged:(id)sender
{
    NSInteger selectedDevice = [_qtkAudioDevicePopup indexOfSelectedItem];
    if (selectedDevice >= _avaudioDevices.count)
        return;

    _avCurrentAudioDeviceUID = [[(AVCaptureDevice *)[_avaudioDevices objectAtIndex:selectedDevice] uniqueID] stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
    [self updateInputDevicesUrl];
}

- (IBAction)qtkToggleUIElements:(id)sender
{
    [_qtkAudioDevicePopup setEnabled:[_qtkAudioCheckbox state]];
    [_qtkVideoDevicePopup setEnabled:[_qtkVideoCheckbox state]];

    [self qtkAudioChanged:sender];
    [self qtkChanged:sender];
}

#pragma mark -
#pragma mark Subtitle Settings

- (IBAction)subsChanged:(id)sender
{
    if ([_fileSubCheckbox state] == NSOnState) {
        [_fileSubSettingsButton setEnabled:YES];
        if (_subPath) {
            [_fileSubtitlesFilenameLabel setStringValue: [[NSFileManager defaultManager] displayNameAtPath:_subPath]];
            [_fileSubtitlesIconWell setImage: [[NSWorkspace sharedWorkspace] iconForFile:_subPath]];
        }
    } else {
        [_fileSubSettingsButton setEnabled:NO];
        [_fileSubtitlesFilenameLabel setStringValue: @""];
        [_fileSubtitlesIconWell setImage: NULL];
    }
}

- (IBAction)subSettings:(id)sender
{
    [NSApp beginSheet: self.fileSubSheet
       modalForWindow: [sender window]
        modalDelegate: self
       didEndSelector: NULL
          contextInfo: nil];
}

- (IBAction)subCloseSheet:(id)sender
{
    [self subsChanged: nil];
    [_fileSubSheet orderOut:sender];
    [NSApp endSheet: _fileSubSheet];
}

- (IBAction)subFileBrowse:(id)sender
{
    NSOpenPanel *openPanel = [NSOpenPanel openPanel];
    [openPanel setAllowsMultipleSelection: NO];
    [openPanel setTitle: _NS("Open File")];
    [openPanel setPrompt: _NS("Open")];

    if ([openPanel runModal] == NSOKButton) {
        _subPath = [[[openPanel URLs] firstObject] path];
        [_fileSubtitlesFilenameLabel setStringValue: [[NSFileManager defaultManager] displayNameAtPath:_subPath]];
        [_fileSubPathTextField setStringValue: [_fileSubtitlesFilenameLabel stringValue]];
        [_fileSubPathLabel setHidden: YES];
        [_fileSubtitlesIconWell setImage: [[NSWorkspace sharedWorkspace] iconForFile:_subPath]];
        [_fileSubIconView setImage: [_fileSubtitlesIconWell image]];
    } else {
        [_fileSubPathLabel setHidden: NO];
        [_fileSubPathTextField setStringValue:@""];
        [_fileSubtitlesFilenameLabel setStringValue:@""];
        [_fileSubtitlesIconWell setImage: nil];
        [_fileSubIconView setImage: nil];
    }
}

- (IBAction)subOverride:(id)sender
{
    BOOL b_state = [_fileSubOverrideCheckbox state];
    [_fileSubDelayTextField setEnabled: b_state];
    [_fileSubDelayStepper setEnabled: b_state];
    [_fileSubFPSTextField setEnabled: b_state];
    [_fileSubFPSStepper setEnabled: b_state];
}

#pragma mark -
#pragma mark Miscellaneous

- (IBAction)panelCancel:(id)sender
{
    [NSApp stopModalWithCode: 0];
}

- (IBAction)panelOk:(id)sender
{
    if ([[self MRL] length])
        [NSApp stopModalWithCode: 1];
    else
        NSBeep();
}

- (void)updateVideoDevices
{
    _avvideoDevices = [[AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo]
                         arrayByAddingObjectsFromArray:[AVCaptureDevice devicesWithMediaType:AVMediaTypeMuxed]];
}

- (void)updateAudioDevices
{
    _avaudioDevices = [[AVCaptureDevice devicesWithMediaType:AVMediaTypeAudio]
                        arrayByAddingObjectsFromArray:[AVCaptureDevice devicesWithMediaType:AVMediaTypeMuxed]];
}

@end
