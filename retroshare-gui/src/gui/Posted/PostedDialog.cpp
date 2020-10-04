/*******************************************************************************
 * retroshare-gui/src/gui/Posted/PostedDialog.cpp                              *
 *                                                                             *
 * Copyright (C) 2013 by Robert Fernie       <retroshare.project@gmail.com>    *
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

#include "PostedDialog.h"
#include "PostedItem.h"
#include "PostedGroupDialog.h"
#include "PostedListWidget.h"
#include "PostedUserNotify.h"
#include "gui/gxs/GxsGroupShareKey.h"
#include "gui/settings/rsharesettings.h"
#include "gui/common/GroupTreeWidget.h"
#include "util/qtthreadsutils.h"

#include <retroshare/rsposted.h>

class PostedGroupInfoData : public RsUserdata
{
public:
	PostedGroupInfoData() : RsUserdata() {}

public:
	QMap<RsGxsGroupId, QIcon> mIcon;
	QMap<RsGxsGroupId, QString> mDescription;
};

/** Constructor */
PostedDialog::PostedDialog(QWidget *parent):
    GxsGroupFrameDialog(rsPosted, parent), mEventHandlerId(0)
{
	// Needs to be asynced because this function is likely to be called by another thread!
	rsEvents->registerEventsHandler(
	            [this](std::shared_ptr<const RsEvent> event)
	{ RsQThreadUtils::postToObject( [=]() { handleEvent_main_thread(event); }, this ); },
	            mEventHandlerId, RsEventType::GXS_POSTED );
}

void PostedDialog::handleEvent_main_thread(std::shared_ptr<const RsEvent> event)
{
	if(event->mType == RsEventType::GXS_POSTED)
	{
		const RsGxsPostedEvent *e = dynamic_cast<const RsGxsPostedEvent*>(event.get());
		if(!e) return;

		switch(e->mPostedEventCode)
		{
		case RsPostedEventCode::NEW_MESSAGE:
		case RsPostedEventCode::UPDATED_MESSAGE:        // [[fallthrough]];
		case RsPostedEventCode::READ_STATUS_CHANGED:   // [[fallthrough]];
			updateGroupStatisticsReal(e->mPostedGroupId); // update the list immediately
            break;

		case RsPostedEventCode::NEW_POSTED_GROUP:       // [[fallthrough]];
		case RsPostedEventCode::SUBSCRIBE_STATUS_CHANGED:   // [[fallthrough]];
            updateDisplay(true);
            break;

        case RsPostedEventCode::STATISTICS_CHANGED:
            updateGroupStatistics(e->mPostedGroupId);
            break;

		default: break;
		}
	}
}


PostedDialog::~PostedDialog()
{
	rsEvents->unregisterEventsHandler(mEventHandlerId);
}

UserNotify *PostedDialog::createUserNotify(QObject *parent)
{
	return new PostedUserNotify(rsPosted, this, parent);
}

QString PostedDialog::getHelpString() const
{
	QString hlp_str = tr("<h1><img width=\"32\" src=\":/icons/help_64.png\">&nbsp;&nbsp;Boards</h1>    \
    <p>The Boards service allows you to share images, blog posts & internet links, that spread among Retroshare nodes like forums and \
	 channels</p> \
	 <p>Posts can be commented by subscribed users. A promotion system also gives the opportunity to  \
	 enlight important links.</p> \
     <p>There is no restriction on which links are shared. Be careful when clicking on them.</p>\
    <p>Boards are kept for %1 days, and sync-ed over the last %2 days, unless you change this.</p>\
                ").arg(QString::number(rsPosted->getDefaultStoragePeriod()/86400)).arg(QString::number(rsPosted->getDefaultSyncPeriod()/86400));

	return hlp_str ;
}

