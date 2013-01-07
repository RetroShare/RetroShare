/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2012, RetroShare Team
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
#include <QMessageBox>
#include <QKeyEvent>

#include "GxsForumThreadWidget.h"
#include "ui_GxsForumThreadWidget.h"
#include "GxsForumsFillThread.h"
#include "gui/GxsForumsDialog.h"
#include "gui/RetroShareLink.h"
#include "gui/common/RSTreeWidgetItem.h"
#include "gui/common/RSItemDelegate.h"
#include "gui/settings/rsharesettings.h"
#include "gui/gxs/GxsIdTreeWidgetItem.h"
#include "util/HandleRichText.h"
#include "CreateGxsForumMsg.h"
#include "gui/msgs/MessageComposer.h"
#include "util/DateTime.h"

#include <retroshare/rsgxsforums.h>
#include <retroshare/rspeers.h>
// These should be in retroshare/ folder.
#include "gxs/rsgxsflags.h"

#include <iostream>

#define DEBUG_FORUMS

/* Images for context menu icons */
#define IMAGE_MESSAGE        ":/images/folder-draft.png"
#define IMAGE_MESSAGEREPLY   ":/images/mail_reply.png"
#define IMAGE_MESSAGEREMOVE  ":/images/mail_delete.png"
#define IMAGE_DOWNLOAD       ":/images/start.png"
#define IMAGE_DOWNLOADALL    ":/images/startall.png"
#define IMAGE_COPYLINK       ":/images/copyrslink.png"

#define VIEW_LAST_POST	0
#define VIEW_THREADED	1
#define VIEW_FLAT       2

/* Thread constants */
#define COLUMN_THREAD_COUNT    6
#define COLUMN_THREAD_TITLE    0
#define COLUMN_THREAD_READ     1
#define COLUMN_THREAD_DATE     2
#define COLUMN_THREAD_AUTHOR   3
#define COLUMN_THREAD_SIGNED   4
#define COLUMN_THREAD_CONTENT  5

#define COLUMN_THREAD_DATA     0 // column for storing the userdata like msgid and parentid

#define ROLE_THREAD_MSGID           Qt::UserRole
#define ROLE_THREAD_STATUS          Qt::UserRole + 1
#define ROLE_THREAD_MISSING         Qt::UserRole + 2
// no need to copy, don't count in ROLE_THREAD_COUNT
#define ROLE_THREAD_READCHILDREN    Qt::UserRole + 3
#define ROLE_THREAD_UNREADCHILDREN  Qt::UserRole + 4
#define ROLE_THREAD_SORT            Qt::UserRole + 5

#define ROLE_THREAD_COUNT           3

#define TOKEN_TYPE_CURRENTFORUM     1
#define TOKEN_TYPE_INSERT_POST      2
#define TOKEN_TYPE_REPLY_MESSAGE    3

GxsForumThreadWidget::GxsForumThreadWidget(const std::string &forumId, QWidget *parent) :
	QWidget(parent),
	ui(new Ui::GxsForumThreadWidget)
{
	ui->setupUi(this);

	mSubscribeFlags = 0;
	mInProcessSettings = false;
	mUnreadCount = 0;
	mNewCount = 0;

	mThreadQueue = new TokenQueue(rsGxsForums->getTokenService(), this);

	mInMsgAsReadUnread = false;

	mThreadCompareRole = new RSTreeWidgetItemCompareRole;
	mThreadCompareRole->setRole(COLUMN_THREAD_DATE, ROLE_THREAD_SORT);

	connect(ui->threadTreeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(threadListCustomPopupMenu(QPoint)));

	connect(ui->newmessageButton, SIGNAL(clicked()), this, SLOT(createmessage()));
	connect(ui->newthreadButton, SIGNAL(clicked()), this, SLOT(createthread()));

	connect(ui->threadTreeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(changedThread()));
	connect(ui->threadTreeWidget, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(clickedThread(QTreeWidgetItem*,int)));
	connect(ui->viewBox, SIGNAL(currentIndexChanged(int)), this, SLOT(changedViewBox()));

	connect(ui->expandButton, SIGNAL(clicked()), this, SLOT(togglethreadview()));
	connect(ui->previousButton, SIGNAL(clicked()), this, SLOT(previousMessage()));
	connect(ui->nextButton, SIGNAL(clicked()), this, SLOT(nextMessage()));
	connect(ui->nextUnreadButton, SIGNAL(clicked()), this, SLOT(nextUnreadMessage()));
	connect(ui->downloadButton, SIGNAL(clicked()), this, SLOT(downloadAllFiles()));

	// HACK - TEMPORARY HIJACKING THIS BUTTON FOR REFRESH.
	connect(ui->refreshButton, SIGNAL(clicked()), this, SLOT(forceUpdateDisplay()));

	connect(ui->filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterItems(QString)));
	connect(ui->filterLineEdit, SIGNAL(filterChanged(int)), this, SLOT(filterColumnChanged(int)));

	/* Set own item delegate */
	RSItemDelegate *itemDelegate = new RSItemDelegate(this);
	itemDelegate->setSpacing(QSize(0, 2));
	ui->threadTreeWidget->setItemDelegate(itemDelegate);

	/* Set header resize modes and initial section sizes */
	QHeaderView * ttheader = ui->threadTreeWidget->header () ;
	ttheader->setResizeMode (COLUMN_THREAD_TITLE, QHeaderView::Interactive);
	ttheader->resizeSection (COLUMN_THREAD_DATE,  140);
	ttheader->resizeSection (COLUMN_THREAD_TITLE, 290);

	ui->threadTreeWidget->sortItems(COLUMN_THREAD_DATE, Qt::DescendingOrder);

	/* Set text of column "Read" to empty - without this the column has a number as header text */
	QTreeWidgetItem *headerItem = ui->threadTreeWidget->headerItem();
	headerItem->setText(COLUMN_THREAD_READ, "");

	/* add filter actions */
	ui->filterLineEdit->addFilter(QIcon(), tr("Title"), COLUMN_THREAD_TITLE, tr("Search Title"));
	ui->filterLineEdit->addFilter(QIcon(), tr("Date"), COLUMN_THREAD_DATE, tr("Search Date"));
	ui->filterLineEdit->addFilter(QIcon(), tr("Author"), COLUMN_THREAD_AUTHOR, tr("Search Author"));
	ui->filterLineEdit->addFilter(QIcon(), tr("Content"), COLUMN_THREAD_CONTENT, tr("Search Content"));
	ui->filterLineEdit->setCurrentFilter(COLUMN_THREAD_TITLE);

	mLastViewType = -1;

	// load settings
	processSettings(true);

	/* Set header sizes for the fixed columns and resize modes, must be set after processSettings */
	ttheader->resizeSection (COLUMN_THREAD_READ,  24);
	ttheader->setResizeMode (COLUMN_THREAD_READ, QHeaderView::Fixed);
	ttheader->hideSection (COLUMN_THREAD_CONTENT);

	ui->progressBar->hide();
	ui->progressText->hide();
	ui->progressBarLayout->setEnabled(false);

//	mTimer = new QTimer;
//	mTimer->setInterval(1000);
//	mTimer->setSingleShot(true);

//	QObject::connect(mTimer, SIGNAL(timeout()), this, SLOT(updateDisplay()));

//	mTimer->start();

	mFillThread = NULL;

	setForumId(forumId);

	ui->threadTreeWidget->installEventFilter(this);
}

GxsForumThreadWidget::~GxsForumThreadWidget()
{
	if (mFillThread) {
		mFillThread->stop();
		delete(mFillThread);
		mFillThread = NULL;
	}

	delete ui;

//	delete(mTimer);
	delete(mThreadQueue);
	delete(mThreadCompareRole);
}

void GxsForumThreadWidget::processSettings(bool load)
{
	mInProcessSettings = true;

	QHeaderView *header = ui->threadTreeWidget->header();

	Settings->beginGroup(QString("GxsForumsDialog"));

	if (load) {
		// load settings

		// expandFiles
		bool bValue = Settings->value("expandButton", true).toBool();
		ui->expandButton->setChecked(bValue);
		togglethreadview_internal();

		// filterColumn
		ui->filterLineEdit->setCurrentFilter(Settings->value("filterColumn", COLUMN_THREAD_TITLE).toInt());

		// index of viewBox
		ui->viewBox->setCurrentIndex(Settings->value("viewBox", VIEW_THREADED).toInt());

		// state of thread tree
		header->restoreState(Settings->value("ThreadTree").toByteArray());

		// state of splitter
		ui->threadSplitter->restoreState(Settings->value("threadSplitter").toByteArray());
	} else {
		// save settings

		// state of thread tree
		Settings->setValue("ThreadTree", header->saveState());

		// state of splitter
		Settings->setValue("threadSplitter", ui->threadSplitter->saveState());
	}

	Settings->endGroup();
	mInProcessSettings = false;
}

