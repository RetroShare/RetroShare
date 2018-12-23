/*******************************************************************************
 * gui/common/RSListWidgetItem.h                                               *
 *                                                                             *
 * Copyright (C) 2012 RetroShare Team <retroshare.project@gmail.com>           *
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

#ifndef _RSLISTWIDGETITEM_H
#define _RSLISTWIDGETITEM_H

#include <QListWidgetItem>

class RSListWidgetItem : public QListWidgetItem
{
public:
	RSListWidgetItem(QListWidget *view = 0, int type = Type);
	RSListWidgetItem(const QString &text, QListWidget *view = 0, int type = Type);
	RSListWidgetItem(const QIcon &icon, const QString &text, QListWidget *view = 0, int type = Type);
	RSListWidgetItem(const QListWidgetItem &other);

	bool operator<(const QListWidgetItem &other) const;
};

#endif

