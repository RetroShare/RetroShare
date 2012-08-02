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

#ifndef _FEEDREADERNOTIFY_H
#define _FEEDREADERNOTIFY_H

#include <QObject>
#include "interface/rsFeedReader.h"

class FeedReaderNotify : public QObject, public RsFeedReaderNotify
{
	Q_OBJECT

public:
	FeedReaderNotify();

	/* RsFeedReaderNotify */
	virtual void feedChanged(const std::string &feedId, int type);
	virtual void msgChanged(const std::string &feedId, const std::string &msgId, int type);

signals:
	void notifyFeedChanged(const QString &feedId, int type);
	void notifyMsgChanged(const QString &feedId, const QString &msgId, int type);
};

#endif