void GxsForumThreadWidget::setForumId(const std::string &forumId)
{
	if (mForumId == forumId) {
		if (!forumId.empty()) {
			return;
		}
	}

	mForumId = forumId;
	ui->forumName->setText(mForumId.empty () ? "" : tr("Loading"));
	mNewCount = 0;
	mUnreadCount = 0;

	emit forumChanged(this);

	insertThreads();
}

QString GxsForumThreadWidget::forumName(bool withUnreadCount)
{
	QString name = mForumId.empty () ? tr("No name") : ui->forumName->text();

	if (withUnreadCount && mUnreadCount) {
		name += QString(" (%1)").arg(mUnreadCount);
	}

	return name;
}

QIcon GxsForumThreadWidget::forumIcon()
{
	if (mThreadQueue->activeRequestExist(TOKEN_TYPE_CURRENTFORUM) || mFillThread) {
		return QIcon(":/images/kalarm.png");
	}

	if (mNewCount) {
		return QIcon(":/images/message-state-new.png");
	}

	return QIcon();
}

void GxsForumThreadWidget::updateInterface()
{
	if (mFillThread) {
		ui->newthreadButton->setEnabled(false);
	} else {
		if (mThreadQueue->activeRequestExist(TOKEN_TYPE_CURRENTFORUM)) {
			ui->newthreadButton->setEnabled(false);
		} else {
			ui->newthreadButton->setEnabled (IS_GROUP_SUBSCRIBED(mSubscribeFlags));
		}
	}

	emit forumChanged(this);
}

void GxsForumThreadWidget::changeEvent(QEvent *e)
{
	QWidget::changeEvent(e);
	switch (e->type()) {
	case QEvent::StyleChange:
		calculateIconsAndFonts();
		break;
	default:
		// remove compiler warnings
		break;
	}
}

#ifdef TODO
void GxsForumThreadWidget::updateDisplay()
{
	std::list<std::string> forumIds;
	std::list<std::string>::iterator it;
	if (!rsGxsForums)
		return;

#if 0
	// TODO groupsChanged... HACK XXX.
	if ((rsGxsForums->groupsChanged(forumIds)) || (rsGxsForums->updated()))
	{
		/* update Forums List */
		insertForums();

		it = std::find(forumIds.begin(), forumIds.end(), mCurrForumId);
		if (it != forumIds.end())
		{
			/* update threads as well */
			insertThreads();
		}
	}
#endif

	/* The proper version (above) can be done with a data request -> TODO */
	if (rsGxsForums->updated())
	{
		/* update Forums List */
		insertThreads();
	}
}
#endif

void GxsForumThreadWidget::threadListCustomPopupMenu(QPoint /*point*/)
{
	if (mFillThread) {
		return;
	}

	QMenu contextMnu(this);

	QAction *replyAct = new QAction(QIcon(IMAGE_MESSAGEREPLY), tr("Reply"), &contextMnu);
	connect(replyAct, SIGNAL(triggered()), this, SLOT(createmessage()));

	QAction *newthreadAct = new QAction(QIcon(IMAGE_DOWNLOADALL), tr("Start New Thread"), &contextMnu);
	newthreadAct->setEnabled (IS_GROUP_SUBSCRIBED(mSubscribeFlags));
	connect(newthreadAct , SIGNAL(triggered()), this, SLOT(createthread()));

	QAction *replyauthorAct = new QAction(QIcon(IMAGE_MESSAGEREPLY), tr("Reply to Author"), &contextMnu);
	connect(replyauthorAct, SIGNAL(triggered()), this, SLOT(replytomessage()));

	QAction* expandAll = new QAction(tr("Expand all"), &contextMnu);
	connect(expandAll, SIGNAL(triggered()), ui->threadTreeWidget, SLOT(expandAll()));

	QAction* collapseAll = new QAction(tr( "Collapse all"), &contextMnu);
	connect(collapseAll, SIGNAL(triggered()), ui->threadTreeWidget, SLOT(collapseAll()));

	QAction *markMsgAsRead = new QAction(QIcon(":/images/message-mail-read.png"), tr("Mark as read"), &contextMnu);
	connect(markMsgAsRead, SIGNAL(triggered()), this, SLOT(markMsgAsRead()));

	QAction *markMsgAsReadChildren = new QAction(QIcon(":/images/message-mail-read.png"), tr("Mark as read") + " (" + tr ("with children") + ")", &contextMnu);
	connect(markMsgAsReadChildren, SIGNAL(triggered()), this, SLOT(markMsgAsReadChildren()));

	QAction *markMsgAsUnread = new QAction(QIcon(":/images/message-mail.png"), tr("Mark as unread"), &contextMnu);
	connect(markMsgAsUnread, SIGNAL(triggered()), this, SLOT(markMsgAsUnread()));

	QAction *markMsgAsUnreadChildren = new QAction(QIcon(":/images/message-mail.png"), tr("Mark as unread") + " (" + tr ("with children") + ")", &contextMnu);
	connect(markMsgAsUnreadChildren, SIGNAL(triggered()), this, SLOT(markMsgAsUnreadChildren()));

	if (IS_GROUP_SUBSCRIBED(mSubscribeFlags)) {
		QList<QTreeWidgetItem*> rows;
		QList<QTreeWidgetItem*> rowsRead;
		QList<QTreeWidgetItem*> rowsUnread;
		int nCount = getSelectedMsgCount(&rows, &rowsRead, &rowsUnread);

		if (rowsUnread.size() == 0) {
			markMsgAsRead->setDisabled(true);
		}
		if (rowsRead.size() == 0) {
			markMsgAsUnread->setDisabled(true);
		}

		bool hasUnreadChildren = false;
		bool hasReadChildren = false;
		int rowCount = rows.count();
		for (int i = 0; i < rowCount; i++) {
			if (hasUnreadChildren || rows[i]->data(COLUMN_THREAD_DATA, ROLE_THREAD_UNREADCHILDREN).toBool()) {
				hasUnreadChildren = true;
			}
			if (hasReadChildren || rows[i]->data(COLUMN_THREAD_DATA, ROLE_THREAD_READCHILDREN).toBool()) {
				hasReadChildren = true;
			}
		}
		markMsgAsReadChildren->setEnabled(hasUnreadChildren);
		markMsgAsUnreadChildren->setEnabled(hasReadChildren);

		if (nCount == 1) {
			replyAct->setEnabled (true);
			replyauthorAct->setEnabled (true);
		} else {
			replyAct->setDisabled (true);
			replyauthorAct->setDisabled (true);
		}
	} else {
		markMsgAsRead->setDisabled(true);
		markMsgAsReadChildren->setDisabled(true);
		markMsgAsUnread->setDisabled(true);
		markMsgAsUnreadChildren->setDisabled(true);
		replyAct->setDisabled (true);
		replyauthorAct->setDisabled (true);
	}

	contextMnu.addAction(replyAct);
	contextMnu.addAction(newthreadAct);
	contextMnu.addAction(replyauthorAct);
	QAction* action = contextMnu.addAction(QIcon(IMAGE_COPYLINK), tr("Copy RetroShare Link"), this, SLOT(copyMessageLink()));
	action->setEnabled(!mForumId.empty() && !mThreadId.empty());
	contextMnu.addSeparator();
	contextMnu.addAction(markMsgAsRead);
	contextMnu.addAction(markMsgAsReadChildren);
	contextMnu.addAction(markMsgAsUnread);
	contextMnu.addAction(markMsgAsUnreadChildren);
	contextMnu.addSeparator();
	contextMnu.addAction(expandAll);
	contextMnu.addAction(collapseAll);

	contextMnu.exec(QCursor::pos());
}

bool GxsForumThreadWidget::eventFilter(QObject *obj, QEvent *event)
{
	if (obj == ui->threadTreeWidget) {
		if (event->type() == QEvent::KeyPress) {
			QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
			if (keyEvent && keyEvent->key() == Qt::Key_Space) {
				// Space pressed
				QTreeWidgetItem *item = ui->threadTreeWidget->currentItem();
				clickedThread (item, COLUMN_THREAD_READ);
				return true; // eat event
			}
		}
	}
	// pass the event on to the parent class
	return QWidget::eventFilter(obj, event);
}

