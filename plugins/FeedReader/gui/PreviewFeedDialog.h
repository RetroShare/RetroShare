/*******************************************************************************
 * plugins/FeedReader/gui/PreviewFeedDialog.h                                  *
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

#ifndef PREVIEWFEEDDIALOG_H
#define PREVIEWFEEDDIALOG_H

#include <QDialog>
#include <QItemDelegate>

#include "interface/rsFeedReader.h"

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

	RsFeedTransformationType getData(std::list<std::string> &xpathsToUse, std::list<std::string> &xpathsToRemove, std::string &xslt);

protected:
	bool eventFilter(QObject *obj, QEvent *ev);

private slots:
	void previousMsg();
	void nextMsg();
	void showStructureFrame();
	void xpathListCustomPopupMenu(QPoint point);
	void xpathCloseEditor(QWidget *editor, QAbstractItemDelegate::EndEditHint hint);
	void addXPath();
	void editXPath();
	void removeXPath();
	void transformationTypeChanged();

	/* FeedReaderNotify */
	void feedChanged(uint32_t feedId, int type);
	void msgChanged(uint32_t feedId, const QString &msgId, int type);

private:
	void processSettings(bool load);
	int getMsgPos();
	void setFeedInfo(const QString &info);
	void setTransformationInfo(const QString &info);
	void fillFeedInfo(const FeedInfo &feedInfo);
	void updateMsgCount();
	void updateMsg();
	void fillStructureTree(bool transform);
	void processTransformation();

	RsFeedReader *mFeedReader;
	FeedReaderNotify *mNotify;
	uint32_t mFeedId;
	std::string mMsgId;
	std::list<std::string> mMsgIds;
	std::string mDescription;
	std::string mDescriptionTransformed;

	Ui::PreviewFeedDialog *ui;
};

#endif // PREVIEWFEEDDIALOG_H
