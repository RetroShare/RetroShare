/*******************************************************************************
 * gui/feeds/PostedGroupItem.cpp                                               *
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

#include "PostedGroupItem.h"
#include "ui_PostedGroupItem.h"

#include "FeedHolder.h"
#include "util/qtthreadsutils.h"
#include "gui/RetroShareLink.h"
#include "gui/gxs/GxsIdDetails.h"
#include "gui/common/FilesDefs.h"
#include "util/DateTime.h"

/****
 * #define DEBUG_ITEM 1
 ****/

PostedGroupItem::PostedGroupItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsGroupId &groupId, bool isHome, bool autoUpdate) :
    GxsGroupFeedItem(feedHolder, feedId, groupId, isHome, rsPosted, autoUpdate)
{
    mLoadingGroup = false;
    mLoadingStatus = LOADING_STATUS_NO_DATA;

    setup();
}

void PostedGroupItem::paintEvent(QPaintEvent *e)
{
    /* This method employs a trick to trigger a deferred loading. The post and group is requested only
     * when actually displayed on the screen. */

    if(mLoadingStatus != LOADING_STATUS_FILLED && !mGroup.mMeta.mGroupId.isNull())
        mLoadingStatus = LOADING_STATUS_HAS_DATA;

    if(mGroup.mMeta.mGroupId.isNull() && !mLoadingGroup)
        loadGroup();

    switch(mLoadingStatus)
    {
    case LOADING_STATUS_FILLED:
    case LOADING_STATUS_NO_DATA:
    default:
        break;

    case LOADING_STATUS_HAS_DATA:
        fill();
        mLoadingStatus = LOADING_STATUS_FILLED;
        break;
    }

    GxsGroupFeedItem::paintEvent(e) ;
}
PostedGroupItem::~PostedGroupItem()
{
    auto timeout = std::chrono::steady_clock::now() + std::chrono::milliseconds(GROUP_ITEM_LOADING_TIMEOUT_ms);

    while( mLoadingGroup && std::chrono::steady_clock::now() < timeout)
    {
        RsDbg() << __PRETTY_FUNCTION__ << " is Waiting "
                << (mLoadingGroup ? "Group " : "")
                << "loading finished." << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
	delete(ui);
}

void PostedGroupItem::setup()
{
	/* Invoke the Qt Designer generated object setup routine */
	ui = new(Ui::PostedGroupItem);
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
	connect(ui->subscribeButton, SIGNAL(clicked()), this, SLOT(subscribePosted()));
	connect(ui->copyLinkButton, SIGNAL(clicked()), this, SLOT(copyGroupLink()));

    //ui->copyLinkButton->hide(); // No link type at this moment

	ui->expandFrame->hide();
}

void PostedGroupItem::loadGroup()
{
    mLoadingGroup = true;

	RsThread::async([this]()
	{
		// 1 - get group data

#ifdef DEBUG_FORUMS
		std::cerr << "Retrieving post data for post " << mThreadId << std::endl;
#endif

		std::vector<RsPostedGroup> groups;
		const std::list<RsGxsGroupId> groupIds = { groupId() };

		if(!rsPosted->getBoardsInfo(groupIds,groups))
		{
			RsErr() << "GxsPostedGroupItem::loadGroup() ERROR getting data" << std::endl;
            mLoadingGroup = false;
            deferred_update();
            return;
		}

		if (groups.size() != 1)
		{
			std::cerr << "GxsPostedGroupItem::loadGroup() Wrong number of Items";
			std::cerr << std::endl;
            mLoadingGroup = false;
            deferred_update();
            return;
		}
		RsPostedGroup group(groups[0]);

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

QString PostedGroupItem::groupName()
{
	return QString::fromUtf8(mGroup.mMeta.mGroupName.c_str());
}

void PostedGroupItem::fill()
{
	/* fill in */

#ifdef DEBUG_ITEM
	std::cerr << "PostedGroupItem::fill()";
	std::cerr << std::endl;
#endif

	// No link type at this moment
	RetroShareLink link = RetroShareLink::createGxsGroupLink(RetroShareLink::TYPE_POSTED, mGroup.mMeta.mGroupId, groupName());
	ui->nameLabel->setText(link.toHtml());
//	ui->nameLabel->setText(groupName());

	ui->descLabel->setText(QString::fromUtf8(mGroup.mDescription.c_str()));
	
    ui->logoLabel->setEnableZoom(false);
    int desired_height = QFontMetricsF(font()).height() * ITEM_HEIGHT_FACTOR;
    ui->logoLabel->setFixedSize(ITEM_PICTURE_FORMAT_RATIO*desired_height,desired_height);

    if (mGroup.mGroupImage.mData != NULL) {
		QPixmap postedImage;
		GxsIdDetails::loadPixmapFromData(mGroup.mGroupImage.mData, mGroup.mGroupImage.mSize, postedImage,GxsIdDetails::ORIGINAL);
		ui->logoLabel->setPixmap(QPixmap(postedImage));
	} else {
		ui->logoLabel->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/icons/feeds_board.png"));
	}

    if(mGroup.mMeta.mLastPost==0)
        ui->infoLastPost->setText(tr("Never"));
    else
        ui->infoLastPost->setText(DateTime::formatDateTime(mGroup.mMeta.mLastPost));

	//TODO - nice icon for subscribed group
//	if (IS_GROUP_PUBLISHER(mGroup.mMeta.mSubscribeFlags)) {
//		ui->logoLabel->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/icons/png/posted.png"));
//	} else {
//		ui->logoLabel->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/icons/png/posted.png"));
//	}

	if (IS_GROUP_SUBSCRIBED(mGroup.mMeta.mSubscribeFlags)) {
		ui->subscribeButton->setEnabled(false);
	} else {
		ui->subscribeButton->setEnabled(true);
	}

//	if (mIsNew)
//	{
		ui->titleLabel->setText(tr("New Board"));
//	}
//	else
//	{
//		ui->titleLabel->setText(tr("Updated Board"));
//	}

	if (mIsHome)
	{
		/* disable buttons */
		ui->clearButton->setEnabled(false);
	}
}

void PostedGroupItem::toggle()
{
	expand(ui->expandFrame->isHidden());
}

void PostedGroupItem::doExpand(bool open)
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

void PostedGroupItem::subscribePosted()
{
#ifdef DEBUG_ITEM
	std::cerr << "PostedGroupItem::subscribePosted()";
	std::cerr << std::endl;
#endif

	subscribe();
}
