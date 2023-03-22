/*****************************************************************************
 * menus.cpp : Qt menus
 *****************************************************************************
 * Copyright © 2006-2011 the VideoLAN team
 * $Id: 460653c93e2fb5a37bacea47b174b69a8941d7ef $
 *
 * Authors: Clément Stenac <zorglub@videolan.org>
 *          Jean-Baptiste Kempf <jb@videolan.org>
 *          Jean-Philippe André <jpeg@videolan.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
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

/** \todo
 * - Remove static currentGroup
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_intf_strings.h>
#include <vlc_vout.h>                             /* vout_thread_t */
#include <vlc_aout.h>                             /* audio_output_t */

#include "menus.hpp"

#include "main_interface.hpp"                     /* View modifications */
#include "dialogs_provider.hpp"                   /* Dialogs display */
#include "input_manager.hpp"                      /* Input Management */
#include "recents.hpp"                            /* Recent Items */
#include "actions_manager.hpp"                    /* Actions Management: play+volume */
#include "extensions_manager.hpp"                 /* Extensions menu */
#include "util/qmenuview.hpp"                     /* Simple Playlist menu */
#include "components/playlist/playlist_model.hpp" /* PLModel getter */
#include "components/playlist/standardpanel.hpp"  /* PLView getter */
#include "components/extended_panels.hpp"

#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QActionGroup>
#include <QSignalMapper>
#include <QStatusBar>

/*
  This file defines the main menus and the pop-up menu (right-click menu)
  and the systray menu (in that order in the file)

  There are 4 menus that have to be rebuilt everytime there are called:
  Audio, Video, Navigation, view
  4 functions are building those menus: AudioMenu, VideoMenu, NavigMenu, View
  and 3 functions associated are collecting the objects :
  InputAutoMenuBuilder, AudioAutoMenuBuilder, VideoAutoMenuBuilder.

  A QSignalMapper decides when to rebuild those menus cf MenuFunc in the .hpp
  Just before one of those menus are aboutToShow(), they are rebuild.
  */

enum
{
    ITEM_NORMAL, /* not a checkbox, nor a radio */
    ITEM_CHECK,  /* Checkbox */
    ITEM_RADIO   /* Radiobox */
};

static QActionGroup *currentGroup;

QMenu *VLCMenuBar::recentsMenu = NULL;
QMenu *VLCMenuBar::audioDeviceMenu = NULL;
QMenu *VLCMenuBar::rendererMenu = NULL;
QActionGroup *VLCMenuBar::rendererGroup = NULL;

/**
 * @brief Add static entries to DP in menus
 **/
QAction *addDPStaticEntry( QMenu *menu,
                       const QString& text,
                       const char *icon,
                       const char *member,
                       const char *shortcut = NULL,
                       QAction::MenuRole role = QAction::NoRole
                       )
{
    QAction *action = NULL;
#ifndef __APPLE__ /* We don't set icons in menus in MacOS X */
    if( !EMPTY_STR( icon ) )
    {
        if( !EMPTY_STR( shortcut ) )
            action = menu->addAction( QIcon( icon ), text, THEDP,
                                      member, qtr( shortcut ) );
        else
            action = menu->addAction( QIcon( icon ), text, THEDP, member );
    }
    else
#endif
    {
        if( !EMPTY_STR( shortcut ) )
            action = menu->addAction( text, THEDP, member, qtr( shortcut ) );
        else
            action = menu->addAction( text, THEDP, member );
    }
#ifdef __APPLE__
    action->setMenuRole( role );
#else
    Q_UNUSED( role );
#endif
    action->setData( VLCMenuBar::ACTION_STATIC );
    return action;
}

/**
 * @brief Add static entries to MIM in menus
 **/
QAction* addMIMStaticEntry( intf_thread_t *p_intf,
                            QMenu *menu,
                            const QString& text,
                            const char *icon,
                            const char *member,
                            bool bStatic = false )
{
    QAction *action;
#ifndef __APPLE__ /* We don't set icons in menus in MacOS X */
    if( !EMPTY_STR( icon ) )
    {
        action = menu->addAction( text, THEMIM,  member );
        action->setIcon( QIcon( icon ) );
    }
    else
#endif
    {
        action = menu->addAction( text, THEMIM, member );
    }
    action->setData( VLCMenuBar::ACTION_STATIC |
                     ( bStatic ) ? VLCMenuBar::ACTION_ALWAYS_ENABLED
                                 : VLCMenuBar::ACTION_NONE
                   );
    return action;
}

/**
 * @brief Enable all static entries of a menu, disable the others
 * @param menu the menu in which the entries will be disabled
 * @param enable if false, disable all entries
 **/
void VLCMenuBar::EnableStaticEntries( QMenu *menu, bool enable = true )
{
    if( !menu ) return;

    QList< QAction* > actions = menu->actions();
    for( int i = 0; i < actions.count(); ++i )
    {
        int actionflags = actions[i]->data().toInt();
        if ( actionflags & ACTION_MANAGED )
            actions[i]->setEnabled(
                ( actionflags & ACTION_ALWAYS_ENABLED )
                ||
                enable
            );
    }
}

/**
 * \return Number of static entries
 **/
inline int DeleteNonStaticEntries( QMenu *menu )
{
    if( !menu ) return VLC_EGENERIC;

    int i_ret = 0;

    QList< QAction* > actions = menu->actions();
    for( int i = 0; i < actions.count(); ++i )
    {
        if( actions[i]->data().toInt() & VLCMenuBar::ACTION_NO_CLEANUP )
            i_ret++;
        else
            delete actions[i];
    }
    return i_ret;
}

/**
 * \return QAction associated to psz_var variable
 **/
static QAction * FindActionWithVar( QMenu *menu, const char *psz_var )
{
    QList< QAction* > actions = menu->actions();
    for( int i = 0; i < actions.count(); ++i )
    {
        if( actions[i]->data().toString() == psz_var )
            return actions[i];
    }
    return NULL;
}

/*****************************************************************************
 * Definitions of variables for the dynamic menus
 *****************************************************************************/
#define PUSH_OBJVAR(object,var) \
    do { \
        varnames.append(var); \
        objects.append(VLC_OBJECT(object)); \
    } while (0)

#define PUSH_VAR(var) PUSH_OBJVAR(p_object, var)
#define PUSH_INPUTVAR(var) PUSH_OBJVAR(p_input, var)
#define PUSH_PLVAR(var) PUSH_OBJVAR(pl, var)

static int InputAutoMenuBuilder( input_thread_t *p_input,
        QVector<vlc_object_t *> &objects, QVector<const char *> &varnames )
{
    PUSH_INPUTVAR( "bookmark" );
    PUSH_INPUTVAR( "title" );
    PUSH_INPUTVAR( "chapter" );
    PUSH_INPUTVAR( "program" );

    return VLC_SUCCESS;
}

static int VideoAutoMenuBuilder( playlist_t *pl, input_thread_t *p_input,
        QVector<vlc_object_t *> &objects, QVector<const char *> &varnames )
{
    vout_thread_t *p_object = p_input ? input_GetVout( p_input ) : NULL;

    PUSH_INPUTVAR( "video-es" );
    PUSH_PLVAR( "fullscreen" );
    PUSH_PLVAR( "video-wallpaper" );
    PUSH_VAR( "video-snapshot" );
    PUSH_VAR( "zoom" );
    PUSH_VAR( "autoscale" );
    PUSH_VAR( "aspect-ratio" );
    PUSH_VAR( "crop" );
    PUSH_VAR( "deinterlace" );
    PUSH_VAR( "deinterlace-mode" );

    if( p_object )
        vlc_object_release( p_object );
    return VLC_SUCCESS;
}

