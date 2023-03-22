/*****************************************************************************
 * VLCBookmarksWindowController.m: MacOS X Bookmarks window
 *****************************************************************************
 * Copyright (C) 2005 - 2015 VLC authors and VideoLAN
 * $Id: e80f2dbf69a883b1fd723e94822979a60555d95b $
 *
 * Authors: Felix Paul Kühne <fkuehne at videolan dot org>
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
 * Note:
 * the code used to bind with VLC's modules is heavily based upon
 * ../wxwidgets/bookmarks.cpp, written by Gildas Bazin.
 * (he is a member of the VideoLAN team)
 *****************************************************************************/


/*****************************************************************************
 * Preamble
 *****************************************************************************/

#import "VLCBookmarksWindowController.h"
#import "CompatibilityFixes.h"

@interface VLCBookmarksWindowController() <NSTableViewDataSource, NSTableViewDelegate>
{
    input_thread_t *p_old_input;
}
@end

@implementation VLCBookmarksWindowController

/*****************************************************************************
 * GUI methods
 *****************************************************************************/

- (id)init
{
    self = [super initWithWindowNibName:@"Bookmarks"];

    return self;
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)windowDidLoad
{
    [self.window setCollectionBehavior: NSWindowCollectionBehaviorFullScreenAuxiliary];

    _dataTable.dataSource = self;
    _dataTable.delegate = self;
    _dataTable.action = @selector(goToBookmark:);
    _dataTable.target = self;

    /* main window */
    [self.window setTitle: _NS("Bookmarks")];
    [_addButton setTitle: _NS("Add")];
    [_clearButton setTitle: _NS("Clear")];
    [_editButton setTitle: _NS("Edit")];
    [_removeButton setTitle: _NS("Remove")];
    [[[_dataTable tableColumnWithIdentifier:@"description"] headerCell]
     setStringValue: _NS("Description")];
    [[[_dataTable tableColumnWithIdentifier:@"time_offset"] headerCell]
     setStringValue: _NS("Time")];

    /* edit window */
    [_editOKButton setTitle: _NS("OK")];
    [_editCancelButton setTitle: _NS("Cancel")];
    [_editNameLabel setStringValue: _NS("Name")];
    [_editTimeLabel setStringValue: _NS("Time")];

    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(inputChangedEvent:)
                                                 name:VLCInputChangedNotification
                                               object:nil];
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
    }
}

-(void)inputChangedEvent:(NSNotification *)o_notification
{
    [_dataTable reloadData];
}

- (IBAction)add:(id)sender
{
    /* add item to list */
    input_thread_t * p_input = pl_CurrentInput(getIntf());

    if (!p_input)
        return;

    seekpoint_t bookmark;

    if (!input_Control(p_input, INPUT_GET_BOOKMARK, &bookmark)) {
        bookmark.psz_name = _("Untitled");
        input_Control(p_input, INPUT_ADD_BOOKMARK, &bookmark);
    }

    vlc_object_release(p_input);

    [_dataTable reloadData];
}

- (IBAction)clear:(id)sender
{
    /* clear table */
    input_thread_t * p_input = pl_CurrentInput(getIntf());

    if (!p_input)
        return;

    input_Control(p_input, INPUT_CLEAR_BOOKMARKS);

    vlc_object_release(p_input);

    [_dataTable reloadData];
}

- (IBAction)edit:(id)sender
{
    /* put values to the sheet's fields and show sheet */
    /* we take the values from the core and not the table, because we cannot
     * really trust it */
    input_thread_t * p_input = pl_CurrentInput(getIntf());
    seekpoint_t **pp_bookmarks;
    int i_bookmarks;
    int row;
    row = [_dataTable selectedRow];

    if (!p_input)
        return;

    if (row < 0) {
        vlc_object_release(p_input);
        return;
    }

    if (input_Control(p_input, INPUT_GET_BOOKMARKS, &pp_bookmarks, &i_bookmarks) != VLC_SUCCESS) {
        vlc_object_release(p_input);
        return;
    }

    [_editNameTextField setStringValue: toNSStr(pp_bookmarks[row]->psz_name)];
    [_editTimeTextField setStringValue:[self timeStringForBookmark:pp_bookmarks[row]]];

    /* Just keep the pointer value to check if it
     * changes. Note, we don't need to keep a reference to the object.
     * so release it now. */
    p_old_input = p_input;
    vlc_object_release(p_input);

    [NSApp beginSheet: _editBookmarksWindow modalForWindow: self.window modalDelegate: _editBookmarksWindow didEndSelector: nil contextInfo: nil];

    // Clear the bookmark list
    for (int i = 0; i < i_bookmarks; i++)
        vlc_seekpoint_Delete(pp_bookmarks[i]);
    free(pp_bookmarks);
}

