/*******************************************************************************
 * retroshare-gui/src/gui/TheWire/WireUserNotify.cpp                           *
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

#include "retroshare/rswire.h"
#include "WireUserNotify.h"
#include "gui/MainWindow.h"
#include "gui/common/FilesDefs.h"

WireUserNotify::WireUserNotify(RsGxsIfaceHelper *ifaceImpl, const GxsStatisticsProvider *g, QObject *parent) :
    GxsUserNotify(ifaceImpl, g, parent)
{
}

bool WireUserNotify::hasSetting(QString *name, QString *group)
{
    if (name) *name = tr("Wire Post");
    if (group) *group = "Wire";

    return true;
}

QIcon WireUserNotify::getIcon()
{
    return FilesDefs::getIconFromQtResourcePath(":/icons/png/wire-circle.png");
}

QIcon WireUserNotify::getMainIcon(bool hasNew)
{
    return hasNew ? FilesDefs::getIconFromQtResourcePath(":/icons/png/wire-notify.png") : FilesDefs::getIconFromQtResourcePath(":/icons/png/wire-circle.png");
}

//QString PostedUserNotify::getTrayMessage(bool plural)
//{
//    return plural ? tr("You have %1 new board posts") : tr("You have %1 new board post");
//}

//QString PostedUserNotify::getNotifyMessage(bool plural)
//{
//    return plural ? tr("%1 new board post") : tr("%1 new board post");
//}

void WireUserNotify::iconClicked()
{
    MainWindow::showWindow(MainWindow::Wire);
}