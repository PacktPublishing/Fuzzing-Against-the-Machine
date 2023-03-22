/*****************************************************************************
 * input_manager.hpp : Manage an input and interact with its GUI elements
 ****************************************************************************
 * Copyright (C) 2006-2008 the VideoLAN team
 * $Id: 2e2116b82b46a31aaac15f74def24838be0b144d $
 *
 * Authors: Clément Stenac <zorglub@videolan.org>
 *          Jean-Baptiste <jb@videolan.org>
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

#ifndef QVLC_INPUT_MANAGER_H_
#define QVLC_INPUT_MANAGER_H_

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_input.h>

#include "qt.hpp"
#include "util/singleton.hpp"
#include "adapters/variables.hpp"

#include <QObject>
#include <QEvent>
class QSignalMapper;

enum { NORMAL,    /* loop: 0, repeat: 0 */
       REPEAT_ONE,/* loop: 0, repeat: 1 */
       REPEAT_ALL,/* loop: 1, repeat: 0 */
};

class IMEvent : public QEvent
{
public:
    enum event_types {
        PositionUpdate = QEvent::User + IMEventTypeOffset + 1,
        ItemChanged,
        ItemStateChanged,
        ItemTitleChanged,
        ItemRateChanged,
        ItemEsChanged,
        ItemTeletextChanged,
        InterfaceVoutUpdate,
        StatisticsUpdate,
        InterfaceAoutUpdate, /* 10 */
        MetaChanged,
        InfoChanged,
        SynchroChanged,
        CachingEvent,
        BookmarksChanged,
        RecordingEvent,
        ProgramChanged,
        RandomChanged,
        LoopOrRepeatChanged, /* 20 */
        EPGEvent,
    /*    SignalChanged, */

        FullscreenControlToggle = QEvent::User + IMEventTypeOffset + 50,
        FullscreenControlShow,
        FullscreenControlHide,
        FullscreenControlPlanHide,
    };

    IMEvent( event_types type, input_item_t *p_input = NULL )
        : QEvent( (QEvent::Type)(type) )
    {
        if( (p_item = p_input) != NULL )
            input_item_Hold( p_item );
    }

    virtual ~IMEvent()
    {
        if( p_item )
            input_item_Release( p_item );
    }

    input_item_t *item() const { return p_item; };

private:
    input_item_t *p_item;
};

class PLEvent : public QEvent
{
public:
    enum PLEventTypes
    {
        PLItemAppended = QEvent::User + PLEventTypeOffset + 1,
        PLItemRemoved,
        LeafToParent,
        PLEmpty
    };

    PLEvent( PLEventTypes t, int i, int p = 0 )
        : QEvent( (QEvent::Type)(t) ), i_item(i), i_parent(p) {}
    int getItemId() const { return i_item; };
    int getParentId() const { return i_parent; };

private:
    /* Needed for "playlist-item*" and "leaf-to-parent" callbacks
     * !! Can be a input_item_t->i_id or a playlist_item_t->i_id */
    int i_item;
    // Needed for "playlist-item-append" callback, notably
    int i_parent;
};

class MainInputManager;

class InputManager : public QObject
{
    Q_OBJECT
    friend class MainInputManager;

public:
    InputManager( MainInputManager *, intf_thread_t * );
    virtual ~InputManager();

    void delInput();
    bool hasInput() const { return p_input != NULL; }

    int playingStatus() const;
    bool hasAudio();
    bool hasVideo() { return hasInput() && b_video; }
    bool hasVisualisation();
    void requestArtUpdate( input_item_t *p_item, bool b_forced );
    void setArt( input_item_t *p_item, QString fileUrl );

    QString getName() { return oldName; }
    static const QString decodeArtURL( input_item_t *p_item );

private:
    intf_thread_t  *p_intf;
    MainInputManager* p_mim;
    input_thread_t *p_input;
    vlc_object_t   *p_input_vbi;
    input_item_t   *p_item;
    int             i_old_playing_status;
    QString         oldName;
    QString         lastURI;
    QString         artUrl;
    float           f_rate;
    float           f_cache;
    bool            b_video;
    mtime_t         timeA, timeB;

    void customEvent( QEvent * );

    void addCallbacks();
    void delCallbacks();

    void UpdateRate();
    void UpdateName();
    void UpdateStatus();
    void UpdateNavigation();
    void UpdatePosition();
    void UpdateTeletext();
    void UpdateArt();
    void UpdateInfo();
    void UpdateMeta();
    void UpdateMeta(input_item_t *);
    void UpdateVout();
    void UpdateAout();
    void UpdateStats();
    void UpdateCaching();
    void UpdateRecord();
    void UpdateProgramEvent();
    void UpdateEPG();