- (IBAction)edit_cancel:(id)sender
{
    /* close sheet */
    [NSApp endSheet:_editBookmarksWindow];
    [_editBookmarksWindow close];
}

- (IBAction)edit_ok:(id)sender
{
    input_thread_t * p_input = pl_CurrentInput(getIntf());

    /* save field contents and close sheet */
    [NSApp endSheet: _editBookmarksWindow];
    [_editBookmarksWindow close];

    if (!p_input) {
        NSBeginCriticalAlertSheet(_NS("No input"), _NS("OK"), @"", @"", self.window, nil, nil, nil, nil, @"%@",_NS("No input found. A stream must be playing or paused for bookmarks to work."));
        return;
    }
    if (p_old_input != p_input) {
        NSBeginCriticalAlertSheet(_NS("Input has changed"), _NS("OK"), @"", @"", self.window, nil, nil, nil, nil, @"%@",_NS("Input has changed, unable to save bookmark. Suspending playback with \"Pause\" while editing bookmarks to ensure to keep the same input."));
        vlc_object_release(p_input);
        return;
    }

    seekpoint_t **pp_bookmarks = NULL;
    int i_bookmarks = 0;
    if (input_Control(p_input, INPUT_GET_BOOKMARKS, &pp_bookmarks, &i_bookmarks) != VLC_SUCCESS) {
        msg_Warn(getIntf(), "Cannot get bookmarks");
        vlc_object_release(p_input);
        return;
    }

    int i = [_dataTable selectedRow];

    free(pp_bookmarks[i]->psz_name);

    pp_bookmarks[i]->psz_name = strdup([[_editNameTextField stringValue] UTF8String]);

    NSArray * components = [[_editTimeTextField stringValue] componentsSeparatedByString:@":"];
    NSUInteger componentCount = [components count];
    if (componentCount == 1)
        pp_bookmarks[i]->i_time_offset = 1000000LL * ([[components firstObject] floatValue]);
    else if (componentCount == 2)
        pp_bookmarks[i]->i_time_offset = 1000000LL * ([[components firstObject] longLongValue] * 60 + [[components objectAtIndex:1] longLongValue]);
    else if (componentCount == 3)
        pp_bookmarks[i]->i_time_offset = 1000000LL * ([[components firstObject] longLongValue] * 3600 + [[components objectAtIndex:1] longLongValue] * 60 + [[components objectAtIndex:2] floatValue]);
    else {
        msg_Err(getIntf(), "Invalid string format for time");
    }

    if (input_Control(p_input, INPUT_CHANGE_BOOKMARK, pp_bookmarks[i], i) != VLC_SUCCESS) {
        msg_Err(getIntf(), "Unable to change the bookmark");
    }

    [_dataTable reloadData];

    vlc_object_release(p_input);

    // Clear the bookmark list
    for (int i = 0; i < i_bookmarks; i++)
        vlc_seekpoint_Delete(pp_bookmarks[i]);
    free(pp_bookmarks);
}


- (IBAction)goToBookmark:(id)sender
{
    input_thread_t * p_input = pl_CurrentInput(getIntf());

    if (!p_input)
        return;

    input_Control(p_input, INPUT_SET_BOOKMARK, [_dataTable selectedRow]);

    vlc_object_release(p_input);
}

- (IBAction)remove:(id)sender
{
    input_thread_t * p_input = pl_CurrentInput(getIntf());

    if (!p_input)
        return;

    int i_focused = [_dataTable selectedRow];

    if (i_focused >= 0)
        input_Control(p_input, INPUT_DEL_BOOKMARK, i_focused);

    vlc_object_release(p_input);

    [_dataTable reloadData];
}

- (NSString *)timeStringForBookmark:(seekpoint_t *)bookmark
{
    assert(bookmark != NULL);

    mtime_t total = bookmark->i_time_offset;
    uint64_t hour = ( total / ( CLOCK_FREQ * 3600 ) );
    uint64_t min = ( total % ( CLOCK_FREQ * 3600 ) ) / ( CLOCK_FREQ * 60 );
    float    sec = ( total % ( CLOCK_FREQ * 60 ) ) / ( CLOCK_FREQ * 1. );

    return [NSString stringWithFormat:@"%02llu:%02llu:%06.3f", hour, min, sec];
}

