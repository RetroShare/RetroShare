/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2007 Ricardo Villalba <rvm@escomposlinux.org>

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

#ifndef _FLOATINGCONTROL_H_
#define _FLOATINGCONTROL_H_

#include <QWidget>
#include "config.h"

class QToolButton;
class TimeSlider;
class MySlider;
class QLCDNumber;
class QLabel;

class FloatingControl : public QWidget
{
	Q_OBJECT

public:

	FloatingControl( QWidget * parent = 0);
	~FloatingControl();

	QToolButton * rewind1;
	QToolButton * rewind2;
	QToolButton * rewind3;
	QToolButton * forward1;
	QToolButton * forward2;
	QToolButton * forward3;
	QToolButton * play;
	QToolButton * stop;
	QToolButton * pause;
	TimeSlider * time;
	QToolButton * fullscreen;
	QToolButton * mute;
	MySlider * volume;
#if NEW_CONTROLWIDGET
	QLabel * time_label;
#else
	QLCDNumber * lcd;
#endif

protected:
	void retranslateStrings();
	virtual void changeEvent ( QEvent * event ) ;
};

#endif

