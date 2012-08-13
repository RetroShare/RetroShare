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

class FeedReaderDialog : public MainPage
{
	Q_OBJECT

public:
	FeedReaderDialog(RsFeedReader *feedReader, QWidget *parent = 0);
	~FeedReaderDialog();

protected:
	virtual void showEvent(QShowEvent *e);
	bool eventFilter(QObject *obj, QEvent *ev);

private slots:
	void feedTreeCustomPopupMenu(QPoint point);
	void msgTreeCustomPopupMenu(QPoint point);
	void feedItemChanged(QTreeWidgetItem *item);
	void msgItemChanged();
	void msgItemClicked(QTreeWidgetItem *item, int column);
	void filterColumnChanged();
	void filterItems(const QString &text);
	void toggleMsgText();
	void newFolder();
	void newFeed();
	void removeFeed();
	void editFeed();
	void activateFeed();
	void processFeed();
	void markAsReadMsg();
	void markAsUnreadMsg();
	void markAllAsReadMsg();
	void copyLinksMsg();
	void removeMsg();
	void openLinkMsg();
	void copyLinkMsg();

	/* FeedReaderNotify */
	void feedChanged(const QString &feedId, int type);
	void msgChanged(const QString &feedId, const QString &msgId, int type);

private:
	std::string currentFeedId();
	std::string currentMsgId();
	void processSettings(bool load);
	void updateFeeds(const std::string &parentId, QTreeWidgetItem *parentItem);
	void updateFeedItem(QTreeWidgetItem *item, FeedInfo &info);
	void updateMsgs(const std::string &feedId);
	void calculateMsgIconsAndFonts(QTreeWidgetItem *item);
	void updateMsgItem(QTreeWidgetItem *item, FeedMsgInfo &info);
	void setMsgAsReadUnread(QList<QTreeWidgetItem*> &rows, bool read);
	void filterItem(QTreeWidgetItem *item, const QString &text, int filterColumn);
	void filterItem(QTreeWidgetItem *item);
	void toggleMsgText_internal();

	void calculateFeedItems();
	void calculateFeedItem(QTreeWidgetItem *item, uint32_t &unreadCount, bool &loading);

	bool mProcessSettings;
	QTreeWidgetItem *mRootItem;
	RSTreeWidgetItemCompareRole *mFeedCompareRole;
	RSTreeWidgetItemCompareRole *mMsgCompareRole;

	// gui interface
	RsFeedReader *mFeedReader;
	FeedReaderNotify *mNotify;

	/** Qt Designer generated object */
	Ui::FeedReaderDialog *ui;
};

#endif

