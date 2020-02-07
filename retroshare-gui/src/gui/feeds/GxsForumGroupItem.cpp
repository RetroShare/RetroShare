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

#include "FeedHolder.h"
#include "gui/RetroShareLink.h"

/****
 * #define DEBUG_ITEM 1
 ****/

GxsForumGroupItem::GxsForumGroupItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsGroupId &groupId, bool isHome, bool autoUpdate) :
    GxsGroupFeedItem(feedHolder, feedId, groupId, isHome, rsGxsForums, autoUpdate)
{
	setup();

	requestGroup();
}

GxsForumGroupItem::GxsForumGroupItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsForumGroup &group, bool isHome, bool autoUpdate) :
    GxsGroupFeedItem(feedHolder, feedId, group.mMeta.mGroupId, isHome, rsGxsForums, autoUpdate)
{
	setup();

	setGroup(group);
}

GxsForumGroupItem::~GxsForumGroupItem()
{
	delete(ui);
}

void GxsForumGroupItem::setup()
{
	/* Invoke the Qt Designer generated object setup routine */
	ui = new(Ui::GxsForumGroupItem);
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
	connect(ui->subscribeButton, SIGNAL(clicked()), this, SLOT(subscribeForum()));
	connect(ui->copyLinkButton, SIGNAL(clicked()), this, SLOT(copyGroupLink()));

	ui->expandFrame->hide();
}

bool GxsForumGroupItem::setGroup(const RsGxsForumGroup &group)
{
	if (groupId() != group.mMeta.mGroupId) {
		std::cerr << "GxsForumGroupItem::setContent() - Wrong id, cannot set post";
		std::cerr << std::endl;
		return false;
	}

	mGroup = group;
	fill();

	return true;
}

void GxsForumGroupItem::loadGroup(const uint32_t &token)
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsForumGroupItem::loadGroup()";
	std::cerr << std::endl;
#endif

	std::vector<RsGxsForumGroup> groups;
	if (!rsGxsForums->getGroupData(token, groups))
	{
		std::cerr << "GxsForumGroupItem::loadGroup() ERROR getting data";
		std::cerr << std::endl;
		return;
	}

	if (groups.size() != 1)
	{
		std::cerr << "GxsForumGroupItem::loadGroup() Wrong number of Items";
		std::cerr << std::endl;
		return;
	}

	setGroup(groups[0]);
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
		ui->forumlogo_label->setPixmap(QPixmap(":/icons/png/forums.png"));
	} else {
		ui->forumlogo_label->setPixmap(QPixmap(":/icons/png/forums-default.png"));
	}

	if (IS_GROUP_SUBSCRIBED(mGroup.mMeta.mSubscribeFlags)) {
		ui->subscribeButton->setEnabled(false);
	} else {
		ui->subscribeButton->setEnabled(true);
	}

//	if (mIsNew)
//	{
		ui->titleLabel->setText(tr("New Forum"));
//	}
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
		ui->expandButton->setIcon(QIcon(QString(":/icons/png/up-arrow.png")));
		ui->expandButton->setToolTip(tr("Hide"));
	}
	else
	{
		ui->expandFrame->hide();
		ui->expandButton->setIcon(QIcon(QString(":/icons/png/down-arrow.png")));
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
