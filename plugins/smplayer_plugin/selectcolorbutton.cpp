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

#include "selectcolorbutton.h"
#include "colorutils.h"
#include <QColorDialog>

#ifdef Q_OS_WIN
#include <QApplication>
#include <QStyle>
#endif

SelectColorButton::SelectColorButton( QWidget * parent ) 
	: QPushButton(parent)
{
	connect(this, SIGNAL(clicked()), this, SLOT(selectColor()));
	
#ifdef Q_OS_WIN
	ignore_change_event = false;
#endif
}

SelectColorButton::~SelectColorButton() {
}

void SelectColorButton::setColor(QColor c) {
	_color = c;

#ifdef Q_OS_WIN
	QString current_style = qApp->style()->objectName();
	qDebug("SelectColorButton::setColor: current style name: %s", current_style.toUtf8().constData());

	ignore_change_event = true;
	
	if ((current_style.startsWith("windowsxp")) || (current_style.startsWith("windowsvista"))) {
		setStyleSheet( "border-width: 1px; border-style: solid; border-color: #000000; background: #" + ColorUtils::colorToRRGGBB(_color.rgb()) + ";");
	} else {
		setStyleSheet("");
		ColorUtils::setBackgroundColor( this, _color );
	}
		
	ignore_change_event = false;
#else
	//setAutoFillBackground(true);
	ColorUtils::setBackgroundColor( this, _color );
#endif
}

void SelectColorButton::selectColor() {
	QColor c = QColorDialog::getColor( _color, 0 );
	if (c.isValid()) {
		setColor( c );
	}
}

#ifdef Q_OS_WIN
void SelectColorButton::changeEvent(QEvent *e) {

	QPushButton::changeEvent(e);
	
	if ((e->type() == QEvent::StyleChange) && (!ignore_change_event)) {
		setColor( color() );
	}

}
#endif

#include "moc_selectcolorbutton.cpp"
