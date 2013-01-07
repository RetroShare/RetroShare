#ifndef GXSFORUMSFILLTHREAD_H
#define GXSFORUMSFILLTHREAD_H

#include <QThread>

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
	std::string mForumId;
	int mFilterColumn;
	int mSubscribeFlags;
	bool mFillComplete;
	int mViewType;
	bool mFlatView;
	bool mUseChildTS;
	bool mExpandNewMessages;
	std::string mFocusMsgId;
	RSTreeWidgetItemCompareRole *mCompareRole;

	QList<QTreeWidgetItem*> mItems;
	QList<QTreeWidgetItem*> mItemToExpand;

private:
	void calculateExpand(const RsGxsForumMsg &msg, QTreeWidgetItem *item);

	GxsForumThreadWidget *mParent;
	volatile bool mStopped;
};

#endif // GXSFORUMSFILLTHREAD_H
