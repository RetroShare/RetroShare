/*******************************************************************************
 * gui/feeds/GxsCircleItem.cpp                                                 *
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

#include "GxsCircleItem.h"
#include "ui_GxsCircleItem.h"

#include "FeedHolder.h"
#include "gui/notifyqt.h"
#include "gui/Circles/CreateCircleDialog.h"
#include "gui/gxs/GxsIdDetails.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include <iostream>

/****
 * #define DEBUG_ITEM 1
 ****/

#define CIRCLESDIALOG_GROUPUPDATE  3


GxsCircleItem::GxsCircleItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsCircleId &circleId, const RsGxsId &gxsId, const uint32_t type)
  :FeedItem(NULL), mFeedHolder(feedHolder), mFeedId(feedId), mType(type), mCircleId(circleId), mGxsId(gxsId)
{
	setup();
}


GxsCircleItem::~GxsCircleItem()
{
	delete(ui);
}

void GxsCircleItem::setup()
{
	/* Invoke the Qt Designer generated object setup routine */
	ui = new Ui::GxsCircleItem;
	ui->setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose, true);

	/* general ones */
	connect(ui->expandButton, SIGNAL(clicked()), this, SLOT(showCircleDetails()));
	connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(removeItem()));

	/* update gxs information */

	RsIdentityDetails idDetails ;
	QString idName ;
	if(rsIdentity->getIdDetails(mGxsId, idDetails))
		idName = QString::fromUtf8(idDetails.mNickname.c_str()) + " (ID=" + QString::fromStdString(mGxsId.toStdString()) + ")" ;
	else
		idName = QString::fromStdString(mGxsId.toStdString()) ;
	
	QPixmap pixmap ;
	if(idDetails.mAvatar.mSize == 0 || !GxsIdDetails::loadPixmapFromData(idDetails.mAvatar.mData, idDetails.mAvatar.mSize, pixmap,GxsIdDetails::SMALL))
		pixmap = GxsIdDetails::makeDefaultIcon(mGxsId,GxsIdDetails::SMALL);


	/* update circle information */

	RsGxsCircleDetails circleDetails;
	if (rsGxsCircles->getCircleDetails(mCircleId, circleDetails))
	{

		if (mType == RS_FEED_ITEM_CIRCLE_MEMB_REQ)
		{
			ui->titleLabel->setText(tr("You received a membership request for circle:"));
			ui->nameLabel->setText(QString::fromUtf8(circleDetails.mCircleName.c_str()));
			ui->gxsIdLabel->setText(idName);
			ui->iconLabel->setPixmap(pixmap);
			ui->gxsIdLabel->setId(mGxsId);



			ui->acceptButton->setToolTip(tr("Grant membership request"));
			ui->revokeButton->setToolTip(tr("Revoke membership request"));
			connect(ui->acceptButton, SIGNAL(clicked()), this, SLOT(grantCircleMembership()));
			connect(ui->revokeButton, SIGNAL(clicked()), this, SLOT(revokeCircleMembership()));
		}
		else if (mType == RS_FEED_ITEM_CIRCLE_INVIT_REC)
		{
			ui->titleLabel->setText(tr("You received an invitation for circle:"));
			ui->nameLabel->setText(QString::fromUtf8(circleDetails.mCircleName.c_str()));
			ui->gxsIdLabel->setText(idName);
			ui->iconLabel->setPixmap(pixmap);
			ui->gxsIdLabel->setId(mGxsId);

			ui->acceptButton->setToolTip(tr("Accept invitation"));
			connect(ui->acceptButton, SIGNAL(clicked()), this, SLOT(acceptCircleSubscription()));
			ui->revokeButton->setHidden(true);
		}

	}
	else
	{
		ui->titleLabel->setText(tr("Received event from unknown Circle:"));
		ui->nameLabel->setText(QString::fromStdString(mCircleId.toStdString()));
		ui->gxsIdLabel->setText(idName);
		ui->gxsIdLabel->setId(mGxsId);
	}

	/* Setup TokenQueue */
	mCircleQueue = new TokenQueue(rsGxsCircles->getTokenService(), this);

}

QString GxsCircleItem::uniqueIdentifier() const
{
    return "GxsCircle " + QString::fromStdString(mCircleId.toStdString()) + " " + QString::fromStdString(mGxsId.toStdString()) + " " + QString::number(mType);
}

