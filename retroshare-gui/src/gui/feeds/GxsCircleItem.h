/*******************************************************************************
 * gui/feeds/GxsCircleItem.h                                                   *
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
