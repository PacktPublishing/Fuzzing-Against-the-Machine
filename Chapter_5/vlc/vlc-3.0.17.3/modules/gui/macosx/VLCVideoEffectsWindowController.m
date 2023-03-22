/*****************************************************************************
 * VLCVideoEffectsWindowController.m: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2011-2015 Felix Paul Kühne
 * $Id: e5904eb81cc903cc3c800ba3ebd0ce1cece192b3 $
 *
 * Authors: Felix Paul Kühne <fkuehne -at- videolan -dot- org>
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

#import "CompatibilityFixes.h"
#import "VLCMain.h"
#import "VLCVideoEffectsWindowController.h"
#import "VLCPopupPanelController.h"
#import "VLCTextfieldPanelController.h"
#import "VLCCoreInteraction.h"
#import "VLCHexNumberFormatter.h"

#define getWidgetBoolValue(w)   ((vlc_value_t){ .b_bool = [w state] })
#define getWidgetIntValue(w)    ((vlc_value_t){ .i_int = [w intValue] })
#define getWidgetFloatValue(w)  ((vlc_value_t){ .f_float = [w floatValue] })
#define getWidgetStringValue(w) ((vlc_value_t){ .psz_string = (char *)[[w stringValue] UTF8String] })


#pragma mark -
#pragma mark Initialization

@implementation VLCVideoEffectsWindowController

+ (void)initialize
{
    /*
     * Video effects profiles starting with 3.0:
     * - Index 0 is assumed to be the default profile from previous versions
     * - Index 0 from settings is never read or written anymore starting with 3.0, as the Default profile
     *   is not persisted anymore.
     */

    NSDictionary *appDefaults = [NSDictionary dictionaryWithObjectsAndKeys:
                                 [NSArray arrayWithObject:[VLCVideoEffectsWindowController defaultProfileString]], @"VideoEffectProfiles",
                                 [NSArray arrayWithObject:_NS("Default")], @"VideoEffectProfileNames",
                                 nil];
    [[NSUserDefaults standardUserDefaults] registerDefaults:appDefaults];
}

+ (NSString *)defaultProfileString
{
    return @";;;0;1.000000;1.000000;1.000000;1.000000;0.050000;16;2.000000;OTA=;4;4;16711680;20;15;120;Z3JhZGllbnQ=;1;0;16711680;6;80;VkxD;-1;;-1;255;2;3;3;0;-180.000000";
}

- (id)init
{
    self = [super initWithWindowNibName:@"VideoEffects"];
    if (self) {
        self.popupPanel = [[VLCPopupPanelController alloc] init];
        self.textfieldPanel = [[VLCTextfieldPanelController alloc] init];

        NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
        if ([defaults boolForKey:@"VideoEffectApplyProfileOnStartup"]) {
            // This does not reset the UI (which does not exist yet), but it initalizes needed playlist vars
            [self resetValues];

            [self loadProfile];
        } else {
            [self saveCurrentProfileIndex:0];
        }

    }

    return self;
}

/// Loads values from profile into variables
- (void)loadProfile
{
    intf_thread_t *p_intf = getIntf();
    playlist_t *p_playlist = pl_Get(p_intf);
    VLCCoreInteraction *vci_si = [VLCCoreInteraction sharedInstance];
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    NSInteger profileIndex = [self currentProfileIndex];

    NSString *profileString;
    if (profileIndex == 0)
        profileString = [VLCVideoEffectsWindowController defaultProfileString];
    else
        profileString = [[defaults objectForKey:@"VideoEffectProfiles"] objectAtIndex:profileIndex];

    NSArray *items = [profileString componentsSeparatedByString:@";"];

    // version 1 of profile string has 32 entries
    if ([items count] < 32) {
        msg_Err(p_intf, "Error in parsing profile string");
        return;
    }

    /* filter handling */
    NSString *tempString = B64DecNSStr([items firstObject]);
    NSArray<NSValue *> *vouts = getVouts();

    /* enable the new filters */
    var_SetString(p_playlist, "video-filter", [tempString UTF8String]);
    if (vouts)
        for (NSValue *ptr in vouts) {
            vout_thread_t *p_vout = [ptr pointerValue];
            var_SetString(p_vout, "video-filter", [tempString UTF8String]);
        }

    tempString = B64DecNSStr([items objectAtIndex:1]);
    /* enable another round of new filters */
    var_SetString(p_playlist, "sub-source", [tempString UTF8String]);
    if (vouts)
        for (NSValue *ptr in vouts) {
            vout_thread_t *p_vout = [ptr pointerValue];
            var_SetString(p_vout, "sub-source", [tempString UTF8String]);
            vlc_object_release(p_vout);
        }

    tempString = B64DecNSStr([items objectAtIndex:2]);
    /* enable another round of new filters */
    char *psz_current_splitter = var_GetString(p_playlist, "video-splitter");
    bool b_filter_changed = ![tempString isEqualToString:toNSStr(psz_current_splitter)];
    free(psz_current_splitter);

    if (b_filter_changed)
        var_SetString(p_playlist, "video-splitter", [tempString UTF8String]);

    /* try to set filter values on-the-fly and store them appropriately */
    // index 3 is deprecated
    [vci_si setVideoFilterProperty: "contrast" forFilter: "adjust" withValue: getWidgetFloatValue([items objectAtIndex:4])];
    [vci_si setVideoFilterProperty: "brightness" forFilter: "adjust" withValue: getWidgetFloatValue([items objectAtIndex:5])];
    [vci_si setVideoFilterProperty: "saturation" forFilter: "adjust" withValue: getWidgetFloatValue([items objectAtIndex:6])];
    [vci_si setVideoFilterProperty: "gamma" forFilter: "adjust" withValue: getWidgetFloatValue([items objectAtIndex:7])];
    [vci_si setVideoFilterProperty: "sharpen-sigma" forFilter: "sharpen" withValue: getWidgetFloatValue([items objectAtIndex:8])];
    [vci_si setVideoFilterProperty: "gradfun-radius" forFilter: "gradfun" withValue: getWidgetIntValue([items objectAtIndex:9])];
    [vci_si setVideoFilterProperty: "grain-variance" forFilter: "grain" withValue: getWidgetFloatValue([items objectAtIndex:10])];
    [vci_si setVideoFilterProperty: "transform-type" forFilter: "transform" withValue: (vlc_value_t){ .psz_string = (char *)[B64DecNSStr([items objectAtIndex:11]) UTF8String] }];
    [vci_si setVideoFilterProperty: "puzzle-rows" forFilter: "puzzle" withValue: getWidgetIntValue([items objectAtIndex:12])];
    [vci_si setVideoFilterProperty: "puzzle-cols" forFilter: "puzzle" withValue: getWidgetIntValue([items objectAtIndex:13])];
    [vci_si setVideoFilterProperty: "colorthres-color" forFilter: "colorthres" withValue: getWidgetIntValue([items objectAtIndex:14])];
    [vci_si setVideoFilterProperty: "colorthres-saturationthres" forFilter: "colorthres" withValue: getWidgetIntValue([items objectAtIndex:15])];
    [vci_si setVideoFilterProperty: "colorthres-similaritythres" forFilter: "colorthres" withValue: getWidgetIntValue([items objectAtIndex:16])];
    [vci_si setVideoFilterProperty: "sepia-intensity" forFilter: "sepia" withValue: getWidgetIntValue([items objectAtIndex:17])];
    [vci_si setVideoFilterProperty: "gradient-mode" forFilter: "gradient" withValue: (vlc_value_t){ .psz_string = (char *)[B64DecNSStr([items objectAtIndex:18]) UTF8String] }];
    [vci_si setVideoFilterProperty: "gradient-cartoon" forFilter: "gradient" withValue: (vlc_value_t){ .b_bool = [[items objectAtIndex:19] intValue] }];
    [vci_si setVideoFilterProperty: "gradient-type" forFilter: "gradient" withValue: getWidgetIntValue([items objectAtIndex:20])];
    [vci_si setVideoFilterProperty: "extract-component" forFilter: "extract" withValue: getWidgetIntValue([items objectAtIndex:21])];
    [vci_si setVideoFilterProperty: "posterize-level" forFilter: "posterize" withValue: getWidgetIntValue([items objectAtIndex:22])];
    [vci_si setVideoFilterProperty: "blur-factor" forFilter: "motionblur" withValue: getWidgetIntValue([items objectAtIndex:23])];
    [vci_si setVideoFilterProperty: "marq-marquee" forFilter: "marq" withValue: (vlc_value_t){ .psz_string = (char *)[B64DecNSStr([items objectAtIndex:24]) UTF8String] }];
    [vci_si setVideoFilterProperty: "marq-position" forFilter: "marq" withValue: getWidgetIntValue([items objectAtIndex:25])];
    [vci_si setVideoFilterProperty: "logo-file" forFilter: "logo" withValue: (vlc_value_t){ .psz_string = (char *)[B64DecNSStr([items objectAtIndex:26]) UTF8String] }];
    [vci_si setVideoFilterProperty: "logo-position" forFilter: "logo" withValue: getWidgetIntValue([items objectAtIndex:27])];
    [vci_si setVideoFilterProperty: "logo-opacity" forFilter: "logo" withValue: getWidgetIntValue([items objectAtIndex:28])];
    [vci_si setVideoFilterProperty: "clone-count" forFilter: "clone" withValue: getWidgetIntValue([items objectAtIndex:29])];
    [vci_si setVideoFilterProperty: "wall-rows" forFilter: "wall" withValue: getWidgetIntValue([items objectAtIndex:30])];
    [vci_si setVideoFilterProperty: "wall-cols" forFilter: "wall" withValue: getWidgetIntValue([items objectAtIndex:31])];

    if ([items count] >= 33) { // version >=2 of profile string
        [vci_si setVideoFilterProperty: "brightness-threshold" forFilter: "adjust" withValue: (vlc_value_t){ .b_bool = [[items objectAtIndex:32] intValue] }];
    }

    vlc_value_t hueValue;
    if ([items count] >= 34) { // version >=3 of profile string
        hueValue.f_float = [[items objectAtIndex:33] floatValue];
    } else {
        hueValue.f_float = [[items objectAtIndex:3] intValue]; // deprecated since 3.0.0
        // convert to new scale ([0,360] --> [-180,180])
        hueValue.f_float -= 180;
    }
    [vci_si setVideoFilterProperty: "hue" forFilter: "adjust" withValue: hueValue];
}

