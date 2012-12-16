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

#ifndef PREVIEWFEEDDIALOG_H
#define PREVIEWFEEDDIALOG_H

#include <QDialog>
#include <QItemDelegate>

namespace Ui {
class PreviewFeedDialog;
}

class QTreeWidget;
class RsFeedReader;
class FeedReaderNotify;
class FeedInfo;

// not yet functional
//class PreviewItemDelegate : public QItemDelegate
//{
//	Q_OBJECT

//public:
//	PreviewItemDelegate(QTreeWidget *parent);

//private slots:
//	void sectionResized(int logicalIndex, int oldSize, int newSize);

//protected:
//	virtual void drawDisplay(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect, const QString &text) const;
//	virtual void drawFocus(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect) const;
//	virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
//};

class PreviewFeedDialog : public QDialog
{
	Q_OBJECT
	
public:
	PreviewFeedDialog(RsFeedReader *feedReader, FeedReaderNotify *notify, const FeedInfo &feedInfo, QWidget *parent = 0);
	~PreviewFeedDialog();

	void getXPaths(std::list<std::string> &xpathsToUse, std::list<std::string> &xpathsToRemove);

protected:
	bool eventFilter(QObject *obj, QEvent *ev);

private slots:
	void previousMsg();
	void nextMsg();
	void showStructureFrame(bool show = false);
	void showXPathFrame(bool show);
	void xpathListCustomPopupMenu(QPoint point);
	void xpathCloseEditor(QWidget *editor, QAbstractItemDelegate::EndEditHint hint);
	void addXPath();
	void editXPath();
	void removeXPath();
	void fillStructureTree();

	/* FeedReaderNotify */
	void feedChanged(const QString &feedId, int type);
	void msgChanged(const QString &feedId, const QString &msgId, int type);

private:
	void processSettings(bool load);
	int getMsgPos();
	void setFeedInfo(const QString &info);
	void setXPathInfo(const QString &info);
	void fillFeedInfo(const FeedInfo &feedInfo);
	void updateMsgCount();
	void updateMsg();
	void processXPath();

	RsFeedReader *mFeedReader;
	FeedReaderNotify *mNotify;
	std::string mFeedId;
	std::string mMsgId;
	std::list<std::string> mMsgIds;
	std::string mDescription;
	std::string mDescriptionXPath;

	Ui::PreviewFeedDialog *ui;
};

#endif // PREVIEWFEEDDIALOG_H
