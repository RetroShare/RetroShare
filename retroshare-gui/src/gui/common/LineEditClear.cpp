/****************************************************************
 *
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

#include "LineEditClear.h"

#include <QToolButton>
#include <QStyle>

LineEditClear::LineEditClear(QWidget *parent)
	: QLineEdit(parent)
{
	findButton = new QToolButton(this);
	QPixmap findPixmap(":/images/find-16.png");
	findButton->setIcon(QIcon(findPixmap));
	findButton->setIconSize(findPixmap.size());
	findButton->setCursor(Qt::ArrowCursor);
	findButton->setStyleSheet("QToolButton { border: none; padding: 0px; }");

	clearButton = new QToolButton(this);
	clearButton->setFixedSize(16, 16);
	clearButton->setIconSize(QSize(16, 16));
	clearButton->setCursor(Qt::ArrowCursor);
	clearButton->setStyleSheet("QToolButton { border: none; padding: 0px; }"
							   "QToolButton { border-image: url(:/images/closenormal.png) }"
							   "QToolButton:hover { border-image: url(:/images/closehover.png) }"
							   "QToolButton:pressed  { border-image: url(:/images/closepressed.png) }");
	clearButton->hide();

	connect(clearButton, SIGNAL(clicked()), this, SLOT(clear()));
	connect(this, SIGNAL(textChanged(const QString&)), this, SLOT(updateCloseButton(const QString&)));

	int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
	setStyleSheet(QString("QLineEdit { padding-right: %1px; padding-left: %2px; } ").
			arg(clearButton->sizeHint().width() + frameWidth + 1).
			arg(findButton->sizeHint().width() + frameWidth + 1));
	QSize msz = minimumSizeHint();
	setMinimumSize(qMax(msz.width(), clearButton->sizeHint().height() + frameWidth * 2), qMax(msz.height(), clearButton->sizeHint().height() + frameWidth * 2));
}

void LineEditClear::resizeEvent(QResizeEvent *)
{
	QSize sz = clearButton->sizeHint();
	int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
	clearButton->move(rect().right() - frameWidth - sz.width() + 2, (rect().bottom() - sz.height()) / 2 + 2);
}

void LineEditClear::updateCloseButton(const QString& text)
{
	clearButton->setVisible(!text.isEmpty());
}