- (void)windowDidLoad
{
    [self.window setTitle: _NS("Video Effects")];
    [self.window setExcludedFromWindowsMenu:YES];
    [self.window setCollectionBehavior: NSWindowCollectionBehaviorFullScreenAuxiliary];

    [_segmentView setLabel:_NS("Basic") forSegment:0];
    [_segmentView setLabel:_NS("Crop") forSegment:1];
    [_segmentView setLabel:_NS("Geometry") forSegment:2];
    [_segmentView setLabel:_NS("Color") forSegment:3];
    [_segmentView setLabel:_NS("Miscellaneous") forSegment:4];

    [_applyProfileCheckbox setState:[[NSUserDefaults standardUserDefaults] boolForKey:@"VideoEffectApplyProfileOnStartup"]];
    [_applyProfileCheckbox setTitle:_NS("Apply profile at next launch")];

    [self resetProfileSelector];

    [_adjustCheckbox setTitle:_NS("Image Adjust")];
    [_adjustHueLabel setStringValue:_NS("Hue")];
    [_adjustContrastLabel setStringValue:_NS("Contrast")];
    [_adjustBrightnessLabel setStringValue:_NS("Brightness")];
    [_adjustBrightnessCheckbox setTitle:_NS("Brightness Threshold")];
    [_adjustSaturationLabel setStringValue:_NS("Saturation")];
    [_adjustGammaLabel setStringValue:_NS("Gamma")];
    [_adjustResetButton setTitle: _NS("Reset")];
    [_sharpenCheckbox setTitle:_NS("Sharpen")];
    [_sharpenLabel setStringValue:_NS("Sigma")];
    [_bandingCheckbox setTitle:_NS("Banding removal")];
    [_bandingLabel setStringValue:_NS("Radius")];
    [_grainCheckbox setTitle:_NS("Film Grain")];
    [_grainLabel setStringValue:_NS("Variance")];
    [_cropTopLabel setStringValue:_NS("Top")];
    [_cropLeftLabel setStringValue:_NS("Left")];
    [_cropRightLabel setStringValue:_NS("Right")];
    [_cropBottomLabel setStringValue:_NS("Bottom")];
    [_cropSyncTopBottomCheckbox setTitle:_NS("Synchronize top and bottom")];
    [_cropSyncLeftRightCheckbox setTitle:_NS("Synchronize left and right")];

    [_transformCheckbox setTitle:_NS("Transform")];
    [_transformPopup removeAllItems];
    [_transformPopup addItemWithTitle: _NS("Rotate by 90 degrees")];
    [[_transformPopup lastItem] setRepresentedObject: @"90"];
    [[_transformPopup lastItem] setTag: 90];
    [_transformPopup addItemWithTitle: _NS("Rotate by 180 degrees")];
    [[_transformPopup lastItem] setRepresentedObject: @"180"];
    [[_transformPopup lastItem] setTag: 180];
    [_transformPopup addItemWithTitle: _NS("Rotate by 270 degrees")];
    [[_transformPopup lastItem] setRepresentedObject: @"270"];
    [[_transformPopup lastItem] setTag: 270];
    [_transformPopup addItemWithTitle: _NS("Flip horizontally")];
    [[_transformPopup lastItem] setRepresentedObject: @"hflip"];
    [[_transformPopup lastItem] setTag: 1];
    [_transformPopup addItemWithTitle: _NS("Flip vertically")];
    [[_transformPopup lastItem] setRepresentedObject: @"vflip"];
    [[_transformPopup lastItem] setTag: 2];
    [_zoomCheckbox setTitle:_NS("Magnification/Zoom")];
    [_puzzleCheckbox setTitle:_NS("Puzzle game")];
    [_puzzleRowsLabel setStringValue:_NS("Rows")];
    [_puzzleColumnsLabel setStringValue:_NS("Columns")];
    [_cloneCheckbox setTitle:_NS("Clone")];
    [_cloneNumberLabel setStringValue:_NS("Number of clones")];
    [_wallCheckbox setTitle:_NS("Wall")];
    [_wallNumbersOfRowsLabel setStringValue:_NS("Rows")];
    [_wallNumberOfColumnsLabel setStringValue:_NS("Columns")];

    [_thresholdCheckbox setTitle:_NS("Color threshold")];
    [_thresholdColorLabel setStringValue:_NS("Color")];
    [_thresholdColorTextField setFormatter:[[VLCHexNumberFormatter alloc] init]];
    [_thresholdSaturationLabel setStringValue:_NS("Saturation")];
    [_thresholdSimilarityLabel setStringValue:_NS("Similarity")];
    [_sepiaCheckbox setTitle:_NS("Sepia")];
    [_sepiaLabel setStringValue:_NS("Intensity")];
    [_gradientCheckbox setTitle:_NS("Gradient")];
    [_gradientModeLabel setStringValue:_NS("Mode")];
    [_gradientModePopup removeAllItems];
    [_gradientModePopup addItemWithTitle: _NS("Gradient")];
    [[_gradientModePopup lastItem] setRepresentedObject: @"gradient"];
    [[_gradientModePopup lastItem] setTag: 1];
    [_gradientModePopup addItemWithTitle: _NS("Edge")];
    [[_gradientModePopup lastItem] setRepresentedObject: @"edge"];
    [[_gradientModePopup lastItem] setTag: 2];
    [_gradientModePopup addItemWithTitle: _NS("Hough")];
    [[_gradientModePopup lastItem] setRepresentedObject: @"hough"];
    [[_gradientModePopup lastItem] setTag: 3];
    [_gradientColorCheckbox setTitle:_NS("Color")];
    [_gradientCartoonCheckbox setTitle:_NS("Cartoon")];
    [_extractCheckbox setTitle:_NS("Color extraction")];
    [_extractLabel setStringValue:_NS("Color")];
    [_extractTextField setFormatter:[[VLCHexNumberFormatter alloc] init]];
    [_invertCheckbox setTitle:_NS("Invert colors")];
    [_posterizeCheckbox setTitle:_NS("Posterize")];
    [_posterizeLabel setStringValue:_NS("Posterize level")];
    [_blurCheckbox setTitle:_NS("Motion blur")];
    [_blurLabel setStringValue:_NS("Factor")];
    [_motiondetectCheckbox setTitle:_NS("Motion Detect")];
    [_watereffectCheckbox setTitle:_NS("Water effect")];
    [_wavesCheckbox setTitle:_NS("Waves")];
    [_psychedelicCheckbox setTitle:_NS("Psychedelic")];
    [_anaglyphCheckbox setTitle:_NS("Anaglyph")];
    [_addTextCheckbox setTitle:_NS("Add text")];
    [_addTextTextLabel setStringValue:_NS("Text")];
    [_addTextPositionLabel setStringValue:_NS("Position")];
    [_addTextPositionPopup removeAllItems];
    [_addTextPositionPopup addItemWithTitle: _NS("Center")];
    [[_addTextPositionPopup lastItem] setTag: 0];
    [_addTextPositionPopup addItemWithTitle: _NS("Left")];
    [[_addTextPositionPopup lastItem] setTag: 1];
    [_addTextPositionPopup addItemWithTitle: _NS("Right")];
    [[_addTextPositionPopup lastItem] setTag: 2];
    [_addTextPositionPopup addItemWithTitle: _NS("Top")];
    [[_addTextPositionPopup lastItem] setTag: 4];
    [_addTextPositionPopup addItemWithTitle: _NS("Bottom")];
    [[_addTextPositionPopup lastItem] setTag: 8];
    [_addTextPositionPopup addItemWithTitle: _NS("Top-Left")];
    [[_addTextPositionPopup lastItem] setTag: 5];
    [_addTextPositionPopup addItemWithTitle: _NS("Top-Right")];
    [[_addTextPositionPopup lastItem] setTag: 6];
    [_addTextPositionPopup addItemWithTitle: _NS("Bottom-Left")];
    [[_addTextPositionPopup lastItem] setTag: 9];
    [_addTextPositionPopup addItemWithTitle: _NS("Bottom-Right")];
    [[_addTextPositionPopup lastItem] setTag: 10];
    [_addLogoCheckbox setTitle:_NS("Add logo")];
    [_addLogoLogoLabel setStringValue:_NS("Logo")];
    [_addLogoPositionLabel setStringValue:_NS("Position")];
    [_addLogoPositionPopup removeAllItems];
    [_addLogoPositionPopup addItemWithTitle: _NS("Center")];
    [[_addLogoPositionPopup lastItem] setTag: 0];
    [_addLogoPositionPopup addItemWithTitle: _NS("Left")];
    [[_addLogoPositionPopup lastItem] setTag: 1];
    [_addLogoPositionPopup addItemWithTitle: _NS("Right")];
    [[_addLogoPositionPopup lastItem] setTag: 2];
    [_addLogoPositionPopup addItemWithTitle: _NS("Top")];
    [[_addLogoPositionPopup lastItem] setTag: 4];
    [_addLogoPositionPopup addItemWithTitle: _NS("Bottom")];
    [[_addLogoPositionPopup lastItem] setTag: 8];
    [_addLogoPositionPopup addItemWithTitle: _NS("Top-Left")];
    [[_addLogoPositionPopup lastItem] setTag: 5];
    [_addLogoPositionPopup addItemWithTitle: _NS("Top-Right")];
    [[_addLogoPositionPopup lastItem] setTag: 6];
    [_addLogoPositionPopup addItemWithTitle: _NS("Bottom-Left")];
    [[_addLogoPositionPopup lastItem] setTag: 9];
    [_addLogoPositionPopup addItemWithTitle: _NS("Bottom-Right")];
    [[_addLogoPositionPopup lastItem] setTag: 10];
    [_addLogoTransparencyLabel setStringValue:_NS("Transparency")];

    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(inputChangedEvent:)
                                                 name:VLCInputChangedNotification
                                               object:nil];

    [self resetValues];
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)updateCocoaWindowLevel:(NSInteger)i_level
{
    if (self.isWindowLoaded && [self.window isVisible] && [self.window level] != i_level)
        [self.window setLevel: i_level];
}

