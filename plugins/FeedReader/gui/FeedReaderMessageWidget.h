/*******************************************************************************
 * plugins/FeedReader/gui/FeedReaderMessageWidget.h                            *
 *                                                                             *
 * Copyright (C) 2012 by RetroShare Team <retroshare.project@gmail.com>        *
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

#ifndef FEEDREADERMESSAGEWIDGET_H
#define FEEDREADERMESSAGEWIDGET_H

#include <QWidget>

#include "interface/rsFeedReader.h"

namespace Ui {
class FeedReaderMessageWidget;
}

class QTimer;
class FeedMsgInfo;
class QTreeWidgetItem;
class RSTreeWidgetItemCompareRole;
class RsFeedReader;
class FeedReaderNotify;

class FeedReaderMessageWidget : public QWidget
{
	Q_OBJECT

public:
	explicit FeedReaderMessageWidget(const std::string &feedId, RsFeedReader *feedReader, FeedReaderNotify *notify, QWidget *parent = 0);
	~FeedReaderMessageWidget();

	std::string feedId() { return mFeedId; }
	void setFeedId(const std::string &feedId);
	QString feedName(bool withUnreadCount);
	QIcon feedIcon();

protected:
	virtual void showEvent(QShowEvent *e);
	bool eventFilter(QObject *obj, QEvent *ev);

signals:
	void feedMessageChanged(QWidget *widget);

private slots:
	void msgTreeCustomPopupMenu(QPoint point);
	void updateCurrentMessage();
	void msgItemChanged();
	void msgItemClicked(QTreeWidgetItem *item, int column);
	void filterColumnChanged(int column);
	void filterItems(const QString &text);
	void toggleMsgText();
	void markAsReadMsg();
	void markAsUnreadMsg();
	void markAllAsReadMsg();
	void copySelectedLinksMsg();
	void removeMsg();
	void processFeed();
	void openLinkMsg();
	void copyLinkMsg();
	void retransformMsg();

	/* FeedReaderNotify */
	void feedChanged(const QString &feedId, int type);
	void msgChanged(const QString &feedId, const QString &msgId, int type);

private:
	std::string currentMsgId();
	void processSettings(bool load);
	void updateMsgs();
	void calculateMsgIconsAndFonts(QTreeWidgetItem *item);
	void updateMsgItem(QTreeWidgetItem *item, FeedMsgInfo &info);
	void setMsgAsReadUnread(QList<QTreeWidgetItem*> &rows, bool read);
	void filterItem(QTreeWidgetItem *item, const QString &text, int filterColumn);
	void filterItem(QTreeWidgetItem *item);
	void toggleMsgText_internal();

	bool mProcessSettings;
	RSTreeWidgetItemCompareRole *mMsgCompareRole;
	std::string mFeedId;
	unsigned int mUnreadCount;
	unsigned int mNewCount;
	QTimer *mTimer;
	FeedInfo mFeedInfo;

	// gui interface
	RsFeedReader *mFeedReader;
	FeedReaderNotify *mNotify;

	Ui::FeedReaderMessageWidget *ui;
};

#endif // FEEDREADERMESSAGEWIDGET_H
