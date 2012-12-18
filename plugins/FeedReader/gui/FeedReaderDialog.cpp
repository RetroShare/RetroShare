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

#include <QDateTime>
#include <QKeyEvent>
#include <QMenu>
#include <QInputDialog>
#include <QPixmap>
#include <QPainter>
#include <QMessageBox>
#include <QClipboard>
#include <QDesktopServices>

#include "FeedReaderDialog.h"
#include "ui_FeedReaderDialog.h"
#include "FeedReaderNotify.h"
#include "AddFeedDialog.h"
#include "FeedReaderStringDefs.h"
#include "gui/common/RSTreeWidgetItem.h"
#include "util/HandleRichText.h"
#include "gui/settings/rsharesettings.h"

#include "interface/rsFeedReader.h"
#include "retroshare/rsiface.h"

#define COLUMN_FEED_COUNT   1
#define COLUMN_FEED_NAME    0
#define COLUMN_FEED_DATA    0

#define ROLE_FEED_ID          Qt::UserRole
#define ROLE_FEED_SORT        Qt::UserRole + 1
#define ROLE_FEED_FOLDER      Qt::UserRole + 2
#define ROLE_FEED_UNREAD      Qt::UserRole + 3
#define ROLE_FEED_NAME        Qt::UserRole + 4
#define ROLE_FEED_WORKSTATE   Qt::UserRole + 5
#define ROLE_FEED_LOADING     Qt::UserRole + 6
#define ROLE_FEED_ICON        Qt::UserRole + 7
#define ROLE_FEED_ERROR       Qt::UserRole + 8
#define ROLE_FEED_DEACTIVATED Qt::UserRole + 9

#define COLUMN_MSG_COUNT    4
#define COLUMN_MSG_TITLE    0
#define COLUMN_MSG_READ     1
#define COLUMN_MSG_PUBDATE  2
#define COLUMN_MSG_AUTHOR   3
#define COLUMN_MSG_DATA     0

#define ROLE_MSG_ID         Qt::UserRole
#define ROLE_MSG_SORT       Qt::UserRole + 1
#define ROLE_MSG_NEW        Qt::UserRole + 2
#define ROLE_MSG_READ       Qt::UserRole + 3
#define ROLE_MSG_LINK       Qt::UserRole + 4

FeedReaderDialog::FeedReaderDialog(RsFeedReader *feedReader, QWidget *parent)
	: MainPage(parent), mFeedReader(feedReader), ui(new Ui::FeedReaderDialog)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui->setupUi(this);

	mNotify = new FeedReaderNotify();
	mFeedReader->setNotify(mNotify);
	connect(mNotify, SIGNAL(notifyFeedChanged(QString,int)), this, SLOT(feedChanged(QString,int)));
	connect(mNotify, SIGNAL(notifyMsgChanged(QString,QString,int)), this, SLOT(msgChanged(QString,QString,int)));

	mProcessSettings = false;

	/* connect signals */
	connect(ui->feedTreeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(feedItemChanged(QTreeWidgetItem*)));
	connect(ui->msgTreeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(msgItemChanged()));
	connect(ui->msgTreeWidget, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(msgItemClicked(QTreeWidgetItem*,int)));

	connect(ui->feedTreeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(feedTreeCustomPopupMenu(QPoint)));
	connect(ui->msgTreeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(msgTreeCustomPopupMenu(QPoint)));

	connect(ui->filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterItems(QString)));
	connect(ui->filterLineEdit, SIGNAL(filterChanged(int)), this, SLOT(filterColumnChanged(int)));

	connect(ui->linkButton, SIGNAL(clicked()), this, SLOT(openLinkMsg()));
	connect(ui->expandButton, SIGNAL(clicked()), this, SLOT(toggleMsgText()));

	mFeedCompareRole = new RSTreeWidgetItemCompareRole;
	mFeedCompareRole->setRole(COLUMN_FEED_NAME, ROLE_FEED_SORT);

	mMsgCompareRole = new RSTreeWidgetItemCompareRole;
	mMsgCompareRole->setRole(COLUMN_MSG_TITLE, ROLE_MSG_SORT);
	mMsgCompareRole->setRole(COLUMN_MSG_READ, ROLE_MSG_SORT);
	mMsgCompareRole->setRole(COLUMN_MSG_PUBDATE, ROLE_MSG_SORT);
	mMsgCompareRole->setRole(COLUMN_MSG_AUTHOR, ROLE_MSG_SORT);

	/* initialize root item */
	mRootItem = new QTreeWidgetItem(ui->feedTreeWidget);
	QString name = tr("Message Folders");
	mRootItem->setText(COLUMN_FEED_NAME, name);
	mRootItem->setIcon(COLUMN_FEED_NAME, QIcon(":/images/Root.png"));
	mRootItem->setData(COLUMN_FEED_DATA, ROLE_FEED_NAME, name);
	mRootItem->setData(COLUMN_FEED_DATA, ROLE_FEED_FOLDER, true);
	mRootItem->setData(COLUMN_FEED_DATA, ROLE_FEED_ICON, QIcon(":/images/Root.png"));
	mRootItem->setExpanded(true);

	/* initialize msg list */
	ui->msgTreeWidget->sortItems(COLUMN_MSG_PUBDATE, Qt::DescendingOrder);

    /* set initial size the splitter */
    QList<int> sizes;
    sizes << 300 << width(); // Qt calculates the right sizes
    ui->splitter->setSizes(sizes);

	/* set header resize modes and initial section sizes */
	QHeaderView *header = ui->msgTreeWidget->header();
	header->setResizeMode(COLUMN_MSG_TITLE, QHeaderView::Interactive);
	header->resizeSection(COLUMN_MSG_TITLE, 350);
	header->resizeSection(COLUMN_MSG_PUBDATE, 140);
	header->resizeSection(COLUMN_MSG_AUTHOR, 150);

	/* set text of column "Read" to empty - without this the column has a number as header text */
	QTreeWidgetItem *headerItem = ui->msgTreeWidget->headerItem();
	headerItem->setText(COLUMN_MSG_READ, "");

	/* add filter actions */
	ui->filterLineEdit->addFilter(QIcon(), tr("Title"), COLUMN_MSG_TITLE, tr("Search Title"));
	ui->filterLineEdit->addFilter(QIcon(), tr("Date"), COLUMN_MSG_PUBDATE, tr("Search Date"));
	ui->filterLineEdit->addFilter(QIcon(), tr("Author"), COLUMN_MSG_AUTHOR, tr("Search Author"));
	ui->filterLineEdit->setCurrentFilter(COLUMN_MSG_TITLE);

	/* load settings */
	processSettings(true);

	/* Set header sizes for the fixed columns and resize modes, must be set after processSettings */
	header->resizeSection(COLUMN_MSG_READ, 24);
	header->setResizeMode(COLUMN_MSG_READ, QHeaderView::Fixed);

	/* initialize feed list */
	ui->feedTreeWidget->sortItems(COLUMN_FEED_NAME, Qt::AscendingOrder);

	/* build menu for link button */
	QMenu *menu = new QMenu(this);
	QAction *action = menu->addAction(tr("Open link in browser"), this, SLOT(openLinkMsg()));
	menu->addAction(tr("Copy link to clipboard"), this, SLOT(copyLinkMsg()));

	QFont font = action->font();
	font.setBold(true);
	action->setFont(font);

	ui->linkButton->setMenu(menu);
	ui->linkButton->setEnabled(false);

	ui->msgTreeWidget->installEventFilter(this);
}

