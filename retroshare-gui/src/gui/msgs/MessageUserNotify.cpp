/*******************************************************************************
 * retroshare-gui/src/gui/msgs/MessageUserNotify.cpp                           *
 *                                                                             *
 * Copyright (C) 2012 by Retroshare Team     <retroshare.project@gmail.com>    *
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

#include "gui/common/FilesDefs.h"
#include "MessageUserNotify.h"
#include "gui/notifyqt.h"
#include "gui/MainWindow.h"
#include "util/qtthreadsutils.h"
#include "gui/settings/rsharesettings.h"

#include "gui/msgs/MessageInterface.h"

MessageUserNotify::MessageUserNotify(QObject *parent) :
	UserNotify(parent)
{
	mEventHandlerId = 0;
	rsEvents->registerEventsHandler( [this](std::shared_ptr<const RsEvent> event) { RsQThreadUtils::postToObject( [this,event]() { handleEvent_main_thread(event); }); }, mEventHandlerId, RsEventType::MAIL_STATUS );
}

MessageUserNotify::~MessageUserNotify()
{
	rsEvents->unregisterEventsHandler(mEventHandlerId);
}

bool MessageUserNotify::hasSetting(QString *name, QString *group)
{
	if (name) *name = tr("Message");
	if (group) *group = "Message";

	return true;
}

QIcon MessageUserNotify::getIcon()
{
    return FilesDefs::getIconFromQtResourcePath(":/icons/png/messages.png");
}

QIcon MessageUserNotify::getMainIcon(bool hasNew)
{
    return hasNew ? FilesDefs::getIconFromQtResourcePath(":/icons/png/messages-notify.png") : FilesDefs::getIconFromQtResourcePath(":/icons/png/messages.png");
}

unsigned int MessageUserNotify::getNewCount()
{
	uint32_t newInboxCount = 0;
	uint32_t a, b, c, d, e; // dummies
	rsMail->getMessageCount(a, newInboxCount, b, c, d, e);

	return newInboxCount;
}

QString MessageUserNotify::getTrayMessage(bool plural)
{
	return plural ? tr("You have %1 new mails") : tr("You have %1 new mail");
}

QString MessageUserNotify::getNotifyMessage(bool plural)
{
	return plural ? tr("%1 new mails") : tr("%1 new mail");
}

void MessageUserNotify::iconClicked()
{
	MainWindow::showWindow(MainWindow::Messages);
}

void MessageUserNotify::handleEvent_main_thread(std::shared_ptr<const RsEvent> event)
{
	if(event->mType != RsEventType::MAIL_STATUS) {
		return;
	}

	const RsMailStatusEvent *fe = dynamic_cast<const RsMailStatusEvent*>(event.get());
	if (!fe) {
		return;
	}

	std::set<RsMailMessageId>::const_iterator it;

	switch (fe->mMailStatusEventCode) {
	case RsMailStatusEventCode::NEW_MESSAGE:
		for (it = fe->mChangedMsgIds.begin(); it != fe->mChangedMsgIds.end(); ++it) {
			MessageInfo msgInfo;
			if (rsMail->getMessage(*it, msgInfo)) {
					NotifyQt::getInstance()->addToaster(RS_POPUP_MSG, msgInfo.msgId.c_str(), msgInfo.title.c_str(), msgInfo.msg.c_str() );
			}
		}
		break;
	case RsMailStatusEventCode::MESSAGE_CHANGED:
	case RsMailStatusEventCode::MESSAGE_REMOVED:
		updateIcon();
		break;
	case RsMailStatusEventCode::MESSAGE_SENT:
	case RsMailStatusEventCode::TAG_CHANGED:
	case RsMailStatusEventCode::MESSAGE_RECEIVED_ACK:
	case RsMailStatusEventCode::SIGNATURE_FAILED:
		break;
	}
}
