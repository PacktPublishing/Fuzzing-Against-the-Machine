/*****************************************************************************
 * seekstyle.hpp : Seek slider style
 ****************************************************************************
 * Copyright (C) 2011-2012 VLC authors and VideoLAN
 *
 * Authors: Ludovic Fauvet <etix@videolan.org>
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

#ifndef SEEKSTYLE_HPP
#define SEEKSTYLE_HPP

#include "qt.hpp"

#include <inttypes.h>
#include <QProxyStyle>
#include <QStyleOptionSlider>

class SeekStyle : public QProxyStyle
{
    Q_OBJECT
public:
    class SeekStyleOption : public QStyleOptionSlider
    {
    public:
        SeekStyleOption();
        float buffering;
        int length;
        bool animate;
        qreal animationopacity;
        qreal animationloading;
        QList<int64_t> points;
    };

public:
    SeekStyle();
    int pixelMetric(PixelMetric metric, const QStyleOption * option = 0, const QWidget * widget = 0) const Q_DECL_OVERRIDE;
    void drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt, QPainter *p, const QWidget *widget) const Q_DECL_OVERRIDE;
};

#endif // SEEKSTYLE_HPP
