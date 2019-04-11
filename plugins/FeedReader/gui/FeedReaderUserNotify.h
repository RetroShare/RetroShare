/*******************************************************************************
 * plugins/FeedReader/gui/FeedReaderUserNotify.h                               *
 *                                                                             *
 * Copyright (C) 2012 by Retroshare Team <retroshare.project@gmail.com>        *
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

#ifndef FEEDREADERUSERNOTIFY_H
#define FEEDREADERUSERNOTIFY_H

#include "gui/common/UserNotify.h"

class FeedReaderDialog;
class RsFeedReader;
class FeedReaderNotify;

class FeedReaderUserNotify : public UserNotify
{
	Q_OBJECT

public:
	FeedReaderUserNotify(FeedReaderDialog *feedReaderDialog, RsFeedReader *feedReader, FeedReaderNotify *notify, QObject *parent);

	virtual bool hasSetting(QString *name, QString *group);

private slots:
	void feedChanged(const QString &feedId, int type);

private:
	virtual QIcon getIcon();
	virtual QIcon getMainIcon(bool hasNew);
	virtual unsigned int getNewCount();
	virtual void iconClicked();

	FeedReaderDialog *mFeedReaderDialog;
	RsFeedReader *mFeedReader;
	FeedReaderNotify *mNotify;
};

#endif // FEEDREADERUSERNOTIFY_H
