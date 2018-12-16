/*******************************************************************************
 * retroshare-gui/src/gui/gxsforums/GxsForumUserNotify.cpp                     *
 *                                                                             *
 * Copyright 2014 Retroshare Team      <retroshare.project@gmail.com>          *
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

#include "GxsForumUserNotify.h"
#include "gui/MainWindow.h"

GxsForumUserNotify::GxsForumUserNotify(RsGxsIfaceHelper *ifaceImpl, QObject *parent) :
    GxsUserNotify(ifaceImpl, parent)
{
	mCountChildMsgs = true;
}

bool GxsForumUserNotify::hasSetting(QString *name, QString *group)
{
	if (name) *name = tr("Forum Post");
	if (group) *group = "Forum";

	return true;
}

QIcon GxsForumUserNotify::getIcon()
{
    return QIcon(":/icons/png/forums.png");
}

QIcon GxsForumUserNotify::getMainIcon(bool hasNew)
{
    return hasNew ? QIcon(":/icons/png/forums-notify.png") : QIcon(":/icons/png/forums.png");
}

void GxsForumUserNotify::iconClicked()
{
	MainWindow::showWindow(MainWindow::Forums);
}