void GxsForumThreadWidget::togglethreadview()
{
	// save state of button
	Settings->setValueToGroup("GxsForumsDialog", "expandButton", ui->expandButton->isChecked());

	togglethreadview_internal();
}

void GxsForumThreadWidget::togglethreadview_internal()
{
	if (ui->expandButton->isChecked()) {
		ui->postText->setVisible(true);
		ui->expandButton->setIcon(QIcon(QString(":/images/edit_remove24.png")));
		ui->expandButton->setToolTip(tr("Hide"));
	} else  {
		ui->postText->setVisible(false);
		ui->expandButton->setIcon(QIcon(QString(":/images/edit_add24.png")));
		ui->expandButton->setToolTip(tr("Expand"));
	}
}

void GxsForumThreadWidget::changedThread()
{
	/* just grab the ids of the current item */
	QTreeWidgetItem *item = ui->threadTreeWidget->currentItem();

	if (!item || !item->isSelected()) {
		mThreadId.clear();
	} else {
		mThreadId = item->data(COLUMN_THREAD_DATA, ROLE_THREAD_MSGID).toString().toStdString();
	}

	if (mFillThread) {
		return;
	}

	insertPost();
}

void GxsForumThreadWidget::clickedThread(QTreeWidgetItem *item, int column)
{
	if (mFillThread) {
		return;
	}

	if (mForumId.empty() || !IS_GROUP_SUBSCRIBED(mSubscribeFlags)) {
		return;
	}

	if (item == NULL) {
		return;
	}

	if (column == COLUMN_THREAD_READ) {
		QList<QTreeWidgetItem*> rows;
		rows.append(item);
		uint32_t status = item->data(COLUMN_THREAD_DATA, ROLE_THREAD_STATUS).toUInt();
		setMsgReadStatus(rows, IS_MSG_UNREAD(status));
	}
}

#ifdef TODO
void GxsForumThreadWidget::forumMsgReadStatusChanged(const QString &forumId, const QString &msgId, int status)
{
	if (mInMsgAsReadUnread) {
		return;
	}

	if (forumId.toStdString() == mCurrForumId) {
		/* Search exisiting item */
		QTreeWidgetItemIterator itemIterator(ui->threadTreeWidget);
		QTreeWidgetItem *item = NULL;
		while ((item = *itemIterator) != NULL) {
			itemIterator++;

			if (item->data(COLUMN_THREAD_DATA, ROLE_THREAD_MSGID).toString() == msgId) {
				// update status
				item->setData(COLUMN_THREAD_DATA, ROLE_THREAD_STATUS, status);

				QTreeWidgetItem *parentItem = item;
				while (parentItem->parent()) {
					parentItem = parentItem->parent();
				}
				calculateIconsAndFonts(parentItem);
				break;
			}
		}
	}
	updateMessageSummaryList(forumId.toStdString());
}
#endif

void GxsForumThreadWidget::calculateIconsAndFonts(QTreeWidgetItem *item, bool &hasReadChilddren, bool &hasUnreadChilddren)
{
	uint32_t status = item->data(COLUMN_THREAD_DATA, ROLE_THREAD_STATUS).toUInt();

	bool isNew = IS_MSG_NEW(status);
	bool unread = IS_MSG_UNREAD(status);
	bool missing = item->data(COLUMN_THREAD_DATA, ROLE_THREAD_MISSING).toBool();

	// set icon
	if (missing) {
		item->setIcon(COLUMN_THREAD_READ, QIcon());
		item->setIcon(COLUMN_THREAD_TITLE, QIcon());
	} else {
		if (unread) {
			item->setIcon(COLUMN_THREAD_READ, QIcon(":/images/message-state-unread.png"));
		} else {
			item->setIcon(COLUMN_THREAD_READ, QIcon(":/images/message-state-read.png"));
		}
		if (isNew) {
			item->setIcon(COLUMN_THREAD_TITLE, QIcon(":/images/message-state-new.png"));
		} else {
			item->setIcon(COLUMN_THREAD_TITLE, QIcon());
		}
	}

	int index;
	int itemCount = item->childCount();

	bool myReadChilddren = false;
	bool myUnreadChilddren = false;

	for (index = 0; index < itemCount; ++index) {
		calculateIconsAndFonts(item->child(index), myReadChilddren, myUnreadChilddren);
	}

	// set font
	for (int i = 0; i < COLUMN_THREAD_COUNT; ++i) {
		QFont qf = item->font(i);

		if (!IS_GROUP_SUBSCRIBED(mSubscribeFlags)) {
			qf.setBold(false);
			item->setTextColor(i, textColorNotSubscribed());
		} else if (unread || isNew) {
			qf.setBold(true);
			item->setTextColor(i, textColorUnread());
		} else if (myUnreadChilddren) {
			qf.setBold(true);
			item->setTextColor(i, textColorUnreadChildren());
		} else {
			qf.setBold(false);
			item->setTextColor(i, textColorRead());
		}
		if (missing) {
			/* Missing message */
			item->setTextColor(i, textColorMissing());
		}
		item->setFont(i, qf);
	}

	item->setData(COLUMN_THREAD_DATA, ROLE_THREAD_READCHILDREN, hasReadChilddren || myReadChilddren);
	item->setData(COLUMN_THREAD_DATA, ROLE_THREAD_UNREADCHILDREN, hasUnreadChilddren || myUnreadChilddren);

	hasReadChilddren = hasReadChilddren || myReadChilddren || !unread;
	hasUnreadChilddren = hasUnreadChilddren || myUnreadChilddren || unread;
}

void GxsForumThreadWidget::calculateUnreadCount()
{
	unsigned int unreadCount = 0;
	unsigned int newCount = 0;

	QTreeWidgetItemIterator itemIterator(ui->threadTreeWidget);
	QTreeWidgetItem *item = NULL;
	while ((item = *itemIterator) != NULL) {
		itemIterator++;

		uint32_t status = item->data(COLUMN_THREAD_DATA, ROLE_THREAD_STATUS).toUInt();
		if (IS_MSG_UNREAD(status)) {
			++unreadCount;
		}
		if (IS_MSG_NEW(status)) {
			++newCount;
		}
	}

	bool changed = false;
	if (mUnreadCount != unreadCount) {
		mUnreadCount = unreadCount;
		changed = true;
	}
	if (mNewCount != newCount) {
		mNewCount = newCount;
		changed = true;
	}

	if (changed) {
		updateInterface();
	}
}

void GxsForumThreadWidget::calculateIconsAndFonts(QTreeWidgetItem *item /*= NULL*/)
{
	bool dummy1 = false;
	bool dummy2 = false;

	if (item) {
		calculateIconsAndFonts(item, dummy1, dummy2);
		return;
	}

	int index;
	int itemCount = ui->threadTreeWidget->topLevelItemCount();

	for (index = 0; index < itemCount; ++index) {
		dummy1 = false;
		dummy2 = false;
		calculateIconsAndFonts(ui->threadTreeWidget->topLevelItem(index), dummy1, dummy2);
	}
}

static void cleanupItems (QList<QTreeWidgetItem *> &items)
{
	QList<QTreeWidgetItem *>::iterator item;
	for (item = items.begin (); item != items.end (); item++) {
		if (*item) {
			delete (*item);
		}
	}
	items.clear();
}