#pragma mark -
#pragma mark internal functions

- (void)saveCurrentProfileIndex:(NSInteger)index
{
    [[NSUserDefaults standardUserDefaults] setInteger:index forKey:@"VideoEffectSelectedProfile"];
}

- (NSInteger)currentProfileIndex
{
    return [[NSUserDefaults standardUserDefaults] integerForKey:@"VideoEffectSelectedProfile"];
}

/// Returns the list of profile names (omitting the Default entry)
- (NSArray *)nonDefaultProfileNames
{
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];

    NSMutableArray *names = [[defaults stringArrayForKey:@"VideoEffectProfileNames"] mutableCopy];
    [names removeObjectAtIndex:0];
    return [names copy];
}

-(void)inputChangedEvent:(NSNotification *)o_notification
{
    // reset crop values when input changed
    [self setCropBottomValue:0];
    [self setCropTopValue:0];
    [self setCropLeftValue:0];
    [self setCropRightValue:0];
}

- (void)resetProfileSelector
{
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    [_profilePopup removeAllItems];

    // Ignore "Default" index 0 from settings
    [_profilePopup addItemWithTitle:_NS("Default")];

    [_profilePopup addItemsWithTitles:[self nonDefaultProfileNames]];

    [[_profilePopup menu] addItem:[NSMenuItem separatorItem]];
    [_profilePopup addItemWithTitle:_NS("Duplicate current profile...")];
    [[_profilePopup lastItem] setTarget: self];
    [[_profilePopup lastItem] setAction: @selector(addProfile:)];

    if ([[self nonDefaultProfileNames] count] > 0) {
        [_profilePopup addItemWithTitle:_NS("Organize profiles...")];
        [[_profilePopup lastItem] setTarget: self];
        [[_profilePopup lastItem] setAction: @selector(removeProfile:)];
    }

    [_profilePopup selectItemAtIndex: [self currentProfileIndex]];
    // Loading only non-default profiles ensures that vlcrc or command line settings are not overwritten
    if ([self currentProfileIndex] > 0)
        [self profileSelectorAction:self];
}

- (void)setWidgetValue: (id)widget forOption: (char *)psz_option enabled: (bool)b_state
{
    intf_thread_t *p_intf = getIntf();
    playlist_t *p_playlist = pl_Get(p_intf);

    vlc_value_t val;
    int i_type = config_GetType(psz_option) & VLC_VAR_CLASS;
    switch (i_type)
    {
    case VLC_VAR_BOOL:
    case VLC_VAR_INTEGER:
    case VLC_VAR_FLOAT:
    case VLC_VAR_STRING:
        break;
    default:
        msg_Err(p_intf, "%s variable is of an unsupported type (%d)", psz_option, i_type);
        return;
    }

    if (var_Create(p_playlist, psz_option, i_type | VLC_VAR_DOINHERIT) ||
        var_GetChecked(p_playlist, psz_option, i_type, &val)) {
        return;
    }

    if (i_type == VLC_VAR_BOOL || i_type == VLC_VAR_INTEGER)
    {
        if ([widget isKindOfClass: [NSSlider class]])
        {
            [widget setIntValue: val.i_int];
            [widget setToolTip: [NSString stringWithFormat:@"%lli", val.i_int]];
        }
        else if ([widget isKindOfClass: [NSButton class]])
            [widget setState: val.i_int ? NSOnState : NSOffState];
        else if ([widget isKindOfClass: [NSTextField class]])
            [widget setIntValue: val.i_int];
        else if ([widget isKindOfClass: [NSStepper class]])
            [widget setIntValue: val.i_int];
        else if ([widget isKindOfClass: [NSPopUpButton class]])
            [widget selectItemWithTag: val.i_int];
    }
    else if (i_type == VLC_VAR_FLOAT)
    {
        if ([widget isKindOfClass: [NSSlider class]])
        {
            [widget setFloatValue: val.f_float];
            [widget setToolTip: [NSString stringWithFormat:@"%0.3f", val.f_float]];
        }
    }
    else if (i_type == VLC_VAR_STRING)
    {
        if ([widget isKindOfClass: [NSPopUpButton class]])
        {
            for (NSMenuItem *item in [widget itemArray])
                if ([item representedObject] &&
                    !strcmp([[item representedObject] UTF8String], val.psz_string))
                {
                    [widget selectItemWithTitle: [item title]];
                    break;
                }
        }
        else if ([widget isKindOfClass: [NSTextField class]])
            [widget setStringValue: toNSStr(val.psz_string)];
        free(val.psz_string);
    }

    [widget setEnabled: b_state];
}

