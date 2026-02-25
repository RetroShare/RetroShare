/*******************************************************************************
 * gui/feeds/FeedItem.h                                                        *
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

#ifndef _FEED_ITEM_H
#define _FEED_ITEM_H

#include <retroshare/rsflags.h>
#include <QWidget>

class FeedHolder;

enum class RsFeedTypeFlags: uint32_t {
    RS_FEED_TYPE_NONE        = 0x0000,
    RS_FEED_TYPE_PEER        = 0x0010,
    RS_FEED_TYPE_CHANNEL     = 0x0020,
    RS_FEED_TYPE_FORUM       = 0x0040,
    RS_FEED_TYPE_CHAT        = 0x0100,
    RS_FEED_TYPE_MSG         = 0x0200,
    RS_FEED_TYPE_FILES       = 0x0400,
    RS_FEED_TYPE_SECURITY    = 0x0800,
    RS_FEED_TYPE_POSTED      = 0x1000,
    RS_FEED_TYPE_SECURITY_IP = 0x2000,
    RS_FEED_TYPE_CIRCLE      = 0x4000,
#ifdef RS_USE_WIRE
    RS_FEED_TYPE_WIRE        = 0x8000,
#endif

    RS_FEED_ITEM_PEER_CONNECT            = RS_FEED_TYPE_PEER  | 0x0001,
    RS_FEED_ITEM_PEER_DISCONNECT         = RS_FEED_TYPE_PEER  | 0x0002,
    RS_FEED_ITEM_PEER_HELLO              = RS_FEED_TYPE_PEER  | 0x0003,
    RS_FEED_ITEM_PEER_NEW                = RS_FEED_TYPE_PEER  | 0x0004,
    RS_FEED_ITEM_PEER_OFFSET             = RS_FEED_TYPE_PEER  | 0x0005,
    RS_FEED_ITEM_PEER_DENIES_CONNEXION   = RS_FEED_TYPE_PEER  | 0x0006,

    RS_FEED_ITEM_SEC_CONNECT_ATTEMPT     = RS_FEED_TYPE_SECURITY  | 0x0001,
    RS_FEED_ITEM_SEC_AUTH_DENIED         = RS_FEED_TYPE_SECURITY  | 0x0002,	// locally denied connection
    RS_FEED_ITEM_SEC_UNKNOWN_IN          = RS_FEED_TYPE_SECURITY  | 0x0003,
    RS_FEED_ITEM_SEC_UNKNOWN_OUT         = RS_FEED_TYPE_SECURITY  | 0x0004,
    RS_FEED_ITEM_SEC_WRONG_SIGNATURE     = RS_FEED_TYPE_SECURITY  | 0x0005,
    RS_FEED_ITEM_SEC_BAD_CERTIFICATE     = RS_FEED_TYPE_SECURITY  | 0x0006,
    RS_FEED_ITEM_SEC_INTERNAL_ERROR      = RS_FEED_TYPE_SECURITY  | 0x0007,
    RS_FEED_ITEM_SEC_MISSING_CERTIFICATE = RS_FEED_TYPE_SECURITY  | 0x0008,

    RS_FEED_ITEM_SEC_IP_BLACKLISTED                = RS_FEED_TYPE_SECURITY_IP  | 0x0001,
    RS_FEED_ITEM_SEC_IP_WRONG_EXTERNAL_IP_REPORTED = RS_FEED_TYPE_SECURITY_IP  | 0x0002,

    RS_FEED_ITEM_CHANNEL_NEW        = RS_FEED_TYPE_CHANNEL  | 0x0002,
    RS_FEED_ITEM_CHANNEL_MSG        = RS_FEED_TYPE_CHANNEL  | 0x0003,
    RS_FEED_ITEM_CHANNEL_PUBLISHKEY = RS_FEED_TYPE_CHANNEL| 0x0004,

    RS_FEED_ITEM_FORUM_NEW        = RS_FEED_TYPE_FORUM | 0x0001,
    RS_FEED_ITEM_FORUM_MSG        = RS_FEED_TYPE_FORUM | 0x0003,
    RS_FEED_ITEM_FORUM_PUBLISHKEY = RS_FEED_TYPE_FORUM | 0x0004,

    RS_FEED_ITEM_POSTED_NEW       = RS_FEED_TYPE_POSTED  | 0x0001,
    RS_FEED_ITEM_POSTED_MSG       = RS_FEED_TYPE_POSTED  | 0x0003,

    RS_FEED_ITEM_CHAT_NEW         = RS_FEED_TYPE_CHAT  | 0x0001,
    RS_FEED_ITEM_MESSAGE          = RS_FEED_TYPE_MSG   | 0x0001,
    RS_FEED_ITEM_FILES_NEW        = RS_FEED_TYPE_FILES | 0x0001,

    RS_FEED_ITEM_CIRCLE_MEMB_REQ        = RS_FEED_TYPE_CIRCLE  | 0x0001,
    RS_FEED_ITEM_CIRCLE_INVITE_REC      = RS_FEED_TYPE_CIRCLE  | 0x0002,
    RS_FEED_ITEM_CIRCLE_MEMB_LEAVE      = RS_FEED_TYPE_CIRCLE  | 0x0003,
    RS_FEED_ITEM_CIRCLE_MEMB_JOIN       = RS_FEED_TYPE_CIRCLE  | 0x0004,
    RS_FEED_ITEM_CIRCLE_MEMB_ACCEPTED   = RS_FEED_TYPE_CIRCLE  | 0x0005,
    RS_FEED_ITEM_CIRCLE_MEMB_REVOKED    = RS_FEED_TYPE_CIRCLE  | 0x0006,
    RS_FEED_ITEM_CIRCLE_INVITE_CANCELLED= RS_FEED_TYPE_CIRCLE  | 0x0007,
};


RS_REGISTER_ENUM_FLAGS_TYPE(RsFeedTypeFlags);

class FeedItem : public QWidget
{
	Q_OBJECT

public:
	/** Default Constructor */
	FeedItem(FeedHolder *fh,uint32_t feedId,QWidget *parent = 0);
	/** Default Destructor */
	virtual ~FeedItem();

	bool wasExpanded() { return mWasExpanded; }
	void expand(bool open);

    /*!
     * \brief uniqueIdentifier
     * \return returns a string that is unique to this specific item. The string will be used to search for an existing item that
     * 			would contain the same information. It should therefore sumarise the data represented by the item.
     */
    virtual uint64_t uniqueIdentifier() const =0;

	static uint64_t hash64(const std::string& s);

protected slots:
	void removeItem();

protected:
	virtual void doExpand(bool open) = 0;
	virtual void expandFill(bool /*first*/) {}

    virtual void toggle() {}

    uint64_t hash_64bits(const std::string& s) const;

	FeedHolder *mFeedHolder;
	uint32_t mFeedId;
    static const int   ITEM_HEIGHT_FACTOR ;
    static const float ITEM_PICTURE_FORMAT_RATIO ;

signals:
	void sizeChanged(FeedItem *feedItem);
	void feedItemNeedsClosing(qulonglong);

private:
	bool mWasExpanded;
    mutable uint64_t mHash;
};

#endif
