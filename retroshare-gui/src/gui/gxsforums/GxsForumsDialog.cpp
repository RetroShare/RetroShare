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
#include "util/qtthreadsutils.h"
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
    mEventHandlerId = 0;
    // Needs to be asynced because this function is likely to be called by another thread!

	rsEvents->registerEventsHandler(RsEventType::GXS_FORUMS, [this](std::shared_ptr<const RsEvent> event) {   RsQThreadUtils::postToObject( [=]() { handleEvent_main_thread(event); }, this ); }, mEventHandlerId );
}

void GxsForumsDialog::handleEvent_main_thread(std::shared_ptr<const RsEvent> event)
{
    if(event->mType == RsEventType::GXS_FORUMS)
    {
        const RsGxsForumEvent *e = dynamic_cast<const RsGxsForumEvent*>(event.get());

        if(!e)
            return;

        switch(e->mForumEventCode)
        {
		case RsForumEventCode::NEW_MESSAGE:
		case RsForumEventCode::UPDATED_MESSAGE:        // [[fallthrough]];
		case RsForumEventCode::READ_STATUS_CHANGED:
			updateGroupStatisticsReal(e->mForumGroupId); // update the list immediately
            break;

		case RsForumEventCode::NEW_FORUM:       // [[fallthrough]];
        case RsForumEventCode::SUBSCRIBE_STATUS_CHANGED:
            updateDisplay(true);
            break;

        case RsForumEventCode::STATISTICS_CHANGED:
            updateGroupStatistics(e->mForumGroupId);   // update the list when redraw less often than once every 2 mins
            break;

        default:
            break;
        }
    }
}

GxsForumsDialog::~GxsForumsDialog()
{
	rsEvents->unregisterEventsHandler(mEventHandlerId);
}

bool GxsForumsDialog::getGroupData(std::list<RsGxsGenericGroupData*>& groupInfo)
{
	std::vector<RsGxsForumGroup> groups;

	if(! rsGxsForums->getForumsInfo(std::list<RsGxsGroupId>(),groups))
		return false;

	for (auto& group: groups)
		groupInfo.push_back(new RsGxsForumGroup(group));

    return true;
}

bool GxsForumsDialog::getGroupStatistics(const RsGxsGroupId& groupId,GxsGroupStatistic& stat)
{
    return rsGxsForums->getForumStatistics(groupId,stat);
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

UserNotify *GxsForumsDialog::createUserNotify(QObject *parent)
{
	return new GxsForumUserNotify(rsGxsForums,this, parent);
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
		return ":/icons/png/forum.png";
	case ICON_NEW:
		return ":/icons/png/add.png";
	case ICON_YOUR_GROUP:
		return "";
	case ICON_SUBSCRIBED_GROUP:
		return "";
	case ICON_POPULAR_GROUP:
		return "";
	case ICON_OTHER_GROUP:
		return "";
	case ICON_SEARCH:
		return ":/images/find.png";
	case ICON_DEFAULT:
		return ":/icons/png/forums-default.png";
	}

	return "";
}

GxsGroupDialog *GxsForumsDialog::createNewGroupDialog()
{
	return new GxsForumGroupDialog(this);
}

GxsGroupDialog *GxsForumsDialog::createGroupDialog(GxsGroupDialog::Mode mode, RsGxsGroupId groupId)
{
	return new GxsForumGroupDialog(mode, groupId, this);
}

int GxsForumsDialog::shareKeyType()
{
	return 0; // Forums are public
}

GxsMessageFrameWidget *GxsForumsDialog::createMessageFrameWidget(const RsGxsGroupId &groupId)
{
	return new GxsForumThreadWidget(groupId);
}

void GxsForumsDialog::groupInfoToGroupItemInfo(const RsGxsGenericGroupData *groupData, GroupItemInfo &groupItemInfo)
{
	GxsGroupFrameDialog::groupInfoToGroupItemInfo(groupData, groupItemInfo);

	const RsGxsForumGroup *forumGroupData = dynamic_cast<const RsGxsForumGroup*>(groupData);

	if (!forumGroupData)
    {
		std::cerr << "GxsChannelDialog::groupInfoToGroupItemInfo() Failed to cast data to GxsChannelGroupInfoData"<< std::endl;
		return;
	}

	groupItemInfo.description = QString::fromUtf8(forumGroupData->mDescription.c_str());

	if(IS_GROUP_ADMIN(groupData->mMeta.mSubscribeFlags))
		groupItemInfo.icon = QIcon(":icons/png/forums.png");
	else if ((IS_GROUP_PGP_AUTHED(groupData->mMeta.mSignFlags)) || (IS_GROUP_MESSAGE_TRACKING(groupData->mMeta.mSignFlags)) )
		groupItemInfo.icon = QIcon(":icons/png/forums-signed.png");
}

