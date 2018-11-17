/*******************************************************************************
 * retroshare-gui/src/gui/gxsforums/GxsForumsFillThread.h                      *
 *                                                                             *
 * Copyright 2012 Retroshare Team      <retroshare.project@gmail.com>          *
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

#ifndef GXSFORUMSFILLTHREAD_H
#define GXSFORUMSFILLTHREAD_H

#include <QThread>
#include <QMap>
#include <QPair>
#include "retroshare/rsgxsifacetypes.h"

class GxsForumThreadWidget;
class RsGxsForumMsg;
class RSTreeWidgetItemCompareRole;
class QTreeWidgetItem;

class GxsForumsFillThread : public QThread
{
	Q_OBJECT

public:
	GxsForumsFillThread(GxsForumThreadWidget *parent);
	~GxsForumsFillThread();

	void run();
	void stop();
	bool wasStopped() { return mStopped; }

signals:
	void progress(int current, int count);
	void status(QString text);

public:
	 RsGxsGroupId mForumId;
	int mFilterColumn;
	bool mFillComplete;
	int mViewType;
	bool mFlatView;
	bool mUseChildTS;
	bool mExpandNewMessages;
	std::string mFocusMsgId;
	RSTreeWidgetItemCompareRole *mCompareRole;

	QList<QTreeWidgetItem*> mItems;
	QList<QTreeWidgetItem*> mItemToExpand;

    QMap<RsGxsMessageId,QVector<QPair<time_t,RsGxsMessageId> > > mPostVersions ;
private:
	void calculateExpand(const RsGxsForumMsg &msg, QTreeWidgetItem *item);

	GxsForumThreadWidget *mParent;
	volatile bool mStopped;
};

#endif // GXSFORUMSFILLTHREAD_H