void GxsForumThreadWidget::fillThreadFinished()
{
#ifdef DEBUG_FORUMS
	std::cerr << "GxsForumThreadWidget::fillThreadFinished" << std::endl;
#endif

	// thread has finished
	GxsForumsFillThread *thread = dynamic_cast<GxsForumsFillThread*>(sender());
	if (thread) {
		if (thread == mFillThread) {
			// current thread has finished, hide progressbar and release thread
			ui->progressBar->hide();
			ui->progressText->hide();
			ui->progressBarLayout->setEnabled(false);
			mFillThread = NULL;
			updateInterface();

			ui->threadTreeWidget->setPlaceholderText("");
		}

		if (thread->wasStopped()) {
			// thread was stopped
#ifdef DEBUG_FORUMS
			std::cerr << "GxsForumThreadWidget::fillThreadFinished Thread was stopped" << std::endl;
#endif
		} else {
#ifdef DEBUG_FORUMS
			std::cerr << "GxsForumThreadWidget::fillThreadFinished Add messages" << std::endl;
#endif

			ui->threadTreeWidget->setSortingEnabled(false);

			/* add all messages in! */
			if (mLastViewType != thread->mViewType || mLastForumID != mForumId) {
				ui->threadTreeWidget->clear();
				mLastViewType = thread->mViewType;
				mLastForumID = mForumId;
				ui->threadTreeWidget->insertTopLevelItems(0, thread->mItems);

				// clear list
				thread->mItems.clear();
			} else {
				fillThreads (thread->mItems, thread->mExpandNewMessages, thread->mItemToExpand);

				// cleanup list
				cleanupItems (thread->mItems);
			}

			ui->threadTreeWidget->setSortingEnabled(true);

			if (thread->mFocusMsgId.empty() == false) {
				/* Search exisiting item */
				QTreeWidgetItemIterator itemIterator(ui->threadTreeWidget);
				QTreeWidgetItem *item = NULL;
				while ((item = *itemIterator) != NULL) {
					itemIterator++;

					if (item->data(COLUMN_THREAD_DATA, ROLE_THREAD_MSGID).toString().toStdString() == thread->mFocusMsgId) {
						ui->threadTreeWidget->setCurrentItem(item);
						ui->threadTreeWidget->setFocus();
						break;
					}
				}
			}

			QList<QTreeWidgetItem*>::iterator itemIt;
			for (itemIt = thread->mItemToExpand.begin(); itemIt != thread->mItemToExpand.end(); ++itemIt) {
				if ((*itemIt)->isHidden() == false) {
					(*itemIt)->setExpanded(true);
				}
			}
			thread->mItemToExpand.clear();

			if (ui->filterLineEdit->text().isEmpty() == false) {
				filterItems(ui->filterLineEdit->text());
			}
			insertPost ();
			calculateIconsAndFonts();
			calculateUnreadCount();
			updateInterface();
		}

#ifdef DEBUG_FORUMS
		std::cerr << "GxsForumThreadWidget::fillThreadFinished Delete thread" << std::endl;
#endif

		thread->deleteLater();
		thread = NULL;
	}

#ifdef DEBUG_FORUMS
	std::cerr << "GxsForumThreadWidget::fillThreadFinished done" << std::endl;
#endif
}

void GxsForumThreadWidget::fillThreadProgress(int current, int count)
{
	// show fill progress
	if (count) {
		ui->progressBar->setValue(current * ui->progressBar->maximum() / count);
	}
}

void GxsForumThreadWidget::fillThreadStatus(QString text)
{
	ui->progressText->setText(text);
}

// HACK until update works.
void GxsForumThreadWidget::forceUpdateDisplay()
{
	std::cerr << "GxsForumThreadWidget::forceUpdateDisplay()";
	std::cerr << std::endl;

	/* update Thread List */
	insertThreads();
}

QTreeWidgetItem *GxsForumThreadWidget::convertMsgToThreadWidget(const RsGxsForumMsg &msg, bool useChildTS, uint32_t filterColumn)
{
	GxsIdTreeWidgetItem *item = new GxsIdTreeWidgetItem(mThreadCompareRole);
	QString text;

	item->setText(COLUMN_THREAD_TITLE, QString::fromUtf8(msg.mMeta.mMsgName.c_str()));

	QDateTime qtime;
	QString sort;

	if (useChildTS)
		qtime.setTime_t(msg.mMeta.mChildTs);
	else
		qtime.setTime_t(msg.mMeta.mPublishTs);

	text = DateTime::formatDateTime(qtime);
	sort = qtime.toString("yyyyMMdd_hhmmss");

	if (useChildTS)
	{
		qtime.setTime_t(msg.mMeta.mPublishTs);
		text += " / ";
		text += DateTime::formatDateTime(qtime);
		sort += "_" + qtime.toString("yyyyMMdd_hhmmss");
	}
	item->setText(COLUMN_THREAD_DATE, text);
	item->setData(COLUMN_THREAD_DATE, ROLE_THREAD_SORT, sort);

	item->setId(msg.mMeta.mAuthorId, COLUMN_THREAD_AUTHOR);
//#TODO
#if 0
	text = QString::fromUtf8(authorName.c_str());

	if (text.isEmpty())
	{
		item->setText(COLUMN_THREAD_AUTHOR, tr("Anonymous"));
	}
	else
	{
		item->setText(COLUMN_THREAD_AUTHOR, text);
	}
#endif
//#TODO
#ifdef TOGXS
	if (msgInfo.mMeta.mMsgFlags & RS_DISTRIB_AUTHEN_REQ)
	{
		item->setText(COLUMN_THREAD_SIGNED, tr("signed"));
		item->setIcon(COLUMN_THREAD_SIGNED, QIcon(":/images/mail-signed.png"));
	}
	else
	{
		item->setText(COLUMN_THREAD_SIGNED, tr("none"));
		item->setIcon(COLUMN_THREAD_SIGNED, QIcon(":/images/mail-signature-unknown.png"));
	}
#endif

	if (filterColumn == COLUMN_THREAD_CONTENT) {
		// need content for filter
		QTextDocument doc;
		doc.setHtml(QString::fromUtf8(msg.mMsg.c_str()));
		item->setText(COLUMN_THREAD_CONTENT, doc.toPlainText().replace(QString("\n"), QString(" ")));
	}

	item->setData(COLUMN_THREAD_DATA, ROLE_THREAD_MSGID, QString::fromStdString(msg.mMeta.mMsgId));
//#TODO
#if 0
	if (IS_GROUP_SUBSCRIBED(subscribeFlags) && !(msginfo.mMsgFlags & RS_DISTRIB_MISSING_MSG)) {
		rsGxsForums->getMessageStatus(msginfo.forumId, msginfo.msgId, status);
	} else {
		// show message as read
		status = RSGXS_MSG_STATUS_READ;
	}
#endif
	item->setData(COLUMN_THREAD_DATA, ROLE_THREAD_STATUS, msg.mMeta.mMsgStatus);

	item->setData(COLUMN_THREAD_DATA, ROLE_THREAD_MISSING, false);

	return item;
}

QTreeWidgetItem *GxsForumThreadWidget::generateMissingItem(const std::string &msgId)
{
	GxsIdTreeWidgetItem *item = new GxsIdTreeWidgetItem(mThreadCompareRole);
	item->setText(COLUMN_THREAD_TITLE, tr("[ ... Missing Message ... ]"));
	item->setData(COLUMN_THREAD_DATA, ROLE_THREAD_MSGID, QString::fromStdString(msgId));
	item->setData(COLUMN_THREAD_DATA, ROLE_THREAD_MISSING, true);

	return item;
}

void GxsForumThreadWidget::insertThreads()
{
#ifdef DEBUG_FORUMS
	/* get the current Forum */
	std::cerr << "GxsForumsDialog::insertThreads()" << std::endl;
#endif

	if (mFillThread) {
#ifdef DEBUG_FORUMS
		std::cerr << "GxsForumThreadWidget::insertThreads() stop current fill thread" << std::endl;
#endif
		// stop current fill thread
		GxsForumsFillThread *thread = mFillThread;
		mFillThread = NULL;
		thread->stop();
		delete(thread);

		ui->progressBar->hide();
		ui->progressText->hide();
	}

	mSubscribeFlags = 0;

	ui->newmessageButton->setEnabled(false);
	ui->newthreadButton->setEnabled(false);

	ui->postText->clear();
	ui->threadTitle->clear();

	if (mForumId.empty())
	{
		/* not an actual forum - clear */
		ui->threadTreeWidget->clear();
		/* when no Thread selected - clear */
		ui->forumName->clear();
		/* clear last stored forumID */
		mForumId.erase();
		mLastForumID.erase();

#ifdef DEBUG_FORUMS
		std::cerr << "GxsForumsDialog::insertThreads() Current Thread Invalid" << std::endl;
#endif

		return;
	}

//#TODO
//	ui->threadTitle->setText(tr("Loading"));

	// Get Current Forum Info... then complete insertForumThreads().
	requestGroupSummary_CurrentForum(mForumId);
}

