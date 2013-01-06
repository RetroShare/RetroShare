/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2012, RetroShare Team
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

#ifndef _RSTREEWIDGET_H
#define _RSTREEWIDGET_H

#include <QTreeWidget>

/* Subclassing QTreeWidget */
class RSTreeWidget : public QTreeWidget
{
	Q_OBJECT

public:
	RSTreeWidget(QWidget *parent = 0);

	void setPlaceholderText(const QString &text);

signals:
	void signalMouseMiddleButtonClicked(QTreeWidgetItem *item);

protected:
	void paintEvent(QPaintEvent *event);
	virtual void mousePressEvent(QMouseEvent *event);

	QString mPlaceholderText;
};

#endif
