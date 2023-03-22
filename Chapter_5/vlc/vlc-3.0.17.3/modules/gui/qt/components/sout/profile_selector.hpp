/*****************************************************************************
 * profile_selector.hpp : A small profile selector and editor
 ****************************************************************************
 * Copyright (C) 2009 the VideoLAN team
 * $Id: feffa235bdafecdea57ab20a2ded5828fb73f528 $
 *
 * Authors: Jean-Baptiste Kempf <jb@videolan.org>
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

#ifndef VLC_QT_PROFILE_SELECTOR_HPP_
#define VLC_QT_PROFILE_SELECTOR_HPP_

#include "qt.hpp"

#include <QWidget>
#include <QSet>
#include <QHash>

#include "util/qvlcframe.hpp"
#include "ui/profiles.h"

class QComboBox;

class VLCProfileSelector : public QWidget
{
    Q_OBJECT

public:
    VLCProfileSelector( QWidget *_parent );
    ~VLCProfileSelector();
    QString getMux() { return mux; }
    QString getTranscode() { return transcode; }
private:
    QComboBox *profileBox;
    void fillProfilesCombo();
    void editProfile( const QString&, const QString& );
    void saveProfiles();
    QString mux;
    QString transcode;
private slots:
    void newProfile();
    void editProfile();
    void deleteProfile();
    void updateOptions( int i );
    void updateOptionsOldFormat( int i );
signals:
    void optionsChanged();
};

class VLCProfileEditor : public QVLCDialog
{
    Q_OBJECT

    Ui::Profiles ui;
public:
    VLCProfileEditor( const QString&, const QString&, QWidget * );

    QString name;
    QString muxValue;
    QString transcodeValue();
    QStringList qpcodecsList;
private:
    void registerCodecs();
    void registerFilters();
    void fillProfile( const QString& qs );
    void fillProfileOldFormat( const QString& qs );
    typedef QSet<QString> resultset;
    QHash<QString, resultset> caps;
    void loadCapabilities();
    void reset();
protected slots:
    void close() Q_DECL_OVERRIDE;
private slots:
    void muxSelected();
    void codecSelected();
    void activatePanels();
    void fixBirateState();
    void fixQPState();
};

#endif
