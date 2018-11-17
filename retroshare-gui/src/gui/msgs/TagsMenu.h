/*******************************************************************************
 * retroshare-gui/src/gui/msgs/TagsMenu.h                                      *
 *                                                                             *
 * Copyright (C) 2007 by Retroshare Team     <retroshare.project@gmail.com>    *
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

#ifndef _TAGSMENU_H
#define _TAGSMENU_H

#include <QMenu>

#include <stdint.h>

class TagsMenu : public QMenu
{
	Q_OBJECT

public:
	TagsMenu(const QString &title, QWidget *parent);

	void activateActions(std::list<uint32_t>& tagIds);

signals:
	void tagSet(int tagId, bool set);
	void tagRemoveAll();

protected:
	virtual void paintEvent(QPaintEvent *e);

private slots:
	void fillTags();
	void tagTriggered(QAction *action);
};

#endif
