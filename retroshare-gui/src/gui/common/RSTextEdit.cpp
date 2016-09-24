/****************************************************************
 *
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2014, RetroShare Team
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

#include <QPainter>

#include "RSTextEdit.h"

RSTextEdit::RSTextEdit(QWidget *parent)
    : QTextEdit(parent)
{
}

#if QT_VERSION < 0x050200
void RSTextEdit::setPlaceholderText(const QString &text)
{
	mPlaceholderText = text;
	viewport()->repaint();
}

void RSTextEdit::paintEvent(QPaintEvent *event)
{
	QTextEdit::paintEvent(event);

	if (!mPlaceholderText.isEmpty() && document()->isEmpty()) {
		QWidget *vieportWidget = viewport();
		QPainter painter(vieportWidget);

		QPen pen = painter.pen();
		QColor color = pen.color();
		color.setAlpha(128);
		pen.setColor(color);
		painter.setPen(pen);

		const int margin = int(document()->documentMargin());
		painter.drawText(viewport()->rect().adjusted(margin, margin, -margin, -margin), Qt::AlignTop | Qt::TextWordWrap, mPlaceholderText);
	}
}
#endif