/// Sets widget values based on variables
- (void)resetValues
{
    intf_thread_t *p_intf = getIntf();
    playlist_t *p_playlist = pl_Get(p_intf);
    NSString *tmpString;
    char *tmpChar;
    BOOL b_state;

    /* do we have any filter enabled? if yes, show it. */
    char * psz_vfilters;
    psz_vfilters = var_InheritString(p_playlist, "video-filter");
    if (psz_vfilters) {
        [_adjustCheckbox setState: (NSInteger)strstr(psz_vfilters, "adjust")];
        [_sharpenCheckbox setState: (NSInteger)strstr(psz_vfilters, "sharpen")];
        [_bandingCheckbox setState: (NSInteger)strstr(psz_vfilters, "gradfun")];
        [_grainCheckbox setState: (NSInteger)strstr(psz_vfilters, "grain")];
        [_transformCheckbox setState: (NSInteger)strstr(psz_vfilters, "transform")];
        [_zoomCheckbox setState: (NSInteger)strstr(psz_vfilters, "magnify")];
        [_puzzleCheckbox setState: (NSInteger)strstr(psz_vfilters, "puzzle")];
        [_thresholdCheckbox setState: (NSInteger)strstr(psz_vfilters, "colorthres")];
        [_sepiaCheckbox setState: (NSInteger)strstr(psz_vfilters, "sepia")];
        [_gradientCheckbox setState: (NSInteger)strstr(psz_vfilters, "gradient")];
        [_extractCheckbox setState: (NSInteger)strstr(psz_vfilters, "extract")];
        [_invertCheckbox setState: (NSInteger)strstr(psz_vfilters, "invert")];
        [_posterizeCheckbox setState: (NSInteger)strstr(psz_vfilters, "posterize")];
        [_blurCheckbox setState: (NSInteger)strstr(psz_vfilters, "motionblur")];
        [_motiondetectCheckbox setState: (NSInteger)strstr(psz_vfilters, "motiondetect")];
        [_watereffectCheckbox setState: (NSInteger)strstr(psz_vfilters, "ripple")];
        [_wavesCheckbox setState: (NSInteger)strstr(psz_vfilters, "wave")];
        [_psychedelicCheckbox setState: (NSInteger)strstr(psz_vfilters, "psychedelic")];
        [_anaglyphCheckbox setState: (NSInteger)strstr(psz_vfilters, "anaglyph")];
        free(psz_vfilters);
    } else {
        [_adjustCheckbox setState: NSOffState];
        [_sharpenCheckbox setState: NSOffState];
        [_bandingCheckbox setState: NSOffState];
        [_grainCheckbox setState: NSOffState];
        [_transformCheckbox setState: NSOffState];
        [_zoomCheckbox setState: NSOffState];
        [_puzzleCheckbox setState: NSOffState];
        [_thresholdCheckbox setState: NSOffState];
        [_sepiaCheckbox setState: NSOffState];
        [_gradientCheckbox setState: NSOffState];
        [_extractCheckbox setState: NSOffState];
        [_invertCheckbox setState: NSOffState];
        [_posterizeCheckbox setState: NSOffState];
        [_blurCheckbox setState: NSOffState];
        [_motiondetectCheckbox setState: NSOffState];
        [_watereffectCheckbox setState: NSOffState];
        [_wavesCheckbox setState: NSOffState];
        [_psychedelicCheckbox setState: NSOffState];
        [_anaglyphCheckbox setState: NSOffState];
    }

    psz_vfilters = var_InheritString(p_playlist, "sub-source");
    if (psz_vfilters) {
        [_addTextCheckbox setState: (NSInteger)strstr(psz_vfilters, "marq")];
        [_addLogoCheckbox setState: (NSInteger)strstr(psz_vfilters, "logo")];
        free(psz_vfilters);
    } else {
        [_addTextCheckbox setState: NSOffState];
        [_addLogoCheckbox setState: NSOffState];
    }

    psz_vfilters = var_InheritString(p_playlist, "video-splitter");
    if (psz_vfilters) {
        [_cloneCheckbox setState: (NSInteger)strstr(psz_vfilters, "clone")];
        [_wallCheckbox setState: (NSInteger)strstr(psz_vfilters, "wall")];
        free(psz_vfilters);
    } else {
        [_cloneCheckbox setState: NSOffState];
        [_wallCheckbox setState: NSOffState];
    }

    /* fetch and show the various values */
    b_state = [_adjustCheckbox state];
    [self setWidgetValue: _adjustHueSlider forOption: "hue" enabled: b_state];
    [self setWidgetValue: _adjustContrastSlider forOption: "contrast" enabled: b_state];
    [self setWidgetValue: _adjustBrightnessSlider forOption: "brightness" enabled: b_state];
    [self setWidgetValue: _adjustSaturationSlider forOption: "saturation" enabled: b_state];
    [self setWidgetValue: _adjustBrightnessCheckbox forOption: "brightness-threshold" enabled: b_state];
    [self setWidgetValue: _adjustGammaSlider forOption: "gamma" enabled: b_state];
    [_adjustBrightnessLabel setEnabled: b_state];
    [_adjustContrastLabel setEnabled: b_state];
    [_adjustGammaLabel setEnabled: b_state];
    [_adjustHueLabel setEnabled: b_state];
    [_adjustSaturationLabel setEnabled: b_state];
    [_adjustResetButton setEnabled: b_state];

    [self setWidgetValue: _sharpenSlider forOption: "sharpen-sigma" enabled: [_sharpenCheckbox state]];
    [_sharpenLabel setEnabled: [_sharpenCheckbox state]];

    [self setWidgetValue: _bandingSlider forOption: "gradfun-radius" enabled: [_bandingCheckbox state]];
    [_bandingLabel setEnabled: [_bandingCheckbox state]];

    [self setWidgetValue: _grainSlider forOption: "grain-variance" enabled: [_grainCheckbox state]];
    [_grainLabel setEnabled: [_grainCheckbox state]];

    [self setCropLeftValue: 0];
    [self setCropTopValue: 0];
    [self setCropRightValue: 0];
    [self setCropBottomValue: 0];
    [_cropSyncTopBottomCheckbox setState: NSOffState];
    [_cropSyncLeftRightCheckbox setState: NSOffState];

    [self setWidgetValue: _transformPopup forOption: "transform-type" enabled: [_transformCheckbox state]];

    b_state = [_puzzleCheckbox state];
    [self setWidgetValue: _puzzleColumnsTextField forOption: "puzzle-cols" enabled: b_state];
    [self setWidgetValue: _puzzleColumnsStepper forOption: "puzzle-cols" enabled: b_state];
    [self setWidgetValue: _puzzleRowsTextField forOption: "puzzle-rows" enabled: b_state];
    [self setWidgetValue: _puzzleRowsStepper forOption: "puzzle-rows" enabled: b_state];
    [_puzzleRowsLabel setEnabled: b_state];
    [_puzzleColumnsLabel setEnabled: b_state];

    b_state = [_cloneCheckbox state];
    [self setWidgetValue: _cloneNumberTextField forOption: "clone-count" enabled: b_state];
    [self setWidgetValue: _cloneNumberStepper forOption: "clone-count" enabled: b_state];
    [_cloneNumberLabel setEnabled: b_state];

    b_state = [_wallCheckbox state];
    [self setWidgetValue: _wallNumbersOfRowsTextField forOption: "wall-rows" enabled: b_state];
    [self setWidgetValue: _wallNumbersOfRowsStepper forOption: "wall-rows" enabled: b_state];
    [self setWidgetValue: _wallNumberOfColumnsTextField forOption: "wall-cols" enabled: b_state];
    [self setWidgetValue: _wallNumberOfColumnsStepper forOption: "wall-cols" enabled: b_state];
    [_wallNumbersOfRowsLabel setEnabled: b_state];
    [_wallNumberOfColumnsLabel setEnabled: b_state];

    b_state = [_thresholdCheckbox state];
    [self setWidgetValue: _thresholdColorTextField forOption: "colorthres-color" enabled: b_state];
    [self setWidgetValue: _thresholdSaturationSlider forOption: "colorthres-saturationthres" enabled: b_state];
    [self setWidgetValue: _thresholdSimilaritySlider forOption: "colorthres-similaritythres" enabled: b_state];
    [_thresholdColorLabel setEnabled: b_state];
    [_thresholdSaturationLabel setEnabled: b_state];
    [_thresholdSimilarityLabel setEnabled: b_state];

    b_state = [_sepiaCheckbox state];
    [self setWidgetValue: _sepiaTextField forOption: "sepia-intensity" enabled: b_state];
    [self setWidgetValue: _sepiaStepper forOption: "sepia-intensity" enabled: b_state];
    [_sepiaLabel setEnabled: b_state];

    b_state = [_gradientCheckbox state];
    [self setWidgetValue: _gradientModePopup forOption: "gradient-mode" enabled: b_state];
    [self setWidgetValue: _gradientCartoonCheckbox forOption: "gradient-cartoon" enabled: b_state];
    [self setWidgetValue: _gradientColorCheckbox forOption: "gradient-type" enabled: b_state];
    [_gradientModeLabel setEnabled: b_state];

    [self setWidgetValue: _extractTextField forOption: "extract-component" enabled: [_extractCheckbox state]];
    [_extractLabel setEnabled: [_extractCheckbox state]];

    b_state = [_posterizeCheckbox state];
    [self setWidgetValue: _posterizeTextField forOption: "posterize-level" enabled: b_state];
    [self setWidgetValue: _posterizeStepper forOption: "posterize-level" enabled: b_state];
    [_posterizeLabel setEnabled: b_state];

    [self setWidgetValue: _blurSlider forOption: "blur-factor" enabled: [_blurCheckbox state]];
    [_blurLabel setEnabled: [_blurCheckbox state]];

    b_state = [_addTextCheckbox state];
    [self setWidgetValue: _addTextTextTextField forOption: "marq-marquee" enabled: b_state];
    [self setWidgetValue: _addTextPositionPopup forOption: "marq-position" enabled: b_state];
    [_addTextPositionLabel setEnabled: b_state];
    [_addTextTextLabel setEnabled: b_state];

    b_state = [_addLogoCheckbox state];
    [self setWidgetValue: _addLogoLogoTextField forOption: "logo-file" enabled: b_state];
    [self setWidgetValue: _addLogoPositionPopup forOption: "logo-position" enabled: b_state];
    [self setWidgetValue: _addLogoTransparencySlider forOption: "logo-opacity" enabled: b_state];
    [_addLogoPositionLabel setEnabled: b_state];
    [_addLogoLogoLabel setEnabled: b_state];
    [_addLogoTransparencyLabel setEnabled: b_state];
}

