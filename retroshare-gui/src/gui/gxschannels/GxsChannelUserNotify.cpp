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

#include "retroshare/rsgxschannels.h"
#include "GxsChannelUserNotify.h"
#include "gui/MainWindow.h"
#include "gui/common/FilesDefs.h"

GxsChannelUserNotify::GxsChannelUserNotify(RsGxsIfaceHelper *ifaceImpl, const GxsGroupFrameDialog *g, QObject *parent) :
    GxsUserNotify(ifaceImpl, g, parent)
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
	return FilesDefs::getIconFromQtResourcePath(":/icons/png/channel.png");
}

QIcon GxsChannelUserNotify::getMainIcon(bool hasNew)
{
    return hasNew ? FilesDefs::getIconFromQtResourcePath(":/icons/png/channels-notify.png") : FilesDefs::getIconFromQtResourcePath(":/icons/png/channel.png");
}

void GxsChannelUserNotify::iconClicked()
{
	MainWindow::showWindow(MainWindow::Channels);
}