void GxsForumThreadWidget::insertForumThreads(const RsGroupMetaData &fi)
{
	mSubscribeFlags = fi.mSubscribeFlags;
	ui->forumName->setText(QString::fromUtf8(fi.mGroupName.c_str()));

	ui->progressBarLayout->setEnabled(true);

	ui->progressText->show();
	ui->progressBar->reset();
	ui->progressBar->show();

	ui->threadTreeWidget->setPlaceholderText(tr("Loading"));

#ifdef DEBUG_FORUMS
	std::cerr << "GxsForumsDialog::insertThreads() Start filling Forum threads" << std::endl;
#endif

	// create fill thread
	mFillThread = new GxsForumsFillThread(this);

	// set data
	mFillThread->mCompareRole = mThreadCompareRole;
	mFillThread->mForumId = mForumId;
	mFillThread->mFilterColumn = ui->filterLineEdit->currentFilter();
	mFillThread->mFilterColumn = COLUMN_THREAD_TITLE;
	mFillThread->mSubscribeFlags = mSubscribeFlags;
	mFillThread->mExpandNewMessages = Settings->getForumExpandNewMessages();
	mFillThread->mViewType = ui->viewBox->currentIndex();
	if (mLastViewType != mFillThread->mViewType || mLastForumID != mForumId) {
		mFillThread->mFillComplete = true;
	}

	mFillThread->mFlatView = false;
	mFillThread->mUseChildTS = false;

	switch (mFillThread->mViewType) {
	case VIEW_LAST_POST:
		mFillThread->mUseChildTS = true;
		break;
	case VIEW_FLAT:
		mFillThread->mFlatView = true;
		break;
	case VIEW_THREADED:
		break;
	}

	ui->threadTreeWidget->setRootIsDecorated(!mFillThread->mFlatView);

	// connect thread
	connect(mFillThread, SIGNAL(finished()), this, SLOT(fillThreadFinished()), Qt::BlockingQueuedConnection);
	connect(mFillThread, SIGNAL(status(QString)), this, SLOT(fillThreadStatus(QString)));
	connect(mFillThread, SIGNAL(progress(int,int)), this, SLOT(fillThreadProgress(int,int)));

#ifdef DEBUG_FORUMS
	std::cerr << "ForumsDialog::insertThreads() Start fill thread" << std::endl;
#endif

	// start thread
	mFillThread->start();
	updateInterface();
}

void GxsForumThreadWidget::fillThreads(QList<QTreeWidgetItem *> &threadList, bool expandNewMessages, QList<QTreeWidgetItem*> &itemToExpand)
{
#ifdef DEBUG_FORUMS
	std::cerr << "GxsForumsDialog::fillThreads()" << std::endl;
#endif

	int index = 0;
	QTreeWidgetItem *threadItem;
	QList<QTreeWidgetItem *>::iterator newThread;

	// delete not existing
	while (index < ui->threadTreeWidget->topLevelItemCount()) {
		threadItem = ui->threadTreeWidget->topLevelItem(index);

		// search existing new thread
		int found = -1;
		for (newThread = threadList.begin (); newThread != threadList.end (); ++newThread) {
			if (threadItem->data(COLUMN_THREAD_DATA, ROLE_THREAD_MSGID) == (*newThread)->data (COLUMN_THREAD_DATA, ROLE_THREAD_MSGID)) {
				// found it
				found = index;
				break;
			}
		}
		if (found >= 0) {
			index++;
		} else {
			delete(ui->threadTreeWidget->takeTopLevelItem(index));
		}
	}

	// iterate all new threads
	for (newThread = threadList.begin (); newThread != threadList.end (); ++newThread) {
		// search existing thread
		int found = -1;
		int count = ui->threadTreeWidget->topLevelItemCount();
		for (index = 0; index < count; ++index) {
			threadItem = ui->threadTreeWidget->topLevelItem(index);
			if (threadItem->data(COLUMN_THREAD_DATA, ROLE_THREAD_MSGID) == (*newThread)->data(COLUMN_THREAD_DATA, ROLE_THREAD_MSGID)) {
				// found it
				found = index;
				break;
			}
		}

		if (found >= 0) {
			// set child data
			int i;
			for (i = 0; i < COLUMN_THREAD_COUNT; ++i) {
				threadItem->setText(i, (*newThread)->text(i));
			}
			for (i = 0; i < ROLE_THREAD_COUNT; ++i) {
				threadItem->setData(COLUMN_THREAD_DATA, Qt::UserRole + i, (*newThread)->data(COLUMN_THREAD_DATA, Qt::UserRole + i));
			}

			// fill recursive
			fillChildren(threadItem, *newThread, expandNewMessages, itemToExpand);
		} else {
			// add new thread
			ui->threadTreeWidget->addTopLevelItem (*newThread);
			threadItem = *newThread;
			*newThread = NULL;
		}

		uint32_t status = threadItem->data(COLUMN_THREAD_DATA, ROLE_THREAD_STATUS).toUInt();
		if (expandNewMessages && IS_MSG_UNREAD(status)) {
			QTreeWidgetItem *parentItem = threadItem;
			while ((parentItem = parentItem->parent()) != NULL) {
				if (std::find(itemToExpand.begin(), itemToExpand.end(), parentItem) == itemToExpand.end()) {
					itemToExpand.push_back(parentItem);
				}
			}
		}
	}

#ifdef DEBUG_FORUMS
	std::cerr << "GxsForumsDialog::fillThreads() done" << std::endl;
#endif
}

void GxsForumThreadWidget::fillChildren(QTreeWidgetItem *parentItem, QTreeWidgetItem *newParentItem, bool expandNewMessages, QList<QTreeWidgetItem*> &itemToExpand)
{
	int index = 0;
	int newIndex;
	int newCount = newParentItem->childCount();

	QTreeWidgetItem *childItem;
	QTreeWidgetItem *newChildItem;

	// delete not existing
	while (index < parentItem->childCount()) {
		childItem = parentItem->child(index);

		// search existing new child
		int found = -1;
		int count = newParentItem->childCount();
		for (newIndex = 0; newIndex < count; ++newIndex) {
			newChildItem = newParentItem->child(newIndex);
			if (newChildItem->data(COLUMN_THREAD_DATA, ROLE_THREAD_MSGID) == childItem->data(COLUMN_THREAD_DATA, ROLE_THREAD_MSGID)) {
				// found it
				found = index;
				break;
			}
		}
		if (found >= 0) {
			index++;
		} else {
			delete(parentItem->takeChild (index));
		}
	}

	// iterate all new children
	for (newIndex = 0; newIndex < newCount; ++newIndex) {
		newChildItem = newParentItem->child(newIndex);

		// search existing child
		int found = -1;
		int count = parentItem->childCount();
		for (index = 0; index < count; ++index) {
			childItem = parentItem->child(index);
			if (childItem->data(COLUMN_THREAD_DATA, ROLE_THREAD_MSGID) == newChildItem->data(COLUMN_THREAD_DATA, ROLE_THREAD_MSGID)) {
				// found it
				found = index;
				break;
			}
		}

		if (found >= 0) {
			// set child data
			int i;
			for (i = 0; i < COLUMN_THREAD_COUNT; ++i) {
				childItem->setText(i, newChildItem->text(i));
			}
			for (i = 0; i < ROLE_THREAD_COUNT; ++i) {
				childItem->setData(COLUMN_THREAD_DATA, Qt::UserRole + i, newChildItem->data(COLUMN_THREAD_DATA, Qt::UserRole + i));
			}

			// fill recursive
			fillChildren(childItem, newChildItem, expandNewMessages, itemToExpand);
		} else {
			// add new child
			childItem = newParentItem->takeChild(newIndex);
			parentItem->addChild(childItem);
			newIndex--;
			newCount--;
		}

		uint32_t status = childItem->data(COLUMN_THREAD_DATA, ROLE_THREAD_STATUS).toUInt();
		if (expandNewMessages && IS_MSG_UNREAD(status)) {
			QTreeWidgetItem *parentItem = childItem;
			while ((parentItem = parentItem->parent()) != NULL) {
				if (std::find(itemToExpand.begin(), itemToExpand.end(), parentItem) == itemToExpand.end()) {
					itemToExpand.push_back(parentItem);
				}
			}
		}
	}
}

