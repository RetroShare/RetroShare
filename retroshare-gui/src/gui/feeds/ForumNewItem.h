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

#ifndef _FORUM_NEW_ITEM_DIALOG_H
#define _FORUM_NEW_ITEM_DIALOG_H

#include "ui_ForumNewItem.h"
#include <stdint.h>

class FeedHolder;

class ForumNewItem : public QWidget, private Ui::ForumNewItem
{
	Q_OBJECT

public:
	/** Default Constructor */
	ForumNewItem(FeedHolder *parent, uint32_t feedId, const std::string &forumId, bool isHome, bool isNew);

	void updateItemStatic();

private slots:
	/* default stuff */
	void removeItem();
	void toggle();

	void unsubscribeForum();
	void subscribeForum();
	void postToForum();

	void updateItem();

private:
	FeedHolder *mParent;
	uint32_t mFeedId;

	std::string mForumId;
	bool mIsHome;
	bool mIsNew;
};

#endif

