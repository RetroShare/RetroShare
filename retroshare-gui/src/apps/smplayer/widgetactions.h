/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2008 Ricardo Villalba <rvm@escomposlinux.org>

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

#ifndef _WIDGETACTIONS_H_
#define _WIDGETACTIONS_H_

#include <QWidgetAction>
#include "timeslider.h"

class MyWidgetAction : public QWidgetAction
{
	Q_OBJECT

public:
	MyWidgetAction( QWidget * parent );
	~MyWidgetAction();

public slots:
	virtual void enable(); 	// setEnabled in QAction is not virtual :(
	virtual void disable();

protected:
	virtual void propagate_enabled(bool);
};

class TimeSliderAction : public MyWidgetAction 
{
	Q_OBJECT

public:
	TimeSliderAction( QWidget * parent );
	~TimeSliderAction();

public slots:
	virtual void setPos(int);
	virtual int pos();
#if ENABLE_DELAYED_DRAGGING
	void setDragDelay(int);
	int dragDelay();

private:
	int drag_delay;
#endif

signals:
	void posChanged(int value);
	void draggingPos(int value);
#if ENABLE_DELAYED_DRAGGING
	void delayedDraggingPos(int);
#endif

protected:
	virtual QWidget * createWidget ( QWidget * parent );
};


class VolumeSliderAction : public MyWidgetAction 
{
	Q_OBJECT

public:
	VolumeSliderAction( QWidget * parent );
	~VolumeSliderAction();

public slots:
	virtual void setValue(int);
	virtual int value();

signals:
	void valueChanged(int value);

protected:
	virtual QWidget * createWidget ( QWidget * parent );
};

#endif