void GxsForumThreadWidget::insertPost()
{
	if (mForumId.empty() || mThreadId.empty())
	{
		ui->postText->setText("");
		ui->threadTitle->setText("");
		ui->previousButton->setEnabled(false);
		ui->nextButton->setEnabled(false);
		ui->newmessageButton->setEnabled (false);
		return;
	}

	QTreeWidgetItem *item = ui->threadTreeWidget->currentItem();
	if (item) {
		QTreeWidgetItem *parentItem = item->parent();
		int index = parentItem ? parentItem->indexOfChild(item) : ui->threadTreeWidget->indexOfTopLevelItem(item);
		int count = parentItem ? parentItem->childCount() : ui->threadTreeWidget->topLevelItemCount();
		ui->previousButton->setEnabled(index > 0);
		ui->nextButton->setEnabled(index < count - 1);
	} else {
		// there is something wrong
		ui->previousButton->setEnabled(false);
		ui->nextButton->setEnabled(false);
		return;
	}

	ui->postText->setPlaceholderText(tr("Loading"));
	ui->threadTitle->setText(tr("Loading"));

	ui->newmessageButton->setEnabled(IS_GROUP_SUBSCRIBED(mSubscribeFlags) && mThreadId.empty() == false);

	/* blank text, incase we get nothing */
	ui->postText->setText("");
	/* request Post */

	// Get Forum Post ... then complete insertPostData().
	RsGxsGrpMsgIdPair postId = std::make_pair(mForumId, mThreadId);
	requestMsgData_InsertPost(postId);
}

void GxsForumThreadWidget::insertPostData(const RsGxsForumMsg &msg)
{
	/* As some time has elapsed since request - check that this is still the current msg.
	 * otherwise, another request will fill the data
	 */

	ui->postText->setPlaceholderText("");
	ui->threadTitle->setText("");

	if ((msg.mMeta.mGroupId != mForumId) || (msg.mMeta.mMsgId != mThreadId))
	{
		std::cerr << "GxsForumsDialog::insertPostData() Ignoring Invalid Data....";
		std::cerr << std::endl;
		std::cerr << "\t CurrForumId: " << mForumId << " != msg.GroupId: " << msg.mMeta.mGroupId;
		std::cerr << std::endl;
		std::cerr << "\t or CurrThdId: " << mThreadId << " != msg.MsgId: " << msg.mMeta.mMsgId;
		std::cerr << std::endl;
		std::cerr << std::endl;
		return;
	}

	QTreeWidgetItem *item = ui->threadTreeWidget->currentItem();

	bool setToReadOnActive = Settings->getForumMsgSetToReadOnActivate();
	uint32_t status = item->data(COLUMN_THREAD_DATA, ROLE_THREAD_STATUS).toUInt();

	QList<QTreeWidgetItem*> row;
	row.append(item);

	if (IS_MSG_NEW(status)) {
		if (setToReadOnActive) {
			/* set to read */
			setMsgReadStatus(row, true);
		} else {
			/* set to unread by user */
			setMsgReadStatus(row, false);
		}
	} else {
		if (setToReadOnActive && IS_MSG_UNREAD(status)) {
			/* set to read */
			setMsgReadStatus(row, true);
		}
	}

	ui->time_label->setText(DateTime::formatLongDateTime(msg.mMeta.mPublishTs));

	std::string authorName = rsPeers->getPeerName(msg.mMeta.mAuthorId);
	QString text = QString::fromUtf8(authorName.c_str());

	if (text.isEmpty())
	{
		ui->by_label->setText( tr("By") + " " + tr("Anonymous"));
	}
	else
	{
		ui->by_label->setText( tr("By") + " " + text );
	}

	QString extraTxt = RsHtml().formatText(ui->postText->document(), QString::fromUtf8(msg.mMsg.c_str()), RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS);

	ui->postText->setHtml(extraTxt);
	ui->threadTitle->setText(QString::fromUtf8(msg.mMeta.mMsgName.c_str()));
}

void GxsForumThreadWidget::previousMessage()
{
	QTreeWidgetItem *item = ui->threadTreeWidget->currentItem();
	if (item == NULL) {
		return;
	}

	QTreeWidgetItem *parentItem = item->parent();
	int index = parentItem ? parentItem->indexOfChild(item) : ui->threadTreeWidget->indexOfTopLevelItem(item);
	if (index > 0) {
		QTreeWidgetItem *previousItem = parentItem ? parentItem->child(index - 1) : ui->threadTreeWidget->topLevelItem(index - 1);
		if (previousItem) {
			ui->threadTreeWidget->setCurrentItem(previousItem);
		}
	}
}

void GxsForumThreadWidget::nextMessage()
{
	QTreeWidgetItem *item = ui->threadTreeWidget->currentItem();
	if (item == NULL) {
		return;
	}

	QTreeWidgetItem *parentItem = item->parent();
	int index = parentItem ? parentItem->indexOfChild(item) : ui->threadTreeWidget->indexOfTopLevelItem(item);
	int count = parentItem ? parentItem->childCount() : ui->threadTreeWidget->topLevelItemCount();
	if (index < count - 1) {
		QTreeWidgetItem *nextItem = parentItem ? parentItem->child(index + 1) : ui->threadTreeWidget->topLevelItem(index + 1);
		if (nextItem) {
			ui->threadTreeWidget->setCurrentItem(nextItem);
		}
	}
}

void GxsForumThreadWidget::downloadAllFiles()
{
	QStringList urls;
	if (RsHtml::findAnchors(ui->postText->toHtml(), urls) == false) {
		return;
	}

	if (urls.count() == 0) {
		return;
	}

	RetroShareLink::process(urls, RetroShareLink::TYPE_FILE/*, true*/);
}

void GxsForumThreadWidget::nextUnreadMessage()
{
	QTreeWidgetItem *currentItem = ui->threadTreeWidget->currentItem();

	while (TRUE) {
		QTreeWidgetItemIterator itemIterator = currentItem ? QTreeWidgetItemIterator(currentItem, QTreeWidgetItemIterator::NotHidden) : QTreeWidgetItemIterator(ui->threadTreeWidget, QTreeWidgetItemIterator::NotHidden);

		QTreeWidgetItem *item;
		while ((item = *itemIterator) != NULL) {
			itemIterator++;

			if (item == currentItem) {
				continue;
			}

			uint32_t status = item->data(COLUMN_THREAD_DATA, ROLE_THREAD_STATUS).toUInt();
			if (IS_MSG_UNREAD(status)) {
				ui->threadTreeWidget->setCurrentItem(item);
				ui->threadTreeWidget->scrollToItem(item, QAbstractItemView::EnsureVisible);
				return;
			}
		}

		if (currentItem == NULL) {
			break;
		}

		/* start from top */
		currentItem = NULL;
	}
}

// TODO
#if 0
void GxsForumsDialog::removemessage()
{
	//std::cerr << "GxsForumsDialog::removemessage()" << std::endl;
	std::string cid, mid;
	if (!getCurrentMsg(cid, mid))
	{
		//std::cerr << "GxsForumsDialog::removemessage()";
		//std::cerr << " No Message selected" << std::endl;
		return;
	}

	rsMsgs -> MessageDelete(mid);
}
#endif

/* get selected messages
   the messages tree is single selected, but who knows ... */
int GxsForumThreadWidget::getSelectedMsgCount(QList<QTreeWidgetItem*> *rows, QList<QTreeWidgetItem*> *rowsRead, QList<QTreeWidgetItem*> *rowsUnread)
{
	if (rowsRead) rowsRead->clear();
	if (rowsUnread) rowsUnread->clear();

	QList<QTreeWidgetItem*> selectedItems = ui->threadTreeWidget->selectedItems();
	for(QList<QTreeWidgetItem*>::iterator it = selectedItems.begin(); it != selectedItems.end(); it++) {
		if (rows) rows->append(*it);
		if (rowsRead || rowsUnread) {
			uint32_t status = (*it)->data(COLUMN_THREAD_DATA, ROLE_THREAD_STATUS).toUInt();
			if (IS_MSG_UNREAD(status)) {
				if (rowsUnread) rowsUnread->append(*it);
			} else {
				if (rowsRead) rowsRead->append(*it);
			}
		}
	}

	return selectedItems.size();
}

