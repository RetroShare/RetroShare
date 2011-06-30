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

#include "eqslider.h"
#include <QSlider>
#include <QLabel>
#include <QPixmap>
#include "verticaltext.h"


EqSlider::EqSlider( QWidget* parent, Qt::WindowFlags f) 
	: QWidget(parent, f)
{
	setupUi(this);

	_icon->setText( QString::null );
	_slider->setFocusPolicy( Qt::StrongFocus );
	_slider->setTickPosition( QSlider::TicksRight );
	_slider->setTickInterval( 10 );
	_slider->setSingleStep( 1 );
	_slider->setPageStep( 10 );

	connect( _slider, SIGNAL(valueChanged(int)),
             this, SLOT(sliderValueChanged(int)) );
}

EqSlider::~EqSlider() {
}

/*
void EqSlider::languageChange() {
}
*/

void EqSlider::setIcon( QPixmap i) {
	_icon->setPixmap(i);
}

const QPixmap * EqSlider::icon() const {
	return _icon->pixmap();
}

void EqSlider::setLabel( QString s) {
	_label->setText(s);
}

QString EqSlider::label() const {
	return _label->text();
}

void EqSlider::setValue(int value) {
	_slider->setValue(value);
	value_label->setNum(value);
}

int EqSlider::value() const {
	return _slider->value();
}

void EqSlider::sliderValueChanged(int v) {
	emit valueChanged( v );
}

#include "moc_eqslider.cpp"
