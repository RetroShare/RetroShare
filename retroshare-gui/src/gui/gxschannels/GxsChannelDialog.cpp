/*******************************************************************************
 * retroshare-gui/src/gui/gxschannels/GxsChannelDialog.cpp                     *
 *                                                                             *
 * Copyright 2013 by Robert Fernie     <retroshare.project@gmail.com>          *
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

#include <QMenu>
#include <QFileDialog>
#include <QMetaObject>

#include <retroshare/rsfiles.h>

#include "GxsChannelDialog.h"
#include "GxsChannelGroupDialog.h"
#include "GxsChannelPostsWidgetWithModel.h"
#include "CreateGxsChannelMsg.h"
#include "GxsChannelUserNotify.h"
#include "gui/gxs/GxsGroupShareKey.h"
#include "gui/feeds/GxsChannelPostItem.h"
#include "gui/settings/rsharesettings.h"
#include "gui/notifyqt.h"
#include "gui/common/GroupTreeWidget.h"
#include "util/qtthreadsutils.h"

// class GxsChannelGroupInfoData : public RsUserdata
// {
// public:
// 	GxsChannelGroupInfoData() : RsUserdata() {}
//
// public:
// 	QMap<RsGxsGroupId, QIcon> mIcon;
// 	QMap<RsGxsGroupId, QString> mDescription;
// };

/** Constructor */
GxsChannelDialog::GxsChannelDialog(QWidget *parent):
    GxsGroupFrameDialog(rsGxsChannels, parent, true), mEventHandlerId(0)
{
	// Needs to be asynced because this function is called by another thread!
	rsEvents->registerEventsHandler(
	            [this](std::shared_ptr<const RsEvent> event)
	{ RsQThreadUtils::postToObject([=]() { handleEvent_main_thread(event); }, this ); },
	            mEventHandlerId, RsEventType::GXS_CHANNELS );
}

void GxsChannelDialog::handleEvent_main_thread(std::shared_ptr<const RsEvent> event)
{
	const RsGxsChannelEvent *e = dynamic_cast<const RsGxsChannelEvent*>(event.get());

	if(e)
        switch(e->mChannelEventCode)
        {
        case RsChannelEventCode::STATISTICS_CHANGED:      // [[fallthrough]];
            updateDisplay(true);                          // no breaks, on purpose!

        case RsChannelEventCode::NEW_MESSAGE:             // [[fallthrough]];
		case RsChannelEventCode::UPDATED_MESSAGE:         // [[fallthrough]];
		case RsChannelEventCode::READ_STATUS_CHANGED:     // [[fallthrough]];
            updateGroupStatisticsReal(e->mChannelGroupId);// update the list immediately
            break;

        case RsChannelEventCode::RECEIVED_PUBLISH_KEY:    // [[fallthrough]];
        case RsChannelEventCode::NEW_CHANNEL:             // [[fallthrough]];
        case RsChannelEventCode::SUBSCRIBE_STATUS_CHANGED:// reloads group summary (calling GxsGroupFrameDialog parent method)
            updateDisplay(true);
            break;

        default:
            break;
        }


    const RsGxsChannelSearchResultEvent*f = dynamic_cast<const RsGxsChannelSearchResultEvent*>(event.get());

	if(nullptr != f)
        for(auto it:f->mSearchResultsMap)
			updateSearchResults(it.first);
}

GxsChannelDialog::~GxsChannelDialog()
{
	rsEvents->unregisterEventsHandler(mEventHandlerId);
}

QString GxsChannelDialog::getHelpString() const
{
	QString hlp_str = tr("<h1><img width=\"32\" src=\":/icons/help_64.png\">&nbsp;&nbsp;Channels</h1>    \
    <p>Channels allow you to post data (e.g. movies, music) that will spread in the network</p>            \
    <p>You can see the channels your friends are subscribed to, and you automatically forward subscribed channels to \
    your friends. This promotes good channels in the network.</p>\
    <p>Only the channel's creator can post on that channel. Other peers                       \
    in the network can only read from it, unless the channel is private. You can however share \
	 the posting rights or the reading rights with friend Retroshare nodes.</p>\
	 <p>Channels can be made anonymous, or attached to a Retroshare identity so that readers can contact you if needed.\
	 Enable \"Allow Comments\" if you want to let users comment on your posts.</p>\
    <p>Channel posts are kept for %1 days, and sync-ed over the last %2 days, unless you change this.</p>\
    <p>UI Tip: use Control + mouse wheel to control image size in the thumbnail view.</p>\
                ").arg(QString::number(rsGxsChannels->getDefaultStoragePeriod()/86400)).arg(QString::number(rsGxsChannels->getDefaultSyncPeriod()/86400));

	return hlp_str ;
}

UserNotify *GxsChannelDialog::createUserNotify(QObject *parent)
{
	return new GxsChannelUserNotify(rsGxsChannels,this, parent);
}

