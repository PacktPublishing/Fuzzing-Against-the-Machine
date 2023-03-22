/*****************************************************************************
 * misc.m: code not specific to vlc
 *****************************************************************************
 * Copyright (C) 2003-2015 VLC authors and VideoLAN
 * $Id: acffad80bb2e5804d906a91227806f709a151783 $
 *
 * Authors: Jon Lech Johansen <jon-vl@nanocrew.net>
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

#import "CompatibilityFixes.h"
#import "misc.h"
#import "VLCMain.h"                                          /* VLCApplication */
#import "VLCMainWindow.h"
#import "VLCMainMenu.h"
#import "VLCControlsBarCommon.h"
#import "VLCCoreInteraction.h"
#import <vlc_actions.h>

/*****************************************************************************
 * VLCDragDropView
 *****************************************************************************/

@implementation VLCDropDisabledImageView

- (void)awakeFromNib
{
    [self unregisterDraggedTypes];
}

@end

/*****************************************************************************
 * VLCDragDropView
 *****************************************************************************/

@interface VLCDragDropView()
{
    bool b_activeDragAndDrop;
}
@end

@implementation VLCDragDropView

- (id)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        // default value
        [self setDrawBorder:YES];
    }

    return self;
}

- (void)enablePlaylistItems
{
    [self registerForDraggedTypes:[NSArray arrayWithObjects:NSFilenamesPboardType, @"VLCPlaylistItemPboardType", nil]];
}

- (BOOL)mouseDownCanMoveWindow
{
    return YES;
}

- (void)dealloc
{
    [self unregisterDraggedTypes];
}

- (void)awakeFromNib
{
    [self registerForDraggedTypes:[NSArray arrayWithObject:NSFilenamesPboardType]];
}

- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender
{
    if ((NSDragOperationGeneric & [sender draggingSourceOperationMask]) == NSDragOperationGeneric) {
        b_activeDragAndDrop = YES;
        [self setNeedsDisplay:YES];

        return NSDragOperationCopy;
    }

    return NSDragOperationNone;
}

- (void)draggingEnded:(id < NSDraggingInfo >)sender
{
    b_activeDragAndDrop = NO;
    [self setNeedsDisplay:YES];
}

- (void)draggingExited:(id < NSDraggingInfo >)sender
{
    b_activeDragAndDrop = NO;
    [self setNeedsDisplay:YES];
}

- (BOOL)prepareForDragOperation:(id <NSDraggingInfo>)sender
{
    return YES;
}

- (BOOL)performDragOperation:(id <NSDraggingInfo>)sender
{
    BOOL b_returned;

    if (_dropHandler && [_dropHandler respondsToSelector:@selector(performDragOperation:)])
        b_returned = [_dropHandler performDragOperation:sender];
    else // default
        b_returned = [[VLCCoreInteraction sharedInstance] performDragOperation:sender];

    [self setNeedsDisplay:YES];
    return b_returned;
}

- (void)concludeDragOperation:(id <NSDraggingInfo>)sender
{
    [self setNeedsDisplay:YES];
}

- (void)drawRect:(NSRect)dirtyRect
{
    if ([self drawBorder] && b_activeDragAndDrop) {
        NSRect frameRect = [self bounds];

        [[NSColor selectedControlColor] set];
        NSFrameRectWithWidthUsingOperation(frameRect, 2., NSCompositeSourceOver);
    }

    [super drawRect:dirtyRect];
}

@end


/*****************************************************************************
 * VLCMainWindowSplitView implementation
 *****************************************************************************/
@implementation VLCMainWindowSplitView : NSSplitView

// Custom color for the dividers
- (NSColor *)dividerColor
{
    return [NSColor colorWithCalibratedRed:.60 green:.60 blue:.60 alpha:1.];
}

// Custom thickness for the divider
- (CGFloat)dividerThickness
{
    return 1.0;
}

@end

/*****************************************************************************
 * VLCThreePartImageView interface
 *****************************************************************************/

@interface VLCThreePartImageView()
{
    NSImage *_left_img;
    NSImage *_middle_img;
    NSImage *_right_img;
}
@end

@implementation VLCThreePartImageView

- (void)setImagesLeft:(NSImage *)left middle: (NSImage *)middle right:(NSImage *)right
{
    _left_img = left;
    _middle_img = middle;
    _right_img = right;
}

