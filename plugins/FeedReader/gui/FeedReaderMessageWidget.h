#ifndef FEEDREADERMESSAGEWIDGET_H
#define FEEDREADERMESSAGEWIDGET_H

#include <QWidget>

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
	void copyLinksMsg();
	void removeMsg();
	void processFeed();
	void openLinkMsg();
	void copyLinkMsg();

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
	QString mFeedName;
	unsigned int mUnreadCount;
	QTimer *timer;

	// gui interface
	RsFeedReader *mFeedReader;
	FeedReaderNotify *mNotify;

	Ui::FeedReaderMessageWidget *ui;
};

#endif // FEEDREADERMESSAGEWIDGET_H
