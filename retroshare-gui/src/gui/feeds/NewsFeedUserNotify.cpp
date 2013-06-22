/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2012 RetroShare Team
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

#include "NewsFeedUserNotify.h"
#include "gui/NewsFeed.h"

NewsFeedUserNotify::NewsFeedUserNotify(NewsFeed *newsFeed, QObject *parent) :
	UserNotify(parent)
{
	mNewFeedCount = 0;

	connect(newsFeed, SIGNAL(newsFeedChanged(int)), this, SLOT(newsFeedChanged(int)));
}

void NewsFeedUserNotify::newsFeedChanged(int count)
{
	mNewFeedCount = count;
	updateIcon();
}

QIcon NewsFeedUserNotify::getMainIcon(bool hasNew)
{
	return hasNew ? QIcon(":/images/newsfeed128.png") : QIcon(":/images/newsfeed128.png");
}

unsigned int NewsFeedUserNotify::getNewCount()
{
	return mNewFeedCount;
}
