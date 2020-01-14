/*******************************************************************************
 * gui/feeds/FeedItem.cpp                                                      *
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

#include <iostream>
#include "FeedItem.h"
#include "FeedHolder.h"

/** Constructor */
FeedItem::FeedItem(FeedHolder *fh, uint32_t feedId, QWidget *parent) : QWidget(parent), mHash(0),mFeedHolder(fh),mFeedId(feedId)
{
	mWasExpanded = false;
}

FeedItem::~FeedItem() { }

void FeedItem::expand(bool open)
{
	if (open) {
		expandFill(!mWasExpanded);
	}

	doExpand(open);

	if (open) {
		mWasExpanded = true;
	}
}

uint64_t FeedItem::hash_64bits(const std::string& s) const
{
    if(mHash == 0)
        mHash = hash64(s);

	return mHash;
}

uint64_t FeedItem::hash64(const std::string& s)
{
	uint64_t hash = 0x01110bbfa09;

	for(uint32_t i=0;i<s.size();++i)
		hash = ~(((hash << 31) ^ (hash >> 3)) + s[i]*0x217898fbba7 + 0x0294379);

    return hash;
}

void FeedItem::removeItem()
{
#ifdef DEBUG_ITEM
	std::cerr << "MsgItem::removeItem()";
	std::cerr << std::endl;
#endif

	mFeedHolder->deleteFeedItem(this,0);

//	mFeedHolder->lockLayout(this, true);
//	hide();
//
//	if (mFeedHolder)
//		mFeedHolder->deleteFeedItem(this, mFeedId);
//
//	mFeedHolder->lockLayout(this, false);
}

