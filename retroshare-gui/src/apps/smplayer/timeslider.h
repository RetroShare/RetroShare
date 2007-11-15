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

#ifndef _TIMESLIDER_H_
#define _TIMESLIDER_H_

#include <QSlider>

class MySlider : public QSlider
{
	Q_OBJECT

public:
	MySlider( QWidget * parent );
	~MySlider();

protected:
	void mousePressEvent ( QMouseEvent * event );
};


class TimeSlider : public MySlider 
{
	Q_OBJECT

public:
	TimeSlider( QWidget * parent );
	~TimeSlider();

public slots:
	virtual void setPos(int); // Don't use setValue!
	virtual int pos();

signals:
	void posChanged(int value);
	void draggingPos(int value);

protected slots:
	void stopUpdate();
	void resumeUpdate();
	void mouseReleased();
	void valueChanged_slot(int);

	virtual void wheelEvent( QWheelEvent * e );

private:
	bool dont_update;
	int position;
};


#endif