void GxsForumThreadWidget::setMsgReadStatus(QList<QTreeWidgetItem*> &rows, bool read)
{
	QList<QTreeWidgetItem*>::iterator row;
	std::list<QTreeWidgetItem*> changedItems;

	mInMsgAsReadUnread = true;

	for (row = rows.begin(); row != rows.end(); ++row) {
		if ((*row)->data(COLUMN_THREAD_DATA, ROLE_THREAD_MISSING).toBool()) {
			/* Missing message */
			continue;
		}

		uint32_t status = (*row)->data(COLUMN_THREAD_DATA, ROLE_THREAD_STATUS).toUInt();

		uint32_t statusNew = (status & ~(GXS_SERV::GXS_MSG_STATUS_UNREAD | GXS_SERV::GXS_MSG_STATUS_UNPROCESSED)); // orig status, without UNREAD and UNPROCESSED.
		if (!read) {
			statusNew |= GXS_SERV::GXS_MSG_STATUS_UNREAD;
		}

		if (status != statusNew) // is it different?
		{
			std::string msgId = (*row)->data(COLUMN_THREAD_DATA, ROLE_THREAD_MSGID).toString().toStdString();

			// NB: MUST BE PART OF ACTIVE THREAD--- OR ELSE WE MUST STORE GROUPID SOMEWHERE!.
			// LIKE THIS BELOW...
			//std::string grpId = (*Row)->data(COLUMN_THREAD_DATA, ROLE_THREAD_GROUPID).toString().toStdString();

			RsGxsGrpMsgIdPair msgPair = std::make_pair(mForumId, msgId);

			uint32_t token;
			rsGxsForums->setMessageReadStatus(token, msgPair, read);

			(*row)->setData(COLUMN_THREAD_DATA, ROLE_THREAD_STATUS, statusNew);

			QTreeWidgetItem *parentItem = *row;
			while (parentItem->parent()) {
				parentItem = parentItem->parent();
			}
			if (std::find(changedItems.begin(), changedItems.end(), parentItem) == changedItems.end()) {
				changedItems.push_back(parentItem);
			}
		}
	}

	mInMsgAsReadUnread = false;

	if (changedItems.size()) {
		for (std::list<QTreeWidgetItem*>::iterator it = changedItems.begin(); it != changedItems.end(); it++) {
			calculateIconsAndFonts(*it);
		}
//#TODO		updateMessageSummaryList(mForumId);
		calculateUnreadCount();
	}
}

void GxsForumThreadWidget::markMsgAsReadUnread (bool read, bool children, bool forum)
{
	if (mForumId.empty() || !IS_GROUP_SUBSCRIBED(mSubscribeFlags)) {
		return;
	}

	/* get selected messages */
	QList<QTreeWidgetItem*> rows;
	if (forum) {
		int itemCount = ui->threadTreeWidget->topLevelItemCount();
		for (int item = 0; item < itemCount; item++) {
			rows.push_back(ui->threadTreeWidget->topLevelItem(item));
		}
	} else {
		getSelectedMsgCount (&rows, NULL, NULL);
	}

	if (children) {
		/* add children */
		QList<QTreeWidgetItem*> allRows;

		while (rows.isEmpty() == false) {
			QTreeWidgetItem *row = rows.takeFirst();

			/* add only items with the right state or with not RSGXS_MSG_STATUS_READ */
			uint32_t status = row->data(COLUMN_THREAD_DATA, ROLE_THREAD_STATUS).toUInt();
			bool isUnread = IS_MSG_UNREAD(status);
			if (isUnread == read || IS_MSG_NEW(status)) {
				allRows.append(row);
			}

			for (int i = 0; i < row->childCount(); i++) {
				/* add child to main list and let the main loop do the work */
				rows.append(row->child(i));
			}
		}

		if (allRows.isEmpty()) {
			/* nothing to do */
			return;
		}

		setMsgReadStatus(allRows, read);

		return;
	}

	setMsgReadStatus(rows, read);
}

void GxsForumThreadWidget::markMsgAsRead()
{
	markMsgAsReadUnread(true, false, false);
}

void GxsForumThreadWidget::markMsgAsReadChildren()
{
	markMsgAsReadUnread(true, true, false);
}

void GxsForumThreadWidget::markMsgAsUnread()
{
	markMsgAsReadUnread(false, false, false);
}

void GxsForumThreadWidget::markMsgAsUnreadChildren()
{
	markMsgAsReadUnread(false, true, false);
}

void GxsForumThreadWidget::setAllMsgReadStatus(bool read)
{
	markMsgAsReadUnread(read, true, true);
}

void GxsForumThreadWidget::copyMessageLink()
{
	if (mForumId.empty() || mThreadId.empty()) {
		return;
	}

// THIS CODE CALLS getForumInfo() to verify that the Ids are valid.
// As we are switching to Request/Response this is now harder to do...
// So not bothering any more - shouldn't be necessary.
// IF we get errors - fix them, rather than patching here.
#if 0
	ForumInfo fi;
	if (rsGxsForums->getForumInfo(mForumId, fi)) {
		RetroShareLink link;
		if (link.createForum(mForumId, mThreadId)) {
			QList<RetroShareLink> urls;
			urls.push_back(link);
			RSLinkClipboard::copyLinks(urls);
		}
	}
#endif

	QMessageBox::warning(this, "RetroShare", "ToDo");
}

void GxsForumThreadWidget::createmessage()
{
	if (mForumId.empty () || !IS_GROUP_SUBSCRIBED(mSubscribeFlags)) {
		return;
	}

	CreateGxsForumMsg *cfm = new CreateGxsForumMsg(mForumId, mThreadId);
	cfm->show();

	/* window will destroy itself! */
}

void GxsForumThreadWidget::createthread()
{
	if (mForumId.empty ()) {
		QMessageBox::information(this, tr("RetroShare"), tr("No Forum Selected!"));
		return;
	}

	CreateGxsForumMsg *cfm = new CreateGxsForumMsg(mForumId, "");
	cfm->show();

	/* window will destroy itself! */
}

static QString buildReplyHeader(const RsMsgMetaData &meta)
{
	RetroShareLink link;
	link.createMessage(meta.mAuthorId, "");
	QString from = link.toHtml();

	QString header = QString("<span>-----%1-----").arg(QApplication::translate("GxsForumsDialog", "Original Message"));
	header += QString("<br><font size='3'><strong>%1: </strong>%2</font><br>").arg(QApplication::translate("GxsForumsDialog", "From"), from);

	header += QString("<br><font size='3'><strong>%1: </strong>%2</font><br>").arg(QApplication::translate("GxsForumsDialog", "Sent"), DateTime::formatLongDateTime(meta.mPublishTs));
	header += QString("<font size='3'><strong>%1: </strong>%2</font></span><br>").arg(QApplication::translate("GxsForumsDialog", "Subject"), QString::fromUtf8(meta.mMsgName.c_str()));
	header += "<br>";

	header += QApplication::translate("GxsForumsDialog", "On %1, %2 wrote:").arg(DateTime::formatDateTime(meta.mPublishTs), from);

	return header;
}

void GxsForumThreadWidget::replytomessage()
{
	if (mForumId.empty() || mThreadId.empty()) {
		QMessageBox::information(this, tr("RetroShare"),tr("You cant reply to a non-existant Message"));
		return;
	}

	// Get Message ... then complete replyMessageData().
	RsGxsGrpMsgIdPair postId = std::make_pair(mForumId, mThreadId);
	requestMsgData_ReplyMessage(postId);
}

void GxsForumThreadWidget::replyMessageData(const RsGxsForumMsg &msg)
{
	if ((msg.mMeta.mGroupId != mForumId) || (msg.mMeta.mMsgId != mThreadId))
	{
		std::cerr << "GxsForumsDialog::replyMessageData() ERROR Message Ids have changed!";
		std::cerr << std::endl;
		return;
	}

	// NB: TODO REMOVE rsPeers references.
	if (rsPeers->getPeerName(msg.mMeta.mAuthorId) !="")
	{
		MessageComposer *msgDialog = MessageComposer::newMsg();
		msgDialog->setTitleText(QString::fromUtf8(msg.mMeta.mMsgName.c_str()), MessageComposer::REPLY);

		msgDialog->setQuotedMsg(QString::fromUtf8(msg.mMsg.c_str()), buildReplyHeader(msg.mMeta));

		msgDialog->addRecipient(MessageComposer::TO, msg.mMeta.mAuthorId, false);
		msgDialog->show();
		msgDialog->activateWindow();

		/* window will destroy itself! */
	}
	else
	{
		QMessageBox::information(this, tr("RetroShare"),tr("You cant reply to an Anonymous Author"));
	}
}

void GxsForumThreadWidget::changedViewBox()
{
	if (mInProcessSettings) {
		return;
	}

	// save index
	Settings->setValueToGroup("GxsForumsDialog", "viewBox", ui->viewBox->currentIndex());

	ui->threadTreeWidget->clear();

	insertThreads();
}

