/*******************************************************************************
 * retroshare-gui/src/gui/WikiPoos/WikiUserNotify.cpp                         *
 *                                                                             *
 * Copyright 2014-2026 Retroshare Team <retroshare.project@gmail.com>          *
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

#include "retroshare/rswiki.h"
#include "retroshare/rsgxsifacehelper.h"
#include "WikiUserNotify.h"
#include "gui/common/FilesDefs.h"

WikiUserNotify::WikiUserNotify(RsGxsIfaceHelper *ifaceImpl, QObject *parent) :
    UserNotify(parent), mInterface(ifaceImpl), mNewCount(0)
{
}

bool WikiUserNotify::hasSetting(QString *name, QString *group)
{
	if (name) *name = tr("Wiki Page");
	if (group) *group = "Wiki";

	return true;
}

void WikiUserNotify::startUpdate()
{
	// Get unread message stats from wiki service
	mNewCount = 0;
	
	if (mInterface)
	{
		// Get group statistics and count new messages
		std::list<RsGroupMetaData> groupList;
		mInterface->getGroupMeta(RS_TOKREQ_ANSTYPE_SUMMARY, groupList);
		
		for (auto& meta : groupList)
		{
			if (meta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED)
			{
				// Count unread messages in subscribed groups
				std::vector<RsMsgMetaData> msgList;
				mInterface->getMsgMeta(RS_TOKREQ_ANSTYPE_SUMMARY, meta.mGroupId, msgList);
				
				for (auto& msg : msgList)
				{
					if (msg.mMsgStatus & GXS_SERV::GXS_MSG_STATUS_UNREAD)
					{
						mNewCount++;
					}
				}
			}
		}
	}
	
	update();
}

unsigned int WikiUserNotify::getNewCount()
{
	return mNewCount;
}

QIcon WikiUserNotify::getIcon()
{
	return FilesDefs::getIconFromQtResourcePath(":/icons/png/wiki.png");
}

QIcon WikiUserNotify::getMainIcon(bool hasNew)
{
	const QString basePath = ":/icons/png/wiki";
	const QString iconPath = basePath + (hasNew ? "-notify" : "") + ".png";
	return FilesDefs::getIconFromQtResourcePath(iconPath);
}

void WikiUserNotify::iconClicked()
{
	// Note: MainWindow::Wiki would need to be added to MainWindow::Page enum
	// For now, this is a placeholder that will be enabled when wiki is built with CONFIG += wikipoos
	// MainWindow::showWindow(MainWindow::Wiki);
}
