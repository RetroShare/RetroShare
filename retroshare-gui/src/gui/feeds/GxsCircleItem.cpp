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
    :FeedItem(feedHolder,feedId,NULL), mType(type), mCircleId(circleId), mGxsId(gxsId)
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

	ui->membershipButton->setToolTip(tr("Grant membership request"));
	ui->inviteeButton->setToolTip(tr("Revoke membership"));

	connect(ui->membershipButton, SIGNAL(clicked()), this, SLOT(toggleCircleMembership()));
	connect(ui->inviteeButton, SIGNAL(clicked()), this, SLOT(toggleCircleInvite()));

	RsGxsCircleDetails circleDetails;

	if (rsGxsCircles->getCircleDetails(mCircleId, circleDetails))
	{
		ui->nameLabel->setText(QString::fromUtf8(circleDetails.mCircleName.c_str()) + " (ID: " + QString::fromStdString(circleDetails.mCircleId.toStdString()) + ")");

        // from here we can figure out if we already have requested membership or not

		if (mType == RS_FEED_ITEM_CIRCLE_MEMB_REQ)
		{
			ui->titleLabel->setText(tr("You received a membership request a circle you're administrating:"));
			ui->iconLabel->setPixmap(pixmap);
			ui->gxsIdLabel->setId(mGxsId);

			ui->inviteeButton->setHidden(false);
            ui->inviteeButton->setText(tr("Grant membership"));
            ui->inviteeButton->setToolTip(tr("Grant membership to this circle, for this identity"));

			ui->membershipButton->setHidden(true);
		}
		else if (mType == RS_FEED_ITEM_CIRCLE_INVITE_REC)
		{
			ui->titleLabel->setText(tr("You received an invitation to join this circle:"));
			ui->iconLabel->setPixmap(pixmap);
			ui->gxsIdLabel->setId(mGxsId);

			ui->membershipButton->setText(tr("Accept"));
			ui->membershipButton->setToolTip(tr("Accept invitation"));
			ui->membershipButton->setHidden(false);

			connect(ui->membershipButton, SIGNAL(clicked()), this, SLOT(requestCircleSubscription()));
			ui->inviteeButton->setHidden(true);
		}
		else if (mType == RS_FEED_ITEM_CIRCLE_MEMB_LEAVE)
		{
			ui->titleLabel->setText(idName + tr(" has left this circle."));
			ui->iconLabel->setPixmap(pixmap);
			ui->gxsIdLabel->setId(mGxsId);

			ui->membershipButton->setHidden(true);
			ui->inviteeButton->setHidden(true);
		}
		else if (mType == RS_FEED_ITEM_CIRCLE_MEMB_JOIN)
		{
            if(circleDetails.mAmIAdmin)
			{
				ui->titleLabel->setText(idName + tr(" which you invited, has joined this circle you're administrating."));
				ui->inviteeButton->setHidden(false);
				ui->inviteeButton->setText(tr("Revoke membership"));
				ui->inviteeButton->setToolTip(tr("Revoke membership for that identity"));
			}
            else
            {
				ui->inviteeButton->setHidden(true);
				ui->titleLabel->setText(idName + tr(" has joined this circle."));
            }

			ui->iconLabel->setPixmap(pixmap);
			ui->gxsIdLabel->setId(mGxsId);

			ui->membershipButton->setHidden(true);
		}
		else if (mType == RS_FEED_ITEM_CIRCLE_MEMB_REVOKED)
		{
			ui->titleLabel->setText(tr("Your identity %1 has been revoked from this circle.").arg(idName));

			ui->iconLabel->setPixmap(pixmap);
			ui->gxsIdLabel->setId(mGxsId);

			ui->membershipButton->setHidden(false);
			ui->membershipButton->setText(tr("Cancel membership request"));
			ui->membershipButton->setToolTip(tr("Cancel your membership request from that circle"));

			ui->inviteeButton->setHidden(true);
		}
		else if (mType == RS_FEED_ITEM_CIRCLE_MEMB_ACCEPTED)
		{
			ui->titleLabel->setText(tr("Your identity %1 as been accepted in this circle.").arg(idName));

			ui->iconLabel->setPixmap(pixmap);
			ui->gxsIdLabel->setId(mGxsId);

			ui->membershipButton->setHidden(false);
			ui->membershipButton->setText(tr("Cancel membership"));
			ui->membershipButton->setToolTip(tr("Cancel your membership from that circle"));

			ui->inviteeButton->setHidden(true);
		}

	}
	else
	{
		ui->titleLabel->setText(tr("Received event from unknown Circle:"));
		ui->nameLabel->setText(QString::fromStdString(mCircleId.toStdString()));
		ui->gxsIdLabel->setText(idName);
		ui->gxsIdLabel->setId(mGxsId);
	}
}

uint64_t GxsCircleItem::uniqueIdentifier() const
{
    return hash_64bits("GxsCircle " + mCircleId.toStdString() + " " + mGxsId.toStdString());
}

/*********** SPECIFIC FUNCTIONS ***********************/

void GxsCircleItem::showCircleDetails()
{
	CreateCircleDialog dlg;

	dlg.editExistingId(RsGxsGroupId(mCircleId), true, mType != RS_FEED_ITEM_CIRCLE_MEMB_REQ) ;
	dlg.exec();
}

void GxsCircleItem::requestCircleSubscription()
{
	rsGxsCircles->requestCircleMembership(mGxsId, mCircleId);
}

void GxsCircleItem::toggleCircleMembership()
{
    if(!rsIdentity->isOwnId(mGxsId))
    {
        RsErr() << __PRETTY_FUNCTION__ << ": inconsistent call: identity " << mGxsId << " doesn't belong to you" << std::endl;
        return;
    }

    if(mType == RS_FEED_ITEM_CIRCLE_INVITE_REC)
        rsGxsCircles->requestCircleMembership(mGxsId,mCircleId);
    else if(mType == RS_FEED_ITEM_CIRCLE_MEMB_REVOKED)
        rsGxsCircles->cancelCircleMembership(mGxsId,mCircleId);
	else
		RsErr() << __PRETTY_FUNCTION__ << ": inconsistent call. mType is " << mType << std::endl;
}

void GxsCircleItem::toggleCircleInvite()
{
	if(mType == RS_FEED_ITEM_CIRCLE_MEMB_JOIN)
		RsThread::async([this]()
		{
			rsGxsCircles->revokeIdsFromCircle(std::set<RsGxsId>( { mGxsId } ),mCircleId);
		});
	else if(mType == RS_FEED_ITEM_CIRCLE_MEMB_REQ)
		RsThread::async([this]()
		{
			rsGxsCircles->inviteIdsToCircle(std::set<RsGxsId>( { mGxsId } ),mCircleId);
		});
    else
        RsErr() << __PRETTY_FUNCTION__ << ": inconsistent call. mType is " << mType << std::endl;
}

