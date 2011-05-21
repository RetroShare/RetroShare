/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2007, RetroShare Team
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

#ifndef _TAGSMENU_H
#define _TAGSMENU_H

#include <QMenu>

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
