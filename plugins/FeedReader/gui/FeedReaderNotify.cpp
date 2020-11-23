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

void FeedReaderNotify::notifyFeedChanged(uint32_t feedId, int type)
{
	emit feedChanged(feedId, type);
}

void FeedReaderNotify::notifyMsgChanged(uint32_t feedId, const std::string &msgId, int type)
{
	emit msgChanged(feedId, QString::fromStdString(msgId), type);
}
