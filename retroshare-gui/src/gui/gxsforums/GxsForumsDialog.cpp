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
#include "gui/gxs/GxsGroupShareKey.h"

/** Constructor */
GxsForumsDialog::GxsForumsDialog(QWidget *parent)
	: GxsGroupFrameDialog(rsGxsForums, parent)
{
	mCountChildMsgs = true;
}

GxsForumsDialog::~GxsForumsDialog()
{
}

QString GxsForumsDialog::getHelpString() const
{
	QString hlp_str = tr(
			"<h1><img width=\"32\" src=\":/images/64px_help.png\">&nbsp;&nbsp;Forums</h1>               \
			<p>Retroshare Forums look like internet forums, but they work in a decentralized way</p>    \
			<p>You see forums your friends are subscribed to, and you forward subscribed forums to      \
			your friends. This automatically promotes interesting forums in the network.</p>            \
			<p>Forums are either Authenticated (<img src=\":/images/konv_message2.png\" width=\"12\"/>) \
			or anonymous (<img src=\":/images/konversation.png\" width=\"12\"/>). The former            \
			class is more resistant to spamming because posts are                                       \
			cryptographically signed using a Retroshare pseudo-identity.</p>") ;

	return hlp_str ;	
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
