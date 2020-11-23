/*******************************************************************************
 * util/WidgetBackgroundImage.cpp                                              *
 *                                                                             *
 * Copyright (c) 2006 Crypton                  <retroshare.project@gmail.com>  *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

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
