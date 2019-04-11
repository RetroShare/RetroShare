/*******************************************************************************
 * plugins/FeedReader/gui/AddFeedDialog.h                                      *
 *                                                                             *
 * Copyright (C) 2012 by Thunder                                               *
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

#ifndef ADDFEEDDIALOG_H
#define ADDFEEDDIALOG_H

#include <QDialog>
#include "interface/rsFeedReader.h"
#include "util/TokenQueue.h"

namespace Ui {
class AddFeedDialog;
}

class RsFeedReader;
class FeedReaderNotify;
class UIStateHelper;

class AddFeedDialog : public QDialog, public TokenResponse
{
	Q_OBJECT
    
public:
	AddFeedDialog(RsFeedReader *feedReader, FeedReaderNotify *notify, QWidget *parent);
	~AddFeedDialog();

	void setParent(const std::string &parentId);
	bool fillFeed(const std::string &feedId);

	/* TokenResponse */
	virtual void loadRequest(const TokenQueue *queue, const TokenRequest &req);

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
	void clearMessageCache();

private:
	void processSettings(bool load);
	void getFeedInfo(FeedInfo &feedInfo);
	void setActiveForumId(const std::string &forumId);

	void requestForumGroups();
	void loadForumGroups(const uint32_t &token);

private:
	RsFeedReader *mFeedReader;
	FeedReaderNotify *mNotify;
	std::string mFeedId;
	std::string mParentId;
	std::string mFillForumId;

	RsFeedTransformationType mTransformationType;
	std::list<std::string> mXPathsToUse;
	std::list<std::string> mXPathsToRemove;
	std::string mXslt;

	TokenQueue *mTokenQueue;
	UIStateHelper *mStateHelper;

	Ui::AddFeedDialog *ui;
};

#endif // ADDFEEDDIALOG_H