FeedReaderDialog::~FeedReaderDialog()
{
	/* save settings */
	processSettings(false);

	delete(mFeedCompareRole);
	delete(mMsgCompareRole);
	delete(ui);

	mFeedReader->setNotify(NULL);
	delete(mNotify);
}

void FeedReaderDialog::processSettings(bool load)
{
	mProcessSettings = true;

	QHeaderView *header = ui->msgTreeWidget->header ();

	Settings->beginGroup(QString("FeedReaderDialog"));

	if (load) {
		// load settings

		// expandButton
		bool value = Settings->value("expandButton", true).toBool();
		ui->expandButton->setChecked(value);
		toggleMsgText_internal();

		// filterColumn
		ui->filterLineEdit->setCurrentFilter(Settings->value("filterColumn", COLUMN_MSG_TITLE).toInt());

		// state of thread tree
		header->restoreState(Settings->value("msgTree").toByteArray());

		// state of splitter
		ui->splitter->restoreState(Settings->value("Splitter").toByteArray());
		ui->msgSplitter->restoreState(Settings->value("msgSplitter").toByteArray());
	} else {
		// save settings

		// state of thread tree
		Settings->setValue("msgTree", header->saveState());

		// state of splitter
		Settings->setValue("Splitter", ui->splitter->saveState());
		Settings->setValue("msgSplitter", ui->msgSplitter->saveState());
	}

	Settings->endGroup();
	mProcessSettings = false;
}

void FeedReaderDialog::showEvent(QShowEvent */*event*/)
{
	updateFeeds("", mRootItem);
}

bool FeedReaderDialog::eventFilter(QObject *obj, QEvent *event)
{
	if (obj == ui->msgTreeWidget) {
		if (event->type() == QEvent::KeyPress) {
			QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
			if (keyEvent) {
				if (keyEvent->key() == Qt::Key_Space) {
					/* Space pressed */
					QTreeWidgetItem *item = ui->msgTreeWidget->currentItem();
					msgItemClicked(item, COLUMN_MSG_READ);
					return true; // eat event
				}
				if (keyEvent->key() == Qt::Key_Delete) {
					/* Delete pressed */
					removeMsg();
					return true; // eat event
				}
			}
		}
	}
	if (obj == ui->feedTreeWidget) {
		if (event->type() == QEvent::KeyPress) {
			QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
			if (keyEvent) {
				if (keyEvent->key() == Qt::Key_Delete) {
					/* Delete pressed */
					removeFeed();
					return true; // eat event
				}
			}
		}
	}
	/* pass the event on to the parent class */
	return MainPage::eventFilter(obj, event);
}

