/*******************************************************************************
 * retroshare-gui/src/retroshare-gui/src/gui/feeds/GxsGroupFeedItem.h          *
 *                                                                             *
 * Copyright 2012-2013  by Robert Fernie      <retroshare.project@gmail.com>   *
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

#ifndef _GXS_GROUPFEEDITEM_H
#define _GXS_GROUPFEEDITEM_H

#include <QMetaType>

#include <retroshare/rsgxsifacehelper.h>
#include "gui/feeds/FeedItem.h"
#include "gui/RetroShareLink.h"

#include <stdint.h>

class FeedHolder;
class RsGxsUpdateBroadcastBase;

class GxsGroupFeedItem : public FeedItem
{
	Q_OBJECT

public:
	/** Note parent can = NULL */
	GxsGroupFeedItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsGroupId &groupId, bool isHome, RsGxsIfaceHelper *iface, bool autoUpdate);
	virtual ~GxsGroupFeedItem();

	RsGxsGroupId groupId() const { return mGroupId; }
	uint32_t feedId() const { return mFeedId; }

protected:
        enum LoadingStatus {
        LOADING_STATUS_NO_DATA      =   0x00,
        LOADING_STATUS_HAS_DATA     =   0x01,
        LOADING_STATUS_FILLED       =   0x02
    };

	/* load group data */
	void requestGroup();

	virtual void loadGroup() = 0;
	virtual RetroShareLink::enumType getLinkType() = 0;
	virtual QString groupName() = 0;

    // This triggers an update in the main thread after a short waiting period. Help loading objects that havn't loaded yet.
    void deferred_update();

protected slots:
	void subscribe();
	void unsubscribe();
	void copyGroupLink();

protected:
	bool mIsHome;
	RsGxsIfaceHelper *mGxsIface;
    static const uint GROUP_ITEM_LOADING_TIMEOUT_ms ;

private slots:
	/* RsGxsUpdateBroadcastBase */
	void fillDisplaySlot(bool complete);

private:
	RsGxsGroupId mGroupId;
    int mLastDelay;
};

Q_DECLARE_METATYPE(RsGxsGroupId)

#endif