- (NSString *)generateProfileString
{
    intf_thread_t *p_intf = getIntf();
    playlist_t *p_playlist = pl_Get(p_intf);
    return [NSString stringWithFormat:@"%@;%@;%@;%lli;%f;%f;%f;%f;%f;%lli;%f;%@;%lli;%lli;%lli;%lli;%lli;%lli;%@;%lli;%lli;%lli;%lli;%lli;%@;%lli;%@;%lli;%lli;%lli;%lli;%lli;%lli;%f",
                     B64EncAndFree(var_InheritString(p_playlist, "video-filter")),
                     B64EncAndFree(var_InheritString(p_playlist, "sub-source")),
                     B64EncAndFree(var_InheritString(p_playlist, "video-splitter")),
                     0LL, // former "hue" value, deprecated since 3.0.0
                     var_InheritFloat(p_playlist, "contrast"),
                     var_InheritFloat(p_playlist, "brightness"),
                     var_InheritFloat(p_playlist, "saturation"),
                     var_InheritFloat(p_playlist, "gamma"),
                     var_InheritFloat(p_playlist, "sharpen-sigma"),
                     var_InheritInteger(p_playlist, "gradfun-radius"),
                     var_InheritFloat(p_playlist, "grain-variance"),
                     B64EncAndFree(var_InheritString(p_playlist, "transform-type")),
                     var_InheritInteger(p_playlist, "puzzle-rows"),
                     var_InheritInteger(p_playlist, "puzzle-cols"),
                     var_InheritInteger(p_playlist, "colorthres-color"),
                     var_InheritInteger(p_playlist, "colorthres-saturationthres"),
                     var_InheritInteger(p_playlist, "colorthres-similaritythres"),
                     var_InheritInteger(p_playlist, "sepia-intensity"),
                     B64EncAndFree(var_InheritString(p_playlist, "gradient-mode")),
                     (int64_t)var_InheritBool(p_playlist, "gradient-cartoon"),
                     var_InheritInteger(p_playlist, "gradient-type"),
                     var_InheritInteger(p_playlist, "extract-component"),
                     var_InheritInteger(p_playlist, "posterize-level"),
                     var_InheritInteger(p_playlist, "blur-factor"),
                     B64EncAndFree(var_InheritString(p_playlist, "marq-marquee")),
                     var_InheritInteger(p_playlist, "marq-position"),
                     B64EncAndFree(var_InheritString(p_playlist, "logo-file")),
                     var_InheritInteger(p_playlist, "logo-position"),
                     var_InheritInteger(p_playlist, "logo-opacity"),
                     var_InheritInteger(p_playlist, "clone-count"),
                     var_InheritInteger(p_playlist, "wall-rows"),
                     var_InheritInteger(p_playlist, "wall-cols"),
                     // version 2 of profile string:
                     (int64_t)var_InheritBool(p_playlist, "brightness-threshold"), // index: 32
                     // version 3 of profile string: (vlc-3.0.0)
                     var_InheritFloat(p_playlist, "hue") // index: 33
            ];
}

#pragma mark -
#pragma mark generic UI code

- (void)saveCurrentProfile
{
    NSInteger currentProfileIndex = [self currentProfileIndex];

    // Do not save default profile
    if (currentProfileIndex == 0) {
        return;
    }

    /* fetch all the current settings in a uniform string */
    NSString *newProfile = [self generateProfileString];

    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    NSMutableArray *workArray = [[NSMutableArray alloc] initWithArray:[defaults objectForKey:@"VideoEffectProfiles"]];
    if (currentProfileIndex >= [workArray count])
        return;

    [workArray replaceObjectAtIndex:currentProfileIndex withObject:newProfile];
    [defaults setObject:[NSArray arrayWithArray:workArray] forKey:@"VideoEffectProfiles"];
    [defaults synchronize];
}

- (void)saveCurrentProfileAtTerminate
{
    if ([self currentProfileIndex] > 0) {
        [self saveCurrentProfile];
        return;
    }

    // A new "Custom profile" is only created if the user wants to load this as new profile at startup ...
    if (_applyProfileCheckbox.state == NSOffState)
        return;

    // ... and some settings are changed
    NSString *newProfile = [self generateProfileString];
    if ([newProfile compare:[VLCVideoEffectsWindowController defaultProfileString]] == NSOrderedSame)
        return;

    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    NSMutableArray *workArray = [[NSMutableArray alloc] initWithArray:[defaults objectForKey:@"VideoEffectProfiles"]];
    [workArray addObject:newProfile];
    [defaults setObject:[NSArray arrayWithArray:workArray] forKey:@"VideoEffectProfiles"];

    NSArray<NSString *> *profileNames = [defaults objectForKey:@"VideoEffectProfileNames"];
    NSString *newProfileName;

    unsigned int num_custom = 0;
    do
        newProfileName = [@"Custom" stringByAppendingString:[NSString stringWithFormat:@"%03i",num_custom++]];
    while ([profileNames containsObject:newProfileName]);

    workArray = [[NSMutableArray alloc] initWithArray:[defaults objectForKey:@"VideoEffectProfileNames"]];
    [workArray addObject:newProfileName];
    [defaults setObject:[NSArray arrayWithArray:workArray] forKey:@"VideoEffectProfileNames"];

    [self saveCurrentProfileIndex:([workArray count] - 1)];

    [defaults synchronize];
}

- (IBAction)toggleWindow:(id)sender
{
    if ([self.window isKeyWindow])
        [self.window orderOut:sender];
    else {
        [self.window setLevel: [[[VLCMain sharedInstance] voutController] currentStatusWindowLevel]];
        [self.window makeKeyAndOrderFront:sender];
    }
}

- (IBAction)profileSelectorAction:(id)sender
{
    [self saveCurrentProfile];

    [self saveCurrentProfileIndex:[_profilePopup indexOfSelectedItem]];

    [self loadProfile];
    [self resetValues];
}