std::string FeedReaderDialog::currentFeedId()
{
	QTreeWidgetItem *item = ui->feedTreeWidget->currentItem();
	if (!item) {
		return "";
	}

	return item->data(COLUMN_FEED_DATA, ROLE_FEED_ID).toString().toStdString();
}

std::string FeedReaderDialog::currentMsgId()
{
	QTreeWidgetItem *item = ui->msgTreeWidget->currentItem();
	if (!item) {
		return "";
	}

	return item->data(COLUMN_MSG_DATA, ROLE_MSG_ID).toString().toStdString();
}

void FeedReaderDialog::feedTreeCustomPopupMenu(QPoint /*point*/)
{
	QMenu contextMnu(this);

	bool folder = false;
	QTreeWidgetItem *item = ui->feedTreeWidget->currentItem();
	if (item) {
		folder = item->data(COLUMN_FEED_DATA, ROLE_FEED_FOLDER).toBool();
	}

	QMenu *menu = contextMnu.addMenu(QIcon(""), tr("New"));
	QAction *action = menu->addAction(QIcon(":/images/FeedAdd.png"), tr("Feed"), this, SLOT(newFeed()));
	if (!item || !folder) {
		action->setEnabled(false);
	}
	action = menu->addAction(QIcon(":/images/FolderAdd.png"), tr("Folder"), this, SLOT(newFolder()));
	if (!item || !folder) {
		action->setEnabled(false);
	}

	contextMnu.addSeparator();

	action = contextMnu.addAction(QIcon(":/images/edit_16.png"), tr("Edit"), this, SLOT(editFeed()));
	if (!item || item == mRootItem) {
		action->setEnabled(false);
	}

	action = contextMnu.addAction(QIcon(":/images/delete.png"), tr("Delete"), this, SLOT(removeFeed()));
	if (!item || item == mRootItem) {
		action->setEnabled(false);
	}

	contextMnu.addSeparator();

	bool deactivated = false;
	if (!folder && item) {
		deactivated = item->data(COLUMN_FEED_DATA, ROLE_FEED_DEACTIVATED).toBool();
	}

	action = contextMnu.addAction(QIcon(":/images/Update.png"), tr("Update"), this, SLOT(processFeed()));
	action->setEnabled(!deactivated);

	action = contextMnu.addAction(QIcon(""), deactivated ? tr("Activate") : tr("Deactivate"), this, SLOT(activateFeed()));
	if (!item || item == mRootItem || folder) {
		action->setEnabled(false);
	}

	contextMnu.exec(QCursor::pos());
}

void FeedReaderDialog::msgTreeCustomPopupMenu(QPoint /*point*/)
{
	QMenu contextMnu(this);

	QList<QTreeWidgetItem*> selectedItems = ui->msgTreeWidget->selectedItems();

	QAction *action = contextMnu.addAction(QIcon(""), tr("Mark as read"), this, SLOT(markAsReadMsg()));
	action->setEnabled(!selectedItems.empty());

	action = contextMnu.addAction(QIcon(""), tr("Mark as unread"), this, SLOT(markAsUnreadMsg()));
	action->setEnabled(!selectedItems.empty());

	action = contextMnu.addAction(QIcon(""), tr("Mark all as read"), this, SLOT(markAllAsReadMsg()));

	contextMnu.addSeparator();

	action = contextMnu.addAction(QIcon(""), tr("Copy link"), this, SLOT(copyLinskMsg()));
	action->setEnabled(!selectedItems.empty());

	action = contextMnu.addAction(QIcon(""), tr("Remove"), this, SLOT(removeMsg()));
	action->setEnabled(!selectedItems.empty());

	contextMnu.exec(QCursor::pos());
}

void FeedReaderDialog::updateFeeds(const std::string &parentId, QTreeWidgetItem *parentItem)
{
	if (!parentItem) {
		return;
	}

	/* get feed infos */
	std::list<FeedInfo> feedInfos;
	mFeedReader->getFeedList(parentId, feedInfos);

	int index = 0;
	QTreeWidgetItem *item;
	std::list<FeedInfo>::iterator feedIt;

	/* update existing and delete not existing feeds */
	while (index < parentItem->childCount()) {
		item = parentItem->child(index);
		std::string feedId = item->data(COLUMN_FEED_DATA, ROLE_FEED_ID).toString().toStdString();

		/* search existing feed */
		int found = -1;
		for (feedIt = feedInfos.begin (); feedIt != feedInfos.end (); ++feedIt) {
			if (feedIt->feedId == feedId) {
				/* found it, update it */
				updateFeedItem(item, *feedIt);

				if (feedIt->flag.folder) {
					/* process child feeds */
					updateFeeds(feedIt->feedId, item);
				}

				feedInfos.erase(feedIt);
				found = index;
				break;
			}
		}
		if (found >= 0) {
			++index;
		} else {
			delete(parentItem->takeChild(index));
		}
	}

	/* add new feeds */
	for (feedIt = feedInfos.begin (); feedIt != feedInfos.end (); ++feedIt) {
		item = new RSTreeWidgetItem(mFeedCompareRole);
		parentItem->addChild(item);
		updateFeedItem(item, *feedIt);

		if (feedIt->flag.folder) {
			/* process child feeds */
			updateFeeds(feedIt->feedId, item);
		}
	}
	calculateFeedItems();
}

