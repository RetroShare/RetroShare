/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2012, RetroShare Team
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
