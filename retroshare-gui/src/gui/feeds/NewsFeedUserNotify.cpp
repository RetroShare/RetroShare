/*******************************************************************************
 * gui/feeds/NewsFeedUserNotify.cpp                                            *
 *                                                                             *
 * Copyright (c) 2012, Retroshare Team <retroshare.project@gmail.com>          *
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

#include "NewsFeedUserNotify.h"
#include "gui/NewsFeed.h"

NewsFeedUserNotify::NewsFeedUserNotify(NewsFeed *newsFeed, QObject *parent) :
	UserNotify(parent)
{
	mNewFeedCount = 0;

	connect(newsFeed, SIGNAL(newsFeedChanged(int)), this, SLOT(newsFeedChanged(int)),Qt::QueuedConnection);
}

void NewsFeedUserNotify::newsFeedChanged(int count)
{
	mNewFeedCount = count;
	updateIcon();
}

QIcon NewsFeedUserNotify::getMainIcon(bool hasNew)
{
    return hasNew ? QIcon(":/icons/png/newsfeed-notify.png") : QIcon(":/icons/png/newsfeed.png");
}

unsigned int NewsFeedUserNotify::getNewCount()
{
	return mNewFeedCount;
}