static int SubsAutoMenuBuilder( input_thread_t *p_input,
        QVector<vlc_object_t *> &objects, QVector<const char *> &varnames )
{
    PUSH_INPUTVAR( "spu-es" );

    return VLC_SUCCESS;
}

static int AudioAutoMenuBuilder( input_thread_t *p_input,
        QVector<vlc_object_t *> &objects, QVector<const char *> &varnames )
{
    audio_output_t *p_object = p_input ? input_GetAout( p_input ) : NULL;

    PUSH_INPUTVAR( "audio-es" );
    PUSH_VAR( "stereo-mode" );
    PUSH_VAR( "visual" );

    if( p_object )
        vlc_object_release( p_object );
    return VLC_SUCCESS;
}

/*****************************************************************************
 * All normal menus
 * Simple Code
 *****************************************************************************/

// Static menu
static inline void addMenuToMainbar( QMenu *func, QString title, QMenuBar *bar ) {
    func->setTitle( title );
    bar->addMenu( func);
}

// Dynamic menu
#define BAR_DADD( func, title, id ) { \
    QMenu *_menu = func; _menu->setTitle( title ); bar->addMenu( _menu ); \
    MenuFunc *f = new MenuFunc( _menu, id ); \
    CONNECT( _menu, aboutToShow(), THEDP->menusUpdateMapper, map() ); \
    THEDP->menusUpdateMapper->setMapping( _menu, f ); }

// Add a simple action
static inline void addAction( QMenu *_menu, QVariant val, QString title ) {
    QAction *_action = new QAction( title, _menu );
    _action->setData( val );
    _menu->addAction( _action );
}

// Add an action with a submenu
static inline QMenu *addActionWithSubmenu( QMenu *_menu, QVariant val, QString title ) {
    QAction *_action = new QAction( title, _menu );
    QMenu *_submenu = new QMenu( _menu );
    _action->setData( val );
    _action->setMenu( _submenu );
    _menu->addAction( _action );
    return _submenu;
}

// Add an action that is a checkbox
static inline void addActionWithCheckbox( QMenu *_menu, QVariant val, QString title ) {
    QAction *_action = new QAction( title, _menu );
    _action->setData( val );
    _action->setCheckable( true );
    _menu->addAction( _action );
}

/**
 * Main Menu Bar Creation
 **/
void VLCMenuBar::createMenuBar( MainInterface *mi,
                              intf_thread_t *p_intf )
{
    /* QMainWindows->menuBar()
       gives the QProcess::destroyed timeout issue on Cleanlooks style with
       setDesktopAware set to false */
    QMenuBar *bar = mi->menuBar();

    addMenuToMainbar( FileMenu( p_intf, bar, mi ), qtr( "&Media" ), bar );

    /* Dynamic menus, rebuilt before being showed */
    BAR_DADD( NavigMenu( p_intf, bar ), qtr( "P&layback" ), 3 );
    BAR_DADD( AudioMenu( p_intf, bar ), qtr( "&Audio" ), 1 );
    BAR_DADD( VideoMenu( p_intf, bar ), qtr( "&Video" ), 2 );
    BAR_DADD( SubtitleMenu( p_intf, bar ), qtr( "Subti&tle" ), 5 );

    addMenuToMainbar( ToolsMenu( p_intf, bar ), qtr( "Tool&s" ), bar );

    /* View menu, a bit different */
    BAR_DADD( ViewMenu( p_intf, NULL, mi ), qtr( "V&iew" ), 4 );

    addMenuToMainbar( HelpMenu( bar ), qtr( "&Help" ), bar );

}

/**
 * Media ( File ) Menu
 * Opening, streaming and quit
 **/
QMenu *VLCMenuBar::FileMenu( intf_thread_t *p_intf, QWidget *parent, MainInterface *mi )
{
    QMenu *menu = new QMenu( parent );
    QAction *action;

    addDPStaticEntry( menu, qtr( "Open &File..." ),
        ":/type/file-asym.svg", SLOT( simpleOpenDialog() ), "Ctrl+O" );
    addDPStaticEntry( menu, qtr( "&Open Multiple Files..." ),
        ":/type/file-asym.svg", SLOT( openFileDialog() ), "Ctrl+Shift+O" );
    addDPStaticEntry( menu, qtr( I_OP_OPDIR ),
        ":/type/folder-grey.svg", SLOT( PLOpenDir() ), "Ctrl+F" );
    addDPStaticEntry( menu, qtr( "Open &Disc..." ),
        ":/type/disc.svg", SLOT( openDiscDialog() ), "Ctrl+D" );
    addDPStaticEntry( menu, qtr( "Open &Network Stream..." ),
        ":/type/network.svg", SLOT( openNetDialog() ), "Ctrl+N" );
    addDPStaticEntry( menu, qtr( "Open &Capture Device..." ),
        ":/type/capture-card.svg", SLOT( openCaptureDialog() ), "Ctrl+C" );

    addDPStaticEntry( menu, qtr( "Open &Location from clipboard" ),
                      NULL, SLOT( openUrlDialog() ), "Ctrl+V" );

    if( !recentsMenu && var_InheritBool( p_intf, "qt-recentplay" ) )
        recentsMenu = new QMenu( qtr( "Open &Recent Media" ) );

    if( recentsMenu )
    {
        updateRecents( p_intf );
        menu->addMenu( recentsMenu );
    }
    menu->addSeparator();

    addDPStaticEntry( menu, qtr( I_PL_SAVE ), "", SLOT( savePlayingToPlaylist() ),
        "Ctrl+Y" );

#ifdef ENABLE_SOUT
    addDPStaticEntry( menu, qtr( "Conve&rt / Save..." ), "",
        SLOT( openAndTranscodingDialogs() ), "Ctrl+R" );
    addDPStaticEntry( menu, qtr( "&Stream..." ),
        ":/menu/stream.svg", SLOT( openAndStreamingDialogs() ), "Ctrl+S" );
    menu->addSeparator();
#endif

    action = addMIMStaticEntry( p_intf, menu, qtr( "Quit at the end of playlist" ), "",
                               SLOT( activatePlayQuit( bool ) ) );
    action->setCheckable( true );
    action->setChecked( THEMIM->getPlayExitState() );

    if( mi && mi->getSysTray() )
    {
        action = menu->addAction( qtr( "Close to systray"), mi,
                                 SLOT( toggleUpdateSystrayMenu() ) );
    }

    addDPStaticEntry( menu, qtr( "&Quit" ) ,
        ":/menu/exit.svg", SLOT( quit() ), "Ctrl+Q" );
    return menu;
}

/**
 * Tools, like Media Information, Preferences or Messages
 **/