- (void)addProfile:(id)sender
{
    /* show panel */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpartial-availability"
#ifdef MAC_OS_X_VERSION_10_10
    if (OSX_YOSEMITE_AND_HIGHER) {
        [[_textfieldPanel window] setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameVibrantDark]];
    }
#endif
#pragma clang diagnostic pop
    [_textfieldPanel setTitleString:_NS("Duplicate current profile for a new profile")];
    [_textfieldPanel setSubTitleString:_NS("Enter a name for the new profile:")];
    [_textfieldPanel setCancelButtonString:_NS("Cancel")];
    [_textfieldPanel setOkButtonString:_NS("Save")];

    // TODO: Change to weak, when dropping 10.7 support
    __unsafe_unretained typeof(self) _self = self;
    [_textfieldPanel runModalForWindow:self.window completionHandler:^(NSInteger returnCode, NSString *resultingText) {

        NSInteger currentProfileIndex = [_self currentProfileIndex];
        if (returnCode != NSOKButton) {
            [_profilePopup selectItemAtIndex:currentProfileIndex];
            return;
        }

        NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
        NSArray *profileNames = [defaults objectForKey:@"VideoEffectProfileNames"];

        // duplicate names are not allowed in the popup control
        if ([resultingText length] == 0 || [profileNames containsObject:resultingText]) {
            [_profilePopup selectItemAtIndex:currentProfileIndex];

            NSAlert *alert = [[NSAlert alloc] init];
            [alert setAlertStyle:NSCriticalAlertStyle];
            [alert setMessageText:_NS("Please enter a unique name for the new profile.")];
            [alert setInformativeText:_NS("Multiple profiles with the same name are not allowed.")];

            [alert beginSheetModalForWindow:_self.window
                              modalDelegate:nil
                             didEndSelector:nil
                                contextInfo:nil];
            return;
        }

        /* fetch all the current settings in a uniform string */
        NSString *newProfile = [_self generateProfileString];

        /* add string to user defaults as well as a label */

        NSMutableArray *workArray = [[NSMutableArray alloc] initWithArray:[defaults objectForKey:@"VideoEffectProfiles"]];
        [workArray addObject:newProfile];
        [defaults setObject:[NSArray arrayWithArray:workArray] forKey:@"VideoEffectProfiles"];

        [self saveCurrentProfileIndex:([workArray count] - 1)];

        workArray = [[NSMutableArray alloc] initWithArray:[defaults objectForKey:@"VideoEffectProfileNames"]];
        [workArray addObject:resultingText];
        [defaults setObject:[NSArray arrayWithArray:workArray] forKey:@"VideoEffectProfileNames"];

        /* save defaults */
        [defaults synchronize];

        /* refresh UI */
        [_self resetProfileSelector];
    }];
}

- (void)removeProfile:(id)sender
{
    /* show panel */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpartial-availability"
#ifdef MAC_OS_X_VERSION_10_10
    if (OSX_YOSEMITE_AND_HIGHER) {
        [[_popupPanel window] setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameVibrantDark]];
    }
#endif
#pragma clang diagnostic pop
    [_popupPanel setTitleString:_NS("Remove a preset")];
    [_popupPanel setSubTitleString:_NS("Select the preset you would like to remove:")];
    [_popupPanel setOkButtonString:_NS("Remove")];
    [_popupPanel setCancelButtonString:_NS("Cancel")];
    [_popupPanel setPopupButtonContent:[self nonDefaultProfileNames]];

    // TODO: Change to weak, when dropping 10.7 support
    __unsafe_unretained typeof(self) _self = self;
    [_popupPanel runModalForWindow:self.window completionHandler:^(NSInteger returnCode, NSInteger selectedIndex) {


        NSInteger activeProfileIndex = [_self currentProfileIndex];

        if (returnCode != NSOKButton) {
            [_profilePopup selectItemAtIndex:activeProfileIndex];
            return;
        }

        // Popup panel does not contain the "Default" entry
        selectedIndex++;

        NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
        /* remove selected profile from settings */
        NSMutableArray *workArray = [[NSMutableArray alloc] initWithArray: [defaults objectForKey:@"VideoEffectProfiles"]];
        [workArray removeObjectAtIndex:selectedIndex];
        [defaults setObject:[NSArray arrayWithArray:workArray] forKey:@"VideoEffectProfiles"];

        workArray = [[NSMutableArray alloc] initWithArray: [defaults objectForKey:@"VideoEffectProfileNames"]];
        [workArray removeObjectAtIndex:selectedIndex];
        [defaults setObject:[NSArray arrayWithArray:workArray] forKey:@"VideoEffectProfileNames"];

        if (activeProfileIndex >= selectedIndex)
            [self saveCurrentProfileIndex:(activeProfileIndex - 1)];

        /* save defaults */
        [defaults synchronize];

        /* refresh UI */
        [_self resetProfileSelector];
    }];
}

- (IBAction)applyProfileCheckboxChanged:(id)sender
{
    [[NSUserDefaults standardUserDefaults] setBool:[sender state] forKey:@"VideoEffectApplyProfileOnStartup"];
}

#pragma mark -
#pragma mark basic
- (IBAction)enableAdjust:(id)sender
{
    if (sender == _adjustResetButton) {
        [_adjustBrightnessSlider setFloatValue: 1.0];
        [_adjustContrastSlider setFloatValue: 1.0];
        [_adjustGammaSlider setFloatValue: 1.0];
        [_adjustHueSlider setFloatValue: 0];
        [_adjustSaturationSlider setFloatValue: 1.0];
        [_adjustBrightnessSlider setToolTip: [NSString stringWithFormat:@"%0.3f", 1.0]];
        [_adjustContrastSlider setToolTip: [NSString stringWithFormat:@"%0.3f", 1.0]];
        [_adjustGammaSlider setToolTip: [NSString stringWithFormat:@"%0.3f", 1.0]];
        [_adjustHueSlider setToolTip: [NSString stringWithFormat:@"%.0f", 0.0]];
        [_adjustSaturationSlider setToolTip: [NSString stringWithFormat:@"%0.3f", 1.0]];

        VLCCoreInteraction *vci_si = [VLCCoreInteraction sharedInstance];
        [vci_si setVideoFilterProperty: "brightness" forFilter: "adjust" withValue: (vlc_value_t){ .f_float = 1.f }];
        [vci_si setVideoFilterProperty: "contrast" forFilter: "adjust" withValue: (vlc_value_t){ .f_float = 1.f }];
        [vci_si setVideoFilterProperty: "gamma" forFilter: "adjust" withValue: (vlc_value_t){ .f_float = 1.f }];
        [vci_si setVideoFilterProperty: "hue" forFilter: "adjust" withValue: (vlc_value_t){ .f_float = .0f }];
        [vci_si setVideoFilterProperty: "saturation" forFilter: "adjust" withValue: (vlc_value_t){ .f_float = 1.f }];
    } else {
        BOOL b_state = [_adjustCheckbox state];

        [[VLCCoreInteraction sharedInstance] setVideoFilter: "adjust" on: b_state];
        [_adjustBrightnessSlider setEnabled: b_state];
        [_adjustBrightnessCheckbox setEnabled: b_state];
        [_adjustBrightnessLabel setEnabled: b_state];
        [_adjustContrastSlider setEnabled: b_state];
        [_adjustContrastLabel setEnabled: b_state];
        [_adjustGammaSlider setEnabled: b_state];
        [_adjustGammaLabel setEnabled: b_state];
        [_adjustHueSlider setEnabled: b_state];
        [_adjustHueLabel setEnabled: b_state];
        [_adjustSaturationSlider setEnabled: b_state];
        [_adjustSaturationLabel setEnabled: b_state];
        [_adjustResetButton setEnabled: b_state];
    }
}

- (IBAction)adjustSliderChanged:(id)sender
{
    char const *psz_property = nil;
    if (sender == _adjustBrightnessSlider)
        psz_property = "brightness";
    else if (sender == _adjustContrastSlider)
        psz_property = "contrast";
    else if (sender == _adjustGammaSlider)
        psz_property = "gamma";
    else if (sender == _adjustHueSlider)
        psz_property = "hue";
    else if (sender == _adjustSaturationSlider)
        psz_property = "saturation";
    assert(psz_property);

    [[VLCCoreInteraction sharedInstance] setVideoFilterProperty: psz_property forFilter: "adjust" withValue: getWidgetFloatValue(sender)];

    if (sender == _adjustHueSlider)
        [_adjustHueSlider setToolTip: [NSString stringWithFormat:@"%.0f", [_adjustHueSlider floatValue]]];
    else
        [sender setToolTip: [NSString stringWithFormat:@"%0.3f", [sender floatValue]]];
}

