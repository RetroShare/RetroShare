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

#include "seekwidget.h"
#include <QLabel>
#include <QDateTimeEdit>

SeekWidget::SeekWidget( QWidget* parent, Qt::WindowFlags f)
	: QWidget(parent, f)
{
	setupUi(this);
	time_edit->setDisplayFormat("mm:ss");
}

SeekWidget::~SeekWidget() {
}

void SeekWidget::setIcon(QPixmap icon) {
	_image->setText("");
	_image->setPixmap(icon);
}

const QPixmap * SeekWidget::icon() const {
	return _image->pixmap();
}

void SeekWidget::setLabel(QString text) {
	_label->setText(text);
}

QString SeekWidget::label() const {
	return _label->text();
}

void SeekWidget::setTime(int secs) {
	QTime t;
	time_edit->setTime(t.addSecs(secs));
}

int SeekWidget::time() const {
	QTime t = time_edit->time();
	return (t.minute() * 60) + t.second();
}

#include "moc_seekwidget.cpp"