QMenu *VLCMenuBar::ToolsMenu( intf_thread_t *p_intf, QMenu *menu )
{
    addDPStaticEntry( menu, qtr( "&Effects and Filters"), ":/menu/settings.svg",
            SLOT( extendedDialog() ), "Ctrl+E" );

    addDPStaticEntry( menu, qtr( "&Track Synchronization"), ":/menu/setting.svgs",
            SLOT( synchroDialog() ), "" );

    addDPStaticEntry( menu, qtr( I_MENU_INFO ) , ":/menu/info.svg",
        SLOT( mediaInfoDialog() ), "Ctrl+I" );
    addDPStaticEntry( menu, qtr( I_MENU_CODECINFO ) ,
        ":/menu/info.svg", SLOT( mediaCodecDialog() ), "Ctrl+J" );

#ifdef ENABLE_VLM
    addDPStaticEntry( menu, qtr( I_MENU_VLM ), "", SLOT( vlmDialog() ),
        "Ctrl+Shift+W" );
#endif

    addDPStaticEntry( menu, qtr( "Program Guide" ), "", SLOT( epgDialog() ),
        "" );

    addDPStaticEntry( menu, qtr( I_MENU_MSG ),
        ":/menu/messages.svg", SLOT( messagesDialog() ), "Ctrl+M" );

    addDPStaticEntry( menu, qtr( "Plu&gins and extensions" ),
        "", SLOT( pluginDialog() ) );
    menu->addSeparator();

    if( !p_intf->p_sys->b_isDialogProvider )
        addDPStaticEntry( menu, qtr( "Customi&ze Interface..." ),
            ":/menu/preferences.svg", SLOT( toolbarDialog() ) );

    addDPStaticEntry( menu, qtr( "&Preferences" ),
        ":/menu/preferences.svg", SLOT( prefsDialog() ), "Ctrl+P", QAction::PreferencesRole );

    return menu;
}

/**
 * View Menu
 * Interface modification, load other interfaces, activate Extensions
 * \param current, set to NULL for menu creation, else for menu update
 **/
QMenu *VLCMenuBar::ViewMenu( intf_thread_t *p_intf, QMenu *current, MainInterface *_mi )
{
    QAction *action;
    QMenu *menu;

    MainInterface *mi = _mi ? _mi : p_intf->p_sys->p_mi;
    assert( mi );

    if( !current )
    {
        menu = new QMenu( qtr( "&View" ), mi );
    }
    else
    {
        menu = current;
        //menu->clear();
        //HACK menu->clear() does not delete submenus
        QList<QAction*> actions = menu->actions();
        foreach( QAction *a, actions )
        {
            QMenu *m = a->menu();
            if( a->parent() == menu ) delete a;
            else menu->removeAction( a );
            if( m && m->parent() == menu ) delete m;
        }
    }

    menu->addAction(
#ifndef __APPLE__
            QIcon( ":/menu/playlist_menu.svg" ),
#endif
            qtr( "Play&list" ), mi,
            SLOT( togglePlaylist() ), qtr( "Ctrl+L" ) );

    /* Docked Playlist */
    action = menu->addAction( qtr( "Docked Playlist" ) );
    action->setCheckable( true );
    action->setChecked( mi->isPlDocked() );
    CONNECT( action, triggered( bool ), mi, dockPlaylist( bool ) );

    if( mi->getPlaylistView() )
        menu->addMenu( StandardPLPanel::viewSelectionMenu( mi->getPlaylistView() ) );

    menu->addSeparator();

    action = menu->addAction( qtr( "Always on &top" ) );
    action->setCheckable( true );
    action->setChecked( mi->isInterfaceAlwaysOnTop() );
    CONNECT( action, triggered( bool ), mi, setInterfaceAlwaysOnTop( bool ) );

    menu->addSeparator();

    /* Minimal View */
    action = menu->addAction( qtr( "Mi&nimal Interface" ) );
    action->setShortcut( qtr( "Ctrl+H" ) );
    action->setCheckable( true );
    action->setChecked( (mi->getControlsVisibilityStatus()
                         & MainInterface::CONTROLS_HIDDEN ) );

    CONNECT( action, triggered( bool ), mi, toggleMinimalView( bool ) );
    CONNECT( mi, minimalViewToggled( bool ), action, setChecked( bool ) );

    /* FullScreen View */
    action = menu->addAction( qtr( "&Fullscreen Interface" ), mi,
            SLOT( toggleInterfaceFullScreen() ), QString( "F11" ) );
    action->setCheckable( true );
    action->setChecked( mi->isInterfaceFullScreen() );
    CONNECT( mi, fullscreenInterfaceToggled( bool ),
             action, setChecked( bool ) );

    /* Advanced Controls */
    action = menu->addAction( qtr( "&Advanced Controls" ), mi,
            SLOT( toggleAdvancedButtons() ) );
    action->setCheckable( true );
    if( mi->getControlsVisibilityStatus() & MainInterface::CONTROLS_ADVANCED )
        action->setChecked( true );
    action->setEnabled(mi->isAdvancedWidgetAvailable());

    action = menu->addAction( qtr( "Status Bar" ) );
    action->setCheckable( true );
    action->setChecked( mi->statusBar()->isVisible() );
    CONNECT( action, triggered( bool ), mi, setStatusBarVisibility( bool) );
#if 0 /* For Visualisations. Not yet working */
    adv = menu->addAction( qtr( "Visualizations selector" ), mi,
                           SLOT( visual() ) );
    adv->setCheckable( true );
    if( visual_selector_enabled ) adv->setChecked( true );
#endif

    menu->addSeparator();

    InterfacesMenu( p_intf, menu );
    menu->addSeparator();

    /* Extensions */
    ExtensionsMenu( p_intf, menu );

    return menu;
}

/**
 * Interface Sub-Menu, to list extras interface and skins
 **/
QMenu *VLCMenuBar::InterfacesMenu( intf_thread_t *p_intf, QMenu *current )
{
    QVector<vlc_object_t *> objects;
    QVector<const char *> varnames;
    varnames.append( "intf-add" );
    objects.append( VLC_OBJECT(p_intf) );

    return Populate( p_intf, current, varnames, objects );
}

/**
 * Extensions menu: populate the current menu with extensions
 **/
void VLCMenuBar::ExtensionsMenu( intf_thread_t *p_intf, QMenu *extMenu )
{
    /* Get ExtensionsManager and load extensions if needed */
    ExtensionsManager *extMgr = ExtensionsManager::getInstance( p_intf );

    if( !var_InheritBool( p_intf, "qt-autoload-extensions")
        && !extMgr->isLoaded() )
    {
        return;
    }

    if( !extMgr->isLoaded() && !extMgr->cannotLoad() )
    {
        extMgr->loadExtensions();
    }

    /* Let the ExtensionsManager build itself the menu */
    extMenu->addSeparator();
    extMgr->menu( extMenu );
}

static inline void VolumeEntries( intf_thread_t *p_intf, QMenu *current )
{
    current->addSeparator();

    QAction *action = current->addAction( QIcon( ":/toolbar/volume-high.svg" ), qtr( "&Increase Volume" ),
                ActionsManager::getInstance( p_intf ), SLOT( AudioUp() ) );
    action->setData( VLCMenuBar::ACTION_STATIC );
    action = current->addAction( QIcon( ":/toolbar/volume-low.svg" ), qtr( "D&ecrease Volume" ),
                ActionsManager::getInstance( p_intf ), SLOT( AudioDown() ) );
    action->setData( VLCMenuBar::ACTION_STATIC );
    action = current->addAction( QIcon( ":/toolbar/volume-muted.svg" ), qtr( "&Mute" ),
                ActionsManager::getInstance( p_intf ), SLOT( toggleMuteAudio() ) );
    action->setData( VLCMenuBar::ACTION_STATIC );
}

