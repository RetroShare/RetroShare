/*******************************************************************************
 * retroshare-gui/src/gui/gxs/GxsFeedItem.h                                    *
 *                                                                             *
 * Copyright 2012-2013 by Robert Fernie   <retroshare.project@gmail.com>       *
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

#ifndef _GXS_GENERIC_FEED_ITEM_H
#define _GXS_GENERIC_FEED_ITEM_H

#include "GxsGroupFeedItem.h"

class GxsFeedItem : public GxsGroupFeedItem
{
	Q_OBJECT

public:
	/** Note parent can = NULL */
	GxsFeedItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsGroupId &groupId, const RsGxsMessageId &messageId, bool isHome, RsGxsIfaceHelper *iface, bool autoUpdate);
	virtual ~GxsFeedItem();

	RsGxsMessageId messageId() const { return mMessageId; }
    const QVector<RsGxsMessageId>& messageVersions() const { return mMessageVersions ; }

	//To be able to update with thread message when comment is received.
	void setMessageId( RsGxsMessageId id) {mMessageId = id;}
	void setMessageVersions( const QVector<RsGxsMessageId>& v) { mMessageVersions = v;}

protected:
	/* load message data */
	void requestMessage();
	void requestComment();

	virtual QString messageName() = 0;
	virtual void loadMessage() = 0;
	virtual void loadComment() = 0;

	/* GxsGroupFeedItem */
	//virtual bool isLoading();
	//virtual void fillDisplay(RsGxsUpdateBroadcastBase *updateBroadcastBase, bool complete);

protected slots:
	void comments(const QString &title);
	void copyMessageLink();

private:
	RsGxsMessageId mMessageId;
    QVector<RsGxsMessageId> mMessageVersions ;
	uint32_t mTokenTypeMessage;
	uint32_t mTokenTypeComment;
};

Q_DECLARE_METATYPE(RsGxsMessageId)

#endif
