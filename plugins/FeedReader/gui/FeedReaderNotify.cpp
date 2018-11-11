/*******************************************************************************
 * plugins/FeedReader/gui/FeedReaderNotify.cpp                                 *
 *                                                                             *
 * Copyright (C) 2012 by Thunder <retroshare.project@gmail.com>                *
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

#include "FeedReaderNotify.h"

FeedReaderNotify::FeedReaderNotify() : QObject()
{
}

void FeedReaderNotify::notifyFeedChanged(const std::string &feedId, int type)
{
	emit feedChanged(QString::fromStdString(feedId), type);
}

void FeedReaderNotify::notifyMsgChanged(const std::string &feedId, const std::string &msgId, int type)
{
	emit msgChanged(QString::fromStdString(feedId), QString::fromStdString(msgId), type);
}
