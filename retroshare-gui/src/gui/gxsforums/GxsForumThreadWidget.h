#ifndef GXSFORUMTHREADWIDGET_H
#define GXSFORUMTHREADWIDGET_H

#include <QWidget>

#include "util/TokenQueue.h"

class QTreeWidgetItem;
class GxsIdTreeWidgetItem;
class RSTreeWidgetItemCompareRole;
class RsGxsForumMsg;

namespace Ui {
class GxsForumThreadWidget;
}

/* These are all the parameters that are required for thread loading.
 * They are kept static for the load duration.
 */

class GxsForumsThreadLoadParameters
{
public:
	std::string ForumId;
	std::string FocusMsgId;

	uint32_t SubscribeFlags;
	int ViewType;
	uint32_t FilterColumn;

	std::map<uint32_t, QTreeWidgetItem *> MsgTokens;
	QList<QTreeWidgetItem*> Items;
	QList<QTreeWidgetItem*> ItemToExpand;

	bool FillComplete;
	bool FlatView;
	bool UseChildTS;
	bool ExpandNewMessages;
};

class GxsForumThreadWidget : public QWidget, public TokenResponse
{
	Q_OBJECT

	Q_PROPERTY(QColor textColorRead READ textColorRead WRITE setTextColorRead)
	Q_PROPERTY(QColor textColorUnread READ textColorUnread WRITE setTextColorUnread)
	Q_PROPERTY(QColor textColorUnreadChildren READ textColorUnreadChildren WRITE setTextColorUnreadChildren)
	Q_PROPERTY(QColor textColorNotSubscribed READ textColorNotSubscribed WRITE setTextColorNotSubscribed)
	Q_PROPERTY(QColor textColorMissing READ textColorMissing WRITE setTextColorMissing)

public:
	explicit GxsForumThreadWidget(const std::string &forumId, QWidget *parent = NULL);
	~GxsForumThreadWidget();
    
    QColor textColorRead() const { return mTextColorRead; }
    QColor textColorUnread() const { return mTextColorUnread; }
    QColor textColorUnreadChildren() const { return mTextColorUnreadChildren; }
    QColor textColorNotSubscribed() const { return mTextColorNotSubscribed; }
    QColor textColorMissing() const { return mTextColorMissing; }

	void setTextColorRead(QColor color) { mTextColorRead = color; }
	void setTextColorUnread(QColor color) { mTextColorUnread = color; }
	void setTextColorUnreadChildren(QColor color) { mTextColorUnreadChildren = color; }
	void setTextColorNotSubscribed(QColor color) { mTextColorNotSubscribed = color; }
	void setTextColorMissing(QColor color) { mTextColorMissing = color; }

	std::string forumId() { return mForumId; }
	QString forumName();

	// Callback for all Loads.
	virtual void loadRequest(const TokenQueue *queue, const TokenRequest &req);

signals:
	void forumChanged(QWidget *widget);

protected:
	bool eventFilter(QObject *obj, QEvent *ev);
	void changeEvent(QEvent *e);

private slots:
	/** Create the context popup menu and it's submenus */
	void threadListCustomPopupMenu(QPoint point);

	void changedThread();
	void clickedThread (QTreeWidgetItem *item, int column);

	void replytomessage();
	void replyMessageData(const RsGxsForumMsg &msg);

	//void print();
	//void printpreview();

	//void removemessage();
	void markMsgAsRead();
	void markMsgAsReadChildren();
	void markMsgAsReadAll();
	void markMsgAsUnread();
	void markMsgAsUnreadAll();
	void markMsgAsUnreadChildren();

	void copyMessageLink();

	/* handle splitter */
	void togglethreadview();

	void createthread();
	void createmessage();

	void previousMessage();
	void nextMessage();
	void nextUnreadMessage();
	void downloadAllFiles();

	void changedViewBox();

	void filterColumnChanged(int column);
	void filterItems(const QString &text);

	void fillThreadFinished();
//	void fillThreadProgress(int current, int count);

private:
	void insertForumThreads(const RsGroupMetaData &fi);
	void insertPostData(const RsGxsForumMsg &msg); // Second Half.

	void insertThreads();
	void insertPost();

//	void forumMsgReadStatusChanged(const QString &forumId, const QString &msgId, int status);

	void fillThreads(QList<QTreeWidgetItem *> &threadList, bool expandNewMessages, QList<QTreeWidgetItem*> &itemToExpand);
	void fillChildren(QTreeWidgetItem *parentItem, QTreeWidgetItem *newParentItem, bool expandNewMessages, QList<QTreeWidgetItem*> &itemToExpand);

	int getSelectedMsgCount(QList<QTreeWidgetItem*> *pRows, QList<QTreeWidgetItem*> *pRowsRead, QList<QTreeWidgetItem*> *pRowsUnread);
	void setMsgReadStatus(QList<QTreeWidgetItem*> &rows, bool read);
	void markMsgAsReadUnread(bool read, bool children, bool forum);
	void calculateIconsAndFonts(QTreeWidgetItem *item = NULL);
	void calculateIconsAndFonts(QTreeWidgetItem *item, bool &hasReadChilddren, bool &hasUnreadChilddren);

	void togglethreadview_internal();

	bool filterItem(QTreeWidgetItem *item, const QString &text, int filterColumn);

	void processSettings(bool bLoad);

	std::string mForumId;
	std::string mLastForumID;
	std::string mThreadId;
	int mSubscribeFlags;
	bool mInProcessSettings;
	bool mInMsgAsReadUnread;
	int mLastViewType;
	RSTreeWidgetItemCompareRole *mThreadCompareRole;
	TokenQueue *mThreadQueue;

	void requestGroupSummary_CurrentForum(const std::string &forumId);
	void loadGroupSummary_CurrentForum(const uint32_t &token);

	void loadCurrentForumThreads(const std::string &forumId);
	void requestGroupThreadData_InsertThreads(const std::string &forumId);
	void loadGroupThreadData_InsertThreads(const uint32_t &token);
	void loadForumBaseThread(const RsGxsForumMsg &msg);

	void requestChildData_InsertThreads(uint32_t &token, const RsGxsGrpMsgIdPair &parentId);
	void loadChildData_InsertThreads(const uint32_t &token);
	void loadForumChildMsg(const RsGxsForumMsg &msg, QTreeWidgetItem *parent);

	void requestMsgData_InsertPost(const RsGxsGrpMsgIdPair &msgId);
	void loadMsgData_InsertPost(const uint32_t &token);
	void requestMsgData_ReplyMessage(const RsGxsGrpMsgIdPair &msgId);
	void loadMsgData_ReplyMessage(const uint32_t &token);

	bool convertMsgToThreadWidget(const RsGxsForumMsg &msgInfo, bool useChildTS, uint32_t filterColumn, GxsIdTreeWidgetItem *item);
//	bool convertMsgToThreadWidget(const RsGxsForumMsg &msgInfo, std::string authorName, bool useChildTS, uint32_t filterColumn, QTreeWidgetItem *item);


	// New Datatypes to replace the FillThread.
	bool mThreadLoading;
	GxsForumsThreadLoadParameters mThreadLoad;

	/* Color definitions (for standard see qss.default) */
	QColor mTextColorRead;
	QColor mTextColorUnread;
	QColor mTextColorUnreadChildren;
	QColor mTextColorNotSubscribed;
	QColor mTextColorMissing;

	Ui::GxsForumThreadWidget *ui;
};

#endif // GXSFORUMTHREADWIDGET_H