void GxsChannelDialog::shareOnChannel(const RsGxsGroupId& channel_id,const QList<RetroShareLink>& file_links)
{
	std::cerr << "Sharing file link on channel " << channel_id << ": Not yet implemented!" << std::endl;

	CreateGxsChannelMsg *msgDialog = new CreateGxsChannelMsg(channel_id) ;

	QString txt ;
	for(QList<RetroShareLink>::const_iterator it(file_links.begin());it!=file_links.end();++it)
		txt += (*it).toHtml() + "\n" ;

	if(!file_links.empty())
	{
		QString subject = (*file_links.begin()).name() ;
		msgDialog->addSubject(subject);
	}

	msgDialog->addHtmlText(txt);
	msgDialog->show();
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
		return ":/icons/png/channels.png";
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
		return ":/icons/png/channels.png";
	}

	return "";
}

GxsGroupDialog *GxsChannelDialog::createNewGroupDialog()
{
	return new GxsChannelGroupDialog(this);
}

GxsGroupDialog *GxsChannelDialog::createGroupDialog(GxsGroupDialog::Mode mode, RsGxsGroupId groupId)
{
	return new GxsChannelGroupDialog(mode, groupId, this);
}

int GxsChannelDialog::shareKeyType()
{
	return CHANNEL_KEY_SHARE;
}

GxsMessageFrameWidget *GxsChannelDialog::createMessageFrameWidget(const RsGxsGroupId &groupId)
{
	return new GxsChannelPostsWidgetWithModel(groupId,this);
}

