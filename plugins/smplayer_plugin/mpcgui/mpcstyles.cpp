/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2008 matt_ <matt@endboss.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "mpcstyles.h"

#include <QWindowsStyle>
#include <QStyleOptionToolBar>
#include <QSlider>
#include <QPainter>


void
MpcToolbarStyle::drawControl(ControlElement control, const QStyleOption *option,
                      QPainter *painter, const QWidget *widget) const
{
    if(control == CE_ToolBar)
    {
        if (const QStyleOptionToolBar *toolbar = qstyleoption_cast<const QStyleOptionToolBar *>(option)) {
            QRect rect = option->rect;

            if( toolbar->toolBarArea == Qt::BottomToolBarArea &&
                toolbar->positionOfLine == QStyleOptionToolBar::End )
            {
                painter->setPen(QPen(option->palette.light().color()));
                painter->drawLine(rect.topLeft().x(),
                            rect.topLeft().y(),
                            rect.topRight().x(),
                            rect.topRight().y());

                painter->setPen(QPen(option->palette.light().color()));
                painter->drawLine(rect.topLeft().x(),
                            rect.topLeft().y(),
                            rect.bottomLeft().x(),
                            rect.bottomLeft().y());

                painter->setPen(QPen(option->palette.dark().color()));
                painter->drawLine(rect.topRight().x(),
                            rect.topRight().y(),
                            rect.bottomRight().x(),
                            rect.bottomRight().y());

            }
            else if( toolbar->toolBarArea == Qt::BottomToolBarArea &&
                toolbar->positionOfLine == QStyleOptionToolBar::Beginning )
            {
                painter->setPen(QPen(option->palette.light().color()));
                painter->drawLine(rect.topLeft().x(),
                            rect.topLeft().y(),
                            rect.bottomLeft().x(),
                            rect.bottomLeft().y());

                painter->setPen(QPen(option->palette.dark().color()));
                painter->drawLine(rect.topRight().x(),
                            rect.topRight().y(),
                            rect.bottomRight().x(),
                            rect.bottomRight().y());

                painter->setPen(QPen(option->palette.dark().color()));
                painter->drawLine(rect.bottomLeft().x(),
                            rect.bottomLeft().y(),
                            rect.bottomRight().x(),
                            rect.bottomRight().y());
            }
            else
            {
                QWindowsStyle::drawControl(control,toolbar, painter, widget);
            }
        }
    }
    else
    {
        QWindowsStyle::drawControl(control,option, painter, widget);
    }
}

// draw custom slider + handle for volume widget
void MpcVolumeSlideStyle::drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt,
                                       QPainter *p, const QWidget *widget) const
{
    if( cc == CC_Slider )
    {
        if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
            QRect groove = subControlRect(CC_Slider, slider, SC_SliderGroove, widget);
            QRect handle = subControlRect(CC_Slider, slider, SC_SliderHandle, widget);

            if ((slider->subControls & SC_SliderGroove) && groove.isValid()) {

                p->setPen(slider->palette.shadow().color());
                if (slider->orientation == Qt::Horizontal) {
                    static QPoint points[3] = {
                                QPoint(groove.x() , groove.y() + slider->rect.height() ),
                                QPoint(groove.x() + groove.width() -2 , groove.y() + slider->rect.height() ),
                                QPoint(groove.x() + groove.width() -2 , groove.y()  ),
                            };
                    QPen oldPen = p->pen();
                    
                    p->setPen(slider->palette.dark().color());
                    p->drawLine(QPoint(points[0].x(), points[0].y() -2 ),QPoint(points[2].x(), points[2].y()));

                    QPoint b[3] = { QPoint(points[0].x(),points[0].y()-1), QPoint(points[1].x()-1, points[1].y()-1), QPoint(points[2].x()-1,points[2].y()) };
                    p->setPen(slider->palette.light().color());
                    p->drawPolyline(b, 3);
                    p->setPen(oldPen);
                }
            }


            if (slider->subControls & SC_SliderTickmarks) {
                QStyleOptionSlider tmpSlider = *slider;
                tmpSlider.subControls = SC_SliderTickmarks;
                QCommonStyle::drawComplexControl(cc, &tmpSlider, p, widget);
            }

            if (slider->subControls & SC_SliderHandle) {
                const QColor c0 = slider->palette.shadow().color();
                const QColor c1 = slider->palette.dark().color();
                // const QColor c2 = g.button();
                const QColor c3 = slider->palette.midlight().color();
                const QColor c4 = slider->palette.light().color();
                QBrush handleBrush;

                if (slider->state & State_Enabled) {
                    handleBrush = slider->palette.color(QPalette::Button);
                } else {
                    handleBrush = QBrush(slider->palette.color(QPalette::Button),
                                         Qt::Dense4Pattern);
                }


                int x = handle.x() , y = handle.y(),
                   wi = handle.width() - 2, he = slider->rect.height();

                if (slider->state & State_HasFocus) {
                    QStyleOptionFocusRect fropt;
                    fropt.QStyleOption::operator=(*slider);
                    fropt.rect = subElementRect(SE_SliderFocusRect, slider, widget);
                    drawPrimitive(PE_FrameFocusRect, &fropt, p, widget);
                }

                Qt::BGMode oldMode = p->backgroundMode();
                p->setBackgroundMode(Qt::OpaqueMode);
                qDrawWinButton(p, QRect(x, y, wi, he), slider->palette, false,
                                   &handleBrush);
                p->setBackgroundMode(oldMode);

            }
        }
    }
    else
    {
        QWindowsStyle::drawComplexControl(cc,opt,p,widget);
    }
}

