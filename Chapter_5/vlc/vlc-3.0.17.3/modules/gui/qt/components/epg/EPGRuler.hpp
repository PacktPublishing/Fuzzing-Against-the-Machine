/*****************************************************************************
 * EPGRuler.hpp: EPGRuler
 ****************************************************************************
 * Copyright © 2009-2010 VideoLAN
 * $Id: 1a8021b6d04aab0f687761fe50889747c860d3f7 $
 *
 * Authors: Ludovic Fauvet <etix@l0cal.com>
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
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifndef EPGRULER_H
#define EPGRULER_H

#include "qt.hpp"

#include <QWidget>
#include <QDateTime>

class EPGRuler : public QWidget
{
    Q_OBJECT

public:
    EPGRuler( QWidget* parent = 0 );
    virtual ~EPGRuler() { }

public slots:
    void setScale( double scale );
    void setRange( const QDateTime&, const QDateTime& );
    void setOffset( int offset );

protected:
    void paintEvent( QPaintEvent *event ) Q_DECL_OVERRIDE;

private:
    qreal m_scale;
    int m_duration;
    int m_offset;
    QDateTime m_startTime;
};

#endif // EPGRULER_H