void GxsForumThreadWidget::filterColumnChanged(int column)
{
	if (mInProcessSettings) {
		return;
	}

	if (column == COLUMN_THREAD_CONTENT) {
		// need content ... refill
		insertThreads();
	} else {
		filterItems(ui->filterLineEdit->text());
	}

	// save index
	Settings->setValueToGroup("GxsForumsDialog", "filterColumn", column);
}

void GxsForumThreadWidget::filterItems(const QString& text)
{
	int filterColumn = ui->filterLineEdit->currentFilter();

	int count = ui->threadTreeWidget->topLevelItemCount();
	for (int index = 0; index < count; ++index) {
		filterItem(ui->threadTreeWidget->topLevelItem(index), text, filterColumn);
	}
}

bool GxsForumThreadWidget::filterItem(QTreeWidgetItem *item, const QString &text, int filterColumn)
{
	bool visible = true;

	if (text.isEmpty() == false) {
		if (item->text(filterColumn).contains(text, Qt::CaseInsensitive) == false) {
			visible = false;
		}
	}

	int visibleChildCount = 0;
	int count = item->childCount();
	for (int nIndex = 0; nIndex < count; ++nIndex) {
		if (filterItem(item->child(nIndex), text, filterColumn)) {
			visibleChildCount++;
		}
	}

	if (visible || visibleChildCount) {
		item->setHidden(false);
	} else {
		item->setHidden(true);
	}

	return (visible || visibleChildCount);
}

/*********************** **** **** **** ***********************/
/** Request / Response of Data ********************************/
/*********************** **** **** **** ***********************/

void GxsForumThreadWidget::requestGroupSummary_CurrentForum(const std::string &forumId)
{
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;

	std::list<uint32_t> tokens;
	mThreadQueue->activeRequestTokens(TOKEN_TYPE_CURRENTFORUM, tokens);
	if (!tokens.empty()) {
		std::list<uint32_t>::iterator tokenIt;
		for (tokenIt = tokens.begin(); tokenIt != tokens.end(); ++tokenIt) {
			std::cerr << "GxsForumThreadWidget::requestGroupSummary() Canceling Request: " << *tokenIt;
			std::cerr << std::endl;

			mThreadQueue->cancelRequest(*tokenIt);
		}
	}

	std::list<std::string> grpIds;
	grpIds.push_back(forumId);

	std::cerr << "GxsForumsDialog::requestGroupSummary_CurrentForum(" << forumId << ")";
	std::cerr << std::endl;

	uint32_t token;
	mThreadQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_SUMMARY, opts, grpIds, TOKEN_TYPE_CURRENTFORUM);
}

void GxsForumThreadWidget::loadGroupSummary_CurrentForum(const uint32_t &token)
{
	std::cerr << "GxsForumsDialog::loadGroupSummary_CurrentForum()";
	std::cerr << std::endl;

	std::list<RsGroupMetaData> groupInfo;
	rsGxsForums->getGroupSummary(token, groupInfo);

	if (groupInfo.size() == 1)
	{
		RsGroupMetaData fi = groupInfo.front();
		insertForumThreads(fi);
	}
	else
	{
		std::cerr << "GxsForumsDialog::loadGroupSummary_CurrentForum() ERROR Invalid Number of Groups...";
		std::cerr << std::endl;

		updateInterface();
	}
}

/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/

void GxsForumThreadWidget::requestMsgData_InsertPost(const RsGxsGrpMsgIdPair &msgId)
{
#if 0
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_RELATED_DATA;

	std::cerr << "GxsForumsDialog::requestMsgData_InsertPost(" << msgId.first << "," << msgId.second << ")";
	std::cerr << std::endl;

	std::vector<RsGxsGrpMsgIdPair> msgIds;
	msgIds.push_back(msgId);
	uint32_t token;
	mForumQueue->requestMsgRelatedInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, msgIds, TOKEN_TYPE_INSERT_POST);
#else

	std::list<uint32_t> tokens;
	mThreadQueue->activeRequestTokens(TOKEN_TYPE_INSERT_POST, tokens);
	if (!tokens.empty()) {
		std::list<uint32_t>::iterator tokenIt;
		for (tokenIt = tokens.begin(); tokenIt != tokens.end(); ++tokenIt) {
#ifdef DEBUG_FORUMS
			std::cerr << "GxsForumThreadWidget::requestMsgData_InsertPost() Canceling Request: " << *tokenIt;
			std::cerr << std::endl;
#endif
			mThreadQueue->cancelRequest(*tokenIt);
		}
	}

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;

	std::cerr << "GxsForumsDialog::requestMsgData_InsertPost(" << msgId.first << "," << msgId.second << ")";
	std::cerr << std::endl;

	GxsMsgReq msgIds;
	std::vector<RsGxsMessageId> &vect = msgIds[msgId.first];
	vect.push_back(msgId.second);

	uint32_t token;
	mThreadQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, msgIds, TOKEN_TYPE_INSERT_POST);
#endif
}

void GxsForumThreadWidget::loadMsgData_InsertPost(const uint32_t &token)
{
	std::cerr << "GxsForumsDialog::loadMsgData_InsertPost()";
	std::cerr << std::endl;

	std::vector<RsGxsForumMsg> msgs;
	if (rsGxsForums->getMsgData(token, msgs)) {
		if (msgs.size() != 1) {
			std::cerr << "GxsForumsDialog::loadMsgData_InsertPost() ERROR Wrong number of answers";
			std::cerr << std::endl;
			return;
		}
		insertPostData(msgs[0]);
	} else {
		std::cerr << "GxsForumsDialog::loadMsgData_InsertPost() ERROR Missing Message Data...";
		std::cerr << std::endl;
	}
}

/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/

void GxsForumThreadWidget::requestMsgData_ReplyMessage(const RsGxsGrpMsgIdPair &msgId)
{
#if 0
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_RELATED_DATA;

	std::cerr << "GxsForumsDialog::requestMsgData_ReplyMessage(" << msgId.first << "," << msgId.second << ")";
	std::cerr << std::endl;

	std::vector<RsGxsGrpMsgIdPair> msgIds;
	msgIds.push_back(msgId);
	uint32_t token;
	mThreadQueue->requestMsgRelatedInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, msgIds, TOKEN_TYPE_REPLY_MESSAGE);
#else

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;

	std::cerr << "GxsForumsDialog::requestMsgData_ReplyMessage(" << msgId.first << "," << msgId.second << ")";
	std::cerr << std::endl;

	GxsMsgReq msgIds;
	std::vector<RsGxsMessageId> &vect = msgIds[msgId.first];
	vect.push_back(msgId.second);

	uint32_t token;
	mThreadQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, msgIds, TOKEN_TYPE_REPLY_MESSAGE);
#endif
}

void GxsForumThreadWidget::loadMsgData_ReplyMessage(const uint32_t &token)
{
	std::cerr << "GxsForumsDialog::loadMsgData_ReplyMessage()";
	std::cerr << std::endl;

	std::vector<RsGxsForumMsg> msgs;
#if 0
	if (rsGxsForums->getRelatedMessages(token, msgs))
#else
	if (rsGxsForums->getMsgData(token, msgs))
#endif
	{
		if (msgs.size() != 1)
		{
			std::cerr << "GxsForumsDialog::loadMsgData_ReplyMessage() ERROR Wrong number of answers";
			std::cerr << std::endl;
			return;
		}

		replyMessageData(msgs[0]);
	}
	else
	{
		std::cerr << "GxsForumsDialog::loadMsgData_ReplyMessage() ERROR Missing Message Data...";
		std::cerr << std::endl;
	}
}

/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/

void GxsForumThreadWidget::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
	std::cerr << "GxsForumsDialog::loadRequest() UserType: " << req.mUserType;
	std::cerr << std::endl;

	if (queue == mThreadQueue)
	{
		/* now switch on req */
		switch(req.mUserType)
		{
		case TOKEN_TYPE_CURRENTFORUM:
			loadGroupSummary_CurrentForum(req.mToken);
			break;

		case TOKEN_TYPE_INSERT_POST:
			loadMsgData_InsertPost(req.mToken);
			break;

		case TOKEN_TYPE_REPLY_MESSAGE:
			loadMsgData_ReplyMessage(req.mToken);
			break;

		default:
			std::cerr << "GxsForumsDialog::loadRequest() ERROR: INVALID TYPE";
			std::cerr << std::endl;
			break;
		}
	}
}
