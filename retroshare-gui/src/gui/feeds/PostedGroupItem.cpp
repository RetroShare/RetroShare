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
#include "gui/RetroShareLink.h"

/****
 * #define DEBUG_ITEM 1
 ****/

PostedGroupItem::PostedGroupItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsGroupId &groupId, bool isHome, bool autoUpdate) :
    GxsGroupFeedItem(feedHolder, feedId, groupId, isHome, rsPosted, autoUpdate)
{
	setup();

	requestGroup();
}

PostedGroupItem::PostedGroupItem(FeedHolder *feedHolder, uint32_t feedId, const RsPostedGroup &group, bool isHome, bool autoUpdate) :
    GxsGroupFeedItem(feedHolder, feedId, group.mMeta.mGroupId, isHome, rsPosted, autoUpdate)
{
	setup();

	setGroup(group);
}

PostedGroupItem::~PostedGroupItem()
{
	delete(ui);
}

void PostedGroupItem::setup()
{
	/* Invoke the Qt Designer generated object setup routine */
	ui = new(Ui::PostedGroupItem);
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
	connect(ui->subscribeButton, SIGNAL(clicked()), this, SLOT(subscribePosted()));
	connect(ui->copyLinkButton, SIGNAL(clicked()), this, SLOT(copyGroupLink()));

    //ui->copyLinkButton->hide(); // No link type at this moment

	ui->expandFrame->hide();
}

bool PostedGroupItem::setGroup(const RsPostedGroup &group)
{
	if (groupId() != group.mMeta.mGroupId) {
		std::cerr << "PostedGroupItem::setContent() - Wrong id, cannot set post";
		std::cerr << std::endl;
		return false;
	}

	mGroup = group;
	fill();

	return true;
}

void PostedGroupItem::loadGroup(const uint32_t &token)
{
#ifdef DEBUG_ITEM
	std::cerr << "PostedGroupItem::loadGroup()";
	std::cerr << std::endl;
#endif

	std::vector<RsPostedGroup> groups;
	if (!rsPosted->getGroupData(token, groups))
	{
		std::cerr << "PostedGroupItem::loadGroup() ERROR getting data";
		std::cerr << std::endl;
		return;
	}

	if (groups.size() != 1)
	{
		std::cerr << "PostedGroupItem::loadGroup() Wrong number of Items";
		std::cerr << std::endl;
		return;
	}

	setGroup(groups[0]);
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
	
	if (mGroup.mImage.mData != NULL) {
		QPixmap postedImage;
		postedImage.loadFromData(mGroup.mImage.mData, mGroup.mImage.mSize, "PNG");
		ui->logoLabel->setPixmap(QPixmap(postedImage));
	} else {
		ui->logoLabel->setPixmap(QPixmap(":/images/posted_64.png"));
	}


	//TODO - nice icon for subscribed group
//	if (IS_GROUP_PUBLISHER(mGroup.mMeta.mSubscribeFlags)) {
//		ui->logoLabel->setPixmap(QPixmap(":/images/posted_64.png"));
//	} else {
//		ui->logoLabel->setPixmap(QPixmap(":/images/posted_64.png"));
//	}

	if (IS_GROUP_SUBSCRIBED(mGroup.mMeta.mSubscribeFlags)) {
		ui->subscribeButton->setEnabled(false);
	} else {
		ui->subscribeButton->setEnabled(true);
	}

//	if (mIsNew)
//	{
		ui->titleLabel->setText(tr("New Posted"));
//	}
//	else
//	{
//		ui->titleLabel->setText(tr("Updated Posted"));
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
		ui->expandButton->setIcon(QIcon(QString(":/images/edit_remove24.png")));
		ui->expandButton->setToolTip(tr("Hide"));
	}
	else
	{
		ui->expandFrame->hide();
		ui->expandButton->setIcon(QIcon(QString(":/images/edit_add24.png")));
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
