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

#include "timeslider.h"

#if QT_VERSION > 0x040000
#include <stdlib.h>
#include <QApplication>
#include <QStyle>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QApplication>
#endif

#define DEBUG 0

MySlider::MySlider( QWidget * parent ) : QSlider(parent)
{
	setOrientation( Qt::Horizontal );
}

MySlider::~MySlider() {
}

// The code from the following function is from Javier Díaz, 
// taken from a post in the Qt-interest mailing list.
void MySlider::mousePressEvent( QMouseEvent * e ) {
#if QT_VERSION < 0x040000
	if (e->button() == Qt::LeftButton) {
		if ( !sliderRect().contains(e->pos()) ) {
			// Pongo como valor el correspondiente al lugar donde hemos hecho click
			int value = Q3RangeControl::valueFromPosition( e->x(), width() );
			setValue(value);
			// Representa un movimiento en el slider
			emit sliderMoved(value);
		}
		else
			QSlider::mousePressEvent(e);
	}
#else
	// FIXME:
	// The code doesn't work well with right to left layout,
	// so it's disabled.
	if (qApp->isRightToLeft()) {
		QSlider::mousePressEvent(e);
		return;
	}

	int range = maximum()-minimum();
	int pos = (e->x() * range) / width();
	//qDebug( "width: %d x: %d", width(), e->x());
	//qDebug( "range: %d pos: %d value: %d", range, pos, value());

	// Calculate how many positions takes the slider handle
	int metric = qApp->style()->pixelMetric( QStyle::PM_SliderLength );
	double one_tick_pixels = (double) width() / range;
	int slider_handle_positions = metric / one_tick_pixels;

	/*
	qDebug("metric: %d", metric );
	qDebug("one_tick_pixels :%f", one_tick_pixels);
	qDebug("width() :%d", width());
	qDebug("slider_handle_positions: %d", slider_handle_positions);
	*/

	if (abs(pos - value()) > slider_handle_positions) { 
		setValue(pos);
		emit sliderMoved( pos );
	} else {
		QSlider::mousePressEvent(e);
	}
#endif
}



TimeSlider::TimeSlider( QWidget * parent ) : MySlider(parent)
{
	dont_update = FALSE;
	setMinimum(0);
	setMaximum(100);

	setFocusPolicy( Qt::NoFocus );
	setSizePolicy( QSizePolicy::Expanding , QSizePolicy::Fixed );

	connect( this, SIGNAL( sliderPressed() ), this, SLOT( stopUpdate() ) );
	connect( this, SIGNAL( sliderReleased() ), this, SLOT( resumeUpdate() ) );
	connect( this, SIGNAL( sliderReleased() ), this, SLOT( mouseReleased() ) );
	connect( this, SIGNAL( valueChanged(int) ), this, SLOT( valueChanged_slot(int) ) );
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