/**
 * Main Audio Menu
 **/
QMenu *VLCMenuBar::AudioMenu( intf_thread_t *p_intf, QMenu * current )
{
    QVector<vlc_object_t *> objects;
    QVector<const char *> varnames;
    audio_output_t *p_aout;
    input_thread_t *p_input;

    if (!audioDeviceMenu)
        audioDeviceMenu = new QMenu( qtr( "Audio &Device" ) );

    if( current->isEmpty() )
    {
        addActionWithSubmenu( current, "audio-es", qtr( "Audio &Track" ) );
        current->addMenu( audioDeviceMenu );
        addActionWithSubmenu( current, "stereo-mode", qtr( "&Stereo Mode" ) );
        current->addSeparator();

        addActionWithSubmenu( current, "visual", qtr( "&Visualizations" ) );
        VolumeEntries( p_intf, current );
    }

    p_input = THEMIM->getInput();
    p_aout = THEMIM->getAout();
    EnableStaticEntries( current, ( p_aout != NULL ) );
    AudioAutoMenuBuilder( p_input, objects, varnames );
    updateAudioDevice( p_intf, p_aout, audioDeviceMenu );
    if( p_aout )
    {
        vlc_object_release( p_aout );
    }

    return Populate( p_intf, current, varnames, objects );
}

/* Subtitles */
QMenu *VLCMenuBar::SubtitleMenu( intf_thread_t *p_intf, QMenu *current, bool b_popup )
{
    input_thread_t *p_input;
    QVector<vlc_object_t *> objects;
    QVector<const char *> varnames;

    if( current->isEmpty() || b_popup )
    {
        addDPStaticEntry( current, qtr( "Add &Subtitle File..." ), "",
                SLOT( loadSubtitlesFile() ) );
        addActionWithSubmenu( current, "spu-es", qtr( "Sub &Track" ) );
        current->addSeparator();
    }

    p_input = THEMIM->getInput();
    SubsAutoMenuBuilder( p_input, objects, varnames );

    return Populate( p_intf, current, varnames, objects );
}

/**
 * Main Video Menu
 * Subtitles are part of Video.
 **/
QMenu *VLCMenuBar::VideoMenu( intf_thread_t *p_intf, QMenu *current )
{
    input_thread_t *p_input;
    QVector<vlc_object_t *> objects;
    QVector<const char *> varnames;

    if( current->isEmpty() )
    {
        addActionWithSubmenu( current, "video-es", qtr( "Video &Track" ) );

        current->addSeparator();
        /* Surface modifiers */
        addActionWithCheckbox( current, "fullscreen", qtr( "&Fullscreen" ) );
        addActionWithCheckbox( current, "autoscale", qtr( "Always Fit &Window" ) );
        addActionWithCheckbox( current, "video-wallpaper", qtr( "Set as Wall&paper" ) );

        current->addSeparator();
        /* Size modifiers */
        addActionWithSubmenu( current, "zoom", qtr( "&Zoom" ) );
        addActionWithSubmenu( current, "aspect-ratio", qtr( "&Aspect Ratio" ) );
        addActionWithSubmenu( current, "crop", qtr( "&Crop" ) );

        current->addSeparator();
        /* Rendering modifiers */
        addActionWithSubmenu( current, "deinterlace", qtr( "&Deinterlace" ) );
        addActionWithSubmenu( current, "deinterlace-mode", qtr( "&Deinterlace mode" ) );

        current->addSeparator();
        /* Other actions */
        addAction( current, "video-snapshot", qtr( "Take &Snapshot" ) );
    }

    p_input = THEMIM->getInput();

    VideoAutoMenuBuilder( THEPL, p_input, objects, varnames );

    return Populate( p_intf, current, varnames, objects );
}

/**
 * Navigation Menu
 * For DVD, MP4, MOV and other chapter based format
 **/
QMenu *VLCMenuBar::NavigMenu( intf_thread_t *p_intf, QMenu *menu )
{
    QAction *action;
    QMenu *submenu;

    addActionWithSubmenu( menu, "title", qtr( "T&itle" ) );
    submenu = addActionWithSubmenu( menu, "chapter", qtr( "&Chapter" ) );
    submenu->setTearOffEnabled( true );
    addActionWithSubmenu( menu, "program", qtr( "&Program" ) );

    submenu = new QMenu( qtr( I_MENU_BOOKMARK ), menu );
    submenu->setTearOffEnabled( true );
    addDPStaticEntry( submenu, qtr( "&Manage" ), "",
                      SLOT( bookmarksDialog() ), "Ctrl+B" );
    submenu->addSeparator();
    action = menu->addMenu( submenu );
    action->setData( "bookmark" );

    menu->addSeparator();

    if ( !rendererMenu )
        rendererMenu = RendererMenu( p_intf );

    menu->addMenu( rendererMenu );
    menu->addSeparator();


    PopupMenuControlEntries( menu, p_intf );

    EnableStaticEntries( menu, ( THEMIM->getInput() != NULL ) );
    return RebuildNavigMenu( p_intf, menu, true );
}

QMenu *VLCMenuBar::RebuildNavigMenu( intf_thread_t *p_intf, QMenu *menu, bool b_keep )
{
    /* */
    input_thread_t *p_object;
    QVector<vlc_object_t *> objects;
    QVector<const char *> varnames;

    /* Get the input and hold it */
    p_object = THEMIM->getInput();

    InputAutoMenuBuilder( p_object, objects, varnames );

    /* Title and so on */
    PUSH_VAR( "prev-title" );
    PUSH_VAR( "next-title" );
    PUSH_VAR( "prev-chapter" );
    PUSH_VAR( "next-chapter" );

    /* */
    EnableStaticEntries( menu, (p_object != NULL ) );
    Populate( p_intf, menu, varnames, objects );

    /* Remove playback actions to recreate them */
    if( !b_keep )
    {
        QList< QAction* > actions = menu->actions();
        for( int i = 0; i < actions.count(); i++ )
            if( actions[i]->data().toInt() & ACTION_DELETE_ON_REBUILD )
                delete actions[i];
    }

    PopupMenuPlaylistEntries( menu, p_intf, p_object );

    return menu;
}

/**
 * Help/About Menu
**/
QMenu *VLCMenuBar::HelpMenu( QWidget *parent )
{
    QMenu *menu = new QMenu( parent );
    addDPStaticEntry( menu, qtr( "&Help" ) ,
        ":/menu/help.svg", SLOT( helpDialog() ), "F1" );
#ifdef UPDATE_CHECK
    addDPStaticEntry( menu, qtr( "Check for &Updates..." ) , "",
                      SLOT( updateDialog() ) );
#endif
    menu->addSeparator();
    addDPStaticEntry( menu, qtr( I_MENU_ABOUT ), ":/menu/info.svg",
            SLOT( aboutDialog() ), "Shift+F1", QAction::AboutRole );
    return menu;
}

/*****************************************************************************
 * Popup menus - Right Click menus                                           *
 *****************************************************************************/
#define POPUP_BOILERPLATE \
    QMenu* menu;  \
    QVector<vlc_object_t *> objects; \
    QVector<const char *> varnames; \
    input_thread_t *p_input = THEMIM->getInput();

