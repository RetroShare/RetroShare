/*******************************************************************************
 * util/ObjectPainter.cpp                                                      *
 *                                                                             *
 * Copyright (c) 2012 Retroshare Team   <retroshare.project@gmail.com>         *
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

#include <QPushButton>
#include <QStyleOption>
#include <QStylePainter>

#include "ObjectPainter.h"

namespace ObjectPainter
{

class DrawButton : public QPushButton
{
public:
	DrawButton(const QString &text) : QPushButton(text) {}

	void drawOnPixmap(QPixmap &pixmap)
	{
		// get a transparent pixmap
		pixmap = QPixmap(size());
		pixmap.fill(Qt::transparent);

		// init options
		QStyleOptionButton option;
		initStyleOption(&option);

		// draw the button onto the pixmap
		QStylePainter painter(&pixmap, this);
		painter.drawControl(QStyle::CE_PushButton, option);
		painter.end();
	}
};

void drawButton(const QString &text, const QString &styleSheet, QPixmap &pixmap)
{
	DrawButton button(text);
	button.setStyleSheet(styleSheet);
	QSize size = button.sizeHint();
	button.setGeometry(0, 0, size.width(), size.height());
	button.drawOnPixmap(pixmap);
}

}
