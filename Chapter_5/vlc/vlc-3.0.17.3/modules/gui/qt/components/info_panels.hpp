/*****************************************************************************
 * info_panels.hpp : Panels for the information dialogs
 ****************************************************************************
 * Copyright (C) 2006-2007 the VideoLAN team
 * $Id: 1490425dd3b662bb9f303bd1ae760e8a4f54e5a4 $
 *
 * Authors: Clément Stenac <zorglub@videolan.org>
 *          Jean-Baptiste Kempf <jb@videolan.org>
 *          Ilkka Ollakka <ileoo@videolan.org>
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

#ifndef VLC_QT_INFO_PANELS_HPP_
#define VLC_QT_INFO_PANELS_HPP_

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "qt.hpp"

#include <vlc_common.h>
#include <QWidget>

class QTreeWidget;
class QTreeWidgetItem;
class QTableWidget;
class QLineEdit;
class CoverArtLabel;
class QTextEdit;
class QLabel;
class VLCStatsView;
class QPushButton;

class MetaPanel: public QWidget
{
    Q_OBJECT
public:
    MetaPanel( QWidget *, struct intf_thread_t * );
    void saveMeta();

    bool isInEditMode();
    void setEditMode( bool );

private:
    input_item_t *p_input;
    struct intf_thread_t *p_intf;
    bool b_inEditMode;

    QLineEdit *title_text;
    QLineEdit *artist_text;
    QLineEdit *genre_text;
    QLineEdit *copyright_text;
    QLineEdit *collection_text;
    QLineEdit *seqnum_text;
    QLineEdit *seqtot_text;

    QTextEdit *description_text;
    QLineEdit *date_text;
    QLineEdit *language_text;
    QLineEdit *nowplaying_text;
    QLineEdit *publisher_text;
    QLineEdit *encodedby_text;
    CoverArtLabel *art_cover;

    QLabel   *lblURL;
    QString  currentURL;

    QPushButton *fingerprintButton;

public slots:
    void update( input_item_t * );
    void clear();
    void fingerprint();
    void fingerprintUpdate( input_item_t * );

private slots:
    void enterEditMode();

signals:
    void uriSet( const QString& );
    void editing();
};

class ExtraMetaPanel: public QWidget
{
    Q_OBJECT
public:
    ExtraMetaPanel( QWidget * );
private:
    QTableWidget *extraMeta;
public slots:
    void update( input_item_t * );
    void clear();
};

class InputStatsPanel: public QWidget
{
    Q_OBJECT
public:
    InputStatsPanel( QWidget * );
protected:
    void hideEvent( QHideEvent * ) Q_DECL_OVERRIDE;
private:
    QTreeWidget *StatsTree;
    QTreeWidgetItem *input;
    QTreeWidgetItem *read_media_stat;
    QTreeWidgetItem *input_bitrate_stat;
    QTreeWidgetItem *input_bitrate_graph;
    QTreeWidgetItem *demuxed_stat;
    QTreeWidgetItem *stream_bitrate_stat;
    QTreeWidgetItem *corrupted_stat;
    QTreeWidgetItem *discontinuity_stat;

    QTreeWidgetItem *video;
    QTreeWidgetItem *vdecoded_stat;
    QTreeWidgetItem *vdisplayed_stat;
    QTreeWidgetItem *vlost_frames_stat;
    QTreeWidgetItem *vfps_stat;

    QTreeWidgetItem *audio;
    QTreeWidgetItem *adecoded_stat;
    QTreeWidgetItem *aplayed_stat;
    QTreeWidgetItem *alost_stat;

    VLCStatsView *statsView;
public slots:
    void update( input_item_t * );
    void clear();
};

class InfoPanel: public QWidget
{
    Q_OBJECT
public:
    InfoPanel( QWidget * );
private:
    QTreeWidget *InfoTree;
public slots:
    void update( input_item_t * );
    void clear();
};

#endif