void GxsChannelDialog::setDefaultDirectory()
{
    RsGxsGroupId grpId = groupId() ;
    if (grpId.isNull())
        return ;

    rsGxsChannels->setChannelDownloadDirectory(grpId,"") ;
}
void GxsChannelDialog::specifyDownloadDirectory()
{
    RsGxsGroupId grpId = groupId() ;
    if (grpId.isNull())
        return ;

    QString dir = QFileDialog::getExistingDirectory(NULL,tr("Select channel download directory")) ;

    if(dir.isNull())
        return ;

    rsGxsChannels->setChannelDownloadDirectory(grpId,std::string(dir.toUtf8())) ;
}
void GxsChannelDialog::setDownloadDirectory()
{
    RsGxsGroupId grpId = groupId() ;
    if (grpId.isNull())
        return ;

    QAction *action = qobject_cast<QAction*>(sender()) ;

    if(!action)
        return ;

    QString directory = action->data().toString() ;

    rsGxsChannels->setChannelDownloadDirectory(grpId,std::string(directory.toUtf8())) ;
}
void GxsChannelDialog::groupTreeCustomActions(RsGxsGroupId grpId, int subscribeFlags, QList<QAction*> &actions)
{
    bool isSubscribed = IS_GROUP_SUBSCRIBED(subscribeFlags);
    bool autoDownload ;
    rsGxsChannels->getChannelAutoDownload(grpId,autoDownload);

    if (isSubscribed)
    {
        QAction *action = autoDownload ? (new QAction(FilesDefs::getIconFromQtResourcePath(":/images/redled.png"), tr("Disable Auto-Download"), this))
                                       : (new QAction(FilesDefs::getIconFromQtResourcePath(":/images/start.png"),tr("Enable Auto-Download"), this));

        connect(action, SIGNAL(triggered()), this, SLOT(toggleAutoDownload()));
        actions.append(action);

        std::string dl_directory;
        rsGxsChannels->getChannelDownloadDirectory(grpId,dl_directory) ;

        QMenu *mnu = new QMenu(tr("Set download directory")) ;

        if(dl_directory.empty())
            mnu->addAction(FilesDefs::getIconFromQtResourcePath(":/images/start.png"),tr("[Default directory]"), this, SLOT(setDefaultDirectory())) ;
        else
            mnu->addAction(tr("[Default directory]"), this, SLOT(setDefaultDirectory())) ;

        std::list<SharedDirInfo> lst ;
        rsFiles->getSharedDirectories(lst) ;
        bool found = false ;

        for(std::list<SharedDirInfo>::const_iterator it(lst.begin());it!=lst.end();++it)
        {
            QAction *action = NULL;

            if(dl_directory == it->filename)
            {
                action = new QAction(FilesDefs::getIconFromQtResourcePath(":/images/start.png"),QString::fromUtf8(it->filename.c_str()),NULL) ;
                found = true ;
            }
            else
                action = new QAction(QString::fromUtf8(it->filename.c_str()),NULL) ;

            connect(action,SIGNAL(triggered()),this,SLOT(setDownloadDirectory())) ;
            action->setData(QString::fromUtf8(it->filename.c_str())) ;

            mnu->addAction(action) ;
        }

        if(!found && !dl_directory.empty())
        {
            QAction *action = new QAction(FilesDefs::getIconFromQtResourcePath(":/images/start.png"),QString::fromUtf8(dl_directory.c_str()),NULL) ;
            connect(action,SIGNAL(triggered()),this,SLOT(setDownloadDirectory())) ;
            action->setData(QString::fromUtf8(dl_directory.c_str())) ;

            mnu->addAction(action) ;
        }

        mnu->addAction(tr("Specify..."), this, SLOT(specifyDownloadDirectory())) ;

        actions.push_back( mnu->menuAction()) ;
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
	if (grpId.isNull()) return;

	bool autoDownload;
	if(!rsGxsChannels->getChannelAutoDownload(grpId, autoDownload))
	{
		std::cerr << __PRETTY_FUNCTION__ << " failed to get autodownload value "
		          << "for channel: " << grpId.toStdString() << std::endl;
		return;
	}

	RsThread::async([this, grpId, autoDownload]()
	{
		if(!rsGxsChannels->setChannelAutoDownload(grpId, !autoDownload))
		{
			std::cerr << __PRETTY_FUNCTION__ << " failed to set autodownload "
			          << "for channel: " << grpId << std::endl;
			return;
		}

		RsQThreadUtils::postToObject( [=]()
		{
			/* Here it goes any code you want to be executed on the Qt Gui
			 * thread, for example to update the data model with new information
			 * after a blocking call to RetroShare API complete, note that
			 * Qt::QueuedConnection is important!
			 */

			std::cerr << __PRETTY_FUNCTION__ << " Has been executed on GUI "
			          << "thread but was scheduled by async thread" << std::endl;
		}, this );
	});
}

bool GxsChannelDialog::getGroupStatistics(const RsGxsGroupId& groupId,GxsGroupStatistic& stat)
{
    return rsGxsChannels->getChannelStatistics(groupId,stat);
}

bool GxsChannelDialog::getGroupData(std::list<RsGxsGenericGroupData*>& groupInfo)
{
	std::vector<RsGxsChannelGroup> groups;

    // request all group infos at once

    if(! rsGxsChannels->getChannelsInfo(std::list<RsGxsGroupId>(),groups))
        return false;

 	/* Save groups to fill icons and description */

	for (auto& group: groups)
       groupInfo.push_back(new RsGxsChannelGroup(group));

    return true;
}

void GxsChannelDialog::groupInfoToGroupItemInfo(const RsGxsGenericGroupData *groupData, GroupItemInfo &groupItemInfo)
{
	GxsGroupFrameDialog::groupInfoToGroupItemInfo(groupData, groupItemInfo);

	const RsGxsChannelGroup *channelGroupData = dynamic_cast<const RsGxsChannelGroup*>(groupData);

	if (!channelGroupData)
    {
		std::cerr << "GxsChannelDialog::groupInfoToGroupItemInfo() Failed to cast data to GxsChannelGroupInfoData"<< std::endl;
		return;
	}

	if(channelGroupData->mImage.mSize > 0)
	{
		QPixmap image;
		GxsIdDetails::loadPixmapFromData(channelGroupData->mImage.mData, channelGroupData->mImage.mSize, image,GxsIdDetails::ORIGINAL);
		groupItemInfo.icon = image;
	}
	else
        groupItemInfo.icon = FilesDefs::getIconFromQtResourcePath(":icons/png/channel.png");

	groupItemInfo.description = QString::fromUtf8(channelGroupData->mDescription.c_str());
}

void GxsChannelDialog::clearDistantSearchResults(TurtleRequestId id)
{
    rsGxsChannels->clearDistantSearchResults(id);
}

TurtleRequestId GxsChannelDialog::distantSearch(const QString& search_string)
{
    return rsGxsChannels->turtleSearchRequest(search_string.toStdString()) ;
}

bool GxsChannelDialog::getDistantSearchResults(TurtleRequestId id, std::map<RsGxsGroupId,RsGxsGroupSearchResults>& group_infos)
{
    return rsGxsChannels->retrieveDistantSearchResults(id,group_infos);
}

RsGxsGenericGroupData *GxsChannelDialog::getDistantSearchResultGroupData(const RsGxsGroupId& group_id)
{
    RsGxsChannelGroup channel_group;

    if(rsGxsChannels->getDistantSearchResultGroupData(group_id,channel_group))
        return new RsGxsGenericGroupData(channel_group);
	else
        return nullptr;
}

void GxsChannelDialog::checkRequestGroup(const RsGxsGroupId& grpId)
{
    RsGxsChannelGroup distant_group;

	if( rsGxsChannels->getDistantSearchResultGroupData(grpId,distant_group)) // normally we should also check that the group meta is not already here.
    {
        std::cerr << "GxsChannelDialog::checkRequestGroup() sending turtle request for group data for group " << grpId << std::endl;
        rsGxsChannels->turtleGroupRequest(grpId);
    }
}
