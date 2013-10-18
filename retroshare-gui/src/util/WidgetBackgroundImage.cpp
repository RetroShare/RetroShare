/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006, 2007 crypton
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#include <util/WidgetBackgroundImage.h>

#include <QString>
#include <QWidget>
#include <QPixmap>
#include <QPalette>
#include <QBrush>

void WidgetBackgroundImage::setBackgroundImage(QWidget * widget, const char * imageFile, WidgetBackgroundImage::AdjustMode adjustMode) {
	widget->setAutoFillBackground(true);

	QPixmap pixmap(imageFile);

	switch (adjustMode) {
	case AdjustNone:
		break;
	case AdjustWidth:
		widget->setMinimumWidth(pixmap.width());
		break;
	case AdjustHeight:
		widget->setMinimumHeight(pixmap.height());
		break;
	case AdjustSize:
		widget->resize(pixmap.size());
		widget->setMinimumSize(pixmap.size());
		break;
	}

	QPalette palette = widget->palette();
	palette.setBrush(widget->backgroundRole(), QBrush(pixmap));
	widget->setPalette(palette);
}
