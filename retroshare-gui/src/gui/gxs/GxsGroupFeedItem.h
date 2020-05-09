/*******************************************************************************
 * retroshare-gui/src/gui/gxs/GxsGroupFeedItem.h                               *
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
	/* load group data */
	void requestGroup();

	virtual void loadGroup() = 0;
	virtual RetroShareLink::enumType getLinkType() = 0;
	virtual QString groupName() = 0;

protected slots:
	void subscribe();
	void unsubscribe();
	void copyGroupLink();

protected:
	bool mIsHome;
	RsGxsIfaceHelper *mGxsIface;

private slots:
	/* RsGxsUpdateBroadcastBase */
	void fillDisplaySlot(bool complete);

private:
	RsGxsGroupId mGroupId;
};

Q_DECLARE_METATYPE(RsGxsGroupId)

#endif