/*****************************************************************************
 * data source methods
 *****************************************************************************/

- (NSInteger)numberOfRowsInTableView:(NSTableView *)theDataTable
{
    /* return the number of bookmarks */
    input_thread_t * p_input = pl_CurrentInput(getIntf());
    seekpoint_t **pp_bookmarks;
    int i_bookmarks;

    if (!p_input)
        return 0;

    int returnValue = input_Control(p_input, INPUT_GET_BOOKMARKS, &pp_bookmarks, &i_bookmarks);
    vlc_object_release(p_input);

    if (returnValue != VLC_SUCCESS)
        return 0;

    for (int i = 0; i < i_bookmarks; i++)
        vlc_seekpoint_Delete(pp_bookmarks[i]);
    free(pp_bookmarks);

    return i_bookmarks;
}

- (id)tableView:(NSTableView *)theDataTable objectValueForTableColumn: (NSTableColumn *)theTableColumn row: (NSInteger)row
{
    /* return the corresponding data as NSString */
    input_thread_t * p_input = pl_CurrentInput(getIntf());
    seekpoint_t **pp_bookmarks;
    int i_bookmarks;
    id ret = @"";

    if (!p_input)
        return @"";
    else if (input_Control(p_input, INPUT_GET_BOOKMARKS, &pp_bookmarks, &i_bookmarks) != VLC_SUCCESS)
        ret = @"";
    else if (row >= i_bookmarks)
        ret = @"";
    else {
        NSString * identifier = [theTableColumn identifier];
        if ([identifier isEqualToString: @"description"])
            ret = toNSStr(pp_bookmarks[row]->psz_name);
		else if ([identifier isEqualToString: @"time_offset"]) {
            ret = [self timeStringForBookmark:pp_bookmarks[row]];
        }

        // Clear the bookmark list
        for (int i = 0; i < i_bookmarks; i++)
            vlc_seekpoint_Delete(pp_bookmarks[i]);
        free(pp_bookmarks);
    }
    vlc_object_release(p_input);
    return ret;
}

/*****************************************************************************
 * delegate methods
 *****************************************************************************/

- (void)tableViewSelectionDidChange:(NSNotification *)aNotification
{
    /* check whether a row is selected and en-/disable the edit/remove buttons */
    if ([_dataTable selectedRow] == -1) {
        /* no row is selected */
        [_editButton setEnabled: NO];
        [_removeButton setEnabled: NO];
    } else {
        /* a row is selected */
        [_editButton setEnabled: YES];
        [_removeButton setEnabled: YES];
    }
}

/* Called when the user hits CMD + C or copy is clicked in the edit menu
 */
- (void) copy:(id)sender {
    NSPasteboard *pasteBoard = [NSPasteboard generalPasteboard];
    NSIndexSet *selectionIndices = [_dataTable selectedRowIndexes];


    input_thread_t *p_input = pl_CurrentInput(getIntf());
    int i_bookmarks;
    seekpoint_t **pp_bookmarks;

    if (input_Control(p_input, INPUT_GET_BOOKMARKS, &pp_bookmarks, &i_bookmarks) != VLC_SUCCESS)
        return;

    [pasteBoard clearContents];
    NSUInteger index = [selectionIndices firstIndex];

    while(index != NSNotFound) {
        /* Get values */
        if (index >= i_bookmarks)
            break;
        NSString *name = toNSStr(pp_bookmarks[index]->psz_name);
        NSString *time = [self timeStringForBookmark:pp_bookmarks[index]];

        NSString *message = [NSString stringWithFormat:@"%@ - %@", name, time];
        [pasteBoard writeObjects:@[message]];

        /* Get next index */
        index = [selectionIndices indexGreaterThanIndex:index];
    }

    // Clear the bookmark list
    for (int i = 0; i < i_bookmarks; i++)
        vlc_seekpoint_Delete(pp_bookmarks[i]);
    free(pp_bookmarks);
}

#pragma mark -
#pragma mark UI validation

/* Validate the copy menu item
 */
- (BOOL)validateUserInterfaceItem:(id <NSValidatedUserInterfaceItem>)anItem
{
    SEL theAction = [anItem action];

    if (theAction == @selector(copy:)) {
        if ([[_dataTable selectedRowIndexes] count] > 0) {
            return YES;
        }
        return NO;
    }
    /* Indicate that we handle the validation method,
     * even if we don’t implement the action
     */
    return YES;
}

@end