#define CREATE_POPUP \
    menu = new QMenu(); \
    Populate( p_intf, menu, varnames, objects ); \
    if( show ) \
        menu->popup( QCursor::pos() ); \

void VLCMenuBar::PopupMenuPlaylistEntries( QMenu *menu,
                                        intf_thread_t *p_intf,
                                        input_thread_t *p_input )
{
    QAction *action;

    /* Play or Pause action and icon */
    if( !p_input || var_GetInteger( p_input, "state" ) != PLAYING_S )
    {
        action = menu->addAction( qtr( "&Play" ),
                ActionsManager::getInstance( p_intf ), SLOT( play() ) );
#ifndef __APPLE__ /* No icons in menus in Mac */
        action->setIcon( QIcon( ":/toolbar/play_b.svg" ) );
#endif
    }
    else
    {
        action = addMIMStaticEntry( p_intf, menu, qtr( "Pause" ),
                ":/toolbar/pause_b.svg", SLOT( togglePlayPause() ) );
    }
    action->setData( ACTION_DELETE_ON_REBUILD );

    /* Stop */
    action = addMIMStaticEntry( p_intf, menu, qtr( "&Stop" ),
            ":/toolbar/stop_b.svg", SLOT( stop() ), true );
    if( !p_input )
        action->setEnabled( false );
    action->setData( ACTION_DELETE_ON_REBUILD );

    /* Next / Previous */
    bool bPlaylistEmpty = THEMIM->hasEmptyPlaylist();
    action = addMIMStaticEntry( p_intf, menu, qtr( "Pre&vious" ),
            ":/toolbar/previous_b.svg", SLOT( prev() ), true );
    action->setEnabled( !bPlaylistEmpty );
    action->setData( ACTION_NO_CLEANUP + ACTION_DELETE_ON_REBUILD );
    CONNECT( THEMIM, playlistNotEmpty(bool), action, setEnabled(bool) );

    action = addMIMStaticEntry( p_intf, menu, qtr( "Ne&xt" ),
            ":/toolbar/next_b.svg", SLOT( next() ), true );
    action->setEnabled( !bPlaylistEmpty );
    action->setData( ACTION_NO_CLEANUP + ACTION_DELETE_ON_REBUILD );
    CONNECT( THEMIM, playlistNotEmpty(bool), action, setEnabled(bool) );

    action = menu->addAction( qtr( "Record" ), THEAM, SLOT( record() ) );
    action->setIcon( QIcon( ":/toolbar/record.svg" ) );
    if( !p_input )
        action->setEnabled( false );
    action->setData( ACTION_NO_CLEANUP + ACTION_DELETE_ON_REBUILD );
    menu->addSeparator();
}

void VLCMenuBar::PopupMenuControlEntries( QMenu *menu, intf_thread_t *p_intf,
                                        bool b_normal )
{
    QAction *action;
    QMenu *rateMenu = new QMenu( qtr( "Sp&eed" ), menu );
    rateMenu->setTearOffEnabled( true );

    if( b_normal )
    {
        /* Faster/Slower */
        action = rateMenu->addAction( qtr( "&Faster" ), THEMIM->getIM(),
                                  SLOT( faster() ) );
#ifndef __APPLE__ /* No icons in menus in Mac */
        action->setIcon( QIcon( ":/toolbar/faster2.svg") );
#endif
        action->setData( ACTION_STATIC );
    }

    action = rateMenu->addAction( QIcon( ":/toolbar/faster2.svg" ), qtr( "Faster (fine)" ), THEMIM->getIM(),
                              SLOT( littlefaster() ) );
    action->setData( ACTION_STATIC );

    action = rateMenu->addAction( qtr( "N&ormal Speed" ), THEMIM->getIM(),
                              SLOT( normalRate() ) );
    action->setData( ACTION_STATIC );

    action = rateMenu->addAction( QIcon( ":/toolbar/slower2.svg" ), qtr( "Slower (fine)" ), THEMIM->getIM(),
                              SLOT( littleslower() ) );
    action->setData( ACTION_STATIC );

    if( b_normal )
    {
        action = rateMenu->addAction( qtr( "Slo&wer" ), THEMIM->getIM(),
                                  SLOT( slower() ) );
#ifndef __APPLE__ /* No icons in menus in Mac */
        action->setIcon( QIcon( ":/toolbar/slower2.svg") );
#endif
        action->setData( ACTION_STATIC );
    }

    action = menu->addMenu( rateMenu );
    action->setData( ACTION_STATIC );

    menu->addSeparator();

    if( !b_normal ) return;

    action = menu->addAction( qtr( "&Jump Forward" ), THEMIM->getIM(),
             SLOT( jumpFwd() ) );
#ifndef __APPLE__ /* No icons in menus in Mac */
    action->setIcon( QIcon( ":/toolbar/skip_fw.svg") );
#endif
    action->setData( ACTION_STATIC );

    action = menu->addAction( qtr( "Jump Bac&kward" ), THEMIM->getIM(),
             SLOT( jumpBwd() ) );
#ifndef __APPLE__ /* No icons in menus in Mac */
    action->setIcon( QIcon( ":/toolbar/skip_back.svg") );
#endif
    action->setData( ACTION_STATIC );

    action = menu->addAction( qtr( I_MENU_GOTOTIME ), THEDP, SLOT( gotoTimeDialog() ), qtr( "Ctrl+T" ) );
    action->setData( ACTION_ALWAYS_ENABLED );

    menu->addSeparator();
}

void VLCMenuBar::PopupMenuStaticEntries( QMenu *menu )
{
    QMenu *openmenu = new QMenu( qtr( "Open Media" ), menu );
    addDPStaticEntry( openmenu, qtr( I_OP_OPF ),
        ":/type/file-asym.svg", SLOT( openFileDialog() ) );
    addDPStaticEntry( openmenu, qtr( I_OP_OPDIR ),
        ":/type/folder-grey.svg", SLOT( PLOpenDir() ) );
    addDPStaticEntry( openmenu, qtr( "Open &Disc..." ),
        ":/type/disc.svg", SLOT( openDiscDialog() ) );
    addDPStaticEntry( openmenu, qtr( "Open &Network..." ),
        ":/type/network.svg", SLOT( openNetDialog() ) );
    addDPStaticEntry( openmenu, qtr( "Open &Capture Device..." ),
        ":/type/capture-card.svg", SLOT( openCaptureDialog() ) );
    menu->addMenu( openmenu );

    menu->addSeparator();
#if 0
    QMenu *helpmenu = HelpMenu( menu );
    helpmenu->setTitle( qtr( "Help" ) );
    menu->addMenu( helpmenu );
#endif

    addDPStaticEntry( menu, qtr( "Quit" ), ":/menu/exit.svg",
                      SLOT( quit() ), "Ctrl+Q", QAction::QuitRole );
}

/* Video Tracks and Subtitles tracks */
QMenu* VLCMenuBar::VideoPopupMenu( intf_thread_t *p_intf, bool show )
{
    POPUP_BOILERPLATE
    if( p_input )
        VideoAutoMenuBuilder( THEPL, p_input, objects, varnames );
    CREATE_POPUP
    return menu;
}

/* Audio Tracks */
QMenu* VLCMenuBar::AudioPopupMenu( intf_thread_t *p_intf, bool show )
{
    POPUP_BOILERPLATE
    if( p_input )
        AudioAutoMenuBuilder( p_input, objects, varnames );
    CREATE_POPUP
    return menu;
}

