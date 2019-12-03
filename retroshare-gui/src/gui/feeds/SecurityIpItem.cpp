/*******************************************************************************
 * gui/feeds/SecurityIpItem.cpp                                                *
 *                                                                             *
 * Copyright (c) 2015, Retroshare Team <retroshare.project@gmail.com>          *
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

#include <QDateTime>
#include <QTimer>

#include "SecurityIpItem.h"
#include "FeedHolder.h"
#include"ui_SecurityIpItem.h"
#include "retroshare-gui/RsAutoUpdatePage.h"
#include "gui/connect/ConfCertDialog.h"
#include "util/DateTime.h"
#include "gui/common/PeerDefs.h"
#include "gui/common/RsBanListDefs.h"

#include <retroshare/rspeers.h>
#include <retroshare/rsbanlist.h>
#include <retroshare/rsnotify.h>

/*****
 * #define DEBUG_ITEM 1
 ****/

/** Constructor */
SecurityIpItem::SecurityIpItem(FeedHolder *parent, const RsPeerId &sslId, const std::string &ipAddr, uint32_t result, uint32_t type, bool isTest) :
    FeedItem(NULL), mParent(parent), mType(type), mSslId(sslId), mIpAddr(ipAddr), mResult(result), mIsTest(isTest),
    ui(new(Ui::SecurityIpItem))
{
	setup();
}

SecurityIpItem::SecurityIpItem(FeedHolder *parent, const RsPeerId &sslId, const std::string& ipAddr, const std::string& ipAddrReported, uint32_t type, bool isTest) :
    FeedItem(NULL), mParent(parent), mType(type), mSslId(sslId), mIpAddr(ipAddr), mIpAddrReported(ipAddrReported), mResult(0), mIsTest(isTest),
    ui(new(Ui::SecurityIpItem))
{
	setup();
}

void SecurityIpItem::setup()
{
	/* Invoke the Qt Designer generated object setup routine */
	ui->setupUi(this);

	ui->peerDetailsButton->setEnabled(false);

	/* general ones */
	connect(ui->expandButton, SIGNAL(clicked(void)), this, SLOT(toggle(void)));
	connect(ui->clearButton, SIGNAL(clicked(void)), this, SLOT(removeItem(void)));

	/* specific ones */
	connect(ui->peerDetailsButton, SIGNAL(clicked()), this, SLOT(peerDetails()));
	connect(ui->rsBanListButton, SIGNAL(banListChanged(QString)), this, SLOT(banIpListChanged(QString)));

	ui->avatar->setId(ChatId(mSslId));
	ui->rsBanListButton->setMode(RsBanListToolButton::LIST_WHITELIST, RsBanListToolButton::MODE_ADD);
	ui->rsBanListChangedLabel->hide();

	ui->expandFrame->hide();

	updateItemStatic();
	updateItem();
}

QString SecurityIpItem::uniqueIdentifier() const
{
    return "SecurityItem " + QString::number(mType) + " " + QString::fromStdString(mSslId.toStdString())
            + " " + QString::fromStdString(mIpAddr) + " " + QString::fromStdString(mIpAddrReported) ;
}

void SecurityIpItem::updateItemStatic()
{
	if (!rsPeers)
		return;

	/* fill in */
#ifdef DEBUG_ITEM
	std::cerr << "SecurityIpItem::updateItemStatic()";
	std::cerr << std::endl;
#endif

	/* Specific type */
	switch (mType) {
	case RS_FEED_ITEM_SEC_IP_BLACKLISTED:
		ui->rsBanListButton->setDisabled(mIsTest);
		ui->ipAddrReported->hide();
		ui->ipAddrReportedLabel->hide();
		break;
	case RS_FEED_ITEM_SEC_IP_WRONG_EXTERNAL_IP_REPORTED:
		ui->rsBanListButton->hide();
		break;
	default:
		std::cerr << "SecurityIpItem::updateItem() Wrong type" << std::endl;
	}

	QDateTime currentTime = QDateTime::currentDateTime();
	ui->timeLabel->setText(DateTime::formatLongDateTime(currentTime.toTime_t()));
}