- (IBAction)enableAdjustBrightnessThreshold:(id)sender
{
    [[VLCCoreInteraction sharedInstance] setVideoFilterProperty: "brightness-threshold"
                                                      forFilter: "adjust"
                                                      withValue: getWidgetBoolValue(sender)];
}

- (IBAction)enableSharpen:(id)sender
{
    BOOL b_state = [_sharpenCheckbox state];

    [[VLCCoreInteraction sharedInstance] setVideoFilter: "sharpen" on: b_state];
    [_sharpenSlider setEnabled: b_state];
    [_sharpenLabel setEnabled: b_state];
}

- (IBAction)sharpenSliderChanged:(id)sender
{
    [[VLCCoreInteraction sharedInstance] setVideoFilterProperty: "sharpen-sigma" forFilter: "sharpen" withValue: getWidgetFloatValue(sender)];
    [sender setToolTip: [NSString stringWithFormat:@"%0.3f", [sender floatValue]]];
}

- (IBAction)enableBanding:(id)sender
{
    BOOL b_state = [_bandingCheckbox state];

    [[VLCCoreInteraction sharedInstance] setVideoFilter: "gradfun" on: b_state];
    [_bandingSlider setEnabled: b_state];
    [_bandingLabel setEnabled: b_state];
}

- (IBAction)bandingSliderChanged:(id)sender
{
    [[VLCCoreInteraction sharedInstance] setVideoFilterProperty: "gradfun-radius" forFilter: "gradfun" withValue: getWidgetIntValue(sender)];
    [sender setToolTip: [NSString stringWithFormat:@"%i", [sender intValue]]];
}

- (IBAction)enableGrain:(id)sender
{
    BOOL b_state = [_grainCheckbox state];

    [[VLCCoreInteraction sharedInstance] setVideoFilter: "grain" on: b_state];
    [_grainSlider setEnabled: b_state];
    [_grainLabel setEnabled: b_state];
}

- (IBAction)grainSliderChanged:(id)sender
{
    [[VLCCoreInteraction sharedInstance] setVideoFilterProperty: "grain-variance" forFilter: "grain" withValue: getWidgetFloatValue(sender)];
    [sender setToolTip: [NSString stringWithFormat:@"%0.3f", [sender floatValue]]];
}


#pragma mark -
#pragma mark crop

- (IBAction)cropObjectChanged:(id)sender
{
    if ([_cropSyncTopBottomCheckbox state]) {
        if (sender == _cropBottomTextField || sender == _cropBottomStepper)
            [self setCropTopValue: [self cropBottomValue]];
        else
            [self setCropBottomValue: [self cropTopValue]];
    }
    if ([_cropSyncLeftRightCheckbox state]) {
        if (sender == _cropRightTextField || sender == _cropRightStepper)
            [self setCropLeftValue: [self cropRightValue]];
        else
            [self setCropRightValue: [self cropLeftValue]];
    }

    NSArray<NSValue *> *vouts = getVouts();
    if (vouts)
        for (NSValue *ptr in vouts) {
            vout_thread_t *p_vout = [ptr pointerValue];
            var_SetInteger(p_vout, "crop-top", [_cropTopTextField intValue]);
            var_SetInteger(p_vout, "crop-bottom", [_cropBottomTextField intValue]);
            var_SetInteger(p_vout, "crop-left", [_cropLeftTextField intValue]);
            var_SetInteger(p_vout, "crop-right", [_cropRightTextField intValue]);
            vlc_object_release(p_vout);
        }
}

#pragma mark -
#pragma mark geometry
- (IBAction)enableTransform:(id)sender
{
    [[VLCCoreInteraction sharedInstance] setVideoFilter: "transform" on: [_transformCheckbox state]];
    [_transformPopup setEnabled: [_transformCheckbox state]];
}

- (IBAction)transformModifierChanged:(id)sender
{
    vlc_value_t value = { .psz_string = (char *)[[[_transformPopup selectedItem] representedObject] UTF8String] };
    [[VLCCoreInteraction sharedInstance] setVideoFilterProperty: "transform-type" forFilter: "transform" withValue: value];
}

- (IBAction)enableZoom:(id)sender
{
    [[VLCCoreInteraction sharedInstance] setVideoFilter: "magnify" on: [_zoomCheckbox state]];
}

- (IBAction)enablePuzzle:(id)sender
{
    BOOL b_state = [_puzzleCheckbox state];

    [[VLCCoreInteraction sharedInstance] setVideoFilter: "puzzle" on: b_state];
    [_puzzleColumnsTextField setEnabled: b_state];
    [_puzzleColumnsStepper setEnabled: b_state];
    [_puzzleColumnsLabel setEnabled: b_state];
    [_puzzleRowsTextField setEnabled: b_state];
    [_puzzleRowsStepper setEnabled: b_state];
    [_puzzleRowsLabel setEnabled: b_state];
}

- (IBAction)puzzleModifierChanged:(id)sender
{
    if (sender == _puzzleColumnsTextField || sender == _puzzleColumnsStepper)
        [[VLCCoreInteraction sharedInstance] setVideoFilterProperty: "puzzle-cols" forFilter: "puzzle" withValue: getWidgetIntValue(sender)];
    else
        [[VLCCoreInteraction sharedInstance] setVideoFilterProperty: "puzzle-rows" forFilter: "puzzle" withValue: getWidgetIntValue(sender)];
}

- (IBAction)enableClone:(id)sender
{
    BOOL b_state = [_cloneCheckbox state];

    if (b_state && [_wallCheckbox state]) {
        [_wallCheckbox setState: NSOffState];
        [self enableWall:_wallCheckbox];
    }

    [[VLCCoreInteraction sharedInstance] setVideoFilter: "clone" on: b_state];
    [_cloneNumberLabel setEnabled: b_state];
    [_cloneNumberTextField setEnabled: b_state];
    [_cloneNumberStepper setEnabled: b_state];
}

- (IBAction)cloneModifierChanged:(id)sender
{
    [[VLCCoreInteraction sharedInstance] setVideoFilterProperty: "clone-count" forFilter: "clone" withValue: getWidgetIntValue(_cloneNumberTextField)];
}

- (IBAction)enableWall:(id)sender
{
    BOOL b_state = [_wallCheckbox state];

    if (b_state && [_cloneCheckbox state]) {
        [_cloneCheckbox setState: NSOffState];
        [self enableClone:_cloneCheckbox];
    }

    [[VLCCoreInteraction sharedInstance] setVideoFilter: "wall" on: b_state];
    [_wallNumberOfColumnsTextField setEnabled: b_state];
    [_wallNumberOfColumnsStepper setEnabled: b_state];
    [_wallNumberOfColumnsLabel setEnabled: b_state];

    [_wallNumbersOfRowsTextField setEnabled: b_state];
    [_wallNumbersOfRowsStepper setEnabled: b_state];
    [_wallNumbersOfRowsLabel setEnabled: b_state];
}

- (IBAction)wallModifierChanged:(id)sender
{
    char const *psz_property;
    if (sender == _wallNumberOfColumnsTextField || sender == _wallNumberOfColumnsStepper)
        psz_property = "wall-cols";
    else
        psz_property = "wall-rows";
    [[VLCCoreInteraction sharedInstance] setVideoFilterProperty: psz_property forFilter: "wall" withValue: getWidgetIntValue(sender)];
}

#pragma mark -
#pragma mark color
- (IBAction)enableThreshold:(id)sender
{
    BOOL b_state = [_thresholdCheckbox state];

    [[VLCCoreInteraction sharedInstance] setVideoFilter: "colorthres" on: b_state];
    [_thresholdColorTextField setEnabled: b_state];
    [_thresholdColorLabel setEnabled: b_state];
    [_thresholdSaturationSlider setEnabled: b_state];
    [_thresholdSaturationLabel setEnabled: b_state];
    [_thresholdSimilaritySlider setEnabled: b_state];
    [_thresholdSimilarityLabel setEnabled: b_state];
}