    void setInput( input_thread_t * );

public slots:
    void inputChangedHandler(); ///< Our controlled input changed
    void sliderUpdate( float ); ///< User dragged the slider. We get new pos
    /* SpeedRate Rate Management */
    void reverse();
    void slower();
    void faster();
    void littlefaster();
    void littleslower();
    void normalRate();
    void setRate( int );
    /* Jumping */
    void jumpFwd();
    void jumpBwd();
    /* Menus */
    void sectionNext();
    void sectionPrev();
    void sectionMenu();
    /* Program */
    void changeProgram( int );
    /* Teletext */
    void telexSetPage( int );          ///< Goto teletext page
    void telexSetTransparency( bool ); ///< Transparency on teletext background
    void activateTeletext( bool );     ///< Toggle buttons after click
    /* A to B Loop */
    void setAtoB();

private slots:
    void AtoBLoop( float, int64_t, int );

signals:
    /// Send new position, new time and new length
    void positionUpdated( float , int64_t, int );
    void remainingTimeChanged( bool );
    void seekRequested( float pos );
    void rateChanged( float );
    void nameChanged( const QString& );
    /// Used to signal whether we should show navigation buttons
    void titleChanged( bool );
    void chapterChanged( bool );
    void inputCanSeek( bool );
    /// You can resume playback
    void resumePlayback( int64_t );
    /// Statistics are updated
    void statisticsUpdated( input_item_t* );
    void infoChanged( input_item_t* );
    void currentMetaChanged( input_item_t* );
    void metaChanged( input_item_t *);
    void artChanged( QString ); /* current item art ( same as item == NULL ) */
    void artChanged( input_item_t * );
    /// Play/pause status
    void playingStatusChanged( int );
    void recordingStateChanged( bool );
    /// Teletext
    void teletextPossible( bool );
    void teletextActivated( bool );
    void teletextTransparencyActivated( bool );
    void newTelexPageSet( int );
    /// Advanced buttons
    void AtoBchanged( bool, bool );
    /// Vout
    void voutChanged( bool );
    void voutListChanged( vout_thread_t **pp_vout, int i_vout );
    /// Other
    void synchroChanged();
    void bookmarksChanged();
    void cachingChanged( float );
    /// Program Event changes
    void encryptionChanged( bool );
    void epgChanged();
};

class MainInputManager : public QObject, public Singleton<MainInputManager>
{
    Q_OBJECT
    friend class Singleton<MainInputManager>;
    friend class VLCMenuBar;

public:
    input_thread_t *getInput() { return p_input; }
    InputManager *getIM() { return im; }
    inline input_item_t *currentInputItem()
    {
        return ( p_input ? input_GetItem( p_input ) : NULL );
    }

    vout_thread_t* getVout();
    QVector<vout_thread_t*> getVouts() const;
    audio_output_t *getAout();

    bool getPlayExitState();
    bool hasEmptyPlaylist();

    void requestVoutUpdate() { return im->UpdateVout(); }
    // Probe for initial input. Doing this from the constructor would cause
    // the getInstance to call itself recursively from the inputChangedHandler
    void probeCurrentInput();

protected:
    QSignalMapper *menusAudioMapper;

private:
    MainInputManager( intf_thread_t * );
    virtual ~MainInputManager();

    void customEvent( QEvent * );

    InputManager            *im;
    input_thread_t          *p_input;
    intf_thread_t           *p_intf;
    QVLCBool random, repeat, loop;
    QVLCFloat volume;
    QVLCBool mute;

private:
    static int ItemChanged( vlc_object_t *, const char *,
                            vlc_value_t, vlc_value_t, void * );
    static int LeafToParent( vlc_object_t *, const char *,
                            vlc_value_t, vlc_value_t, void * );
    static int PLItemChanged( vlc_object_t *, const char *,
                            vlc_value_t, vlc_value_t, void * );
    static int PLItemAppended( vlc_object_t *, const char *,
                            vlc_value_t, vlc_value_t, void * );
    static int PLItemRemoved( vlc_object_t *, const char *,
                            vlc_value_t, vlc_value_t, void * );

public slots:
    void togglePlayPause();
    void play();
    void pause();
    void toggleRandom();
    void stop();
    void next();
    void prev();
    void prevOrReset();
    void activatePlayQuit( bool );

    void loopRepeatLoopStatus();
    void changeFullscreen( bool );

private slots:
    void notifyRandom( bool );
    void notifyRepeatLoop( bool );
    void notifyVolume( float );
    void notifyMute( bool );
    void menusUpdateAudio( const QString& );

signals:
    void inputChanged( bool );
    void volumeChanged( float );
    void soundMuteChanged( bool );
    void playlistItemAppended( int itemId, int parentId );
    void playlistItemRemoved( int itemId );
    void playlistNotEmpty( bool );
    void randomChanged( bool );
    void repeatLoopChanged( int );
    void leafBecameParent( int );
};

#endif
