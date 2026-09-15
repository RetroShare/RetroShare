/*******************************************************************************
 * gui/FriendRequestsDialog.cpp                                                *
 *                                                                             *
 * Copyright (C) 2026 Retroshare Team <retroshare.project@gmail.com>           *
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

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDateTime>
#include <QVariant>
#include <QVariantMap>

#include <retroshare/rspeers.h>

#include "FriendRequestsDialog.h"
#include "gui/common/RSFeedWidget.h"
#include "gui/feeds/SecurityItem.h"
#include "gui/chat/ChatDialog.h"
#include "settings/rsharesettings.h"
#include "util/qtthreadsutils.h"

FriendRequestsDialog::FriendRequestsDialog(QWidget *parent)
    : QWidget(parent), mAuthEventHandlerId(0), mFriendEventHandlerId(0)
{
	QVBoxLayout *vLayout = new QVBoxLayout(this);
	vLayout->setContentsMargins(0, 0, 0, 0);
	vLayout->setSpacing(0);

	/* Header bar */
	QHBoxLayout *hLayout = new QHBoxLayout();
	hLayout->setContentsMargins(6, 4, 6, 4);
	hLayout->setSpacing(6);

	QLabel *headerIcon = new QLabel(this);
	headerIcon->setPixmap(QPixmap(":/images/user/user_request48.png").scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation));
	headerIcon->setMaximumSize(24, 24);
	hLayout->addWidget(headerIcon);

	QLabel *headerLabel = new QLabel(tr("Friend Requests"), this);
	QFont headerFont = headerLabel->font();
	headerFont.setPointSize(11);
	headerFont.setBold(true);
	headerLabel->setFont(headerFont);
	hLayout->addWidget(headerLabel);

	hLayout->addStretch();

	mClearAllButton = new QPushButton(tr("Clear All"), this);
	mClearAllButton->setToolTip(tr("Remove all friend request entries"));
	connect(mClearAllButton, SIGNAL(clicked()), this, SLOT(clearAll()));
	hLayout->addWidget(mClearAllButton);

	vLayout->addLayout(hLayout);

	/* Feed widget */
	mFeedWidget = new RSFeedWidget(this);
	mFeedWidget->setPlaceholderText(tr("No friend requests"));
	mFeedWidget->setSortRole(FEED_TREEWIDGET_SORTROLE, Qt::DescendingOrder);
	vLayout->addWidget(mFeedWidget);

	connect(mFeedWidget, SIGNAL(feedCountChanged()), this, SLOT(updateCount()));

	setLayout(vLayout);

	/* Register event handlers */
	rsEvents->registerEventsHandler(
	    [this](std::shared_ptr<const RsEvent> event) {
		    handleAuthSslEvent(event);
	    },
	    mAuthEventHandlerId, RsEventType::AUTHSSL_CONNECTION_AUTENTICATION);

	rsEvents->registerEventsHandler(
	    [this](std::shared_ptr<const RsEvent> event) {
		    handleFriendListEvent(event);
	    },
	    mFriendEventHandlerId, RsEventType::FRIEND_LIST);

	/* Load persistent state and keyring peers */
	loadStoredRequests();
}

FriendRequestsDialog::~FriendRequestsDialog()
{
	rsEvents->unregisterEventsHandler(mAuthEventHandlerId);
	rsEvents->unregisterEventsHandler(mFriendEventHandlerId);
}

QString FriendRequestsDialog::requestKey(const AttemptEntry &entry) const
{
	return entry.pgpId.isEmpty() ? entry.sslId : entry.pgpId;
}

bool FriendRequestsDialog::shouldIgnoreRequest(const AttemptEntry &entry) const
{
	if (!rsPeers)
		return true;

	RsPgpId pgpId(entry.pgpId.toStdString());
	RsPeerId sslId(entry.sslId.toStdString());

	if (!entry.pgpId.isEmpty())
	{
		if (pgpId == rsPeers->getGPGOwnId() || rsPeers->isPgpFriend(pgpId))
			return true;

		RsPeerDetails gpgDetails;
		if (rsPeers->getGPGDetails(pgpId, gpgDetails) && gpgDetails.accept_connection)
			return true;
	}

	if (!entry.sslId.isEmpty())
	{
		if (sslId == rsPeers->getOwnId() || rsPeers->isFriend(sslId))
			return true;

		std::list<RsPeerId> ownSslIds;
		if (rsPeers->getAssociatedSSLIds(rsPeers->getGPGOwnId(), ownSslIds))
		{
			for (const RsPeerId &ownSslId : ownSslIds)
				if (sslId == ownSslId)
					return true;
		}

		RsPeerDetails sslDetails;
		if (rsPeers->getPeerDetails(sslId, sslDetails))
		{
			if (sslDetails.id == rsPeers->getOwnId() ||
			    sslDetails.gpg_id == rsPeers->getGPGOwnId() ||
			    sslDetails.accept_connection)
				return true;
		}
	}

	return false;
}

