/*******************************************************************************
 * gui/feeds/GxsForumGroupItem.cpp                                             *
 *                                                                             *
 * Copyright (c) 2014, Retroshare Team <retroshare.project@gmail.com>          *
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

#include "GxsForumGroupItem.h"
#include "ui_GxsForumGroupItem.h"
#include "gui/NewsFeed.h"

#include "gui/common/FilesDefs.h"
#include "FeedHolder.h"
#include "gui/RetroShareLink.h"
#include "util/qtthreadsutils.h"

/****
 * #define DEBUG_ITEM 1
 ****/

GxsForumGroupItem::GxsForumGroupItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsGroupId &groupId, bool isHome, bool autoUpdate) :
    GxsGroupFeedItem(feedHolder, feedId, groupId, isHome, rsGxsForums, autoUpdate)
{
    mLoadingGroup = false;
    mLoadingStatus = NO_DATA;
    setup();
    addEventHandler();
}

GxsForumGroupItem::GxsForumGroupItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsGroupId &groupId, const std::list<RsGxsId>& added_moderators,const std::list<RsGxsId>& removed_moderators,bool isHome, bool autoUpdate):
    GxsGroupFeedItem(feedHolder, feedId, groupId, isHome, rsGxsForums, autoUpdate),
    mAddedModerators(added_moderators),
    mRemovedModerators(removed_moderators)
{
    mLoadingGroup = false;
    mLoadingStatus = NO_DATA;
    setup();
    addEventHandler();
}

void GxsForumGroupItem::addEventHandler()
{
    mEventHandlerId = 0;
    rsEvents->registerEventsHandler( [this](std::shared_ptr<const RsEvent> event)
    {
        RsQThreadUtils::postToObject([=]()
        {
            const auto *e = dynamic_cast<const RsGxsForumEvent*>(event.get());

            if(!e || e->mForumGroupId != this->groupId())
                return;

            switch(e->mForumEventCode)
            {
                case RsForumEventCode::SUBSCRIBE_STATUS_CHANGED:
                case RsForumEventCode::UPDATED_FORUM:
                case RsForumEventCode::MODERATOR_LIST_CHANGED:
                    mLoadingStatus = NO_DATA;
                    mGroup = RsGxsForumGroup();
                    break;
                default:
                    break;
            }
        }, this );
    }, mEventHandlerId, RsEventType::GXS_FORUMS );
}

void GxsForumGroupItem::paintEvent(QPaintEvent *e)
{
    /* This method employs a trick to trigger a deferred loading. The post and group is requested only
     * when actually displayed on the screen. */

    if(mLoadingStatus != FILLED && !mGroup.mMeta.mGroupId.isNull())
        mLoadingStatus = HAS_DATA;

    if(mGroup.mMeta.mGroupId.isNull() && !mLoadingGroup)
        loadGroup();

    switch(mLoadingStatus)
    {
    case FILLED:
    case NO_DATA:
    default:
        break;

    case HAS_DATA:
        fill();
        mLoadingStatus = FILLED;
        break;
    }

    GxsGroupFeedItem::paintEvent(e) ;
}
GxsForumGroupItem::~GxsForumGroupItem()
{
    auto timeout = std::chrono::steady_clock::now() + std::chrono::milliseconds(GROUP_ITEM_LOADING_TIMEOUT_ms);

    while( mLoadingGroup && std::chrono::steady_clock::now() < timeout )
    {
        RsDbg() << __PRETTY_FUNCTION__ << " is Waiting for data to load " << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    rsEvents->unregisterEventsHandler(mEventHandlerId);
    delete(ui);
}

void GxsForumGroupItem::setup()
{
	/* Invoke the Qt Designer generated object setup routine */
	ui = new(Ui::GxsForumGroupItem);
	ui->setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose, true);

	/* clear ui */
	ui->nameLabel->setText(tr("Loading..."));
	ui->titleLabel->clear();
	ui->descLabel->clear();

	/* general ones */
	connect(ui->expandButton, SIGNAL(clicked()), this, SLOT(toggle()));
	connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(removeItem()));

	/* specific */
	connect(ui->subscribeButton, SIGNAL(clicked()), this, SLOT(subscribeForum()));
	connect(ui->copyLinkButton, SIGNAL(clicked()), this, SLOT(copyGroupLink()));

	ui->expandFrame->hide();
}