void SecurityIpItem::updateItem()
{
	if (!rsPeers)
		return;

	/* fill in */
#ifdef DEBUG_ITEM
	std::cerr << "SecurityIpItem::updateItem()";
	std::cerr << std::endl;
#endif

	if(!RsAutoUpdatePage::eventsLocked()) {
		switch (mType) {
		case RS_FEED_ITEM_SEC_IP_BLACKLISTED:
			ui->titleLabel->setText(RsBanListDefs::resultString(mResult));
			ui->ipAddr->setText(QString::fromStdString(mIpAddr));

			if (!mIsTest) {
				switch (mResult) {
				case RSBANLIST_CHECK_RESULT_NOCHECK:
				case RSBANLIST_CHECK_RESULT_ACCEPTED:
					ui->rsBanListButton->hide();
					break;
				case RSBANLIST_CHECK_RESULT_NOT_WHITELISTED:
				case RSBANLIST_CHECK_RESULT_BLACKLISTED:
					ui->rsBanListButton->setVisible(ui->rsBanListButton->setIpAddress(QString::fromStdString(mIpAddr)));
					break;
				default:
					ui->rsBanListButton->hide();
				}
			}
			break;
		case RS_FEED_ITEM_SEC_IP_WRONG_EXTERNAL_IP_REPORTED:
			ui->titleLabel->setText(tr("Wrong external ip address reported"));
			ui->ipAddr->setText(QString::fromStdString(mIpAddr));
            ui->ipAddr->setToolTip(tr("<p>This is the external IP your Retroshare node thinks it is using.</p>")) ;
            ui->ipAddrReported->setText(QString::fromStdString(mIpAddrReported));
            ui->ipAddrReported->setToolTip(tr("<p>This is the IP your friend claims it is connected to. If you just changed IPs, this is a false warning. If not, that means your connection to this friend is forwarded by an intermediate peer, which would be suspicious.</p>")) ;
            break;
		default:
			std::cerr << "SecurityIpItem::updateItem() Wrong type" << std::endl;
		}

		RsPeerDetails details;
		if (!rsPeers->getPeerDetails(mSslId, details))
		{
			/* set peer name */
			ui->peer->setText(tr("Unknown Peer"));

			/* expanded Info */
			ui->peerID->setText(QString::fromStdString(mSslId.toStdString()));
			ui->peerName->setText(tr("Unknown Peer"));
			ui->locationLabel->setText(tr("Unknown Peer"));
		} else {
			/* set peer name */
			ui->peer->setText(PeerDefs::nameWithLocation(details));

			/* expanded Info */
			ui->peerID->setText(QString::fromStdString(details.id.toStdString()));
			ui->peerName->setText(QString::fromUtf8(details.name.c_str()));
			ui->location->setText(QString::fromUtf8(details.location.c_str()));

			/* Buttons */
			ui->peerDetailsButton->setEnabled(true);
		}
	}

	/* slow Tick  */
	int msec_rate = 10129;

	QTimer::singleShot( msec_rate, this, SLOT(updateItem(void)));
}

void SecurityIpItem::toggle()
{
	expand(ui->expandFrame->isHidden());
}

void SecurityIpItem::doExpand(bool open)
{
	if (mParent) {
		mParent->lockLayout(this, true);
	}

	if (open)
	{
		ui->expandFrame->show();
		ui->expandButton->setIcon(QIcon(":/icons/png/up-arrow.png"));
		ui->expandButton->setToolTip(tr("Hide"));
	}
	else
	{
		ui->expandFrame->hide();
		ui->expandButton->setIcon(QIcon(":/icons/png/down-arrow.png"));
		ui->expandButton->setToolTip(tr("Expand"));
	}

	emit sizeChanged(this);

	if (mParent) {
		mParent->lockLayout(this, false);
	}
}

void SecurityIpItem::removeItem()
{
#ifdef DEBUG_ITEM
	std::cerr << "SecurityIpItem::removeItem()";
	std::cerr << std::endl;
#endif

	mParent->lockLayout(this, true);
	hide();
	mParent->lockLayout(this, false);

	if (mParent)
	{
		mParent->deleteFeedItem(this, mFeedId);
	}
}

///*********** SPECIFIC FUNCTIONS ***********************/

void SecurityIpItem::peerDetails()
{
#ifdef DEBUG_ITEM
	std::cerr << "SecurityIpItem::peerDetails()";
	std::cerr << std::endl;
#endif

	RsPeerDetails details;
	if (rsPeers->getPeerDetails(mSslId, details)) {
		ConfCertDialog::showIt(mSslId, ConfCertDialog::PageDetails);
	}
}

void SecurityIpItem::banIpListChanged(const QString &ipAddress)
{
	ui->rsBanListChangedLabel->setText(tr("IP address %1 was added to the whitelist").arg(ipAddress));
	ui->rsBanListChangedLabel->show();
}