void FriendRequestsDialog::loadStoredRequests()
{
	Settings->beginGroup("FriendRequests");
	QStringList rejected = Settings->value("Rejected").toStringList();
	QVariantList list = Settings->value("StoredAttempts").toList();
	Settings->endGroup();

	for (const QString &s : rejected)
		mRejectedPeers.insert(s);

	bool changedStored = false;
	for (const QVariant &v : list)
	{
		QVariantMap map = v.toMap();
		AttemptEntry entry;
		entry.pgpId = map["pgpId"].toString();
		entry.sslId = map["sslId"].toString();
		entry.sslCn = map["sslCn"].toString();
		entry.locator = map["locator"].toString();
		entry.timestamp = map["timestamp"].toLongLong();

		/* Ignore if rejected */
		if (mRejectedPeers.contains(requestKey(entry)))
		{
			changedStored = true;
			continue;
		}

		/* Ignore own locations and accepted peers that became stale while stored */
		if (shouldIgnoreRequest(entry))
		{
			changedStored = true;
			continue;
		}

		/* Ensure no duplicates */
		if (!mStoredAttempts.contains(entry))
		{
			mStoredAttempts.append(entry);
			addSecurityItem(entry);
		}
		else
			changedStored = true;
	}

	/* Scan PGP keyring for non-friend peers who have signed us */
	if (rsPeers)
	{
		std::list<RsPgpId> pgpIds;
		rsPeers->getGPGAllList(pgpIds);

		bool addedNew = false;
		for (const RsPgpId &pgpId : pgpIds)
		{
			RsPeerDetails details;
			if (!rsPeers->getGPGDetails(pgpId, details))
				continue;

			if (pgpId == rsPeers->getGPGOwnId() || !details.hasSignedMe || details.accept_connection)
				continue;

			QString pgpStr = QString::fromStdString(pgpId.toStdString());
			if (mRejectedPeers.contains(pgpStr))
				continue;

			AttemptEntry entry;
			entry.pgpId = pgpStr;
			entry.sslCn = QString::fromUtf8(details.name.c_str());
			entry.timestamp = QDateTime::currentDateTime().toMSecsSinceEpoch();

			RsPeerId sslId;
			std::list<RsPeerId> sslIds;
			if (rsPeers->getAssociatedSSLIds(pgpId, sslIds) && !sslIds.empty())
				sslId = sslIds.front();
			entry.sslId = QString::fromStdString(sslId.toStdString());

			if (shouldIgnoreRequest(entry))
				continue;

			if (!mStoredAttempts.contains(entry))
			{
				mStoredAttempts.append(entry);
				addSecurityItem(entry);
				addedNew = true;
			}
		}

		if (addedNew || changedStored)
			saveStoredRequests();
	}
	else if (changedStored)
		saveStoredRequests();

	updateCount();
}

void FriendRequestsDialog::saveStoredRequests()
{
	QVariantList list;
	for (const AttemptEntry &entry : mStoredAttempts)
	{
		QVariantMap map;
		map["pgpId"] = entry.pgpId;
		map["sslId"] = entry.sslId;
		map["sslCn"] = entry.sslCn;
		map["locator"] = entry.locator;
		map["timestamp"] = entry.timestamp;
		list.append(map);
	}

	Settings->beginGroup("FriendRequests");
	Settings->setValue("StoredAttempts", list);
	Settings->endGroup();
}

void FriendRequestsDialog::handleAuthSslEvent(std::shared_ptr<const RsEvent> event)
{
	RsQThreadUtils::postToObject([=]()
	{
		const RsAuthSslConnectionAutenticationEvent *pe =
		    dynamic_cast<const RsAuthSslConnectionAutenticationEvent *>(event.get());

		if (!pe || pe->mErrorCode != RsAuthSslError::NOT_A_FRIEND)
			return;

		QString pgpStr = QString::fromStdString(pe->mPgpId.toStdString());
		QString sslStr = QString::fromStdString(pe->mSslId.toStdString());

		AttemptEntry entry;
		entry.pgpId = pgpStr;
		entry.sslId = sslStr;
		entry.sslCn = QString::fromUtf8(pe->mSslCn.c_str());
		entry.locator = QString::fromStdString(pe->mLocator.toString());
		entry.timestamp = QDateTime::currentDateTime().toMSecsSinceEpoch();

		if (mRejectedPeers.contains(requestKey(entry)) || shouldIgnoreRequest(entry))
			return;

		int idx = mStoredAttempts.indexOf(entry);
		if (idx >= 0)
		{
			/* Update existing entry data */
			mStoredAttempts[idx].timestamp = entry.timestamp;
			if (!entry.sslCn.isEmpty())
				mStoredAttempts[idx].sslCn = entry.sslCn;
			if (!entry.locator.isEmpty())
				mStoredAttempts[idx].locator = entry.locator;

			saveStoredRequests();
		}
		else
		{
			mStoredAttempts.append(entry);
			saveStoredRequests();
			addSecurityItem(entry);
			updateCount();
		}
	}, this);
}