// draw custom slider + handle for timeslide widget
void MpcTimeSlideStyle::drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt,
                                       QPainter *p, const QWidget *widget) const
{
    if( cc == CC_Slider )
    {
        if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
            QRect groove = subControlRect(CC_Slider, slider, SC_SliderGroove, widget);
            QRect handle = subControlRect(CC_Slider, slider, SC_SliderHandle, widget);

            if ((slider->subControls & SC_SliderGroove) && groove.isValid()) {
                if (slider->orientation == Qt::Horizontal) {
                    int x = groove.x() + 2;
                    int y = slider->rect.height() / 2 - 4;
                    int w = groove.width() - 4;
                    int h = 7;
                    qDrawShadeRect (p,x,y,w,h, slider->palette, true,1,0,
                        &slider->palette.brush(QPalette::Light));
                }
            }


            if (slider->subControls & SC_SliderTickmarks) {
                QStyleOptionSlider tmpSlider = *slider;
                tmpSlider.subControls = SC_SliderTickmarks;
                QCommonStyle::drawComplexControl(cc, &tmpSlider, p, widget);
            }

            if (slider->subControls & SC_SliderHandle) {
                const QColor c0 = slider->palette.shadow().color();
                const QColor c1 = slider->palette.dark().color();
                // const QColor c2 = g.button();
                const QColor c3 = slider->palette.midlight().color();
                const QColor c4 = slider->palette.light().color();
                QBrush handleBrush;

                if (slider->state & State_Enabled) {
                    handleBrush = slider->palette.color(QPalette::Button);
                } else {
                    handleBrush = QBrush(slider->palette.color(QPalette::Button),
                                         Qt::Dense4Pattern);
                }


                int x = handle.x() , y = handle.y() + 1,
                   wi = 13, he = 14;

                if (slider->state & State_HasFocus) {
                    QStyleOptionFocusRect fropt;
                    fropt.QStyleOption::operator=(*slider);
                    fropt.rect = subElementRect(SE_SliderFocusRect, slider, widget);
                    drawPrimitive(PE_FrameFocusRect, &fropt, p, widget);
                }

                Qt::BGMode oldMode = p->backgroundMode();
                p->setBackgroundMode(Qt::OpaqueMode);
                qDrawWinPanel(p, QRect(x, y, wi, he), slider->palette, false,
                               &handleBrush);
                qDrawShadeRect (p, QRect(x+2,y+3, wi-4, he-6), slider->palette, true,1,0,
                               &slider->palette.brush(QPalette::Light));
                p->setBackgroundMode(oldMode);
            }
        }
    }
    else
    {
        QWindowsStyle::drawComplexControl(cc,opt,p,widget);
    }
}

