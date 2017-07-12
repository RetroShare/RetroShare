/*
 * Retroshare Gxs Feed Item
 *
 * Copyright 2014 RetroShare Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#ifndef _GXSCIRCLEITEM_H
#define _GXSCIRCLEITEM_H

#include <retroshare/rsgxscircles.h>
#include "FeedItem.h"

#include "util/TokenQueue.h"

namespace Ui {
class GxsCircleItem;
}

class FeedHolder;

struct CircleUpdateOrder
{
    enum { UNKNOWN_ACTION=0x00, GRANT_MEMBERSHIP=0x01, REVOKE_MEMBERSHIP=0x02 };

    uint32_t token ;
    RsGxsId  gxs_id ;
    uint32_t action ;
};


class GxsCircleItem : public FeedItem, public TokenResponse
{
	Q_OBJECT

public:

	/** Default Constructor */
	GxsCircleItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsCircleId &circleId, const RsGxsId &gxsId, const uint32_t type);
	virtual ~GxsCircleItem();

	bool isSame(const RsGxsCircleId &circleId, const RsGxsId &gxsId, uint32_t type);

	void loadRequest(const TokenQueue *queue, const TokenRequest &req);


protected:
	/* FeedItem */
	virtual void doExpand(bool /*open*/) {}

	void updateCircleGroup(const uint32_t& token);



private slots:
	/* default stuff */
	void removeItem();

	void showCircleDetails();
	void acceptCircleSubscription();
	void grantCircleMembership() ;
	void revokeCircleMembership();

private:
	void setup();

	FeedHolder *mFeedHolder;
	uint32_t mFeedId;
	uint32_t mType;

	RsGxsCircleId mCircleId;
	RsGxsId mGxsId;

	TokenQueue *mCircleQueue;
	std::map<uint32_t, CircleUpdateOrder> mCircleUpdates ;


	/** Qt Designer generated object */
	Ui::GxsCircleItem *ui;
};

#endif
