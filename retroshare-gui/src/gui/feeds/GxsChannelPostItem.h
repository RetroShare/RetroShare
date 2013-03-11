/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008 Robert Fernie
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#ifndef _GXS_CHANNEL_POST_ITEM_H
#define _GXS_CHANNEL_POST_ITEM_H

#include <retroshare/rsgxschannels.h>
#include "ui_GxsChannelPostItem.h"
#include <stdint.h>

class FeedHolder;
class SubFileItem;

class GxsChannelPostItem : public QWidget, private Ui::GxsChannelPostItem
{
	Q_OBJECT

public:
	/** Default Constructor */
	GxsChannelPostItem(FeedHolder *parent, uint32_t feedId, const std::string &chanId, const std::string &msgId, bool isHome);
	GxsChannelPostItem(FeedHolder *parent, uint32_t feedId, const RsGxsChannelPost &post, uint32_t subscribeFlags, bool isHome);
	void updateItemStatic();
	void setFileCleanUpWarning(uint32_t time_left);

	const std::string &msgId() { return mMsgId; }

private slots:
	/* default stuff */
	void removeItem();
	void toggle();

	void readAndClearItem();
	void unsubscribeChannel();
	void download();
	void play();
	void copyLink();
	void comments();

	void readToggled(bool checked);
	void channelMsgReadSatusChanged(const QString& channelId, const QString& msgId, int status);

	void updateItem();

private:
	void setup();
	void loadPost(const RsGxsChannelPost &post);


	FeedHolder *mParent;
	uint32_t mFeedId;

	std::string mChanId;
	std::string mMsgId;

	bool mIsHome;
	bool m_inUpdateItemStatic;

        uint32_t mMode;
        uint32_t mSubscribeFlags;

	std::list<SubFileItem *> mFileItems;
};

#endif