- (void)drawRect:(NSRect)rect
{
    NSRect bnds = [self bounds];
    NSDrawThreePartImage( bnds, _left_img, _middle_img, _right_img, NO, NSCompositeSourceOver, 1, NO );
}

@end

@interface PositionFormatter()
{
    NSCharacterSet *o_forbidden_characters;
}
@end

@implementation PositionFormatter

- (id)init
{
    self = [super init];
    NSMutableCharacterSet *nonNumbers = [[[NSCharacterSet decimalDigitCharacterSet] invertedSet] mutableCopy];
    [nonNumbers removeCharactersInString:@"-:"];
    o_forbidden_characters = [nonNumbers copy];

    return self;
}

- (NSString*)stringForObjectValue:(id)obj
{
    if([obj isKindOfClass:[NSString class]])
        return obj;
    if([obj isKindOfClass:[NSNumber class]])
        return [obj stringValue];

    return nil;
}

- (BOOL)getObjectValue:(id*)obj forString:(NSString*)string errorDescription:(NSString**)error
{
    *obj = [string copy];
    return YES;
}

- (BOOL)isPartialStringValid:(NSString*)partialString newEditingString:(NSString**)newString errorDescription:(NSString**)error
{
    if ([partialString rangeOfCharacterFromSet:o_forbidden_characters options:NSLiteralSearch].location != NSNotFound) {
        return NO;
    } else {
        return YES;
    }
}

@end

@implementation NSView (EnableSubviews)

- (void)enableSubviews:(BOOL)b_enable
{
    for (NSView *o_view in [self subviews]) {
        [o_view enableSubviews:b_enable];

        // enable NSControl
        if ([o_view respondsToSelector:@selector(setEnabled:)]) {
            [(NSControl *)o_view setEnabled:b_enable];
        }
        // also "enable / disable" text views
        if ([o_view respondsToSelector:@selector(setTextColor:)]) {
            if (b_enable == NO) {
                [(NSTextField *)o_view setTextColor:[NSColor disabledControlTextColor]];
            } else {
                [(NSTextField *)o_view setTextColor:[NSColor controlTextColor]];
            }
        }

    }
}

@end

/*****************************************************************************
 * VLCByteCountFormatter addition
 *****************************************************************************/

@implementation VLCByteCountFormatter

+ (NSString *)stringFromByteCount:(long long)byteCount countStyle:(NSByteCountFormatterCountStyle)countStyle
{
    // Use native implementation on >= mountain lion
    Class byteFormatterClass = NSClassFromString(@"NSByteCountFormatter");
    if (byteFormatterClass && [byteFormatterClass respondsToSelector:@selector(stringFromByteCount:countStyle:)]) {
        return [byteFormatterClass stringFromByteCount:byteCount countStyle:NSByteCountFormatterCountStyleFile];
    }

    float devider = 0.;
    float returnValue = 0.;
    NSString *suffix;

    NSNumberFormatter *theFormatter = [[NSNumberFormatter alloc] init];
    [theFormatter setLocale:[NSLocale currentLocale]];
    [theFormatter setAllowsFloats:YES];

    NSString *returnString = @"";

    if (countStyle != NSByteCountFormatterCountStyleDecimal)
        devider = 1024.;
    else
        devider = 1000.;

    if (byteCount < 1000) {
        returnValue = byteCount;
        suffix = _NS("B");
        [theFormatter setMaximumFractionDigits:0];
        goto end;
    }

    if (byteCount < 1000000) {
        returnValue = byteCount / devider;
        suffix = _NS("KB");
        [theFormatter setMaximumFractionDigits:0];
        goto end;
    }

    if (byteCount < 1000000000) {
        returnValue = byteCount / devider / devider;
        suffix = _NS("MB");
        [theFormatter setMaximumFractionDigits:1];
        goto end;
    }

    [theFormatter setMaximumFractionDigits:2];
    if (byteCount < 1000000000000) {
        returnValue = byteCount / devider / devider / devider;
        suffix = _NS("GB");
        goto end;
    }

    returnValue = byteCount / devider / devider / devider / devider;
    suffix = _NS("TB");

end:
    returnString = [NSString stringWithFormat:@"%@ %@", [theFormatter stringFromNumber:[NSNumber numberWithFloat:returnValue]], suffix];

    return returnString;
}

@end