- (IBAction)thresholdModifierChanged:(id)sender
{
    if (sender == _thresholdColorTextField)
        [[VLCCoreInteraction sharedInstance] setVideoFilterProperty: "colorthres-color" forFilter: "colorthres" withValue: getWidgetIntValue(sender)];
    else if (sender == _thresholdSaturationSlider) {
        [[VLCCoreInteraction sharedInstance] setVideoFilterProperty: "colorthres-saturationthres" forFilter: "colorthres" withValue: getWidgetIntValue(sender)];
        [_thresholdSaturationSlider setToolTip: [NSString stringWithFormat:@"%i", [_thresholdSaturationSlider intValue]]];
    } else {
        [[VLCCoreInteraction sharedInstance] setVideoFilterProperty: "colorthres-similaritythres" forFilter: "colorthres" withValue: getWidgetIntValue(sender)];
        [_thresholdSimilaritySlider setToolTip: [NSString stringWithFormat:@"%i", [_thresholdSimilaritySlider intValue]]];
    }
}

- (IBAction)enableSepia:(id)sender
{
    BOOL b_state = [_sepiaCheckbox state];

    [[VLCCoreInteraction sharedInstance] setVideoFilter: "sepia" on: b_state];
    [_sepiaTextField setEnabled: b_state];
    [_sepiaStepper setEnabled: b_state];
    [_sepiaLabel setEnabled: b_state];
}

- (IBAction)sepiaModifierChanged:(id)sender
{
    [[VLCCoreInteraction sharedInstance] setVideoFilterProperty: "sepia-intensity" forFilter: "sepia" withValue: getWidgetIntValue(sender)];
}

- (IBAction)enableGradient:(id)sender
{
    BOOL b_state = [_gradientCheckbox state];

    [[VLCCoreInteraction sharedInstance] setVideoFilter: "gradient" on: b_state];
    [_gradientModePopup setEnabled: b_state];
    [_gradientModeLabel setEnabled: b_state];
    [_gradientColorCheckbox setEnabled: b_state];
    [_gradientCartoonCheckbox setEnabled: b_state];
}

- (IBAction)gradientModifierChanged:(id)sender
{
    if (sender == _gradientModePopup) {
        vlc_value_t value = { .psz_string = (char *)[[[sender selectedItem] representedObject] UTF8String] };
        [[VLCCoreInteraction sharedInstance] setVideoFilterProperty: "gradient-mode" forFilter: "gradient" withValue: value];
    } else if (sender == _gradientColorCheckbox)
        [[VLCCoreInteraction sharedInstance] setVideoFilterProperty: "gradient-type" forFilter: "gradient" withValue: getWidgetBoolValue(sender)];
    else
        [[VLCCoreInteraction sharedInstance] setVideoFilterProperty: "gradient-cartoon" forFilter: "gradient" withValue: getWidgetBoolValue(sender)];
}

- (IBAction)enableExtract:(id)sender
{
    BOOL b_state = [_extractCheckbox state];
    [[VLCCoreInteraction sharedInstance] setVideoFilter: "extract" on: b_state];
    [_extractTextField setEnabled: b_state];
    [_extractLabel setEnabled: b_state];
}

- (IBAction)extractModifierChanged:(id)sender
{
    [[VLCCoreInteraction sharedInstance] setVideoFilterProperty: "extract-component" forFilter: "extract" withValue: getWidgetIntValue(sender)];
}

- (IBAction)enableInvert:(id)sender
{
    [[VLCCoreInteraction sharedInstance] setVideoFilter: "invert" on: [_invertCheckbox state]];
}

- (IBAction)enablePosterize:(id)sender
{
    BOOL b_state = [_posterizeCheckbox state];

    [[VLCCoreInteraction sharedInstance] setVideoFilter: "posterize" on: b_state];
    [_posterizeTextField setEnabled: b_state];
    [_posterizeStepper setEnabled: b_state];
    [_posterizeLabel setEnabled: b_state];
}

- (IBAction)posterizeModifierChanged:(id)sender
{
    [[VLCCoreInteraction sharedInstance] setVideoFilterProperty: "posterize-level" forFilter: "posterize" withValue: getWidgetIntValue(sender)];
}

- (IBAction)enableBlur:(id)sender
{
    BOOL b_state = [_blurCheckbox state];

    [[VLCCoreInteraction sharedInstance] setVideoFilter: "motionblur" on: b_state];
    [_blurSlider setEnabled: b_state];
    [_blurLabel setEnabled: b_state];
}

- (IBAction)blurModifierChanged:(id)sender
{
    [[VLCCoreInteraction sharedInstance] setVideoFilterProperty: "blur-factor" forFilter: "motionblur" withValue: getWidgetIntValue(sender)];
    [sender setToolTip: [NSString stringWithFormat:@"%i", [sender intValue]]];
}

- (IBAction)enableMotionDetect:(id)sender
{
    [[VLCCoreInteraction sharedInstance] setVideoFilter: "motiondetect" on: [_motiondetectCheckbox state]];
}

- (IBAction)enableWaterEffect:(id)sender
{
    [[VLCCoreInteraction sharedInstance] setVideoFilter: "ripple" on: [_watereffectCheckbox state]];
}

- (IBAction)enableWaves:(id)sender
{
    [[VLCCoreInteraction sharedInstance] setVideoFilter: "wave" on: [_wavesCheckbox state]];
}

- (IBAction)enablePsychedelic:(id)sender
{
    [[VLCCoreInteraction sharedInstance] setVideoFilter: "psychedelic" on: [_psychedelicCheckbox state]];
}

#pragma mark -
#pragma mark Miscellaneous
- (IBAction)enableAddText:(id)sender
{
    BOOL b_state = [_addTextCheckbox state];
    VLCCoreInteraction *vci_si = [VLCCoreInteraction sharedInstance];

    [_addTextPositionPopup setEnabled: b_state];
    [_addTextPositionLabel setEnabled: b_state];
    [_addTextTextLabel setEnabled: b_state];
    [_addTextTextTextField setEnabled: b_state];
    [vci_si setVideoFilter: "marq" on: b_state];
    [vci_si setVideoFilterProperty: "marq-marquee" forFilter: "marq" withValue: getWidgetStringValue(_addTextTextTextField)];
    [vci_si setVideoFilterProperty: "marq-position" forFilter: "marq" withValue: (vlc_value_t){ .i_int = [[_addTextPositionPopup selectedItem] tag] }];
}

- (IBAction)addTextModifierChanged:(id)sender
{
    if (sender == _addTextTextTextField)
        [[VLCCoreInteraction sharedInstance] setVideoFilterProperty: "marq-marquee" forFilter: "marq" withValue: getWidgetStringValue(sender)];
    else
        [[VLCCoreInteraction sharedInstance] setVideoFilterProperty: "marq-position" forFilter: "marq" withValue: (vlc_value_t){ .i_int = [[sender selectedItem] tag] }];
}

- (IBAction)enableAddLogo:(id)sender
{
    BOOL b_state = [_addLogoCheckbox state];

    [_addLogoPositionPopup setEnabled: b_state];
    [_addLogoPositionLabel setEnabled: b_state];
    [_addLogoLogoTextField setEnabled: b_state];
    [_addLogoLogoLabel setEnabled: b_state];
    [_addLogoTransparencySlider setEnabled: b_state];
    [_addLogoTransparencyLabel setEnabled: b_state];
    [[VLCCoreInteraction sharedInstance] setVideoFilter: "logo" on: b_state];
}

- (IBAction)addLogoModifierChanged:(id)sender
{
    if (sender == _addLogoLogoTextField)
        [[VLCCoreInteraction sharedInstance] setVideoFilterProperty: "logo-file" forFilter: "logo" withValue: getWidgetStringValue(sender)];
    else if (sender == _addLogoPositionPopup)
        [[VLCCoreInteraction sharedInstance] setVideoFilterProperty: "logo-position" forFilter: "logo" withValue: (vlc_value_t){ .i_int = [[_addLogoPositionPopup selectedItem] tag] }];
    else {
        [[VLCCoreInteraction sharedInstance] setVideoFilterProperty: "logo-opacity" forFilter: "logo" withValue: getWidgetIntValue(sender)];
        [_addLogoTransparencySlider setToolTip: [NSString stringWithFormat:@"%i", [_addLogoTransparencySlider intValue]]];
    }
}

- (IBAction)enableAnaglyph:(id)sender
{
    [[VLCCoreInteraction sharedInstance] setVideoFilter: "anaglyph" on: [_anaglyphCheckbox state]];
}

@end
