/*******************************************************************************
 * plugins/FeedReader/gui/FeedReaderNotify.h                                   *
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
	virtual void notifyFeedChanged(const std::string &feedId, int type);
	virtual void notifyMsgChanged(const std::string &feedId, const std::string &msgId, int type);

signals:
	void feedChanged(const QString &feedId, int type);
	void msgChanged(const QString &feedId, const QString &msgId, int type);
};

#endif

