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

#include "timeslider.h"

#include <QWheelEvent>
#include <QTimer>

#define DEBUG 0

TimeSlider::TimeSlider( QWidget * parent ) : MySlider(parent)
{
	dont_update = FALSE;
	setMinimum(0);
#ifdef SEEKBAR_RESOLUTION
	setMaximum(SEEKBAR_RESOLUTION);
#else
	setMaximum(100);
#endif

	setFocusPolicy( Qt::NoFocus );
	setSizePolicy( QSizePolicy::Expanding , QSizePolicy::Fixed );

	connect( this, SIGNAL( sliderPressed() ), this, SLOT( stopUpdate() ) );
	connect( this, SIGNAL( sliderReleased() ), this, SLOT( resumeUpdate() ) );
	connect( this, SIGNAL( sliderReleased() ), this, SLOT( mouseReleased() ) );
	connect( this, SIGNAL( valueChanged(int) ), this, SLOT( valueChanged_slot(int) ) );
#if ENABLE_DELAYED_DRAGGING
	connect( this, SIGNAL(draggingPos(int) ), this, SLOT(checkDragging(int)) );
	
	last_pos_to_send = -1;
	timer = new QTimer(this);
	connect( timer, SIGNAL(timeout()), this, SLOT(sendDelayedPos()) );
	timer->start(200);
#endif
}

TimeSlider::~TimeSlider() {
}

void TimeSlider::stopUpdate() {
	#if DEBUG
	qDebug("TimeSlider::stopUpdate");
	#endif
	dont_update = TRUE;
}

void TimeSlider::resumeUpdate() {
	#if DEBUG
	qDebug("TimeSlider::resumeUpdate");
	#endif
	dont_update = FALSE;
}

void TimeSlider::mouseReleased() {
	#if DEBUG
	qDebug("TimeSlider::mouseReleased");
	#endif
	emit posChanged( value() );
}

void TimeSlider::valueChanged_slot(int v) {
	#if DEBUG
	qDebug("TimeSlider::changedValue_slot: %d", v);
	#endif

	// Only to make things clear:
	bool dragging = dont_update;
	if (!dragging) {
		if (v!=position) {
			#if DEBUG
			qDebug(" emitting posChanged");
			#endif
			emit posChanged(v);
		}
	} else {
		#if DEBUG
		qDebug(" emitting draggingPos");
		#endif
		emit draggingPos(v);
	}
}

#if ENABLE_DELAYED_DRAGGING
void TimeSlider::setDragDelay(int d) {
	qDebug("TimeSlider::setDragDelay: %d", d);
	timer->setInterval(d);
}

int TimeSlider::dragDelay() {
	return timer->interval();
}

void TimeSlider::checkDragging(int v) {
	qDebug("TimeSlider::checkDragging: %d", v);
	last_pos_to_send = v;
}

void TimeSlider::sendDelayedPos() {
	if (last_pos_to_send != -1) {
		qDebug("TimeSlider::sendDelayedPos: %d", last_pos_to_send);
		emit delayedDraggingPos(last_pos_to_send);
		last_pos_to_send = -1;
	}
}
#endif

void TimeSlider::setPos(int v) {
	#if DEBUG
	qDebug("TimeSlider::setPos: %d", v);
	qDebug(" dont_update: %d", dont_update);
	#endif

	if (v!=pos()) {
		if (!dont_update) {
			position = v;
			setValue(v);
		}
	}
}

int TimeSlider::pos() {
	return position;
}

void TimeSlider::wheelEvent( QWheelEvent * e ) {
	e->ignore();
}


#include "moc_timeslider.cpp"
