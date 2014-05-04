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

#include "GxsChannelDialog.h"
#include "GxsChannelGroupDialog.h"
#include "GxsChannelPostsWidget.h"
#include "gui/channels/ShareKey.h"

/****
 * #define DEBUG_CHANNEL
 ***/

/** Constructor */
GxsChannelDialog::GxsChannelDialog(QWidget *parent)
	: GxsGroupFrameDialog(rsGxsChannels, parent), GxsServiceDialog(dynamic_cast<GxsCommentContainer *>(parent))
{
	//#TODO: add settings like forums
	setSingleTab(true);
}

GxsChannelDialog::~GxsChannelDialog()
{
}

QString GxsChannelDialog::text(TextType type)
{
	switch (type) {
	case TEXT_NAME:
		return tr("Channels");
	case TEXT_NEW:
		return tr("Create Channel");
	case TEXT_TODO:
		return "<b>Open points:</b><ul>"
		        "<li>Threaded load of messages"
		        "<li>Share key"
		        "<li>Restore channel keys"
		        "<li>Copy/navigate channel link"
		        "<li>Display count of unread messages"
		        "<li>Set all as read"
		        "<li>Set read/unread status"
		        "</ul>";

	case TEXT_YOUR_GROUP:
		return tr("My Channels");
	case TEXT_SUBSCRIBED_GROUP:
		return tr("Subscribed Channels");
	case TEXT_POPULAR_GROUP:
		return tr("Popular Channels");
	case TEXT_OTHER_GROUP:
		return tr("Other Channels");
	}

	return "";
}

QString GxsChannelDialog::icon(IconType type)
{
	switch (type) {
	case ICON_NAME:
		return ":/images/add_channel24.png";
	case ICON_NEW:
		return ":/images/new_forum16.png";
	case ICON_YOUR_GROUP:
		return ":/images/channelsblue.png";
	case ICON_SUBSCRIBED_GROUP:
		return ":/images/channelsred.png";
	case ICON_POPULAR_GROUP:
		return ":/images/channelsgreen.png";
	case ICON_OTHER_GROUP:
		return ":/images/channelsyellow.png";
	case ICON_DEFAULT:
		return ":/images/channels.png";
	}

	return "";
}

//UserNotify *GxsChannelDialog::getUserNotify(QObject *parent)
//{
//	return new ChannelUserNotify(parent);
//}

GxsGroupDialog *GxsChannelDialog::createNewGroupDialog(TokenQueue *tokenQueue)
{
	return new GxsChannelGroupDialog(tokenQueue, this);
}

GxsGroupDialog *GxsChannelDialog::createGroupDialog(TokenQueue *tokenQueue, RsTokenService *tokenService, GxsGroupDialog::Mode mode, RsGxsGroupId groupId)
{
	return new GxsChannelGroupDialog(tokenQueue, tokenService, mode, groupId, this);
}

int GxsChannelDialog::shareKeyType()
{
	return CHANNEL_KEY_SHARE;
}

GxsMessageFrameWidget *GxsChannelDialog::createMessageFrameWidget(const RsGxsGroupId &groupId)
{
	GxsChannelPostsWidget *widget = new GxsChannelPostsWidget(groupId);

	//#TODO: find better solution
	connect(widget, SIGNAL(commentLoad(RsGxsGroupId,RsGxsMessageId,QString)), this, SLOT(loadComment(RsGxsGroupId,RsGxsMessageId,QString)));

	return widget;
}

void GxsChannelDialog::loadComment(const RsGxsGroupId &grpId, const RsGxsMessageId &msgId, const QString &title)
{
	commentLoad(grpId, msgId, title);
}

void GxsChannelDialog::groupTreeCustomActions(RsGxsGroupId grpId, int subscribeFlags, QList<QAction*> &actions)
{
	bool isSubscribed = IS_GROUP_SUBSCRIBED(subscribeFlags);
	bool autoDownload = rsGxsChannels->getChannelAutoDownload(grpId);

	if (isSubscribed)
	{
		QAction *action = autoDownload ? (new QAction(QIcon(":/images/redled.png"), tr("Disable Auto-Download"), this))
		                               : (new QAction(QIcon(":/images/start.png"),tr("Enable Auto-Download"), this));

		connect(action, SIGNAL(triggered()), this, SLOT(toggleAutoDownload()));
		actions.append(action);
	}
}

void GxsChannelDialog::toggleAutoDownload()
{
	RsGxsGroupId grpId = groupId();
	if (grpId.isNull()) {
		return;
	}

	bool autoDownload = rsGxsChannels->getChannelAutoDownload(grpId);
	if (!rsGxsChannels->setChannelAutoDownload(grpId, !autoDownload))
	{
		std::cerr << "GxsChannelDialog::toggleAutoDownload() Auto Download failed to set";
		std::cerr << std::endl;
	}
}
