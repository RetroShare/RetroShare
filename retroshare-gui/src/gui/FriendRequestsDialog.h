/*******************************************************************************
 * gui/FriendRequestsDialog.h                                                  *
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

#ifndef _FRIENDREQUESTSDIALOG_H
#define _FRIENDREQUESTSDIALOG_H

#include <QWidget>
#include <QSet>
#include <QList>
#include <QMap>
#include <QString>

#include <retroshare/rsevents.h>
#include <retroshare/rspeers.h>
#include "gui/feeds/FeedHolder.h"

class RSFeedWidget;
class FeedItem;
class QPushButton;

#define FRIENDREQUESTS_FEEDID 0x0020

struct AttemptEntry
{
	QString pgpId;
	QString sslId;
	QString sslCn;
	QString locator;
	qint64 timestamp;

	bool operator==(const AttemptEntry &other) const
	{
		if (!pgpId.isEmpty() && !other.pgpId.isEmpty())
			return pgpId == other.pgpId;
		return sslId == other.sslId;
	}
};

class FriendRequestsDialog : public QWidget, public FeedHolder
{
	Q_OBJECT

public:
	FriendRequestsDialog(QWidget *parent = 0);
	~FriendRequestsDialog() override;

	int pendingRequestCount() const;

	/* FeedHolder Interface */
	QScrollArea *getScrollArea() override;
	void deleteFeedItem(FeedItem *item, uint32_t type) override;
	void openChat(const RsPeerId &peerId) override;
	void openComments(uint32_t type, const RsGxsGroupId &groupId, const QVector<RsGxsMessageId> &msg_versions, const RsGxsMessageId &msgId, const QString &title) override;

signals:
	void requestCountChanged(int count);

private slots:
	void clearAll();
	void onItemDestroyed(QObject *obj);

private:
	void loadStoredRequests();
	void saveStoredRequests();
	void handleAuthSslEvent(std::shared_ptr<const RsEvent> event);
	void handleFriendListEvent(std::shared_ptr<const RsEvent> event);
	void addSecurityItem(const AttemptEntry &entry);
	void updateCount();

	RSFeedWidget *mFeedWidget;
	QPushButton *mClearAllButton;

	QSet<QString> mRejectedPeers;
	QList<AttemptEntry> mStoredAttempts;
	QMap<FeedItem *, AttemptEntry> mItemMap;

	RsEventsHandlerId_t mAuthEventHandlerId;
	RsEventsHandlerId_t mFriendEventHandlerId;
};

#endif