/* Navigation stuff, and general menus ( open ), used only for skins */
QMenu* VLCMenuBar::MiscPopupMenu( intf_thread_t *p_intf, bool show )
{
    POPUP_BOILERPLATE

    menu = new QMenu();
    if( p_input )
    {
        InputAutoMenuBuilder( p_input, objects, varnames );
        menu->addSeparator();
    }

    Populate( p_intf, menu, varnames, objects );

    menu->addSeparator();
    PopupMenuPlaylistEntries( menu, p_intf, p_input );

    menu->addSeparator();
    PopupMenuControlEntries( menu, p_intf );

    menu->addSeparator();
    PopupMenuStaticEntries( menu );

    if( show )
        menu->popup( QCursor::pos() );
    return menu;
}

/* Main Menu that sticks everything together  */
QMenu* VLCMenuBar::PopupMenu( intf_thread_t *p_intf, bool show )
{
    POPUP_BOILERPLATE

    /* */
    menu = new QMenu();
    QAction *action;
    bool b_isFullscreen = false;
    MainInterface *mi = p_intf->p_sys->p_mi;

    PopupMenuPlaylistEntries( menu, p_intf, p_input );
    menu->addSeparator();

    if( p_input )
    {
        QMenu *submenu;
        vout_thread_t *p_vout = THEMIM->getVout();

        /* Add a fullscreen switch button, since it is the most used function */
        if( p_vout )
        {
            vlc_value_t val; var_Get( p_vout, "fullscreen", &val );

            b_isFullscreen = !( !val.b_bool );
            if( b_isFullscreen )
            {
                val.b_bool = false;
                CreateAndConnect( menu, "fullscreen",
                        qtr( "Leave Fullscreen" ),"" , ITEM_NORMAL,
                        VLC_OBJECT(THEPL), val, VLC_VAR_BOOL, b_isFullscreen );
            }
            vlc_object_release( p_vout );

            menu->addSeparator();
        }

        /* Input menu */
        InputAutoMenuBuilder( p_input, objects, varnames );

        /* Audio menu */
        submenu = new QMenu( menu );
        action = menu->addMenu( AudioMenu( p_intf, submenu ) );
        action->setText( qtr( "&Audio" ) );
        if( action->menu()->isEmpty() )
            action->setEnabled( false );

        /* Video menu */
        submenu = new QMenu( menu );
        action = menu->addMenu( VideoMenu( p_intf, submenu ) );
        action->setText( qtr( "&Video" ) );
        if( action->menu()->isEmpty() )
            action->setEnabled( false );

        /* Subtitles menu */
        submenu = new QMenu( menu );
        action = menu->addMenu( SubtitleMenu( p_intf, submenu, true ) );
        action->setText( qtr( "Subti&tle") );
        UpdateItem( p_intf, submenu, "spu-es", VLC_OBJECT(p_input), true );

        /* Playback menu for chapters */
        submenu = new QMenu( menu );
        action = menu->addMenu( NavigMenu( p_intf, submenu ) );
        action->setText( qtr( "&Playback" ) );
        if( action->menu()->isEmpty() )
            action->setEnabled( false );
    }

    menu->addSeparator();

    /* Add some special entries for windowed mode: Interface Menu */
    if( !b_isFullscreen )
    {
        QMenu *submenu = new QMenu( qtr( "Tool&s" ), menu );
        /*QMenu *tools =*/ ToolsMenu( p_intf, submenu );
        submenu->addSeparator();

        if( mi )
        {
            QMenu *bar = menu; // Needed for next macro
            BAR_DADD( ViewMenu( p_intf, NULL, mi ), qtr( "V&iew" ), 4 );
        }

        /* In skins interface, append some items */
        if( p_intf->p_sys->b_isDialogProvider )
        {
            vlc_object_t* p_object = p_intf->obj.parent;
            submenu->setTitle( qtr( "Interface" ) );

            /* Open skin dialog box */
            objects.clear(); varnames.clear();
            objects.append( p_object );
            varnames.append( "intf-skins-interactive" );
            Populate( p_intf, submenu, varnames, objects );
            QAction* action = submenu->actions().back();
            action->setShortcut( QKeySequence( "Ctrl+Shift+S" ));

            /* list of skins available */
            objects.clear(); varnames.clear();
            objects.append( p_object );
            varnames.append( "intf-skins" );
            Populate( p_intf, submenu, varnames, objects );

            submenu->addSeparator();

            /* list of extensions */
            ExtensionsMenu( p_intf, submenu );
        }

        menu->addMenu( submenu );
    }

    /* */
    QMenuView *plMenu = new QMenuView( menu, 25 );
    plMenu->setTitle( qtr("Playlist") );
    PLModel *model = PLModel::getPLModel( p_intf );
    plMenu->setModel( model );
    CONNECT( plMenu, activated(const QModelIndex&),
             model, activateItem(const QModelIndex&));
    menu->addMenu( plMenu );

    /* Static entries for ending, like open */
    if( p_intf->p_sys->b_isDialogProvider )
    {
        QMenu *openmenu = FileMenu( p_intf, menu );
        openmenu->setTitle( qtr( "Open Media" ) );
        menu->addMenu( openmenu );

        menu->addSeparator();

        QMenu *helpmenu = HelpMenu( menu );
        helpmenu->setTitle( qtr( "Help" ) );
        menu->addMenu( helpmenu );

        addDPStaticEntry( menu, qtr( "Quit" ), ":/menu/exit.svg",
                          SLOT( quit() ), "Ctrl+Q", QAction::QuitRole );
    }
    else
        PopupMenuStaticEntries( menu );

    if( show )
        menu->popup( QCursor::pos() );
    return menu;
}

#undef CREATE_POPUP
#undef POPUP_BOILERPLATE
#undef BAR_DADD

/************************************************************************
 * Systray Menu                                                         *
 ************************************************************************/

void VLCMenuBar::updateSystrayMenu( MainInterface *mi,
                                  intf_thread_t *p_intf,
                                  bool b_force_visible )
{
    input_thread_t *p_input = THEMIM->getInput();

    /* Get the systray menu and clean it */
    QMenu *sysMenu = mi->getSysTrayMenu();
    sysMenu->clear();

#ifndef Q_OS_MAC
    /* Hide / Show VLC and cone */
    if( mi->isVisible() || b_force_visible )
    {
        sysMenu->addAction( QIcon( ":/logo/vlc16.png" ),
                            qtr( "&Hide VLC media player in taskbar" ), mi,
                            SLOT( hideUpdateSystrayMenu() ) );
    }
    else
    {
        sysMenu->addAction( QIcon( ":/logo/vlc16.png" ),
                            qtr( "Sho&w VLC media player" ), mi,
                            SLOT( showUpdateSystrayMenu() ) );
    }
    sysMenu->addSeparator();
#endif

    PopupMenuPlaylistEntries( sysMenu, p_intf, p_input );
    PopupMenuControlEntries( sysMenu, p_intf, false );

    VolumeEntries( p_intf, sysMenu );
    sysMenu->addSeparator();
    addDPStaticEntry( sysMenu, qtr( "&Open Media" ),
            ":/type/file-wide.svg", SLOT( openFileDialog() ) );
    addDPStaticEntry( sysMenu, qtr( "&Quit" ) ,
            ":/menu/exit.svg", SLOT( quit() ) );

    /* Set the menu */
    mi->getSysTray()->setContextMenu( sysMenu );
}


