/*
 * Retroshare Posted Dialog
 *
 * Copyright 2012-2012 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#include "PostedDialog.h"
#include "PostedItem.h"
#include "PostedGroupDialog.h"
#include "PostedListWidget.h"
#include "PostedUserNotify.h"
//#include "gui/channels/ShareKey.h"
//#include "gui/settings/rsharesettings.h"
//#include "gui/notifyqt.h"

#include <retroshare/rsposted.h>

/** Constructor */
PostedDialog::PostedDialog(QWidget *parent)
    : GxsGroupFrameDialog(rsPosted, parent)
{
//	connect(NotifyQt::getInstance(), SIGNAL(settingsChanged()), this, SLOT(settingsChanged()));

	settingsChanged();
}

PostedDialog::~PostedDialog()
{
}

UserNotify *PostedDialog::getUserNotify(QObject *parent)
{
	return new PostedUserNotify(rsPosted, parent);
}

QString PostedDialog::text(TextType type)
{
	switch (type) {
	case TEXT_NAME:
		return tr("Posted Links");
	case TEXT_NEW:
		return tr("Create Topic");
	case TEXT_TODO:
		return "<b>Open points:</b><ul>"
			   "<li>Subreddits/tag to posts support"
			   "<li>Picture Support"
			   "</ul>";

	case TEXT_YOUR_GROUP:
		return tr("My Topics");
	case TEXT_SUBSCRIBED_GROUP:
		return tr("Subscribed Topics");
	case TEXT_POPULAR_GROUP:
		return tr("Popular Topics");
	case TEXT_OTHER_GROUP:
		return tr("Other Topics");
	}

	return "";
}

QString PostedDialog::icon(IconType type)
{
	switch (type) {
	case ICON_NAME:
		return ":/images/posted_24.png";
	case ICON_NEW:
		return ":/images/posted_add_24.png";
	case ICON_YOUR_GROUP:
		return ":/images/folder16.png";
	case ICON_SUBSCRIBED_GROUP:
		return ":/images/folder_red.png";
	case ICON_POPULAR_GROUP:
		return ":/images/folder_green.png";
	case ICON_OTHER_GROUP:
		return ":/images/folder_yellow.png";
	case ICON_DEFAULT:
		return "";
	}

	return "";
}

void PostedDialog::settingsChanged()
{
	setSingleTab(true /*!Settings->getPostedOpenAllInNewTab()*/);
	setHideTabBarWithOneTab(true /*Settings->getPostedHideTabBarWithOneTab()*/);
}

GxsGroupDialog *PostedDialog::createNewGroupDialog(TokenQueue *tokenQueue)
{
	return new PostedGroupDialog(tokenQueue, this);
}

GxsGroupDialog *PostedDialog::createGroupDialog(TokenQueue *tokenQueue, RsTokenService *tokenService, GxsGroupDialog::Mode mode, RsGxsGroupId groupId)
{
	return new PostedGroupDialog(tokenQueue, tokenService, mode, groupId, this);
}

int PostedDialog::shareKeyType()
{
	return 0; //POSTED_KEY_SHARE;
}

GxsMessageFrameWidget *PostedDialog::createMessageFrameWidget(const RsGxsGroupId &groupId)
{
	return new PostedListWidget(groupId);
}

RsGxsCommentService *PostedDialog::getCommentService()
{
	return rsPosted;
}

QWidget *PostedDialog::createCommentHeaderWidget(const RsGxsGroupId &grpId, const RsGxsMessageId &msgId)
{
	return new PostedItem(NULL, 0, grpId, msgId, true);
}