void GxsCircleItem::removeItem()
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsCircleItem::removeItem()" << std::endl;
#endif

	if (mFeedHolder)
	{
		mFeedHolder->lockLayout(this, true);
		hide();
		mFeedHolder->lockLayout(this, false);

		mFeedHolder->deleteFeedItem(this, mFeedId);
	}
}

void GxsCircleItem::loadRequest(const TokenQueue * queue, const TokenRequest &req)
{
#ifdef ID_DEBUG
	std::cerr << "GxsCircleItem::loadRequest() UserType: " << req.mUserType;
	std::cerr << std::endl;
#endif
	if(queue == mCircleQueue)
	{
#ifdef ID_DEBUG
		std::cerr << "CirclesDialog::loadRequest() UserType: " << req.mUserType;
		std::cerr << std::endl;
#endif

		/* now switch on req */
		switch(req.mUserType)
		{
			case CIRCLESDIALOG_GROUPUPDATE:
				updateCircleGroup(req.mToken);
			break;

			default:
				std::cerr << "GxsCircleItem::loadRequest() ERROR: INVALID TYPE";
				std::cerr << std::endl;
			break;
		}
	}
}


/*********** SPECIFIC FUNCTIONS ***********************/

void GxsCircleItem::showCircleDetails()
{
	CreateCircleDialog dlg;

	dlg.editExistingId(RsGxsGroupId(mCircleId), true, mType != RS_FEED_ITEM_CIRCLE_MEMB_REQ) ;
	dlg.exec();
}

void GxsCircleItem::acceptCircleSubscription()
{
	if (rsGxsCircles->requestCircleMembership(mGxsId, mCircleId))
		removeItem();
}

void GxsCircleItem::grantCircleMembership()
{

    RsTokReqOptions opts;
    opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

    std::list<RsGxsGroupId> grps ;
    grps.push_back(RsGxsGroupId(mCircleId));

    uint32_t token;
    mCircleQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, grps, CIRCLESDIALOG_GROUPUPDATE);

    CircleUpdateOrder c ;
    c.token = token ;
    c.gxs_id = mGxsId ;
    c.action = CircleUpdateOrder::GRANT_MEMBERSHIP ;

    mCircleUpdates[token] = c ;
}

void GxsCircleItem::revokeCircleMembership()
{

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

	std::list<RsGxsGroupId> grps;
	grps.push_back(RsGxsGroupId(mCircleId));

	uint32_t token;
	mCircleQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, grps, CIRCLESDIALOG_GROUPUPDATE);

	CircleUpdateOrder c;
	c.token = token;
	c.gxs_id = mGxsId;
	c.action = CircleUpdateOrder::REVOKE_MEMBERSHIP;

	mCircleUpdates[token] = c;
}

void GxsCircleItem::updateCircleGroup(const uint32_t& token)
{
#ifdef ID_DEBUG
	std::cerr << "Loading circle info" << std::endl;
#endif

	std::vector<RsGxsCircleGroup> circle_grp_v ;
	rsGxsCircles->getGroupData(token, circle_grp_v);

	if (circle_grp_v.empty())
	{
		std::cerr << "(EE) unexpected empty result from getGroupData. Cannot process circle now!" << std::endl;
		return ;
	}

	if (circle_grp_v.size() != 1)
	{
		std::cerr << "(EE) very weird result from getGroupData. Should get exactly one circle" << std::endl;
		return ;
	}

	RsGxsCircleGroup cg = circle_grp_v.front();

	/* now mark all the members */

	//std::set<RsGxsId> members = cg.mInvitedMembers;

	std::map<uint32_t,CircleUpdateOrder>::iterator it = mCircleUpdates.find(token) ;

	if(it == mCircleUpdates.end())
	{
		std::cerr << "(EE) Cannot find token " << token << " to perform group update!" << std::endl;
		return ;
	}

	if(it->second.action == CircleUpdateOrder::GRANT_MEMBERSHIP)
		cg.mInvitedMembers.insert(it->second.gxs_id) ;
	else if(it->second.action == CircleUpdateOrder::REVOKE_MEMBERSHIP)
		cg.mInvitedMembers.erase(it->second.gxs_id) ;
	else
	{
		std::cerr << "(EE) unrecognised membership action to perform: " << it->second.action << "!" << std::endl;
		return ;
	}

	uint32_t token2 ;
	rsGxsCircles->updateGroup(token2,cg) ;

	mCircleUpdates.erase(it) ;
}