void FriendRequestsDialog::handleFriendListEvent(std::shared_ptr<const RsEvent> event)
{
	RsQThreadUtils::postToObject([=]()
	{
		auto fe = dynamic_cast<const RsFriendListEvent *>(event.get());
		if (!fe || !rsPeers)
			return;

		bool changed = false;
		QList<FeedItem *> itemsToRemove;

		for (auto it = mItemMap.begin(); it != mItemMap.end(); ++it)
		{
			const AttemptEntry &entry = it.value();
			if (shouldIgnoreRequest(entry))
			{
				itemsToRemove.append(it.key());
				mStoredAttempts.removeAll(entry);
				changed = true;
			}
		}

		for (FeedItem *item : itemsToRemove)
		{
			mFeedWidget->removeFeedItem(item);
			item->close();
		}

		if (changed)
		{
			saveStoredRequests();
			updateCount();
		}
	}, this);
}

void FriendRequestsDialog::addSecurityItem(const AttemptEntry &entry)
{
	RsPgpId gpgId(entry.pgpId.toStdString());
	RsPeerId sslId(entry.sslId.toStdString());

	SecurityItem *item = new SecurityItem(
	    this, FRIENDREQUESTS_FEEDID, gpgId, sslId, entry.sslCn.toStdString(),
	    entry.locator.toStdString(),
	    RsFeedTypeFlags::RS_FEED_ITEM_SEC_CONNECT_ATTEMPT, false);

	item->setAttribute(Qt::WA_DeleteOnClose, true);
	connect(item, SIGNAL(destroyed(QObject*)), this, SLOT(onItemDestroyed(QObject*)));

	mItemMap.insert(item, entry);

	QDateTime dt = QDateTime::fromMSecsSinceEpoch(entry.timestamp);
	mFeedWidget->addFeedItem(item, FEED_TREEWIDGET_SORTROLE, dt);
}

void FriendRequestsDialog::onItemDestroyed(QObject *obj)
{
	mItemMap.remove(static_cast<FeedItem *>(obj));
}

void FriendRequestsDialog::clearAll()
{
	for (const AttemptEntry &entry : mStoredAttempts)
	{
		QString rejKey = requestKey(entry);
		if (!rejKey.isEmpty())
			mRejectedPeers.insert(rejKey);
	}

	Settings->beginGroup("FriendRequests");
	Settings->setValue("Rejected", QStringList(mRejectedPeers.values()));
	Settings->endGroup();

	mStoredAttempts.clear();
	saveStoredRequests();

	mFeedWidget->clear();
	updateCount();
}

int FriendRequestsDialog::pendingRequestCount() const
{
	return mFeedWidget->feedItemCount();
}

void FriendRequestsDialog::updateCount()
{
	emit requestCountChanged(mFeedWidget->feedItemCount());
}

/* FeedHolder Interface Implementation */

QScrollArea *FriendRequestsDialog::getScrollArea()
{
	return nullptr;
}

void FriendRequestsDialog::deleteFeedItem(FeedItem *item, uint32_t /*type*/)
{
	if (!item)
		return;

	if (mItemMap.contains(item))
	{
		AttemptEntry entry = mItemMap.value(item);
		QString rejKey = requestKey(entry);
		if (!rejKey.isEmpty())
		{
			mRejectedPeers.insert(rejKey);
			Settings->beginGroup("FriendRequests");
			Settings->setValue("Rejected", QStringList(mRejectedPeers.values()));
			Settings->endGroup();
		}

		mStoredAttempts.removeAll(entry);
		saveStoredRequests();
	}

	mFeedWidget->removeFeedItem(item);
	item->close();
}

void FriendRequestsDialog::openChat(const RsPeerId &peerId)
{
	ChatDialog::chatFriend(ChatId(peerId));
}

void FriendRequestsDialog::openComments(uint32_t /*type*/, const RsGxsGroupId &/*groupId*/, const QVector<RsGxsMessageId> &/*msg_versions*/, const RsGxsMessageId &/*msgId*/, const QString &/*title*/)
{
	/* Not applicable for friend requests */
}
