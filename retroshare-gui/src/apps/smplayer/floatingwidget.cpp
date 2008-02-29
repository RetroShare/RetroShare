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

#include "floatingwidget.h"
#include <QToolBar>
#include <QTimer>
#include <QHBoxLayout>

FloatingWidget::FloatingWidget( QWidget * parent )
	: QWidget( parent, Qt::Window | Qt::FramelessWindowHint |
                       Qt::WindowStaysOnTopHint |
                       Qt::X11BypassWindowManagerHint )
{
	setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Minimum );

	tb = new QToolBar;
	tb->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum );

	QHBoxLayout *layout = new QHBoxLayout;
	layout->setSpacing(2);
	layout->setMargin(2);
	layout->addWidget(tb);

	setLayout(layout);

	_animated = false;
	animation_timer = new QTimer(this);
	animation_timer->setInterval(2);
	connect( animation_timer, SIGNAL(timeout()), this, SLOT(animate()) );
}

FloatingWidget::~FloatingWidget() {
}

void FloatingWidget::showOver(QWidget * widget, int size, Place place) {
	qDebug("FloatingWidget::showOver");

	int w = widget->width() * size / 100;
	int h = height();
	resize( w, h );

	//qDebug("widget x: %d, y: %d, h: %d, w: %d", widget->x(), widget->y(), widget->width(), widget->height());

	int x = (widget->width() - width() ) / 2;
	int y;
	if (place == Top) 
		y = 0;
	else
		y = widget->height() - height();

	QPoint p = widget->mapToGlobal(QPoint(x, y));

	//qDebug("FloatingWidget::showOver: x: %d, y: %d, w: %d, h: %d", x, y, w, h);
	//qDebug("FloatingWidget::showOver: global x: %d global y: %d", p.x(), p.y());
	move(p);

	if (isAnimated()) {
		Movement m = Upward;
		if (place == Top) m = Downward;
		showAnimated(p, m);
	} else {
		show();
	}
}

void FloatingWidget::showAnimated(QPoint final_position, Movement movement) {
	current_movement = movement;
	final_y = final_position.y();

	if (movement == Upward) {
		current_y = final_position.y() + height();
	} else {
		current_y = final_position.y() - height();
	}

	move(x(), current_y);
	show();

	animation_timer->start();
}

void FloatingWidget::animate() {
	if (current_y == final_y) {
		animation_timer->stop();
	} else {
		if (current_movement == Upward) current_y--; else current_y++;
		move(x(), current_y);
	}
}

#include "moc_floatingwidget.cpp"
