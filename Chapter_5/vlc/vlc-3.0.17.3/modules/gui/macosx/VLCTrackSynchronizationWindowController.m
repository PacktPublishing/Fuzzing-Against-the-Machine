/*****************************************************************************
 * VLCTrackSynchronizationWindowController.m: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2011-2014 VLC authors and VideoLAN
 * Copyright (C) 2011-2015 Felix Paul Kühne
 * $Id: 85d6d7f33d94cb4657a282aa774b4497b9da9092 $
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
#import <vlc_common.h>
#import "VLCTrackSynchronizationWindowController.h"
#import "VLCCoreInteraction.h"

#define SUBSDELAY_CFG_MODE                     "subsdelay-mode"
#define SUBSDELAY_CFG_FACTOR                   "subsdelay-factor"
#define SUBSDELAY_MODE_ABSOLUTE                0
#define SUBSDELAY_MODE_RELATIVE_SOURCE_DELAY   1
#define SUBSDELAY_MODE_RELATIVE_SOURCE_CONTENT 2

@implementation VLCTrackSynchronizationWindowController

- (id)init
{
    self = [super initWithWindowNibName:@"SyncTracks"];

    return self;
}

- (void)windowDidLoad
{
    [self.window setTitle:_NS("Track Synchronization")];
    [_resetButton setTitle:_NS("Reset")];
    [_avLabel setStringValue:_NS("Audio/Video")];
    [_av_advanceLabel setStringValue: _NS("Audio track synchronization:")];
    [[_av_advanceTextField formatter] setFormat:[NSString stringWithFormat:@"#,##0.000 %@", _NS("s")]];
    [_av_advanceTextField setToolTip: _NS("A positive value means that the audio is ahead of the video")];
    [_svLabel setStringValue: _NS("Subtitles/Video")];
    [_sv_advanceLabel setStringValue: _NS("Subtitle track synchronization:")];
    [[_sv_advanceTextField formatter] setFormat:[NSString stringWithFormat:@"#,##0.000 %@", _NS("s")]];
    [_sv_advanceTextField setToolTip: _NS("A positive value means that the subtitles are ahead of the video")];
    [_sv_speedLabel setStringValue: _NS("Subtitle speed:")];
    [[_sv_speedTextField formatter] setFormat:[NSString stringWithFormat:@"#,##0.000 %@", _NS("fps")]];
    [_sv_durLabel setStringValue: _NS("Subtitle duration factor:")];

    int i_mode = var_InheritInteger(getIntf(), SUBSDELAY_CFG_MODE);
    NSString * o_toolTip, * o_suffix;

    switch (i_mode) {
        default:
        case SUBSDELAY_MODE_ABSOLUTE:
            o_toolTip = _NS("Extend subtitle duration by this value.\nSet 0 to disable.");
            o_suffix = @" s";
            break;
        case SUBSDELAY_MODE_RELATIVE_SOURCE_DELAY:
            o_toolTip = _NS("Multiply subtitle duration by this value.\nSet 0 to disable.");
            o_suffix = @"";
            break;
        case SUBSDELAY_MODE_RELATIVE_SOURCE_CONTENT:
            o_toolTip = _NS("Recalculate subtitle duration according\nto their content and this value.\nSet 0 to disable.");
            o_suffix = @"";
            break;
    }

    [[_sv_durTextField formatter] setFormat:[NSString stringWithFormat:@"#,##0.000%@", o_suffix]];
    [_sv_durTextField setToolTip: o_toolTip];

    [self.window setCollectionBehavior: NSWindowCollectionBehaviorFullScreenAuxiliary];

    [self resetValues:self];
}

- (void)updateCocoaWindowLevel:(NSInteger)i_level
{
    if (self.isWindowLoaded && [self.window isVisible] && [self.window level] != i_level)
        [self.window setLevel: i_level];
}

- (IBAction)toggleWindow:(id)sender
{
    if ([self.window isVisible])
        [self.window orderOut:sender];
    else {
        [self.window setLevel: [[[VLCMain sharedInstance] voutController] currentStatusWindowLevel]];
        [self.window makeKeyAndOrderFront:sender];

        [self updateValues];
    }
}

- (IBAction)resetValues:(id)sender
{
    [_av_advanceTextField setFloatValue:0.0];
    [_sv_advanceTextField setFloatValue:0.0];
    [_sv_speedTextField setFloatValue:1.0];
    [_sv_durTextField setFloatValue:0.0];
    [_avStepper setFloatValue:0.0];
    [_sv_advanceStepper setFloatValue:0.0];
    [_sv_speedStepper setFloatValue:1.0];
    [_sv_durStepper setFloatValue:0.0];

    input_thread_t * p_input = pl_CurrentInput(getIntf());

    if (p_input) {
        var_SetInteger(p_input, "audio-delay", 0.0);
        var_SetInteger(p_input, "spu-delay", 0.0);
        var_SetFloat(p_input, "sub-fps", 1.0);
        [self svDurationValueChanged:nil];
        vlc_object_release(p_input);
    }
}

- (void)updateValues
{
    input_thread_t * p_input = pl_CurrentInput(getIntf());

    if (p_input) {
        [_av_advanceTextField setDoubleValue: var_GetInteger(p_input, "audio-delay") / 1000000.];
        [_sv_advanceTextField setDoubleValue: var_GetInteger(p_input, "spu-delay") / 1000000.];
        [_sv_speedTextField setFloatValue: var_GetFloat(p_input, "sub-fps")];
        vlc_object_release(p_input);
    }
    [_avStepper setDoubleValue: [_av_advanceTextField doubleValue]];
    [_sv_advanceStepper setDoubleValue: [_sv_advanceTextField doubleValue]];
    [_sv_speedStepper setDoubleValue: [_sv_speedTextField doubleValue]];
}

- (IBAction)avValueChanged:(id)sender
{
    if (sender == _avStepper)
        [_av_advanceTextField setDoubleValue: [_avStepper doubleValue]];
    else
        [_avStepper setDoubleValue: [_av_advanceTextField doubleValue]];

    input_thread_t * p_input = pl_CurrentInput(getIntf());

    if (p_input) {
        var_SetInteger(p_input, "audio-delay", [_av_advanceTextField doubleValue] * 1000000.);
        vlc_object_release(p_input);
    }
}

- (IBAction)svAdvanceValueChanged:(id)sender
{
    if (sender == _sv_advanceStepper)
        [_sv_advanceTextField setDoubleValue: [_sv_advanceStepper doubleValue]];
    else
        [_sv_advanceStepper setDoubleValue: [_sv_advanceTextField doubleValue]];

    input_thread_t * p_input = pl_CurrentInput(getIntf());

    if (p_input) {
        var_SetInteger(p_input, "spu-delay", [_sv_advanceTextField doubleValue] * 1000000.);
        vlc_object_release(p_input);
    }
}

- (IBAction)svSpeedValueChanged:(id)sender
{
    if (sender == _sv_speedStepper)
        [_sv_speedTextField setFloatValue: [_sv_speedStepper floatValue]];
    else
        [_sv_speedStepper setFloatValue: [_sv_speedTextField floatValue]];

    input_thread_t * p_input = pl_CurrentInput(getIntf());

    if (p_input) {
        var_SetFloat(p_input, "sub-fps", [_sv_speedTextField floatValue]);
        vlc_object_release(p_input);
    }
}

- (IBAction)svDurationValueChanged:(id)sender
{
    if (sender == _sv_durStepper)
        [_sv_durTextField setFloatValue: [_sv_durStepper floatValue]];
    else
        [_sv_durStepper setFloatValue: [_sv_durTextField floatValue]];

    input_thread_t * p_input = pl_CurrentInput(getIntf());

    if (p_input) {
        float f_factor = [_sv_durTextField floatValue];
        NSArray<NSValue *> *vouts = getVouts();

        if (vouts)
            for (NSValue *ptr in vouts) {
                vout_thread_t *p_vout = [ptr pointerValue];

                var_SetFloat(p_vout, SUBSDELAY_CFG_FACTOR, f_factor);
                vlc_object_release(p_vout);
            }
        [[VLCCoreInteraction sharedInstance] setVideoFilter: "subsdelay" on: f_factor > 0];

        vlc_object_release(p_input);
    }
}

@end