#undef PUSH_VAR

/*************************************************************************
 * Builders for automenus
 *************************************************************************/
QMenu * VLCMenuBar::Populate( intf_thread_t *p_intf,
                            QMenu *current,
                            QVector< const char *> & varnames,
                            QVector<vlc_object_t *> & objects )
{
    QMenu *menu = current;
    assert( menu );

    currentGroup = NULL;

    for( int i = 0; i < (int)objects.count() ; i++ )
    {
        if( !varnames[i] || !*varnames[i] )
        {
            menu->addSeparator();
            continue;
        }

        UpdateItem( p_intf, menu, varnames[i], objects[i], true );
    }
    return menu;
}

/*****************************************************************************
 * Private methods.
 *****************************************************************************/

static bool IsMenuEmpty( const char *psz_var, vlc_object_t *p_object )
{
    /* Check if we want to display the variable */
    if( !(var_Type( p_object, psz_var) & VLC_VAR_HASCHOICE) )
        return false;

    vlc_value_t val;
    var_Change( p_object, psz_var, VLC_VAR_CHOICESCOUNT, &val, NULL );
    return val.i_int == 0 || val.i_int == 1;
}

#define TEXT_OR_VAR qfue ( text.psz_string ? text.psz_string : psz_var )

void VLCMenuBar::UpdateItem( intf_thread_t *p_intf, QMenu *menu,
        const char *psz_var, vlc_object_t *p_object, bool b_submenu )
{
    vlc_value_t val, text;
    int i_type;

    QAction *action = FindActionWithVar( menu, psz_var );
    if( action )
        DeleteNonStaticEntries( action->menu() );

    if( !p_object )
    {
        if( action )
            action->setEnabled( false );
        return;
    }

    /* Check the type of the object variable */
    /* This HACK is needed so that we have:
     *  - a radio button for audio/video tracks instread of a checkbox, and;
     *  - an always enabled bookmark menu (even if there are no bookmarks)
     **/
    if( !strcmp( psz_var, "audio-es" )
     || !strcmp( psz_var, "video-es" )
     || !strcmp( psz_var, "bookmark" ) )
        i_type = VLC_VAR_INTEGER | VLC_VAR_HASCHOICE;
    else
        i_type = var_Type( p_object, psz_var );

    switch( i_type & VLC_VAR_TYPE )
    {
        case VLC_VAR_VOID:
        case VLC_VAR_BOOL:
        case VLC_VAR_STRING:
        case VLC_VAR_INTEGER:
        case VLC_VAR_FLOAT:
            break;
        default:
            /* Variable doesn't exist or isn't handled */
            if( action )
                action->setEnabled( false );
            return;
    }

    /* Make sure we want to display the variable */
    if( menu->isEmpty() && IsMenuEmpty( psz_var, p_object ) )
    {
        if( action )
            action->setEnabled( false );
        return;
    }

    /* Get the descriptive name of the variable */
    int i_ret = var_Change( p_object, psz_var, VLC_VAR_GETTEXT, &text, NULL );
    if( i_ret != VLC_SUCCESS )
    {
        text.psz_string = NULL;
    }

    if( !action )
    {
        action = new QAction( TEXT_OR_VAR, menu );
        menu->addAction( action );
        action->setData( psz_var );
    }

    if( i_type & VLC_VAR_HASCHOICE )
    {
        /* Append choices menu */
        if( b_submenu )
        {
            QMenu *submenu;
            submenu = action->menu();
            if( !submenu )
            {
                submenu = new QMenu( menu );
                action->setMenu( submenu );
            }

            action->setEnabled(
                CreateChoicesMenu( submenu, psz_var, p_object ) == 0 );
        }
        else
        {
            action->setEnabled(
                CreateChoicesMenu( menu, psz_var, p_object ) == 0 );
        }
        FREENULL( text.psz_string );
        return;
    }
    else
        action->setEnabled( false );

    switch( i_type & VLC_VAR_TYPE )
    {
        case VLC_VAR_VOID:
            val.i_int = 0;  // Prevent the copy of an uninitialized value
            CreateAndConnect( menu, psz_var, TEXT_OR_VAR, "", ITEM_NORMAL,
                    p_object, val, i_type );
            break;

        case VLC_VAR_BOOL:
            var_Get( p_object, psz_var, &val );
            val.b_bool = !val.b_bool;
            CreateAndConnect( menu, psz_var, TEXT_OR_VAR, "", ITEM_CHECK,
                    p_object, val, i_type, !val.b_bool );
            break;
    }
    FREENULL( text.psz_string );
}

#undef TEXT_OR_VAR

/** HACK for the navigation submenu:
 * "title %2u" variables take the value 0 if not set
 */
static bool CheckTitle( vlc_object_t *p_object, const char *psz_var )
{
    unsigned i_title = 0;
    if( sscanf( psz_var, "title %2u", &i_title ) <= 0 )
        return true;

    unsigned i_current_title = var_GetInteger( p_object, "title" );
    return ( i_title == i_current_title );
}


int VLCMenuBar::CreateChoicesMenu( QMenu *submenu, const char *psz_var,
                                   vlc_object_t *p_object )
{
    vlc_value_t val, val_list, text_list;
    int i_type, i;

    /* Check the type of the object variable */
    i_type = var_Type( p_object, psz_var );

    /* Make sure we want to display the variable */
    if( submenu->isEmpty() && IsMenuEmpty( psz_var, p_object ) )
        return VLC_EGENERIC;

    switch( i_type & VLC_VAR_TYPE )
    {
        case VLC_VAR_VOID:
        case VLC_VAR_BOOL:
        case VLC_VAR_STRING:
        case VLC_VAR_INTEGER:
        case VLC_VAR_FLOAT:
            break;
        default:
            /* Variable doesn't exist or isn't handled */
            return VLC_EGENERIC;
    }

    if( var_Change( p_object, psz_var, VLC_VAR_GETCHOICES,
                    &val_list, &text_list ) < 0 )
    {
        return VLC_EGENERIC;
    }

#define CURVAL val_list.p_list->p_values[i]
#define CURTEXT text_list.p_list->p_values[i].psz_string
#define RADIO_OR_COMMAND  ( i_type & ( VLC_VAR_ISCOMMAND | VLC_VAR_HASCHOICE ) ) ? ITEM_RADIO : ITEM_NORMAL

    for( i = 0; i < val_list.p_list->i_count; i++ )
    {
        vlc_value_t another_val;
        QString menutext;

        switch( i_type & VLC_VAR_TYPE )
        {
            case VLC_VAR_STRING:
                var_Get( p_object, psz_var, &val );
                another_val.psz_string = strdup( CURVAL.psz_string );
                menutext = qfue( CURTEXT ? CURTEXT : another_val.psz_string );
                CreateAndConnect( submenu, psz_var, menutext, "", RADIO_OR_COMMAND,
                        p_object, another_val, i_type,
                        val.psz_string && !strcmp( val.psz_string, CURVAL.psz_string ) );

                free( val.psz_string );
                break;

            case VLC_VAR_INTEGER:
                var_Get( p_object, psz_var, &val );
                if( CURTEXT ) menutext = qfue( CURTEXT );
                else menutext = QString::number( CURVAL.i_int );
                CreateAndConnect( submenu, psz_var, menutext, "", RADIO_OR_COMMAND,
                        p_object, CURVAL, i_type,
                        ( CURVAL.i_int == val.i_int )
                        && CheckTitle( p_object, psz_var ) );
                break;

            case VLC_VAR_FLOAT:
                var_Get( p_object, psz_var, &val );
                if( CURTEXT ) menutext = qfue( CURTEXT );
                else menutext.sprintf( "%.2f", CURVAL.f_float );
                CreateAndConnect( submenu, psz_var, menutext, "", RADIO_OR_COMMAND,
                        p_object, CURVAL, i_type,
                        CURVAL.f_float == val.f_float );
                break;

            default:
                break;
        }
    }
    currentGroup = NULL;

    /* clean up everything */
    var_FreeList( &val_list, &text_list );

#undef RADIO_OR_COMMAND
#undef CURVAL
#undef CURTEXT

    return submenu->isEmpty() ? VLC_EGENERIC : VLC_SUCCESS;
}

