/****************************************************************
 *  RetroShare GUI is distributed under the following license:
 *
 *  Copyright (C) 2012 by Thunder
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
