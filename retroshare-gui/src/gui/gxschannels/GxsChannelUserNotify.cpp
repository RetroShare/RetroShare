/*******************************************************************************
 * retroshare-gui/src/gui/gxschannels/GxsChannelPostsUserNotify.cpp            *
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

#include "GxsChannelUserNotify.h"
#include "gui/MainWindow.h"

GxsChannelUserNotify::GxsChannelUserNotify(RsGxsIfaceHelper *ifaceImpl, QObject *parent) :
    GxsUserNotify(ifaceImpl, parent)
{
}

bool GxsChannelUserNotify::hasSetting(QString *name, QString *group)
{
	if (name) *name = tr("Channel Post");
	if (group) *group = "Channel";

	return true;
}

QIcon GxsChannelUserNotify::getIcon()
{
	return QIcon(":/icons/png/channels.png");
}

QIcon GxsChannelUserNotify::getMainIcon(bool hasNew)
{
    return hasNew ? QIcon(":/icons/png/channels-notify.png") : QIcon(":/icons/png/channels.png");
}

void GxsChannelUserNotify::iconClicked()
{
	MainWindow::showWindow(MainWindow::Channels);
}
