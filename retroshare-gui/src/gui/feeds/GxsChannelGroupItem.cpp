/*******************************************************************************
 * gui/feeds/GxsChannelGroupItem.cpp                                           *
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
	setup();

	requestGroup();
}

GxsChannelGroupItem::GxsChannelGroupItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsChannelGroup &group, bool isHome, bool autoUpdate) :
    GxsGroupFeedItem(feedHolder, feedId, group.mMeta.mGroupId, isHome, rsGxsChannels, autoUpdate)
{
	setup();

	setGroup(group);
}

GxsChannelGroupItem::~GxsChannelGroupItem()
{
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

bool GxsChannelGroupItem::setGroup(const RsGxsChannelGroup &group)
{
	if (groupId() != group.mMeta.mGroupId) {
		std::cerr << "GxsChannelGroupItem::setContent() - Wrong id, cannot set post";
		std::cerr << std::endl;
		return false;
	}

	mGroup = group;
	fill();

	return true;
}

void GxsChannelGroupItem::loadGroup()
{
	RsThread::async([this]()
	{
		// 1 - get group data

		std::vector<RsGxsChannelGroup> groups;
		const std::list<RsGxsGroupId> groupIds = { groupId() };

		if(!rsGxsChannels->getChannelsInfo(groupIds,groups))
		{
			RsErr() << "PostedItem::loadGroup() ERROR getting data" << std::endl;
			return;
		}

		if (groups.size() != 1)
		{
			std::cerr << "GxsGxsChannelGroupItem::loadGroup() Wrong number of Items";
			std::cerr << std::endl;
			return;
		}
		RsGxsChannelGroup group(groups[0]);

		RsQThreadUtils::postToObject( [group,this]()
		{
			/* Here it goes any code you want to be executed on the Qt Gui
			 * thread, for example to update the data model with new information
			 * after a blocking call to RetroShare API complete */

			setGroup(group);

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

	ui->descLabel->setText(QString::fromUtf8(mGroup.mDescription.c_str()));

	if (mGroup.mImage.mData != NULL) {
		QPixmap chanImage;
		GxsIdDetails::loadPixmapFromData(mGroup.mImage.mData, mGroup.mImage.mSize, chanImage,GxsIdDetails::ORIGINAL);
        ui->logoLabel->setPixmap(chanImage);
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