QString PostedDialog::text(TextType type)
{
	switch (type) {
	case TEXT_NAME:
		return tr("Boards");
	case TEXT_NEW:
		return tr("Create Board");
	case TEXT_TODO:
		return "<b>Open points:</b><ul>"
		       "<li>Subreddits/tag to posts support"
		       "<li>Picture Support"
		       "<li>Navigate channel link"
		       "</ul>";

	case TEXT_YOUR_GROUP:
		return tr("My Boards");
	case TEXT_SUBSCRIBED_GROUP:
		return tr("Subscribed Boards");
	case TEXT_POPULAR_GROUP:
		return tr("Popular Boards");
	case TEXT_OTHER_GROUP:
		return tr("Other Boards");
	}

	return "";
}

QString PostedDialog::icon(IconType type)
{
	switch (type) {
	case ICON_NAME:
		return ":/icons/png/postedlinks.png";
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
		return ":/icons/png/posted.png";
	}

	return "";
}

bool PostedDialog::getGroupData(std::list<RsGxsGenericGroupData*>& groupInfo)
{
	std::vector<RsPostedGroup> groups;

    // request all group infos at once

    if(! rsPosted->getBoardsInfo(std::list<RsGxsGroupId>(),groups))
        return false;

 	/* Save groups to fill icons and description */

	for (auto& group: groups)
       groupInfo.push_back(new RsPostedGroup(group));

	return true;
}

bool PostedDialog::getGroupStatistics(const RsGxsGroupId& groupId,GxsGroupStatistic& stat)
{
    return rsPosted->getBoardStatistics(groupId,stat);
}

GxsGroupDialog *PostedDialog::createNewGroupDialog()
{
	return new PostedGroupDialog(this);
}

GxsGroupDialog *PostedDialog::createGroupDialog(GxsGroupDialog::Mode mode, RsGxsGroupId groupId)
{
	return new PostedGroupDialog(mode, groupId, this);
}

int PostedDialog::shareKeyType()
{
    return POSTED_KEY_SHARE;
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
	return new PostedItem(NULL, 0, grpId, msgId, true, false);
}

#ifdef TO_REMOVE
void PostedDialog::loadGroupSummaryToken(const uint32_t &token, std::list<RsGroupMetaData> &groupInfo, RsUserdata *&userdata)
{
	std::vector<RsPostedGroup> groups;
	rsPosted->getGroupData(token, groups);

	/* Save groups to fill description */
	PostedGroupInfoData *postedData = new PostedGroupInfoData;
	userdata = postedData;

	std::vector<RsPostedGroup>::iterator groupIt;
	for (groupIt = groups.begin(); groupIt != groups.end(); ++groupIt) {
		RsPostedGroup &group = *groupIt;
		groupInfo.push_back(group.mMeta);
		
		if (group.mGroupImage.mData != NULL) {
			QPixmap image;
			GxsIdDetails::loadPixmapFromData(group.mGroupImage.mData, group.mGroupImage.mSize, image,GxsIdDetails::ORIGINAL);
			postedData->mIcon[group.mMeta.mGroupId] = image;
		}

		if (!group.mDescription.empty()) {
			postedData->mDescription[group.mMeta.mGroupId] = QString::fromUtf8(group.mDescription.c_str());
		}
	}
}
#endif

void PostedDialog::groupInfoToGroupItemInfo(const RsGxsGenericGroupData *groupData, GroupItemInfo &groupItemInfo)
{
	GxsGroupFrameDialog::groupInfoToGroupItemInfo(groupData, groupItemInfo);

	const RsPostedGroup *postedGroupData = dynamic_cast<const RsPostedGroup*>(groupData);

	if (!postedGroupData)
    {
		std::cerr << "PostedDialog::groupInfoToGroupItemInfo() Failed to cast data to RsPostedGroup"<< std::endl;
		return;
	}

    if(postedGroupData->mGroupImage.mSize > 0)
    {
	QPixmap image;
	GxsIdDetails::loadPixmapFromData(postedGroupData->mGroupImage.mData, postedGroupData->mGroupImage.mSize, image,GxsIdDetails::ORIGINAL);
	groupItemInfo.icon        = image;
    }
    else
    groupItemInfo.icon        = FilesDefs::getIconFromQtResourcePath(":icons/png/postedlinks.png");

	groupItemInfo.description = QString::fromUtf8(postedGroupData->mDescription.c_str());
}
