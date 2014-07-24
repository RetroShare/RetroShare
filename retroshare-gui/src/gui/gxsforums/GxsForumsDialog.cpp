/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008 Robert Fernie
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

#include "GxsForumsDialog.h"
#include "GxsForumGroupDialog.h"
#include "GxsForumThreadWidget.h"
#include "GxsForumUserNotify.h"
#include "gui/notifyqt.h"
#include "gui/channels/ShareKey.h"

/** Constructor */
GxsForumsDialog::GxsForumsDialog(QWidget *parent)
	: GxsGroupFrameDialog(rsGxsForums, parent)
{
}

GxsForumsDialog::~GxsForumsDialog()
{
}

UserNotify *GxsForumsDialog::getUserNotify(QObject *parent)
{
	return new GxsForumUserNotify(rsGxsForums, parent);
}

QString GxsForumsDialog::text(TextType type)
{
	switch (type) {
	case TEXT_NAME:
		return tr("Forums");
	case TEXT_NEW:
		return tr("Create Forum");
	case TEXT_TODO:
		return "<b>Open points:</b><ul>"
		       "<li>Restore forum keys"
		       "<li>Display AUTHD"
		       "<li>Navigate forum link"
		       "<li>Don't show own posts as unread"
		       "<li>Remove messages"
		       "</ul>";

	case TEXT_YOUR_GROUP:
		return tr("My Forums");
	case TEXT_SUBSCRIBED_GROUP:
		return tr("Subscribed Forums");
	case TEXT_POPULAR_GROUP:
		return tr("Popular Forums");
	case TEXT_OTHER_GROUP:
		return tr("Other Forums");
	}

	return "";
}

QString GxsForumsDialog::icon(IconType type)
{
	switch (type) {
	case ICON_NAME:
		return ":/images/konversation.png";
	case ICON_NEW:
		return ":/images/new_forum16.png";
	case ICON_YOUR_GROUP:
		return ":/images/folder16.png";
	case ICON_SUBSCRIBED_GROUP:
		return ":/images/folder_red.png";
	case ICON_POPULAR_GROUP:
		return ":/images/folder_green.png";
	case ICON_OTHER_GROUP:
		return ":/images/folder_yellow.png";
	case ICON_DEFAULT:
		return ":/images/konversation.png";
	}

	return "";
}

GxsGroupDialog *GxsForumsDialog::createNewGroupDialog(TokenQueue *tokenQueue)
{
	return new GxsForumGroupDialog(tokenQueue, this);
}

GxsGroupDialog *GxsForumsDialog::createGroupDialog(TokenQueue *tokenQueue, RsTokenService *tokenService, GxsGroupDialog::Mode mode, RsGxsGroupId groupId)
{
	return new GxsForumGroupDialog(tokenQueue, tokenService, mode, groupId, this);
}

int GxsForumsDialog::shareKeyType()
{
	return FORUM_KEY_SHARE;
}

GxsMessageFrameWidget *GxsForumsDialog::createMessageFrameWidget(const RsGxsGroupId &groupId)
{
	return new GxsForumThreadWidget(groupId);
}
