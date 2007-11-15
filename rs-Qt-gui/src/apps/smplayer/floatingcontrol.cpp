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

#include "floatingcontrol.h"

#include <QToolButton>
#include <QIcon>
#include <QLCDNumber>
#include <QLabel>
#include <QFrame>
#include <QHBoxLayout>
#include <QEvent>
#include <QApplication>

#include "timeslider.h"
#include "images.h"
#include "helper.h"


class MyToolButton : public QToolButton {
public:
	MyToolButton ( QWidget * parent );
};

MyToolButton::MyToolButton( QWidget * parent ) : QToolButton(parent) 
{
	setAutoRaise(true);
	setIconSize( QSize(32,32) );
}


FloatingControl::FloatingControl( QWidget * parent )
	: QWidget( parent, Qt::Window | Qt::FramelessWindowHint | 
                       Qt::WindowStaysOnTopHint | 
                       Qt::X11BypassWindowManagerHint )
{
	play = new MyToolButton(this);
	pause = new MyToolButton(this);
	stop = new MyToolButton(this);

	rewind3 = new MyToolButton(this);
	rewind2 = new MyToolButton(this);
	rewind1 = new MyToolButton(this);

	time = new TimeSlider(this);

	forward1 = new MyToolButton(this);
	forward2 = new MyToolButton(this);
	forward3 = new MyToolButton(this);

#if NEW_CONTROLWIDGET
	time_label = new QLabel(this);
	time_label->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
	time_label->setAutoFillBackground(TRUE);

	Helper::setBackgroundColor( time_label, QColor(0,0,0) );
	Helper::setForegroundColor( time_label, QColor(255,255,255) );
	time_label->setText( "00:00:00" );
	time_label->setFrameShape( QFrame::Panel );
	time_label->setFrameShadow( QFrame::Sunken );
#else
	lcd = new QLCDNumber(this);
	lcd->setNumDigits(10); // maximum time with 10 digits is 9999:59:59
	lcd->setFrameShape( QFrame::WinPanel );
	lcd->setFrameShadow( QFrame::Sunken );
	lcd->setAutoFillBackground(TRUE);

	Helper::setBackgroundColor( lcd, QColor(0,0,0) );
	Helper::setForegroundColor( lcd, QColor(200,200,200) );
	lcd->setSegmentStyle( QLCDNumber::Flat );
	lcd->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed);
#endif

	fullscreen = new MyToolButton(this);
	fullscreen->setCheckable(true);

	mute = new MyToolButton(this);
	mute->setCheckable(true);

	volume = new MySlider(this);
	volume->setMinimum(0);
	volume->setMaximum(100);
	volume->setValue(50);
	volume->setTickPosition( QSlider::TicksBelow );
	volume->setTickInterval( 10 );
	volume->setSingleStep( 1 );
	volume->setPageStep( 10 );
	volume->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed);

	// No focus on widgets
	rewind3->setFocusPolicy( Qt::NoFocus );
	rewind2->setFocusPolicy( Qt::NoFocus );
	rewind1->setFocusPolicy( Qt::NoFocus );

	forward1->setFocusPolicy( Qt::NoFocus );
	forward2->setFocusPolicy( Qt::NoFocus );
	forward3->setFocusPolicy( Qt::NoFocus );

	play->setFocusPolicy( Qt::NoFocus );
	pause->setFocusPolicy( Qt::NoFocus );
	stop->setFocusPolicy( Qt::NoFocus );
#if !NEW_CONTROLWIDGET
	lcd->setFocusPolicy( Qt::NoFocus );
#endif
	fullscreen->setFocusPolicy( Qt::NoFocus );
	mute->setFocusPolicy( Qt::NoFocus );
	volume->setFocusPolicy( Qt::NoFocus );

	// Layout
#if NEW_CONTROLWIDGET
	QHBoxLayout * l = new QHBoxLayout;
	l->setMargin(2);
	l->setSpacing(1);

	l->addWidget( play );
	l->addWidget( pause );
	l->addWidget( stop );
	l->addSpacing( 10 );
	l->addWidget( rewind3 );
	l->addWidget( rewind2 );
	l->addWidget( rewind1 );
	l->addWidget( time );
	l->addWidget( forward1 );
	l->addWidget( forward2 );
	l->addWidget( forward3 );
	l->addSpacing( 10 );
	l->addWidget( fullscreen );
	l->addWidget( mute );
	l->addWidget( volume );
	l->addSpacing( 10 );
	l->addWidget( time_label );

	setLayout(l);
#else
	QHBoxLayout * l1 = new QHBoxLayout;
	l1->setMargin(0);
	l1->setSpacing(0);

	l1->addWidget( rewind3 );
	l1->addWidget( rewind2 );
	l1->addWidget( rewind1 );
	l1->addWidget( time );
	l1->addWidget( forward1 );
	l1->addWidget( forward2 );
	l1->addWidget( forward3 );

	QSpacerItem * spacer1 = new QSpacerItem( 20, 10, QSizePolicy::Expanding,
                                             QSizePolicy::Minimum );

	QSpacerItem * spacer2 = new QSpacerItem( 20, 10, QSizePolicy::Expanding,
                                             QSizePolicy::Minimum );

	QHBoxLayout * l2 = new QHBoxLayout;
	l2->setMargin(0);
	l2->setSpacing(0);

	l2->addWidget( play );
	l2->addWidget( pause );
	l2->addWidget( stop );
	l2->addItem( spacer1 );
	l2->addWidget( lcd );
	l2->addItem( spacer2 );
	l2->addWidget( fullscreen );
	l2->addWidget( mute );
	l2->addWidget( volume );

	QVBoxLayout * l = new QVBoxLayout;
	l->setMargin(0);
	l->setSpacing(0);

	l->addLayout(l1);
	l->addLayout(l2);

	setLayout(l);
#endif

	retranslateStrings();

	setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed);
	adjustSize();
}


FloatingControl::~FloatingControl()
{
}

void FloatingControl::retranslateStrings() {
	int size = 22;

	pause->setIcon( Images::icon("pause", size) );
	stop->setIcon( Images::icon("stop", size) );

	if (qApp->isLeftToRight()) {
		play->setIcon( Images::icon("play", size) );

		forward1->setIcon( Images::icon("forward10s", size) );
		forward2->setIcon( Images::icon("forward1m", size) );
		forward3->setIcon( Images::icon("forward10m", size) );

		rewind1->setIcon( Images::icon("rewind10s", size) );
		rewind2->setIcon( Images::icon("rewind1m", size) );
		rewind3->setIcon( Images::icon("rewind10m", size) );
	} else {
		play->setIcon( Images::flippedIcon("play", size) );

		forward1->setIcon( Images::flippedIcon("forward10s", size) );
		forward2->setIcon( Images::flippedIcon("forward1m", size) );
		forward3->setIcon( Images::flippedIcon("forward10m", size) );

		rewind1->setIcon( Images::flippedIcon("rewind10s", size) );
		rewind2->setIcon( Images::flippedIcon("rewind1m", size) );
		rewind3->setIcon( Images::flippedIcon("rewind10m", size) );
	}

	fullscreen->setIcon( Images::icon("fullscreen", size) );

	QIcon icset( Images::icon("volume", size) );
	icset.addPixmap( Images::icon("mute", size), QIcon::Normal, QIcon::On  );
	mute->setIcon( icset );
}

// Language change stuff
void FloatingControl::changeEvent(QEvent *e) {
	if (e->type() == QEvent::LanguageChange) {
		retranslateStrings();
	} else {
		QWidget::changeEvent(e);
	}
}

#include "moc_floatingcontrol.cpp"
