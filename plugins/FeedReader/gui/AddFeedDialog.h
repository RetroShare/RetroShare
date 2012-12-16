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

#ifndef ADDFEEDDIALOG_H
#define ADDFEEDDIALOG_H

#include <QDialog>
#include "interface/rsFeedReader.h"

namespace Ui {
class AddFeedDialog;
}

class RsFeedReader;
class FeedReaderNotify;

class AddFeedDialog : public QDialog
{
	Q_OBJECT
    
public:
	AddFeedDialog(RsFeedReader *feedReader, FeedReaderNotify *notify, QWidget *parent);
	~AddFeedDialog();

	void setParent(const std::string &parentId);
	bool fillFeed(const std::string &feedId);

private slots:
	void authenticationToggled();
	void useStandardStorageTimeToggled();
	void useStandardUpdateIntervalToggled();
	void useStandardProxyToggled();
	void typeForumToggled();
	void denyForumToggled();
	void validate();
	void createFeed();
	void preview();

private:
	void processSettings(bool load);
	void getFeedInfo(FeedInfo &feedInfo);

	RsFeedReader *mFeedReader;
	FeedReaderNotify *mNotify;
	std::string mFeedId;
	std::string mParentId;

	std::list<std::string> mXPathsToUse;
	std::list<std::string> mXPathsToRemove;

	Ui::AddFeedDialog *ui;
};

#endif // ADDFEEDDIALOG_H