void VLCMenuBar::CreateAndConnect( QMenu *menu, const char *psz_var,
        const QString& text, const QString& help,
        int i_item_type, vlc_object_t *p_obj,
        vlc_value_t val, int i_val_type,
        bool checked )
{
    QAction *action = FindActionWithVar( menu, psz_var );

    bool b_new = false;
    if( !action )
    {
        action = new QAction( text, menu );
        menu->addAction( action );
        b_new = true;
    }

    action->setToolTip( help );
    action->setEnabled( p_obj != NULL );

    if( i_item_type == ITEM_CHECK )
    {
        action->setCheckable( true );
    }
    else if( i_item_type == ITEM_RADIO )
    {
        action->setCheckable( true );
        if( !currentGroup )
            currentGroup = new QActionGroup( menu );
        currentGroup->addAction( action );
    }

    action->setChecked( checked );

    MenuItemData *itemData = action->findChild<MenuItemData*>( QString() );
    delete itemData;
    itemData = new MenuItemData( action, p_obj, i_val_type, val, psz_var );

    /* remove previous signal-slot connection(s) if any */
    action->disconnect( );

    CONNECT( action, triggered(), THEDP->menusMapper, map() );
    THEDP->menusMapper->setMapping( action, itemData );

    if( b_new )
        menu->addAction( action );
}

void VLCMenuBar::DoAction( QObject *data )
{
    MenuItemData *itemData = qobject_cast<MenuItemData *>( data );
    vlc_object_t *p_object = itemData->p_obj;
    if( p_object == NULL ) return;
    const char *var = itemData->psz_var;
    vlc_value_t val = itemData->val;

    if ((var_Type( p_object, var) & VLC_VAR_CLASS) == VLC_VAR_VOID)
        var_TriggerCallback( p_object, var );
    else
        var_Set( p_object, var, val );

    if( !strcmp( var, "fullscreen" )
     || !strcmp( var, "video-on-top" )
     || !strcmp( var, "video-wallpaper" ) ) /* FIXME: reverse abstraction */
    {   /* Apply playlist variables to current existing vout too */
        input_thread_t *input = playlist_CurrentInput((playlist_t *)p_object);
        if( input != NULL )
        {
            vout_thread_t *vout = input_GetVout( input );
            vlc_object_release( input );
            if( vout != NULL )
            {
                var_Set( vout, var, val ); /* never void class */
                vlc_object_release( vout );
            }
        }
    }
}

void VLCMenuBar::updateAudioDevice( intf_thread_t * p_intf, audio_output_t *p_aout, QMenu *current )
{
    char **ids, **names;
    char *selected;

    if( !p_aout || !current )
        return;

    current->clear();
    int i_result = aout_DevicesList( p_aout, &ids, &names);
    if( i_result < 0 )
        return;

    selected = aout_DeviceGet( p_aout );

    QActionGroup *actionGroup = new QActionGroup(current);
    QAction *action;

    for( int i = 0; i < i_result; i++ )
    {
        action = new QAction( qfue( names[i] ), actionGroup );
        action->setData( ids[i] );
        action->setCheckable( true );
        if( (selected && !strcmp( ids[i], selected ) ) ||
            (selected == NULL && ids[i] && ids[i][0] == '\0' ) )
            action->setChecked( true );
        actionGroup->addAction( action );
        current->addAction( action );
        CONNECT(action, triggered(), THEMIM->menusAudioMapper, map());
        THEMIM->menusAudioMapper->setMapping(action, ids[i]);
        free( ids[i] );
        free( names[i] );
    }
    free( ids );
    free( names );
    free( selected );
}

void VLCMenuBar::updateRecents( intf_thread_t *p_intf )
{
    if( recentsMenu )
    {
        QAction* action;
        RecentsMRL* rmrl = RecentsMRL::getInstance( p_intf );
        QStringList l = rmrl->recentList();

        recentsMenu->clear();

        if( !l.count() )
        {
            recentsMenu->setEnabled( false );
        }
        else
        {
            for( int i = 0; i < __MIN( l.count(), 10) ; ++i )
            {
                QString mrl = l.at( i );
                char *psz = vlc_uri_decode_duplicate( qtu( mrl ) );
                QString text = qfu( psz );

                text.replace("&", "&&");
#ifdef _WIN32
# define FILE_SCHEME "file:///"
#else
# define FILE_SCHEME "file://"
#endif
                if ( text.startsWith( FILE_SCHEME ) )
                    text.remove( 0, strlen( FILE_SCHEME ) );
#undef FILE_SCHEME

                free( psz );
                action = recentsMenu->addAction(
                        QString( i < 9 ? "&%1: ": "%1: " ).arg( i + 1 ) +
                            QApplication::fontMetrics().elidedText( text,
                                                          Qt::ElideLeft, 400 ),
                        rmrl->signalMapper, SLOT( map() ),
                        i < 9 ? QString( "Ctrl+%1" ).arg( i + 1 ) : "" );
                rmrl->signalMapper->setMapping( action, l.at( i ) );
            }

            recentsMenu->addSeparator();
            recentsMenu->addAction( qtr("&Clear"), rmrl, SLOT( clear() ) );
            recentsMenu->setEnabled( true );
        }
    }
}

QMenu *VLCMenuBar::RendererMenu(intf_thread_t *p_intf, QMenu *menu )
{
    QMenu *submenu = new QMenu( qtr("&Renderer"), menu );

    rendererGroup = new QActionGroup(submenu);

    QAction *action = new QAction( qtr("<Local>"), submenu );
    action->setCheckable(true);
    submenu->addAction( action );
    rendererGroup->addAction(action);

    char *psz_renderer = var_InheritString( THEPL, "sout" );
    if ( psz_renderer == NULL )
        action->setChecked( true );
    else
        free( psz_renderer );

    submenu->addSeparator();

    action = new QAction( qtr("Scanning..."), submenu );
    action->setEnabled( false );
    submenu->addAction( action );

    CONNECT( submenu, aboutToShow(), ActionsManager::getInstance( p_intf ), StartRendererScan() );
    CONNECT( submenu, aboutToHide(), ActionsManager::getInstance( p_intf ), RendererMenuCountdown() );
    CONNECT( rendererGroup, triggered(QAction*), ActionsManager::getInstance( p_intf ), RendererSelected( QAction* ) );

    return submenu;
}
