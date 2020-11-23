/*******************************************************************************
 * retroshare-gui/src/gui/Posted/PostedListWidget.cpp                          *
 *                                                                             *
 * Copyright (C) 2014 by Retroshare Team     <retroshare.project@gmail.com>    *
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

#include "retroshare/rsposted.h"
#include "PostedUserNotify.h"
#include "gui/MainWindow.h"
#include "gui/common/FilesDefs.h"

PostedUserNotify::PostedUserNotify(RsGxsIfaceHelper *ifaceImpl, const GxsGroupFrameDialog *g, QObject *parent) :
    GxsUserNotify(ifaceImpl, g, parent)
{
}

bool PostedUserNotify::hasSetting(QString *name, QString *group)
{
	if (name) *name = tr("Board Post");
	if (group) *group = "Board";

	return true;
}

QIcon PostedUserNotify::getIcon()
{
    return FilesDefs::getIconFromQtResourcePath(":/icons/png/posted.png");
}

QIcon PostedUserNotify::getMainIcon(bool hasNew)
{
    return hasNew ? FilesDefs::getIconFromQtResourcePath(":/icons/png/posted-notify.png") : FilesDefs::getIconFromQtResourcePath(":/icons/png/posted.png");
}

void PostedUserNotify::iconClicked()
{
	MainWindow::showWindow(MainWindow::Posted);
}
