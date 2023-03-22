/*****************************************************************************
 * seekstyle.cpp : Seek slider style
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

#include "seekstyle.hpp"
#include "util/input_slider.hpp"
#include "adapters/seekpoints.hpp"

#include <QProxyStyle>
#include <QStyleFactory>
#include <QStyleOptionSlider>
#include <QPainter>
#include <QDebug>

#define RADIUS 3
#define CHAPTERSSPOTSIZE 3

SeekStyle::SeekStyleOption::SeekStyleOption()
    : QStyleOptionSlider(), buffering( 1.0 ), length(0), animate(false), animationopacity( 1.0 ),
      animationloading(0.0)
{

}

SeekStyle::SeekStyle() : QProxyStyle( QStyleFactory::create( QLatin1String("Windows") ) )
{

}

int SeekStyle::pixelMetric( PixelMetric metric, const QStyleOption *option, const QWidget *widget ) const
{
    const QStyleOptionSlider *slider;

    if ( widget && ( slider = qstyleoption_cast<const QStyleOptionSlider *>( option ) ) )
    {
        switch( metric )
        {
        case QStyle::PM_SliderThickness:
        case QStyle::PM_SliderLength:
            return widget->minimumSize().height();
        default:
            break;
        }
    }

    return QProxyStyle::pixelMetric( metric, option, widget );
}

void SeekStyle::drawComplexControl( ComplexControl cc, const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget ) const
{
    if( cc == CC_Slider )
    {
        painter->setRenderHints( QPainter::Antialiasing );

        if ( const SeekStyle::SeekStyleOption *slideroptions =
             qstyleoption_cast<const SeekStyle::SeekStyleOption *>( option ) )
        {
            qreal sliderPos = -1;

            /* Get the needed subcontrols to draw the slider */
            QRect groove = subControlRect(CC_Slider, slideroptions, SC_SliderGroove, widget);
            QRect handle = subControlRect(CC_Slider, slideroptions, SC_SliderHandle, widget);

            /* Adjust the size of the groove so the handle stays centered */
            groove.adjust( handle.width() / 2, 0, -handle.width() / 2, 0 );

            /* Reduce the height of the groove */
            // Note: in the original 2.0.0 code, the groove had a height of 9px and to
            // comply with the original style (but still allow the widget to expand) I
            // had to remove 1 from the rect bottom.
            groove.adjust( 0, (qreal)groove.height() / 3.7, 0, (qreal)-groove.height() / 3.7 - 1 );

            if ( ( slideroptions->subControls & SC_SliderGroove ) && groove.isValid() )
            {
                sliderPos = ( ( (qreal)groove.width() ) / (qreal)slideroptions->maximum )
                        * (qreal)slideroptions->sliderPosition;

                /* set the background color and gradient */
                QColor backgroundBase( slideroptions->palette.window().color() );
                QLinearGradient backgroundGradient( 0, 0, 0, slideroptions->rect.height() );
                backgroundGradient.setColorAt( 0.0, backgroundBase.darker( 140 ) );
                backgroundGradient.setColorAt( 1.0, backgroundBase );

                /* set the foreground color and gradient */
                QColor foregroundBase( 50, 156, 255 );
                QLinearGradient foregroundGradient( 0, 0, 0, groove.height() );
                foregroundGradient.setColorAt( 0.0,  foregroundBase );
                foregroundGradient.setColorAt( 1.0,  foregroundBase.darker( 125 ) );

                /* draw a slight 3d effect on the bottom */
                painter->setPen( QColor( 230, 230, 230 ) );
                painter->setBrush( Qt::NoBrush );
                painter->drawRoundedRect( groove.adjusted( 0, 2, 0, 0 ), RADIUS, RADIUS );

                /* draw background */
                painter->setPen( Qt::NoPen );
                painter->setBrush( backgroundGradient );
                painter->drawRoundedRect( groove, RADIUS, RADIUS );

                /* adjusted foreground rectangle */
                QRect valueRect = groove.adjusted( 1, 1, -1, 0 );

                valueRect.setWidth( sliderPos );

                /* draw foreground */
                if ( slideroptions->sliderPosition > slideroptions->minimum && slideroptions->sliderPosition <= slideroptions->maximum )
                {
                    painter->setPen( Qt::NoPen );
                    painter->setBrush( foregroundGradient );
                    painter->drawRoundedRect( valueRect, RADIUS, RADIUS );
                }

                if ( slideroptions->buffering == 0.0 && slideroptions->animationloading > 0.0 )
                {
                    int width = groove.width() - groove.width() / 6;
                    QRect innerRect = groove.adjusted( slideroptions->animationloading * width + 1, 1,
                            width * ( -1.0 + slideroptions->animationloading ) - 1, 0);
                    QColor overlayColor = QColor( "Orange" );
                    overlayColor.setAlpha( 128 );
                    painter->setBrush( overlayColor );
                    painter->drawRoundedRect( innerRect, RADIUS, RADIUS );
                }

                /* draw buffering overlay */
                if ( slideroptions->buffering > 0.0 && slideroptions->buffering < 1.0 )
                {
                    QRect innerRect = groove.adjusted( 1, 1,
                                        groove.width() * ( -1.0 + slideroptions->buffering ) - 1, 0 );
                    QColor overlayColor = QColor( "Orange" );
                    overlayColor.setAlpha( 128 );
                    painter->setBrush( overlayColor );
                    painter->drawRoundedRect( innerRect, RADIUS, RADIUS );
                }
            }

            if ( slideroptions->subControls & SC_SliderTickmarks ) {
                QStyleOptionSlider tmpSlider = *slideroptions;
                tmpSlider.subControls = SC_SliderTickmarks;
                QProxyStyle::drawComplexControl(cc, &tmpSlider, painter, widget);
            }

            if ( slideroptions->subControls & SC_SliderHandle && handle.isValid() )
            {
                /* Useful for debugging */
                //painter->setBrush( QColor( 0, 0, 255, 150 ) );
                //painter->drawRect( handle );

                if ( option->state & QStyle::State_MouseOver || slideroptions->animate )
                {
                    QPalette p = slideroptions->palette;

                    /* draw chapters tickpoints */
                    if ( slideroptions->points.size() && slideroptions->length && groove.width() )
                    {
                        QColor background = p.color( QPalette::Active, QPalette::Window );
                        QColor foreground = p.color( QPalette::Active, QPalette::WindowText );
                        foreground.setHsv( foreground.hue(),
                                        ( background.saturation() + foreground.saturation() ) / 2,
                                        ( background.value() + foreground.value() ) / 2 );
                        if ( slideroptions->orientation == Qt::Horizontal ) /* TODO: vertical */
                        {
                            foreach( int64_t time, slideroptions->points )
                            {
                                int x = groove.x() + time / 1000000.0 / slideroptions->length * groove.width();
                                painter->setPen( foreground );
                                painter->setBrush( Qt::NoBrush );
                                painter->drawLine( x, slideroptions->rect.height(), x, slideroptions->rect.height() - CHAPTERSSPOTSIZE );
                            }
                        }
                    }

                    /* draw handle */
                    if ( option->state & QStyle::State_Enabled && sliderPos != -1 )
                    {
                        QSize hSize = QSize( handle.height(), handle.height() ) - QSize( 6, 6 );;
                        QPoint pos = QPoint( handle.center().x() - ( hSize.width() / 2 ), handle.center().y() - ( hSize.height() / 2 ) );

                        QPoint shadowPos( pos - QPoint( 2, 2 ) );
                        QSize sSize( QSize( handle.height(), handle.height() ) - QSize( 2, 2 ) );

                        /* prepare the handle's gradient */
                        QLinearGradient handleGradient( 0, 0, 0, hSize.height() );
                        handleGradient.setColorAt( 0.0, p.window().color().lighter( 120 ) );
                        handleGradient.setColorAt( 0.9, p.window().color().darker( 120 ) );

                        /* prepare the handle's shadow gradient */
                        QColor shadowBase = p.shadow().color();
                        if( shadowBase.lightness() > 100 )
                            shadowBase = QColor( 60, 60, 60 ); // Palette's shadow is too bright
                        QColor shadowDark( shadowBase.darker( 150 ) );
                        QColor shadowLight( shadowBase.lighter( 180 ) );
                        shadowLight.setAlpha( 50 );

                        QRadialGradient shadowGradient( shadowPos.x() + ( sSize.width() / 2 ),
                                                        shadowPos.y() + ( sSize.height() / 2 ),
                                                        qMax( sSize.width(), sSize.height() ) / 2 );
                        shadowGradient.setColorAt( 0.4, shadowDark );
                        shadowGradient.setColorAt( 1.0, shadowLight );

                        painter->setPen( Qt::NoPen );
                        painter->setOpacity( slideroptions->animationopacity );

                        /* draw the handle's shadow */
                        painter->setBrush( shadowGradient );
                        painter->drawEllipse( shadowPos.x(), shadowPos.y() + 1, sSize.width(), sSize.height() );

                        /* finally draw the handle */
                        painter->setBrush( handleGradient );
                        painter->drawEllipse( pos.x(), pos.y(), hSize.width(), hSize.height() );
                    }
                }
            }
        }
    }
    else
    {
        qWarning() << "SeekStyle: Drawing an unmanaged control";
        QProxyStyle::drawComplexControl( cc, option, painter, widget );
    }
}
