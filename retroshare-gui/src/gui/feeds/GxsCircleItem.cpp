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

			if(circleDetails.mAmIAdmin)
			{
				ui->acceptButton->setToolTip(tr("Grant membership request"));
				ui->revokeButton->setToolTip(tr("Revoke membership request"));
				connect(ui->acceptButton, SIGNAL(clicked()), this, SLOT(grantCircleMembership()));
				connect(ui->revokeButton, SIGNAL(clicked()), this, SLOT(revokeCircleMembership()));
			}
            else
            {
				ui->acceptButton->setEnabled(false);
				ui->revokeButton->setEnabled(false);
            }
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
		else if (mType == RS_FEED_ITEM_CIRCLE_MEMB_LEAVE)
		{
			ui->titleLabel->setText(idName + tr(" has left this circle you belong to."));
			ui->nameLabel->setText(QString::fromUtf8(circleDetails.mCircleName.c_str()));
			ui->gxsIdLabel->setText(idName);
			ui->iconLabel->setPixmap(pixmap);
			ui->gxsIdLabel->setId(mGxsId);

			ui->acceptButton->setHidden(true);
			ui->revokeButton->setHidden(true);
		}
		else if (mType == RS_FEED_ITEM_CIRCLE_MEMB_JOIN)
		{
			ui->titleLabel->setText(idName + tr(" has join this circle you also belong to."));
			ui->nameLabel->setText(QString::fromUtf8(circleDetails.mCircleName.c_str()));
			ui->gxsIdLabel->setText(idName);
			ui->iconLabel->setPixmap(pixmap);
			ui->gxsIdLabel->setId(mGxsId);

			ui->acceptButton->setHidden(true);
			ui->revokeButton->setHidden(true);
		}
		else if (mType == RS_FEED_ITEM_CIRCLE_MEMB_REVOQUED)
		{
            if(rsIdentity->isOwnId(mGxsId))
				ui->titleLabel->setText(tr("Your identity %1 has been revoqued from this circle.").arg(idName));
            else
				ui->titleLabel->setText(tr("Identity %1 has been revoqued from this circle you belong to.").arg(idName));

			ui->nameLabel->setText(QString::fromUtf8(circleDetails.mCircleName.c_str()));
			ui->gxsIdLabel->setText(idName);
			ui->iconLabel->setPixmap(pixmap);
			ui->gxsIdLabel->setId(mGxsId);

			ui->acceptButton->setHidden(true);
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
}

uint64_t GxsCircleItem::uniqueIdentifier() const
{
    return hash_64bits("GxsCircle " + mCircleId.toStdString() + " " + mGxsId.toStdString() + " " + QString::number(mType).toStdString());
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
	RsThread::async([this]()
	{
        rsGxsCircles->inviteIdsToCircle(std::set<RsGxsId>( { mGxsId } ),mCircleId);
    });
}

void GxsCircleItem::revokeCircleMembership()
{
	RsThread::async([this]()
	{
        rsGxsCircles->revokeIdsFromCircle(std::set<RsGxsId>( { mGxsId } ),mCircleId);
    });
}

