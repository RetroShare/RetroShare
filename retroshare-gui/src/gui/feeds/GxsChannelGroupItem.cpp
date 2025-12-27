/*******************************************************************************
 * retroshare-gui/src/gui/feeds/GxsChannelGroupItem.cpp                        *
 *                                                                             *
 * Copyright (c) 2008, Robert Fernie   <retroshare.project@gmail.com>          *
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

#include "gui/gxs/GxsIdDetails.h"
#include "GxsChannelGroupItem.h"
#include "ui_GxsChannelGroupItem.h"

#include "FeedHolder.h"
#include "util/qtthreadsutils.h"
#include "gui/common/FilesDefs.h"
#include "gui/NewsFeed.h"
#include "gui/RetroShareLink.h"
#include "util/DateTime.h"

/****
 * #define DEBUG_ITEM 1
 ****/

GxsChannelGroupItem::GxsChannelGroupItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsGroupId &groupId, bool isHome, bool autoUpdate) :
    GxsGroupFeedItem(feedHolder, feedId, groupId, isHome, rsGxsChannels, autoUpdate)
{
    mLoadingGroup = false;
    mLoadingStatus = LOADING_STATUS_NO_DATA;

    setup();
	requestGroup();
    addEventHandler();
}

void GxsChannelGroupItem::addEventHandler()
{
    mEventHandlerId = 0;
    rsEvents->registerEventsHandler( [this](std::shared_ptr<const RsEvent> event)
    {
        RsQThreadUtils::postToObject([=]()
        {
            const auto *e = dynamic_cast<const RsGxsChannelEvent*>(event.get());

            if(!e || e->mChannelGroupId != this->groupId())
                return;

            switch(e->mChannelEventCode)
            {
                case RsChannelEventCode::SUBSCRIBE_STATUS_CHANGED:
                case RsChannelEventCode::UPDATED_CHANNEL:
                case RsChannelEventCode::RECEIVED_PUBLISH_KEY:
                    mLoadingStatus = LOADING_STATUS_NO_DATA;
                    mGroup = RsGxsChannelGroup();
                    break;
                default:
                    break;
            }
        }, this );
    }, mEventHandlerId, RsEventType::GXS_CHANNELS );
}

void GxsChannelGroupItem::paintEvent(QPaintEvent *e)
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
GxsChannelGroupItem::~GxsChannelGroupItem()
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

void GxsChannelGroupItem::setup()
{
	/* Invoke the Qt Designer generated object setup routine */
	ui = new(Ui::GxsChannelGroupItem);
	ui->setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose, true);

	/* clear ui */
	ui->nameLabel->setText(tr("Loading"));
	ui->titleLabel->clear();
	ui->descLabel->clear();

	/* general ones */
	connect(ui->expandButton, SIGNAL(clicked()), this, SLOT(toggle()));
	connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(removeItem()));

	/* specific */
	connect(ui->subscribeButton, SIGNAL(clicked()), this, SLOT(subscribeChannel()));
	connect(ui->copyLinkButton, SIGNAL(clicked()), this, SLOT(copyGroupLink()));

	ui->expandFrame->hide();
}

void GxsChannelGroupItem::loadGroup()
{
    mLoadingGroup = true;

	RsThread::async([this]()
	{
		// 1 - get group data

		std::vector<RsGxsChannelGroup> groups;
		const std::list<RsGxsGroupId> groupIds = { groupId() };

		if(!rsGxsChannels->getChannelsInfo(groupIds,groups))
		{
			RsErr() << "PostedItem::loadGroup() ERROR getting data" << std::endl;
            mLoadingGroup = false;
            deferred_update();
            return;
		}

		if (groups.size() != 1)
		{
			std::cerr << "GxsGxsChannelGroupItem::loadGroup() Wrong number of Items";
			std::cerr << std::endl;
            deferred_update();
            mLoadingGroup = false;
            return;
		}
		RsGxsChannelGroup group(groups[0]);

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

QString GxsChannelGroupItem::groupName()
{
	return QString::fromUtf8(mGroup.mMeta.mGroupName.c_str());
}

void GxsChannelGroupItem::fill()
{
	/* fill in */

#ifdef DEBUG_ITEM
	std::cerr << "GxsChannelGroupItem::fill()";
	std::cerr << std::endl;
#endif

	RetroShareLink link = RetroShareLink::createGxsGroupLink(RetroShareLink::TYPE_CHANNEL, mGroup.mMeta.mGroupId, groupName());
	ui->nameLabel->setText(link.toHtml());

    ui->logoLabel->setEnableZoom(false);
    int desired_height = QFontMetricsF(font()).height() * ITEM_HEIGHT_FACTOR;
    ui->logoLabel->setFixedSize(ITEM_PICTURE_FORMAT_RATIO*desired_height,desired_height);

    ui->descLabel->setText(QString::fromUtf8(mGroup.mDescription.c_str()));

	if (mGroup.mImage.mData != NULL) {
		QPixmap chanImage;
		GxsIdDetails::loadPixmapFromData(mGroup.mImage.mData, mGroup.mImage.mSize, chanImage,GxsIdDetails::ORIGINAL);

        ui->logoLabel->setPixmap(chanImage);
	} else {
        ui->logoLabel->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/icons/feeds_channel.png"));
	}

	if (IS_GROUP_SUBSCRIBED(mGroup.mMeta.mSubscribeFlags)) {
		ui->subscribeButton->setEnabled(false);
	} else {
		ui->subscribeButton->setEnabled(true);
	}

	switch(mFeedId)
	{
	case NEWSFEED_CHANNELPUBKEYLIST:	ui->titleLabel->setText(tr("Publish permission received for channel: "));
										break ;

	case NEWSFEED_CHANNELNEWLIST:	 	ui->titleLabel->setText(tr("New Channel: "));
										break ;
	}

	if(mGroup.mMeta.mLastPost==0)
		ui->infoLastPost->setText(tr("Never"));
	else
		ui->infoLastPost->setText(DateTime::formatLongDateTime(mGroup.mMeta.mLastPost));

	if (mIsHome)
	{
		/* disable buttons */
		ui->clearButton->setEnabled(false);
	}
}

void GxsChannelGroupItem::toggle()
{
	expand(ui->expandFrame->isHidden());
}

void GxsChannelGroupItem::doExpand(bool open)
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

void GxsChannelGroupItem::subscribeChannel()
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsChannelGroupItem::subscribeChannel()";
	std::cerr << std::endl;
#endif

	subscribe();
}
