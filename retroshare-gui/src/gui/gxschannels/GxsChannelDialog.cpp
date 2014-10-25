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
#include "GxsChannelUserNotify.h"
#include "gui/gxs/GxsGroupShareKey.h"
#include "gui/feeds/GxsChannelPostItem.h"
#include "gui/settings/rsharesettings.h"
#include "gui/notifyqt.h"
#include "gui/common/GroupTreeWidget.h"

class GxsChannelGroupInfoData : public RsUserdata
{
public:
	GxsChannelGroupInfoData() : RsUserdata() {}

public:
	QMap<RsGxsGroupId, QIcon> mIcon;
};

/** Constructor */
GxsChannelDialog::GxsChannelDialog(QWidget *parent)
	: GxsGroupFrameDialog(rsGxsChannels, parent)
{
}

GxsChannelDialog::~GxsChannelDialog()
{
}

QString GxsChannelDialog::getHelpString() const
{
	QString hlp_str = tr("<h1><img width=\"32\" src=\":/images/64px_help.png\">&nbsp;&nbsp;Channels</h1>    \
    <p>Channels allow you to post data (e.g. movies, music) that will spread in the network</p>            \
    <p>You can see the channels your friends are subscribed to, and you automatically forward subscribed channels to \
    your friends. This promotes good channels in the network.</p>\
    <p>Only the channel's creator can post on that channel. Other peers                       \
    in the network can only read from it, unless the channel is private. You can however share \
	 the posting rights or the reading rights with friend Retroshare nodes.</p>\
	 <p>Channels can be made anonymous, or attached to a Retroshare identity so that readers can contact you if needed.\
	 Enable \"Allow Comments\" if you want to let users comment on your posts.</p>\
    ") ;
				
	return hlp_str ;
}

UserNotify *GxsChannelDialog::getUserNotify(QObject *parent)
{
	return new GxsChannelUserNotify(rsGxsChannels, parent);
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
		        "<li>Restore channel keys"
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
		return ":/images/channels24.png";
	case ICON_NEW:
		return ":/images/add_channel24.png";
	case ICON_YOUR_GROUP:
		return ":/images/folder16.png";
	case ICON_SUBSCRIBED_GROUP:
		return ":/images/folder_red.png";
	case ICON_POPULAR_GROUP:
		return ":/images/folder_green.png";
	case ICON_OTHER_GROUP:
		return ":/images/folder_yellow.png";
	case ICON_DEFAULT:
		return ":/images/channels.png";
	}

	return "";
}

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
	return new GxsChannelPostsWidget(groupId);
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

RsGxsCommentService *GxsChannelDialog::getCommentService()
{
	return rsGxsChannels;
}

QWidget *GxsChannelDialog::createCommentHeaderWidget(const RsGxsGroupId &grpId, const RsGxsMessageId &msgId)
{
	return new GxsChannelPostItem(NULL, 0, grpId, msgId, true, true);
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

void GxsChannelDialog::loadGroupSummaryToken(const uint32_t &token, std::list<RsGroupMetaData> &groupInfo, RsUserdata *&userdata)
{
	std::vector<RsGxsChannelGroup> groups;
	rsGxsChannels->getGroupData(token, groups);

	/* Save groups to fill icons */
	GxsChannelGroupInfoData *channelData = new GxsChannelGroupInfoData;
	userdata = channelData;

	std::vector<RsGxsChannelGroup>::iterator groupIt;
	for (groupIt = groups.begin(); groupIt != groups.end(); ++groupIt) {
		RsGxsChannelGroup &group = *groupIt;
		groupInfo.push_back(group.mMeta);

		if (group.mImage.mData != NULL) {
			QPixmap image;
			image.loadFromData(group.mImage.mData, group.mImage.mSize, "PNG");
			channelData->mIcon[group.mMeta.mGroupId] = image;
		}
	}
}

void GxsChannelDialog::groupInfoToGroupItemInfo(const RsGroupMetaData &groupInfo, GroupItemInfo &groupItemInfo, const RsUserdata *userdata)
{
	GxsGroupFrameDialog::groupInfoToGroupItemInfo(groupInfo, groupItemInfo, userdata);

	const GxsChannelGroupInfoData *channelData = dynamic_cast<const GxsChannelGroupInfoData*>(userdata);
	if (!channelData) {
		std::cerr << "GxsChannelDialog::groupInfoToGroupItemInfo() Failed to cast data to GxsChannelGroupInfoData";
		std::cerr << std::endl;
		return;
	}

	QMap<RsGxsGroupId, QIcon>::const_iterator iconIt = channelData->mIcon.find(groupInfo.mGroupId);
	if (iconIt != channelData->mIcon.end()) {
		groupItemInfo.icon = iconIt.value();
	}
}
