/*******************************************************************************
 * gui/common/FeedNotify.cpp                                                   *
 *                                                                             *
 * Copyright (C) 2012, Retroshare Team <retroshare.project@gmail.com>          *
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

#include "FeedNotify.h"

FeedNotify::FeedNotify(QObject *parent) :
    QObject(parent)
{
}

FeedNotify::~FeedNotify()
{
}

bool FeedNotify::hasSetting(QString &/*name*/)
{
	return false;
}

bool FeedNotify::notifyEnabled()
{
	return false;
}

void FeedNotify::setNotifyEnabled(bool /*enabled*/)
{
}

FeedItem *FeedNotify::feedItem(FeedHolder */*parent*/)
{
	return NULL;
}

FeedItem *FeedNotify::testFeedItem(FeedHolder */*parent*/)
{
	return NULL;
}
