/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2008 Ricardo Villalba <rvm@escomposlinux.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) version 3, or any
    later version accepted by the membership of KDE e.V. (or its
    successor approved by the membership of KDE e.V.), Trolltech ASA
    (or its successors, if any) and the KDE Free Qt Foundation, which shall
    act as a proxy defined in Section 6 of version 3 of the license.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _MYSLIDER_H_
#define _MYSLIDER_H_

#include <QSlider>
#include "config.h"

#define CODE_FOR_CLICK 1 // 0 = old code, 1 = code copied from QSlider, 2 = button swap

class QTimer;

class MySlider : public QSlider
{
	Q_OBJECT

public:
	MySlider( QWidget * parent );
	~MySlider();

protected:
	void mousePressEvent ( QMouseEvent * event );
#if CODE_FOR_CLICK == 1
	inline int pick(const QPoint &pt) const;
	int pixelPosToRangeValue(int pos) const;
#if QT_VERSION < 0x040300
    void initStyleOption(QStyleOptionSlider *option) const;
#endif
#endif
};

#endif

