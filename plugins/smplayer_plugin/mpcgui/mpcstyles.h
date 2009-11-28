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

#ifndef _MPC_STYLES_H_
#define _MPC_STYLES_H_

#include <QWindowsStyle>
#include <QStyleOptionToolBar>

enum QSliderDirection { SlUp, SlDown, SlLeft, SlRight };

class MpcToolbarStyle : public QWindowsStyle
{
    Q_OBJECT

public:
    MpcToolbarStyle() {};
    void drawControl(ControlElement control, const QStyleOption *option,
                      QPainter *painter, const QWidget *widget) const;
};

class MpcTimeSlideStyle : public QWindowsStyle
{
    Q_OBJECT

public:
    MpcTimeSlideStyle() {};
    void drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt,
                                       QPainter *p, const QWidget *widget) const;
};

class MpcVolumeSlideStyle : public QWindowsStyle
{
    Q_OBJECT

public:
    MpcVolumeSlideStyle() {};
    void drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt,
                                       QPainter *p, const QWidget *widget) const;
};


#endif