void FeedReaderDialog::calculateFeedItem(QTreeWidgetItem *item, uint32_t &unreadCount, bool &loading)
{
	uint32_t unreadCountItem = 0;
	bool loadingItem = false;

	if (item->data(COLUMN_FEED_DATA, ROLE_FEED_FOLDER).toBool()) {
		int childCount = item->childCount();
		for (int index = 0; index < childCount; ++index) {
			calculateFeedItem(item->child(index), unreadCountItem, loadingItem);
		}
	} else {
		unreadCountItem = item->data(COLUMN_FEED_DATA, ROLE_FEED_UNREAD).toUInt();
		loadingItem = item->data(COLUMN_FEED_DATA, ROLE_FEED_LOADING).toBool();
	}

	unreadCount += unreadCountItem;
	loading = loading || loadingItem;

	QString name = item->data(COLUMN_FEED_DATA, ROLE_FEED_NAME).toString();
	QString workState = item->data(COLUMN_FEED_DATA, ROLE_FEED_WORKSTATE).toString();

	if (unreadCountItem) {
		name += QString(" (%1)").arg(unreadCountItem);
	}
	if (!workState.isEmpty()) {
		name += QString(" (%1)").arg(workState);
	}

	item->setText(COLUMN_FEED_NAME, name);

	bool deactivated = item->data(COLUMN_FEED_DATA, ROLE_FEED_DEACTIVATED).toBool();

	QColor colorActivated;
	QColor colorDeactivated = QColor(Qt::gray);
	for (int i = 0; i < COLUMN_FEED_COUNT; i++) {
		QFont font = item->font(i);
		font.setBold(unreadCountItem != 0);
		item->setFont(i, font);

		item->setTextColor(COLUMN_FEED_NAME, deactivated ? colorDeactivated : colorActivated);
	}

	QIcon icon = item->data(COLUMN_FEED_DATA, ROLE_FEED_ICON).value<QIcon>();

	if (deactivated) {
		/* create disabled icon */
		icon = icon.pixmap(QSize(16, 16), QIcon::Disabled);
	}

	QImage overlayIcon;
	if (loadingItem) {
		/* overlaying icon */
		overlayIcon = QImage(":/images/FeedProcessOverlay.png");
	} else if (item->data(COLUMN_FEED_DATA, ROLE_FEED_ERROR).toBool()) {
		overlayIcon = QImage(":/images/FeedErrorOverlay.png");
	}
	if (!overlayIcon.isNull()) {
		if (icon.isNull()) {
			icon = QPixmap::fromImage(overlayIcon);
		} else {
			QPixmap pixmap = icon.pixmap(QSize(16, 16));
			QPainter painter(&pixmap);
			painter.drawImage(0, 0, overlayIcon.scaled(pixmap.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
			painter.end();
			icon = pixmap;
		}
	}

	item->setIcon(COLUMN_FEED_NAME, icon);
}

void FeedReaderDialog::calculateFeedItems()
{
	uint32_t unreadCount = 0;
	bool loading = false;

	calculateFeedItem(mRootItem, unreadCount, loading);
	ui->feedTreeWidget->sortItems(COLUMN_FEED_NAME, Qt::AscendingOrder);
}

void FeedReaderDialog::updateFeedItem(QTreeWidgetItem *item, FeedInfo &info)
{

	QIcon icon;
	if (info.flag.folder) {
		/* use folder icon */
		icon = QIcon(":/images/Folder.png");
	} else {
		long todo; // show icon from feed
//		if (info.icon.empty()) {
			/* use standard icon */
			icon = QIcon(":/images/Feed.png");
//		} else {
//			/* use icon from feed */
//			icon = QIcon(QPixmap::fromImage(QImage((uchar*) QByteArray::fromBase64(info.icon.c_str()).constData(), 16, 16, QImage::Format_RGB32)));
//		}
	}

	item->setData(COLUMN_FEED_DATA, ROLE_FEED_ICON, icon);

	QString name = QString::fromUtf8(info.name.c_str());
	item->setData(COLUMN_FEED_DATA, ROLE_FEED_NAME, name.isEmpty() ? tr("No name") : name);
	item->setData(COLUMN_FEED_DATA, ROLE_FEED_WORKSTATE, FeedReaderStringDefs::workState(info.workstate));

	uint32_t unreadCount;
	mFeedReader->getMessageCount(info.feedId, NULL, NULL, &unreadCount);

	item->setData(COLUMN_FEED_NAME, ROLE_FEED_SORT, QString("%1_%2").arg(QString(info.flag.folder ? "0" : "1"), name));
	item->setData(COLUMN_FEED_DATA, ROLE_FEED_UNREAD, unreadCount);
	item->setData(COLUMN_FEED_DATA, ROLE_FEED_LOADING, info.workstate != FeedInfo::WAITING);
	item->setData(COLUMN_FEED_DATA, ROLE_FEED_ID, QString::fromStdString(info.feedId));
	item->setData(COLUMN_FEED_DATA, ROLE_FEED_FOLDER, info.flag.folder);
	item->setData(COLUMN_FEED_DATA, ROLE_FEED_DEACTIVATED, info.flag.deactivated);
	item->setData(COLUMN_FEED_DATA, ROLE_FEED_ERROR, (bool) (info.errorState != RS_FEED_ERRORSTATE_OK));
	item->setToolTip(COLUMN_FEED_NAME, (info.errorState != RS_FEED_ERRORSTATE_OK) ? FeedReaderStringDefs::errorString(info) : "");
}

void FeedReaderDialog::updateMsgs(const std::string &feedId)
{
	std::list<FeedMsgInfo> msgInfos;
	if (!mFeedReader->getFeedMsgList(feedId, msgInfos)) {
		ui->msgTreeWidget->clear();
		return;
	}

	int index = 0;
	QTreeWidgetItem *item;
	std::list<FeedMsgInfo>::iterator msgIt;

	/* update existing and delete not existing msgs */
	while (index < ui->msgTreeWidget->topLevelItemCount()) {
		item = ui->msgTreeWidget->topLevelItem(index);
		std::string msgId = item->data(COLUMN_MSG_DATA, ROLE_MSG_ID).toString().toStdString();

		/* search existing msg */
		int found = -1;
		for (msgIt = msgInfos.begin (); msgIt != msgInfos.end (); ++msgIt) {
			if (msgIt->msgId == msgId) {
				/* found it, update it */
				updateMsgItem(item, *msgIt);

				msgInfos.erase(msgIt);
				found = index;
				break;
			}
		}
		if (found >= 0) {
			++index;
		} else {
			delete(ui->msgTreeWidget->takeTopLevelItem(index));
		}
	}

	/* add new msgs */
	for (msgIt = msgInfos.begin (); msgIt != msgInfos.end (); ++msgIt) {
		item = new RSTreeWidgetItem(mMsgCompareRole);
		updateMsgItem(item, *msgIt);
		ui->msgTreeWidget->addTopLevelItem(item);
	}

	filterItems(ui->filterLineEdit->text());
}

void FeedReaderDialog::calculateMsgIconsAndFonts(QTreeWidgetItem *item)
{
	if (!item) {
		return;
	}

	bool isnew = item->data(COLUMN_MSG_DATA, ROLE_MSG_NEW).toBool();
	bool read = item->data(COLUMN_MSG_DATA, ROLE_MSG_READ).toBool();

	if (read) {
		item->setIcon(COLUMN_MSG_READ, QIcon(":/images/message-state-read.png"));
	} else {
		item->setIcon(COLUMN_MSG_READ, QIcon(":/images/message-state-unread.png"));
	}
	if (isnew) {
		item->setIcon(COLUMN_MSG_TITLE, QIcon(":/images/message-state-new.png"));
	} else {
		item->setIcon(COLUMN_MSG_TITLE, QIcon());
	}

	for (int i = 0; i < COLUMN_FEED_COUNT; i++) {
		QFont font = item->font(i);
		font.setBold(isnew || !read);
		item->setFont(i, font);
	}

	item->setData(COLUMN_MSG_READ, ROLE_MSG_SORT, QString("%1_%2_%3").arg(QString(isnew ? "1" : "0"), QString(read ? "0" : "1"), item->data(COLUMN_MSG_TITLE, ROLE_MSG_SORT).toString()));
}

void FeedReaderDialog::updateMsgItem(QTreeWidgetItem *item, FeedMsgInfo &info)
{
	QString title = QString::fromUtf8(info.title.c_str());
	QDateTime qdatetime;
	qdatetime.setTime_t(info.pubDate);

	/* add string to all data */
	QString sort = QString("%1_%2_%2").arg(title, qdatetime.toString("yyyyMMdd_hhmmss"), QString::fromStdString(info.feedId));

	item->setText(COLUMN_MSG_TITLE, title);
	item->setData(COLUMN_MSG_TITLE, ROLE_MSG_SORT, sort);

	QString author = QString::fromUtf8(info.author.c_str());
	item->setText(COLUMN_MSG_AUTHOR, author);
	item->setData(COLUMN_MSG_AUTHOR, ROLE_MSG_SORT, author + "_" + sort);

	/* if the message is on same date show only time */
	if (qdatetime.daysTo(QDateTime::currentDateTime()) == 0) {
		item->setData(COLUMN_MSG_PUBDATE, Qt::DisplayRole, qdatetime.time());
	} else {
		item->setData(COLUMN_MSG_PUBDATE, Qt::DisplayRole, qdatetime);
	}
	item->setData(COLUMN_MSG_PUBDATE, ROLE_MSG_SORT, QString("%1_%2_%3").arg(qdatetime.toString("yyyyMMdd_hhmmss"), title, QString::fromStdString(info.msgId)));

	item->setData(COLUMN_MSG_DATA, ROLE_MSG_ID, QString::fromStdString(info.msgId));
	item->setData(COLUMN_MSG_DATA, ROLE_MSG_LINK, QString::fromUtf8(info.link.c_str()));
	item->setData(COLUMN_MSG_DATA, ROLE_MSG_READ, info.flag.read);
	item->setData(COLUMN_MSG_DATA, ROLE_MSG_NEW, info.flag.isnew);

	calculateMsgIconsAndFonts(item);
}

void FeedReaderDialog::feedChanged(const QString &feedId, int type)
{
	if (!isVisible()) {
		/* complete update in showEvent */
		return;
	}

	if (feedId.isEmpty()) {
		return;
	}

	FeedInfo feedInfo;
	if (type != NOTIFY_TYPE_DEL) {
		if (!mFeedReader->getFeedInfo(feedId.toStdString(), feedInfo)) {
			return;
		}

		if (feedInfo.flag.preview) {
			return;
		}
	}

	if (type == NOTIFY_TYPE_MOD || type == NOTIFY_TYPE_DEL) {
		QTreeWidgetItemIterator it(ui->feedTreeWidget);
		QTreeWidgetItem *item;
		while ((item = *it) != NULL) {
			if (item->data(COLUMN_FEED_DATA, ROLE_FEED_ID).toString() == feedId) {
				if (type == NOTIFY_TYPE_MOD) {
					updateFeedItem(item, feedInfo);
				} else {
					delete(item);
				}
				break;
			}
			++it;
		}
	}

	if (type == NOTIFY_TYPE_ADD) {
		QString id = QString::fromStdString(feedInfo.parentId);

		QTreeWidgetItemIterator it(ui->feedTreeWidget);
		QTreeWidgetItem *itemParent;
		while ((itemParent = *it) != NULL) {
			if (itemParent->data(COLUMN_FEED_DATA, ROLE_FEED_ID).toString() == id) {
				QTreeWidgetItem *item = new RSTreeWidgetItem(mFeedCompareRole);
				itemParent->addChild(item);
				updateFeedItem(item, feedInfo);
				break;
			}
			++it;
		}
	}
	calculateFeedItems();
}

void FeedReaderDialog::msgChanged(const QString &feedId, const QString &msgId, int type)
{
	if (!isVisible()) {
		/* complete update in showEvent */
		return;
	}

	if (feedId.isEmpty() || msgId.isEmpty()) {
		return;
	}

	if (feedId.toStdString() != currentFeedId()) {
		return;
	}

	FeedMsgInfo msgInfo;
	if (type != NOTIFY_TYPE_DEL) {
		if (!mFeedReader->getMsgInfo(feedId.toStdString(), msgId.toStdString(), msgInfo)) {
			return;
		}
	}

	if (type == NOTIFY_TYPE_MOD || type == NOTIFY_TYPE_DEL) {
		QTreeWidgetItemIterator it(ui->msgTreeWidget);
		QTreeWidgetItem *item;
		while ((item = *it) != NULL) {
			if (item->data(COLUMN_MSG_DATA, ROLE_MSG_ID).toString() == msgId) {
				if (type == NOTIFY_TYPE_MOD) {
					updateMsgItem(item, msgInfo);
					filterItem(item);
				} else {
					delete(item);
				}
				break;
			}
			++it;
		}
	}

	if (type == NOTIFY_TYPE_ADD) {
		QTreeWidgetItem *item = new RSTreeWidgetItem(mMsgCompareRole);
		updateMsgItem(item, msgInfo);
		ui->msgTreeWidget->addTopLevelItem(item);
		filterItem(item);
	}
}

void FeedReaderDialog::feedItemChanged(QTreeWidgetItem *item)
{
	if (!item) {
		ui->msgTreeWidget->clear();
		return;
	}

	std::string feedId = item->data(COLUMN_FEED_DATA, ROLE_FEED_ID).toString().toStdString();
	updateMsgs(feedId);
}

void FeedReaderDialog::msgItemClicked(QTreeWidgetItem *item, int column)
{
	if (item == NULL) {
		return;
	}

	if (column == COLUMN_MSG_READ) {
		QList<QTreeWidgetItem*> rows;
		rows.append(item);
		bool read = item->data(COLUMN_MSG_DATA, ROLE_MSG_READ).toBool();
		setMsgAsReadUnread(rows, !read);
		return;
	}
}

void FeedReaderDialog::msgItemChanged()
{
	long todo; // show link somewhere

	std::string feedId = currentFeedId();
	std::string msgId = currentMsgId();

	if (feedId.empty() || msgId.empty()) {
		ui->msgTitle->clear();
//		ui->msgLink->clear();
		ui->msgText->clear();
		ui->linkButton->setEnabled(false);
		return;
	}

	QTreeWidgetItem *item = ui->msgTreeWidget->currentItem();
	if (!item) {
		/* there is something wrong */
		ui->msgTitle->clear();
//		ui->msgLink->clear();
		ui->msgText->clear();
		ui->linkButton->setEnabled(false);
		return;
	}

	/* get msg */
	FeedMsgInfo msgInfo;
	if (!mFeedReader->getMsgInfo(feedId, msgId, msgInfo)) {
		ui->msgTitle->clear();
//		ui->msgLink->clear();
		ui->msgText->clear();
		ui->linkButton->setEnabled(false);
		return;
	}

	bool setToReadOnActive = Settings->valueFromGroup("FeedReaderDialog", "SetMsgToReadOnActivate", true).toBool();
	bool isnew = item->data(COLUMN_MSG_DATA, ROLE_MSG_NEW).toBool();
	bool read = item->data(COLUMN_MSG_DATA, ROLE_MSG_READ).toBool();

	QList<QTreeWidgetItem*> row;
	row.append(item);
	if (read) {
		if (isnew) {
			/* something wrong, but set as read again to clear the new flag */
			setMsgAsReadUnread(row, true);
		}
	} else {
		/* set to read/unread */
		setMsgAsReadUnread(row, setToReadOnActive);
	}

	QString msgTxt = RsHtml().formatText(ui->msgText->document(), QString::fromUtf8(msgInfo.description.c_str()), RSHTML_FORMATTEXT_EMBED_LINKS);

	ui->msgText->setHtml(msgTxt);
	ui->msgTitle->setText(QString::fromUtf8(msgInfo.title.c_str()));

	ui->linkButton->setEnabled(!msgInfo.link.empty());
}

void FeedReaderDialog::setMsgAsReadUnread(QList<QTreeWidgetItem *> &rows, bool read)
{
	QList<QTreeWidgetItem*>::iterator rowIt;

	std::string feedId = currentFeedId();
	if (feedId.empty()) {
		return;
	}

	for (rowIt = rows.begin(); rowIt != rows.end(); rowIt++) {
		QTreeWidgetItem *item = *rowIt;
		bool rowRead = item->data(COLUMN_MSG_DATA, ROLE_MSG_READ).toBool();
		bool rowNew = item->data(COLUMN_MSG_DATA, ROLE_MSG_NEW).toBool();

		if (rowNew || read != rowRead) {
			std::string msgId = item->data(COLUMN_MSG_DATA, ROLE_MSG_ID).toString().toStdString();
			mFeedReader->setMessageRead(feedId, msgId, read);
		}
	}
}

void FeedReaderDialog::filterColumnChanged(int column)
{
	if (mProcessSettings) {
		return;
	}

	filterItems(ui->filterLineEdit->text());

	// save index
	Settings->setValueToGroup("FeedReaderDialog", "filterColumn", column);
}

void FeedReaderDialog::filterItems(const QString& text)
{
	int filterColumn = ui->filterLineEdit->currentFilter();

	int count = ui->msgTreeWidget->topLevelItemCount();
	for (int index = 0; index < count; ++index) {
		filterItem(ui->msgTreeWidget->topLevelItem(index), text, filterColumn);
	}
}

void FeedReaderDialog::filterItem(QTreeWidgetItem *item, const QString &text, int filterColumn)
{
	if (text.isEmpty() == false) {
		if (item->text(filterColumn).contains(text, Qt::CaseInsensitive)) {
			item->setHidden(false);
		} else {
			item->setHidden(true);
		}
	} else {
		item->setHidden(false);
	}
}

void FeedReaderDialog::filterItem(QTreeWidgetItem *item)
{
	filterItem(item, ui->filterLineEdit->text(), ui->filterLineEdit->currentFilter());
}

void FeedReaderDialog::toggleMsgText()
{
	// save state of button
	Settings->setValueToGroup("FeedReaderDialog", "expandButton", ui->expandButton->isChecked());

	toggleMsgText_internal();
}

void FeedReaderDialog::toggleMsgText_internal()
{
	if (ui->expandButton->isChecked()) {
		ui->msgText->setVisible(true);
		ui->expandButton->setIcon(QIcon(QString(":/images/edit_remove24.png")));
		ui->expandButton->setToolTip(tr("Hide"));
	} else  {
		ui->msgText->setVisible(false);
		ui->expandButton->setIcon(QIcon(QString(":/images/edit_add24.png")));
		ui->expandButton->setToolTip(tr("Expand"));
	}
}

void FeedReaderDialog::newFolder()
{
	QInputDialog dialog;
	dialog.setWindowTitle(tr("Add new folder"));
	dialog.setLabelText(tr("Please enter a name for the folder"));
	dialog.setWindowIcon(QIcon(":/images/FeedReader.png"));

	if (dialog.exec() == QDialog::Accepted && !dialog.textValue().isEmpty()) {
		std::string feedId;
		RsFeedAddResult result = mFeedReader->addFolder(currentFeedId(), dialog.textValue().toUtf8().constData(), feedId);
		FeedReaderStringDefs::showError(this, result, tr("Create folder"), tr("Cannot create folder."));
	}
}

void FeedReaderDialog::newFeed()
{
	AddFeedDialog dialog(mFeedReader, mNotify, this);
	dialog.setParent(currentFeedId());
	dialog.exec();
}

void FeedReaderDialog::removeFeed()
{
	std::string feedId = currentFeedId();
	if (feedId.empty()) {
		return;
	}

	QTreeWidgetItem *item = ui->feedTreeWidget->currentItem();
	if (!item) {
		return;
	}

	bool folder = item->data(COLUMN_FEED_DATA, ROLE_FEED_FOLDER).toBool();
	QString name = item->data(COLUMN_FEED_DATA, ROLE_FEED_NAME).toString();

	if (QMessageBox::question(this, folder ? tr("Remove folder") : tr("Remove feed"), folder ? tr("Do you want to remove the folder %1?").arg(name) : tr("Do you want to remove the feed %1?").arg(name), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
		mFeedReader->removeFeed(feedId);
	}
}

void FeedReaderDialog::editFeed()
{
	std::string feedId = currentFeedId();
	if (feedId.empty()) {
		return;
	}

	QTreeWidgetItem *item = ui->feedTreeWidget->currentItem();
	if (!item) {
		return;
	}

	bool folder = item->data(COLUMN_FEED_DATA, ROLE_FEED_FOLDER).toBool();
	if (folder) {
		QInputDialog dialog;
		dialog.setWindowTitle(tr("Edit folder"));
		dialog.setLabelText(tr("Please enter a new name for the folder"));
		dialog.setWindowIcon(QIcon(":/images/FeedReader.png"));
		dialog.setTextValue(item->data(COLUMN_FEED_DATA, ROLE_FEED_NAME).toString());

		if (dialog.exec() == QDialog::Accepted && !dialog.textValue().isEmpty()) {
			RsFeedAddResult result = mFeedReader->setFolder(feedId, dialog.textValue().toUtf8().constData());
			FeedReaderStringDefs::showError(this, result, tr("Create folder"), tr("Cannot create folder."));
		}
	} else {
		AddFeedDialog dialog(mFeedReader, mNotify, this);
		if (!dialog.fillFeed(feedId)) {
			return;
		}
		dialog.exec();

	}
}

void FeedReaderDialog::activateFeed()
{
	std::string feedId = currentFeedId();
	if (feedId.empty()) {
		return;
	}

	FeedInfo feedInfo;
	if (!mFeedReader->getFeedInfo(feedId, feedInfo)) {
		return;
	}

	if (feedInfo.flag.folder) {
		return;
	}

	feedInfo.flag.deactivated = !feedInfo.flag.deactivated;

	mFeedReader->setFeed(feedId, feedInfo);
}

void FeedReaderDialog::processFeed()
{
	std::string feedId = currentFeedId();
	/* empty feed id process all feeds */

	mFeedReader->processFeed(feedId);
}

void FeedReaderDialog::markAsReadMsg()
{
	QList<QTreeWidgetItem*> selectedItems = ui->msgTreeWidget->selectedItems();
	setMsgAsReadUnread(selectedItems, true);
}

void FeedReaderDialog::markAsUnreadMsg()
{
	QList<QTreeWidgetItem*> selectedItems = ui->msgTreeWidget->selectedItems();
	setMsgAsReadUnread(selectedItems, false);
}

void FeedReaderDialog::markAllAsReadMsg()
{
	QList<QTreeWidgetItem*> items;

	QTreeWidgetItemIterator it(ui->msgTreeWidget);
	QTreeWidgetItem *item;
	while ((item = *it) != NULL) {
		if (!item->isHidden()) {
			items.push_back(item);
		}
		++it;
	}
	setMsgAsReadUnread(items, true);
}

void FeedReaderDialog::copyLinksMsg()
{
	QString links;

	QTreeWidgetItemIterator it(ui->msgTreeWidget, QTreeWidgetItemIterator::Selected);
	QTreeWidgetItem *item;
	while ((item = *it) != NULL) {
		QString link = item->data(COLUMN_MSG_DATA, ROLE_MSG_LINK).toString();
		if (!link.isEmpty()) {
			links += link + "\n";
		}
		++it;
	}

	if (links.isEmpty()) {
		return;
	}

	QApplication::clipboard()->setText(links);
}

void FeedReaderDialog::removeMsg()
{
	std::string feedId = currentFeedId();
	if (feedId.empty()) {
		return;
	}

	QList<QTreeWidgetItem*> selectedItems = ui->msgTreeWidget->selectedItems();
	QList<QTreeWidgetItem*>::iterator it;
	std::list<std::string> msgIds;

	for (it = selectedItems.begin(); it != selectedItems.end(); ++it) {
		msgIds.push_back((*it)->data(COLUMN_MSG_DATA, ROLE_MSG_ID).toString().toStdString());
	}
	mFeedReader->removeMsgs(feedId, msgIds);
}

void FeedReaderDialog::copyLinkMsg()
{
	QTreeWidgetItem *item = ui->msgTreeWidget->currentItem();
	if (!item) {
		return;
	}

	QString link = item->data(COLUMN_MSG_DATA, ROLE_MSG_LINK).toString();
	if (link.isEmpty()) {
		return;
	}

	QApplication::clipboard()->setText(link);
}

void FeedReaderDialog::openLinkMsg()
{
	QTreeWidgetItem *item = ui->msgTreeWidget->currentItem();
	if (!item) {
		return;
	}

	QString link = item->data(COLUMN_MSG_DATA, ROLE_MSG_LINK).toString();
	if (link.isEmpty()) {
		return;
	}

	QDesktopServices::openUrl(QUrl(link));
}