void GxsForumGroupItem::loadGroup()
{
    mLoadingGroup = true;

 	RsThread::async([this]()
	{
		// 1 - get group data

#ifdef DEBUG_FORUMS
		std::cerr << "Retrieving post data for post " << mThreadId << std::endl;
#endif

		std::vector<RsGxsForumGroup> groups;
		const std::list<RsGxsGroupId> forumIds = { groupId() };

		if(!rsGxsForums->getForumsInfo(forumIds,groups))
		{
			RsErr() << "GxsForumGroupItem::loadGroup() ERROR getting data" << std::endl;
            mLoadingGroup = false;
            return;
		}

		if (groups.size() != 1)
		{
			std::cerr << "GxsForumGroupItem::loadGroup() Wrong number of Items";
			std::cerr << std::endl;
            mLoadingGroup = false;
            return;
		}
        RsGxsForumGroup group(groups[0]);// no reference to teporary accross threads!

		RsQThreadUtils::postToObject( [group,this]()
		{
			/* Here it goes any code you want to be executed on the Qt Gui
			 * thread, for example to update the data model with new information
			 * after a blocking call to RetroShare API complete */

            mGroup = group;
            mLoadingGroup = false;

		}, this );
	});
}

QString GxsForumGroupItem::groupName()
{
	return QString::fromUtf8(mGroup.mMeta.mGroupName.c_str());
}

void GxsForumGroupItem::fill()
{
	/* fill in */

#ifdef DEBUG_ITEM
	std::cerr << "GxsForumGroupItem::fill()";
	std::cerr << std::endl;
#endif

	RetroShareLink link = RetroShareLink::createGxsGroupLink(RetroShareLink::TYPE_FORUM, mGroup.mMeta.mGroupId, groupName());
	ui->nameLabel->setText(link.toHtml());

	ui->descLabel->setText(QString::fromUtf8(mGroup.mDescription.c_str()));

	if (IS_GROUP_PUBLISHER(mGroup.mMeta.mSubscribeFlags)) {
        ui->forumlogo_label->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/icons/png/forums.png"));
	} else {
        ui->forumlogo_label->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/icons/png/forums-default.png"));
	}

	if (IS_GROUP_SUBSCRIBED(mGroup.mMeta.mSubscribeFlags)) {
		ui->subscribeButton->setEnabled(false);
	} else {
		ui->subscribeButton->setEnabled(true);
	}

    if(feedId() == NEWSFEED_UPDATED_FORUM)
    {
        if(!mAddedModerators.empty() || !mRemovedModerators.empty())
        {
			ui->titleLabel->setText(tr("Moderator list changed"));
            ui->moderatorList_GB->show();

            QString msg;

            if(!mAddedModerators.empty())
            {
                msg += "<b>Added moderators:</b>" ;
                msg += "<p>";
                for(auto& gxsid: mAddedModerators)
                {
                    RsIdentityDetails det;
                    if(rsIdentity->getIdDetails(gxsid,det))
						msg += QString::fromUtf8(det.mNickname.c_str())+" ("+QString::fromStdString(gxsid.toStdString())+"), ";
					else
						msg += QString("[Unknown name]") + " ("+QString::fromStdString(gxsid.toStdString())+"), ";
                }
                msg.resize(msg.size()-2);
                msg += "</p>";
            }
			if(!mRemovedModerators.empty())
            {
                msg += "<b>Removed moderators:</b>" ;
                msg += "<p>";
                for(auto& gxsid: mRemovedModerators)
                {
					RsIdentityDetails det;

                    if( rsIdentity->getIdDetails(gxsid,det))
						msg += QString::fromUtf8(det.mNickname.c_str())+" ("+QString::fromStdString(gxsid.toStdString())+"), ";
					else
						msg += QString("[Unknown name]") + " ("+QString::fromStdString(gxsid.toStdString())+"), ";
                }
                msg.resize(msg.size()-2);
                msg += "</p>";
            }
            ui->moderatorList_TE->setText(msg);
        }
		else
        {
            ui->moderatorList_GB->hide();

			ui->titleLabel->setText(tr("Forum updated"));
            ui->moderatorList_GB->hide();
		}
    }
	else
    {
		ui->titleLabel->setText(tr("New Forum"));
		ui->moderatorList_GB->hide();
    }

//	else
//	{
//		ui->titleLabel->setText(tr("Updated Forum"));
//	}

	if (mIsHome)
	{
		/* disable buttons */
		ui->clearButton->setEnabled(false);
	}
}
void GxsForumGroupItem::toggle()
{
	expand(ui->expandFrame->isHidden());
}

void GxsForumGroupItem::doExpand(bool open)
{
	if (mFeedHolder)
	{
		mFeedHolder->lockLayout(this, true);
	}

	if (open)
	{
		ui->expandFrame->show();
        ui->expandButton->setIcon(FilesDefs::getIconFromQtResourcePath(QString(":/icons/png/up-arrow.png")));
		ui->expandButton->setToolTip(tr("Hide"));
	}
	else
	{
		ui->expandFrame->hide();
        ui->expandButton->setIcon(FilesDefs::getIconFromQtResourcePath(QString(":/icons/png/down-arrow.png")));
		ui->expandButton->setToolTip(tr("Expand"));
	}

	emit sizeChanged(this);

	if (mFeedHolder)
	{
		mFeedHolder->lockLayout(this, false);
	}
}

void GxsForumGroupItem::subscribeForum()
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsForumGroupItem::subscribeForum()";
	std::cerr << std::endl;
#endif

	subscribe();
}
