/*******************************************************************************
 * retroshare-gui/src/gui/gxsforums/GxsForumsDialog.cpp                        *
 *                                                                             *
 * Copyright 2013 Robert Fernie        <retroshare.project@gmail.com>          *
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

#include "GxsForumsDialog.h"
#include "GxsForumGroupDialog.h"
#include "GxsForumThreadWidget.h"
#include "CreateGxsForumMsg.h"
#include "GxsForumUserNotify.h"
#include "gui/notifyqt.h"
#include "gui/gxs/GxsGroupShareKey.h"
#include "gui/common/GroupTreeWidget.h"

class GxsForumGroupInfoData : public RsUserdata
{
public:
	GxsForumGroupInfoData() : RsUserdata() {}

public:
	QMap<RsGxsGroupId, QString> mDescription;
};

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
			"<h1><img width=\"32\" src=\":/icons/help_64.png\">&nbsp;&nbsp;Forums</h1>               \
			<p>Retroshare Forums look like internet forums, but they work in a decentralized way</p>    \
			<p>You see forums your friends are subscribed to, and you forward subscribed forums to      \
			your friends. This automatically promotes interesting forums in the network.</p>            \
            <p>Forum messages are kept for %1 days and sync-ed over the last %2 days, unless you configure it otherwise.</p>\
                ").arg(QString::number(rsGxsForums->getDefaultStoragePeriod()/86400)).arg(QString::number(rsGxsForums->getDefaultSyncPeriod()/86400));

	return hlp_str ;	
}

void GxsForumsDialog::shareInMessage(const RsGxsGroupId& forum_id,const QList<RetroShareLink>& file_links)
{
	CreateGxsForumMsg *msgDialog = new CreateGxsForumMsg(forum_id,RsGxsMessageId(),RsGxsMessageId(),RsGxsId()) ;

	QString txt ;
	for(QList<RetroShareLink>::const_iterator it(file_links.begin());it!=file_links.end();++it)
		txt += (*it).toHtml() + "\n" ;

	if(!file_links.empty())
	{
		QString subject = (*file_links.begin()).name() ;
		msgDialog->setSubject(subject);
	}

	msgDialog->insertPastedText(txt);
	msgDialog->show();
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
		return ":/icons/png/forums.png";
	case ICON_NEW:
		return ":/icons/png/add.png";
	case ICON_YOUR_GROUP:
		return ":/icons/png/feedreader.png";
	case ICON_SUBSCRIBED_GROUP:
		return ":/icons/png/feed-subscribed.png";
	case ICON_POPULAR_GROUP:
		return ":/icons/png/feed-popular.png";
	case ICON_OTHER_GROUP:
		return ":/icons/png/feed-other.png";
	case ICON_DEFAULT:
		return ":/icons/png/forums-default.png";
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
	return 0; // Forums are public
}

GxsMessageFrameWidget *GxsForumsDialog::createMessageFrameWidget(const RsGxsGroupId &groupId)
{
	return new GxsForumThreadWidget(groupId);
}

void GxsForumsDialog::loadGroupSummaryToken(const uint32_t &token, std::list<RsGroupMetaData> &groupInfo, RsUserdata *&userdata)
{
	std::vector<RsGxsForumGroup> groups;
	rsGxsForums->getGroupData(token, groups);

	/* Save groups to fill description */
	GxsForumGroupInfoData *forumData = new GxsForumGroupInfoData;
	userdata = forumData;

	std::vector<RsGxsForumGroup>::iterator groupIt;
	for (groupIt = groups.begin(); groupIt != groups.end(); ++groupIt) {
		RsGxsForumGroup &group = *groupIt;
		groupInfo.push_back(group.mMeta);

		if (!group.mDescription.empty()) {
			forumData->mDescription[group.mMeta.mGroupId] = QString::fromUtf8(group.mDescription.c_str());
		}
	}
}

void GxsForumsDialog::groupInfoToGroupItemInfo(const RsGroupMetaData &groupInfo, GroupItemInfo &groupItemInfo, const RsUserdata *userdata)
{
	GxsGroupFrameDialog::groupInfoToGroupItemInfo(groupInfo, groupItemInfo, userdata);

	const GxsForumGroupInfoData *forumData = dynamic_cast<const GxsForumGroupInfoData*>(userdata);
	if (!forumData) {
		std::cerr << "GxsForumsDialog::groupInfoToGroupItemInfo() Failed to cast data to GxsForumGroupInfoData";
		std::cerr << std::endl;
		return;
	}

	QMap<RsGxsGroupId, QString>::const_iterator descriptionIt = forumData->mDescription.find(groupInfo.mGroupId);
	if (descriptionIt != forumData->mDescription.end()) {
		groupItemInfo.description = descriptionIt.value();
	}
	
	//if (IS_GROUP_ADMIN(groupInfo.mSubscribeFlags)) 
	//	groupItemInfo.icon = QIcon(":images/konv_message2.png");
	if ((IS_GROUP_PGP_AUTHED(groupInfo.mSignFlags)) || (IS_GROUP_MESSAGE_TRACKING(groupInfo.mSignFlags)) )
		groupItemInfo.icon = QIcon(":icons/png/forums-signed.png");

}
