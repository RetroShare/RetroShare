/*******************************************************************************
 * plugins/FeedReader/gui/FeedReaderDialog.h                                   *
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

#ifndef _FEEDREADERDIALOG_H
#define _FEEDREADERDIALOG_H

#include <retroshare-gui/mainpage.h>
#include "interface/rsFeedReader.h"

namespace Ui {
class FeedReaderDialog;
}

class QTreeWidgetItem;
class RsFeedReader;
class RSTreeWidgetItemCompareRole;
class FeedReaderNotify;
class FeedReaderMessageWidget;

class FeedReaderDialog : public MainPage
{
	Q_OBJECT

public:
	FeedReaderDialog(RsFeedReader *feedReader, FeedReaderNotify *notify, QWidget *parent = 0);
	~FeedReaderDialog();

	static QIcon iconFromFeed(const FeedInfo &feedInfo);

protected:
	virtual UserNotify *createUserNotify(QObject *parent) override;
	virtual void showEvent(QShowEvent *event);
	bool eventFilter(QObject *obj, QEvent *ev);

private slots:
	void settingsChanged();
	void feedTreeCustomPopupMenu(QPoint point);
	void feedTreeItemActivated(QTreeWidgetItem *item);
	void feedTreeMiddleButtonClicked(QTreeWidgetItem *item);
	void openInNewTab();
	void newFolder();
	void newFeed();
	void removeFeed();
	void editFeed();
	void activateFeed();
	void processFeed();
	void feedTreeReparent(QTreeWidgetItem *item, QTreeWidgetItem *newParent);

	void messageTabCloseRequested(int index);
	void messageTabChanged(int index);
	void messageTabInfoChanged(QWidget *widget);

	/* FeedReaderNotify */
	void feedChanged(uint32_t feedId, int type);
	void optimizeImage();

private:
	uint32_t currentFeedId();
	void setCurrentFeedId(uint32_t feedId);
	void processSettings(bool load);
	void addFeedToExpand(uint32_t feedId);
	void getExpandedFeedIds(QList<uint32_t> &feedIds);
	void updateFeeds(uint32_t parentId, QTreeWidgetItem *parentItem);
	void updateFeedItem(QTreeWidgetItem *item, const FeedInfo &feedInfo);
	void openFeedInNewTab(uint32_t feedId);

	void calculateFeedItems();
	void calculateFeedItem(QTreeWidgetItem *item, uint32_t &unreadCount, uint32_t &newCount, bool &loading);

	FeedReaderMessageWidget *feedMessageWidget(uint32_t feedId);
	FeedReaderMessageWidget *createMessageWidget(uint32_t feedId);

private:
	bool mProcessSettings;
	QList<uint32_t> *mOpenFeedIds;
	QTreeWidgetItem *mRootItem;
	RSTreeWidgetItemCompareRole *mFeedCompareRole;
	FeedReaderMessageWidget *mMessageWidget;

	// gui interface
	RsFeedReader *mFeedReader;
	FeedReaderNotify *mNotify;

	/** Qt Designer generated object */
	Ui::FeedReaderDialog *ui;
};

#endif

