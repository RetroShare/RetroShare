/*******************************************************************************
 * gui/common/RSTreeView.h                                                     *
 *                                                                             *
 * Copyright (C) 2010 RetroShare Team <retroshare.project@gmail.com>           *
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

#ifndef _RSTREEVIEW_H
#define _RSTREEVIEW_H

#include <QTreeView>

/* Subclassing QTreeView */
class RSTreeView : public QTreeView
{
	Q_OBJECT

public:
	RSTreeView(QWidget *parent = 0);

	void setPlaceholderText(const QString &text);

protected:
	void paintEvent(QPaintEvent *event);

	QString placeholderText;
};

#endif
