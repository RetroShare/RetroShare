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
#include <QScrollBar>

#include "GxsForumThreadWidget.h"
#include "ui_GxsForumThreadWidget.h"
#include "GxsForumsFillThread.h"
#include "GxsForumsDialog.h"
#include "gui/RetroShareLink.h"
#include "gui/common/RSTreeWidgetItem.h"
#include "gui/common/RSItemDelegate.h"
#include "gui/settings/rsharesettings.h"
#include "gui/gxs/GxsIdTreeWidgetItem.h"
#include "gui/gxs/GxsIdDetails.h"
#include "util/HandleRichText.h"
#include "CreateGxsForumMsg.h"
#include "gui/msgs/MessageComposer.h"
#include "util/DateTime.h"
#include "gui/common/UIStateHelper.h"
#include "util/QtVersion.h"
#include "util/imageutil.h"

#include <retroshare/rsgxsforums.h>
#include <retroshare/rsgrouter.h>
#include <retroshare/rsreputations.h>
#include <retroshare/rspeers.h>
// These should be in retroshare/ folder.
#include "retroshare/rsgxsflags.h"

#include <iostream>
#include <algorithm>

//#define DEBUG_FORUMS

/* Images for context menu icons */
#define IMAGE_MESSAGE        ":/images/mail_new.png"
#define IMAGE_MESSAGEREPLY   ":/images/mail_reply.png"
#define IMAGE_MESSAGEREMOVE  ":/images/mail_delete.png"
#define IMAGE_DOWNLOAD       ":/images/start.png"
#define IMAGE_DOWNLOADALL    ":/images/startall.png"
#define IMAGE_COPYLINK       ":/images/copyrslink.png"
#define IMAGE_BIOHAZARD      ":/icons/yellow_biohazard64.png"

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
#define ROLE_THREAD_AUTHOR          Qt::UserRole + 3
// no need to copy, don't count in ROLE_THREAD_COUNT
#define ROLE_THREAD_READCHILDREN    Qt::UserRole + 4
#define ROLE_THREAD_UNREADCHILDREN  Qt::UserRole + 5
#define ROLE_THREAD_SORT            Qt::UserRole + 6

#define ROLE_THREAD_COUNT           4

GxsForumThreadWidget::GxsForumThreadWidget(const RsGxsGroupId &forumId, QWidget *parent) :
	GxsMessageFrameWidget(rsGxsForums, parent),
	ui(new Ui::GxsForumThreadWidget)
{
	ui->setupUi(this);

	mTokenTypeGroupData = nextTokenType();
	mTokenTypeInsertThreads = nextTokenType();
	mTokenTypeMessageData = nextTokenType();
	mTokenTypeReplyMessage = nextTokenType();
	mTokenTypeReplyForumMessage = nextTokenType();
    mTokenTypeBanAuthor = nextTokenType();

	setUpdateWhenInvisible(true);

	/* Setup UI helper */
	mStateHelper->addWidget(mTokenTypeGroupData, ui->subscribeToolButton);
	mStateHelper->addWidget(mTokenTypeGroupData, ui->newthreadButton);

	mStateHelper->addClear(mTokenTypeGroupData, ui->forumName);

	mStateHelper->addWidget(mTokenTypeInsertThreads, ui->progressBar, UISTATE_LOADING_VISIBLE);
	mStateHelper->addWidget(mTokenTypeInsertThreads, ui->progressText, UISTATE_LOADING_VISIBLE);
	mStateHelper->addWidget(mTokenTypeInsertThreads, ui->threadTreeWidget, UISTATE_ACTIVE_ENABLED);
	mStateHelper->addLoadPlaceholder(mTokenTypeInsertThreads, ui->progressText);
	mStateHelper->addWidget(mTokenTypeInsertThreads, ui->nextUnreadButton);
	mStateHelper->addWidget(mTokenTypeInsertThreads, ui->previousButton);
	mStateHelper->addWidget(mTokenTypeInsertThreads, ui->nextButton);

	mStateHelper->addClear(mTokenTypeInsertThreads, ui->threadTreeWidget);

	mStateHelper->addWidget(mTokenTypeMessageData, ui->newmessageButton);
//	mStateHelper->addWidget(mTokenTypeMessageData, ui->postText);
	mStateHelper->addWidget(mTokenTypeMessageData, ui->downloadButton);

	mStateHelper->addLoadPlaceholder(mTokenTypeMessageData, ui->postText);
    //mStateHelper->addLoadPlaceholder(mTokenTypeMessageData, ui->threadTitle);

	mSubscribeFlags = 0;
    mSignFlags = 0;
	mInProcessSettings = false;
	mUnreadCount = 0;
	mNewCount = 0;

	mInMsgAsReadUnread = false;

	mThreadCompareRole = new RSTreeWidgetItemCompareRole;
	mThreadCompareRole->setRole(COLUMN_THREAD_DATE, ROLE_THREAD_SORT);

	connect(ui->threadTreeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(threadListCustomPopupMenu(QPoint)));
	connect(ui->postText, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuTextBrowser(QPoint)));

    ui->subscribeToolButton->hide() ;
	connect(ui->subscribeToolButton, SIGNAL(subscribe(bool)), this, SLOT(subscribeGroup(bool)));
	connect(ui->newmessageButton, SIGNAL(clicked()), this, SLOT(replytoforummessage()));
	connect(ui->newthreadButton, SIGNAL(clicked()), this, SLOT(createthread()));

	connect(ui->threadTreeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(changedThread()));
	connect(ui->threadTreeWidget, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(clickedThread(QTreeWidgetItem*,int)));
	connect(ui->viewBox, SIGNAL(currentIndexChanged(int)), this, SLOT(changedViewBox()));

	connect(ui->expandButton, SIGNAL(clicked()), this, SLOT(togglethreadview()));
	connect(ui->previousButton, SIGNAL(clicked()), this, SLOT(previousMessage()));
	connect(ui->nextButton, SIGNAL(clicked()), this, SLOT(nextMessage()));
	connect(ui->nextUnreadButton, SIGNAL(clicked()), this, SLOT(nextUnreadMessage()));
	connect(ui->downloadButton, SIGNAL(clicked()), this, SLOT(downloadAllFiles()));

	connect(ui->filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterItems(QString)));
	connect(ui->filterLineEdit, SIGNAL(filterChanged(int)), this, SLOT(filterColumnChanged(int)));

	connect(ui->actionSave_image, SIGNAL(triggered()), this, SLOT(saveImage()));

	/* Set own item delegate */
	RSItemDelegate *itemDelegate = new RSItemDelegate(this);
	itemDelegate->setSpacing(QSize(0, 2));
	ui->threadTreeWidget->setItemDelegate(itemDelegate);

	/* Set header resize modes and initial section sizes */
	QHeaderView * ttheader = ui->threadTreeWidget->header () ;
	QHeaderView_setSectionResizeModeColumn(ttheader, COLUMN_THREAD_TITLE, QHeaderView::Interactive);
	ttheader->resizeSection (COLUMN_THREAD_DATE,  140);
	ttheader->resizeSection (COLUMN_THREAD_TITLE, 440);
	ttheader->resizeSection (COLUMN_THREAD_AUTHOR, 150);

	ui->threadTreeWidget->sortItems(COLUMN_THREAD_DATE, Qt::DescendingOrder);

	/* Set text of column "Read" to empty - without this the column has a number as header text */
	QTreeWidgetItem *headerItem = ui->threadTreeWidget->headerItem();
	headerItem->setText(COLUMN_THREAD_READ, "");

	/* add filter actions */
	ui->filterLineEdit->addFilter(QIcon(), tr("Title"), COLUMN_THREAD_TITLE, tr("Search Title"));
	ui->filterLineEdit->addFilter(QIcon(), tr("Date"), COLUMN_THREAD_DATE, tr("Search Date"));
	ui->filterLineEdit->addFilter(QIcon(), tr("Author"), COLUMN_THREAD_AUTHOR, tr("Search Author"));
	ui->filterLineEdit->addFilter(QIcon(), tr("Content"), COLUMN_THREAD_CONTENT, tr("Search Content"));
	// see processSettings
	//ui->filterLineEdit->setCurrentFilter(COLUMN_THREAD_TITLE);

	mLastViewType = -1;

	// load settings
	processSettings(true);

	/* Set header sizes for the fixed columns and resize modes, must be set after processSettings */
	ttheader->resizeSection (COLUMN_THREAD_READ,  24);
	QHeaderView_setSectionResizeModeColumn(ttheader, COLUMN_THREAD_READ, QHeaderView::Fixed);
	ttheader->hideSection (COLUMN_THREAD_CONTENT);

	ui->progressBar->hide();
	ui->progressText->hide();

	mFillThread = NULL;

	setGroupId(forumId);

    ui->threadTreeWidget->installEventFilter(this);

    ui->postText->clear();
    ui->by_label->setId(RsGxsId());
    ui->time_label->clear() ;
    ui->line->hide() ;
    ui->line_2->hide() ;
    ui->by_text_label->hide() ;
    ui->by_label->hide() ;
    ui->postText->setImageBlockWidget(ui->imageBlockWidget);
    ui->postText->resetImagesStatus(Settings->getForumLoadEmbeddedImages()) ;

    ui->subscribeToolButton->setToolTip(tr("<p>Subscribing to the forum will gather \
                                           available posts from your subscribed friends, and make the \
                                           forum visible to all other friends.</p><p>Afterwards you can unsubscribe from the context menu of the forum list at left.</p>"));
    ui->threadTreeWidget->enableColumnCustomize(true);
}

GxsForumThreadWidget::~GxsForumThreadWidget()
{
	if (mFillThread) {
		mFillThread->stop();
		delete(mFillThread);
		mFillThread = NULL;
	}

	// save settings
	processSettings(false);

	delete ui;

	delete(mThreadCompareRole);
}

void GxsForumThreadWidget::processSettings(bool load)
{
	mInProcessSettings = true;

	QHeaderView *header = ui->threadTreeWidget->header();

	Settings->beginGroup(QString("ForumThreadWidget"));

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

void GxsForumThreadWidget::groupIdChanged()
{
	ui->forumName->setText(groupId().isNull () ? "" : tr("Loading"));
	mNewCount = 0;
	mUnreadCount = 0;

	emit groupChanged(this);

	fillComplete();
}

QString GxsForumThreadWidget::groupName(bool withUnreadCount)
{
	QString name = groupId().isNull () ? tr("No name") : ui->forumName->text();

	if (withUnreadCount && mUnreadCount) {
		name += QString(" (%1)").arg(mUnreadCount);
	}

	return name;
}

QIcon GxsForumThreadWidget::groupIcon()
{
	if (mStateHelper->isLoading(mTokenTypeGroupData) || mFillThread) {
		return QIcon(":/images/kalarm.png");
	}

	if (mNewCount) {
		return QIcon(":/images/message-state-new.png");
	}

	return QIcon();
}

void GxsForumThreadWidget::changeEvent(QEvent *e)
{
	RsGxsUpdateBroadcastWidget::changeEvent(e);
	switch (e->type()) {
	case QEvent::StyleChange:
		calculateIconsAndFonts();
		break;
	default:
		// remove compiler warnings
		break;
	}
}

static void removeMessages(std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > &msgIds, QList<RsGxsMessageId> &removeMsgId)
{
	QList<RsGxsMessageId> removedMsgId;

	std::map<RsGxsGroupId, std::vector<RsGxsMessageId> >::iterator grpIt;
	for (grpIt = msgIds.begin(); grpIt != msgIds.end(); ) {
		std::vector<RsGxsMessageId> &msgs = grpIt->second;

		QList<RsGxsMessageId>::const_iterator removeMsgIt;
		for (removeMsgIt = removeMsgId.begin(); removeMsgIt != removeMsgId.end(); ++removeMsgIt) {
			std::vector<RsGxsMessageId>::iterator msgIt = std::find(msgs.begin(), msgs.end(), *removeMsgIt);
			if (msgIt != msgs.end()) {
				removedMsgId.push_back(*removeMsgIt);
				msgs.erase(msgIt);
			}
		}

		if (msgs.empty()) {
			std::map<RsGxsGroupId, std::vector<RsGxsMessageId> >::iterator grpItErase = grpIt++;
			msgIds.erase(grpItErase);
		} else {
			++grpIt;
		}
	}

	if (!removedMsgId.isEmpty()) {
		QList<RsGxsMessageId>::const_iterator removedMsgIt;
		for (removedMsgIt = removedMsgId.begin(); removedMsgIt != removedMsgId.end(); ++removedMsgIt) {
			// remove first message id
			removeMsgId.removeOne(*removedMsgIt);
		}
	}
}

void GxsForumThreadWidget::updateDisplay(bool complete)
{
	if (complete) {
		/* Fill complete */
		requestGroupData();
		insertThreads();
		insertMessage();

		mIgnoredMsgId.clear();

		return;
	}

	bool updateGroup = false;
	const std::list<RsGxsGroupId> &grpIdsMeta = getGrpIdsMeta();
	if (std::find(grpIdsMeta.begin(), grpIdsMeta.end(), groupId()) != grpIdsMeta.end()) {
		updateGroup = true;
	}

	const std::list<RsGxsGroupId> &grpIds = getGrpIds();
	if (std::find(grpIds.begin(), grpIds.end(), groupId()) != grpIds.end()) {
		updateGroup = true;
		/* Update threads */
		insertThreads();
	} else {
		std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > msgIds;
		getAllMsgIds(msgIds);

		if (!mIgnoredMsgId.empty()) {
			/* Filter ignored messages */
			removeMessages(msgIds, mIgnoredMsgId);
		}

		if (msgIds.find(groupId()) != msgIds.end()) {
			/* Update threads */
			insertThreads();
		}
	}

	if (updateGroup) {
		requestGroupData();
	}
}

void GxsForumThreadWidget::threadListCustomPopupMenu(QPoint /*point*/)
{
	if (mFillThread) {
		return;
	}

	QMenu contextMnu(this);

	QAction *replyAct = new QAction(QIcon(IMAGE_MESSAGEREPLY), tr("Reply"), &contextMnu);
	connect(replyAct, SIGNAL(triggered()), this, SLOT(replytoforummessage()));

    QAction *replyauthorAct = new QAction(QIcon(IMAGE_MESSAGEREPLY), tr("Reply with private message"), &contextMnu);
    connect(replyauthorAct, SIGNAL(triggered()), this, SLOT(replytomessage()));

    QAction *flagasbadAct = new QAction(QIcon(IMAGE_BIOHAZARD), tr("Ban this author"), &contextMnu);
    flagasbadAct->setToolTip(tr("This will block/hide messages from this person, and notify neighbor nodes.")) ;
    connect(flagasbadAct, SIGNAL(triggered()), this, SLOT(flagpersonasbad()));

    QAction *newthreadAct = new QAction(QIcon(IMAGE_MESSAGE), tr("Start New Thread"), &contextMnu);
	newthreadAct->setEnabled (IS_GROUP_SUBSCRIBED(mSubscribeFlags));
	connect(newthreadAct , SIGNAL(triggered()), this, SLOT(createthread()));

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

		if (rowsUnread.isEmpty()) {
			markMsgAsRead->setDisabled(true);
		}
		if (rowsRead.isEmpty()) {
			markMsgAsUnread->setDisabled(true);
		}

		bool hasUnreadChildren = false;
		bool hasReadChildren = false;
		int rowCount = rows.count();
		for (int i = 0; i < rowCount; ++i) {
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
    QAction* action = contextMnu.addAction(QIcon(IMAGE_COPYLINK), tr("Copy RetroShare Link"), this, SLOT(copyMessageLink()));
	action->setEnabled(!groupId().isNull() && !mThreadId.isNull());
	contextMnu.addSeparator();
	contextMnu.addAction(markMsgAsRead);
	contextMnu.addAction(markMsgAsReadChildren);
	contextMnu.addAction(markMsgAsUnread);
	contextMnu.addAction(markMsgAsUnreadChildren);
    contextMnu.addSeparator();
    contextMnu.addAction(expandAll);
	contextMnu.addAction(collapseAll);

    contextMnu.addSeparator();
    contextMnu.addAction(flagasbadAct);
    contextMnu.addSeparator();
    contextMnu.addAction(replyauthorAct);

	contextMnu.exec(QCursor::pos());
}

void GxsForumThreadWidget::contextMenuTextBrowser(QPoint point)
{
	QMatrix matrix;
	matrix.translate(ui->postText->horizontalScrollBar()->value(), ui->postText->verticalScrollBar()->value());

	QMenu *contextMnu = ui->postText->createStandardContextMenu(matrix.map(point));

	contextMnu->addSeparator();

	QTextCursor cursor = ui->postText->cursorForPosition(point);
	if(ImageUtil::checkImage(cursor))
	{
		ui->actionSave_image->setData(point);
		contextMnu->addAction(ui->actionSave_image);
	}

	contextMnu->exec(ui->postText->viewport()->mapToGlobal(point));
	delete(contextMnu);
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
	return RsGxsUpdateBroadcastWidget::eventFilter(obj, event);
}

void GxsForumThreadWidget::togglethreadview()
{
	// save state of button
	Settings->setValueToGroup("ForumThreadWidget", "expandButton", ui->expandButton->isChecked());

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
    Q_ASSERT(item != NULL);

	if (!item || !item->isSelected()) {
		mThreadId.clear();
	} else {
		mThreadId = RsGxsMessageId(item->data(COLUMN_THREAD_DATA, ROLE_THREAD_MSGID).toString().toStdString());
	}

    	// Show info about who passed on this message.
    	if(mForumGroup.mMeta.mSignFlags & GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_TRACK_MESSAGES) 
        {
            RsPeerId providerId ;
            if(item != NULL) {
                std::string msgId = item->data(COLUMN_THREAD_DATA, ROLE_THREAD_MSGID).toString().toStdString();
                RsGxsMessageId mid(msgId);
                if(rsGRouter->getTrackingInfo(mid,providerId) && !providerId.isNull() )
                    item->setToolTip(COLUMN_THREAD_TITLE,tr("This message was obtained from %1").arg(QString::fromUtf8(rsPeers->getPeerName(providerId).c_str())));
            }
        }
	if (mFillThread) {
		return;
	}
	ui->postText->resetImagesStatus(Settings->getForumLoadEmbeddedImages()) ;

	insertMessage();
}

void GxsForumThreadWidget::clickedThread(QTreeWidgetItem *item, int column)
{
	if (item == NULL) {
		return;
	}

	if (mFillThread) {
		return;
	}

	if (groupId().isNull() || !IS_GROUP_SUBSCRIBED(mSubscribeFlags)) {
		return;
	}

	if (column == COLUMN_THREAD_READ) {
		QList<QTreeWidgetItem*> rows;
		rows.append(item);
		uint32_t status = item->data(COLUMN_THREAD_DATA, ROLE_THREAD_STATUS).toUInt();
		setMsgReadStatus(rows, IS_MSG_UNREAD(status));
	}
}

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
		++itemIterator;

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
		emit groupChanged(this);
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
	for (item = items.begin (); item != items.end (); ++item) {
		if (*item) {
			delete (*item);
		}
	}
	items.clear();
}

void GxsForumThreadWidget::insertGroupData()
{
#ifdef DEBUG_FORUMS
	std::cerr << "GxsForumThreadWidget::insertGroupData" << std::endl;
#endif
    GxsIdDetails::process(mForumGroup.mMeta.mAuthorId, &loadAuthorIdCallback, this);
	calculateIconsAndFonts();
}

/*static*/ void GxsForumThreadWidget::loadAuthorIdCallback(GxsIdDetailsType type, const RsIdentityDetails &details, QObject *object, const QVariant &)
{
    GxsForumThreadWidget *tw = dynamic_cast<GxsForumThreadWidget*>(object);
    if(!tw)
        return;

    QString author;
    switch (type) {
    case GXS_ID_DETAILS_TYPE_EMPTY:
        author = GxsIdDetails::getEmptyIdText();
        break;
    case GXS_ID_DETAILS_TYPE_FAILED:
        author = GxsIdDetails::getFailedText(details.mId);
        break;
    case GXS_ID_DETAILS_TYPE_LOADING:
        author = GxsIdDetails::getLoadingText(details.mId);
        break;
    case GXS_ID_DETAILS_TYPE_BANNED:
        author = tr("[Banned]") ;
        break ;
    case GXS_ID_DETAILS_TYPE_DONE:
        author = GxsIdDetails::getName(details);
        break;
    }

    const RsGxsForumGroup& group = tw->mForumGroup;

    tw->mSubscribeFlags = group.mMeta.mSubscribeFlags;
    tw->mSignFlags = group.mMeta.mSignFlags;
    tw->ui->forumName->setText(QString::fromUtf8(group.mMeta.mGroupName.c_str()));

    QString anti_spam_features1 ;
    if(IS_GROUP_PGP_AUTHED(tw->mSignFlags)) anti_spam_features1 = tr("Anonymous IDs reputation threshold set to 0.4");
            
    QString anti_spam_features2 ;
    if(IS_GROUP_MESSAGE_TRACKING(tw->mSignFlags)) anti_spam_features2 = tr("Message routing info kept for 10 days");
    
    tw->mForumDescription = QString("<b>%1: \t</b>%2<br/>").arg(tr("Forum name"), QString::fromUtf8( group.mMeta.mGroupName.c_str()));
    tw->mForumDescription += QString("<b>%1: \t</b>%2<br/>").arg(tr("Subscribers")).arg(group.mMeta.mPop);
    tw->mForumDescription += QString("<b>%1: \t</b>%2<br/>").arg(tr("Posts (at neighbor nodes)")).arg(group.mMeta.mVisibleMsgCount);
    
    QString distrib_string = tr("[unknown]");
    switch(group.mMeta.mCircleType)
    {
    case GXS_CIRCLE_TYPE_PUBLIC: distrib_string = tr("Public") ;
	    break ;
    case GXS_CIRCLE_TYPE_EXTERNAL: 
    {
	    RsGxsCircleDetails det ;
        
        	// !! What we need here is some sort of CircleLabel, which loads the circle and updates the label when done.
        
	    if(rsGxsCircles->getCircleDetails(group.mMeta.mCircleId,det)) 
		    distrib_string = tr("Restricted to members of circle \"")+QString::fromUtf8(det.mCircleName.c_str()) +"\"";
	    else
		    distrib_string = tr("Restricted to members of circle ")+QString::fromStdString(group.mMeta.mCircleId.toStdString()) ;
    }
	    break ;
    case GXS_CIRCLE_TYPE_YOUREYESONLY: distrib_string = tr("Your eyes only");
	    break ;
    case GXS_CIRCLE_TYPE_LOCAL: distrib_string = tr("You and your friend nodes");
	    break ;
    default:
	    std::cerr << "(EE) badly initialised group distribution ID = " << group.mMeta.mCircleType << std::endl;
    }
            
    tw->mForumDescription += QString("<b>%1: \t</b>%2<br/>").arg(tr("Distribution"), distrib_string);
    tw->mForumDescription += QString("<b>%1: \t</b>%2<br/>").arg(tr("Author"), author);
       
       if(!anti_spam_features1.isNull())
    		tw->mForumDescription += QString("<b>%1: \t</b>%2<br/>").arg(tr("Anti-spam")).arg(anti_spam_features1);
       
       if(!anti_spam_features2.isNull())
    		tw->mForumDescription += QString("<b>%1: \t</b>%2<br/>").arg(tr("Anti-spam")).arg(anti_spam_features2);
       
    tw->mForumDescription += QString("<b>%1: </b><br/><br/>%2").arg(tr("Description"), QString::fromUtf8(group.mDescription.c_str()));

    tw->ui->subscribeToolButton->setSubscribed(IS_GROUP_SUBSCRIBED(tw->mSubscribeFlags));
    tw->mStateHelper->setWidgetEnabled(tw->ui->newthreadButton, (IS_GROUP_SUBSCRIBED(tw->mSubscribeFlags)));

    if (tw->mThreadId.isNull() && !tw->mStateHelper->isLoading(tw->mTokenTypeMessageData))
    {
        //ui->threadTitle->setText(tr("Forum Description"));
        tw->ui->postText->setText(tw->mForumDescription);
    }
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
			mFillThread = NULL;

			mStateHelper->setLoading(mTokenTypeInsertThreads, false);
			emit groupChanged(this);
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

			mStateHelper->setActive(mTokenTypeInsertThreads, true);
			ui->threadTreeWidget->setSortingEnabled(false);

			GxsIdDetails::enableProcess(false);

			/* add all messages in! */
			if (mLastViewType != thread->mViewType || mLastForumID != groupId()) {
				ui->threadTreeWidget->clear();
				mLastViewType = thread->mViewType;
				mLastForumID = groupId();
				ui->threadTreeWidget->insertTopLevelItems(0, thread->mItems);

				// clear list
				thread->mItems.clear();
			} else {
				fillThreads(thread->mItems, thread->mExpandNewMessages, thread->mItemToExpand);

				// cleanup list
				cleanupItems(thread->mItems);
			}

			/* Move value from ROLE_THREAD_AUTHOR to GxsIdRSTreeWidgetItem::setId */
			QTreeWidgetItemIterator itemIterator(ui->threadTreeWidget);
			QTreeWidgetItem *item = NULL;
			while ((item = *itemIterator) != NULL) {
				++itemIterator;

				QString gxsId = item->data(COLUMN_THREAD_DATA, ROLE_THREAD_AUTHOR).toString();
				if (gxsId.isEmpty()) {
					continue;
				}

				item->setData(COLUMN_THREAD_DATA, ROLE_THREAD_AUTHOR, QVariant());

				GxsIdRSTreeWidgetItem *gxsIdItem = dynamic_cast<GxsIdRSTreeWidgetItem*>(item);
				if (gxsIdItem) {
					gxsIdItem->setId(RsGxsId(gxsId.toStdString()), COLUMN_THREAD_AUTHOR, false);
				}
			}

			GxsIdDetails::enableProcess(true);

			ui->threadTreeWidget->setSortingEnabled(true);

			if (thread->mFocusMsgId.empty() == false) {
				/* Search exisiting item */
				QTreeWidgetItemIterator itemIterator(ui->threadTreeWidget);
				QTreeWidgetItem *item = NULL;
				while ((item = *itemIterator) != NULL) {
					++itemIterator;

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
			calculateIconsAndFonts();
			calculateUnreadCount();
			emit groupChanged(this);

			if (!mNavigatePendingMsgId.isNull()) {
				navigate(mNavigatePendingMsgId);
				mNavigatePendingMsgId.clear();
			}
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

QTreeWidgetItem *GxsForumThreadWidget::convertMsgToThreadWidget(const RsGxsForumMsg &msg, bool useChildTS, uint32_t filterColumn)
{
    // Early check for a message that should be hidden because its author 
    // is flagged with a bad reputation
    
    
    bool redacted =  rsReputations->isIdentityBanned(msg.mMeta.mAuthorId) ;
                
    GxsIdRSTreeWidgetItem *item = new GxsIdRSTreeWidgetItem(mThreadCompareRole,GxsIdDetails::ICON_TYPE_ALL || (redacted?(GxsIdDetails::ICON_TYPE_REDACTED):0));
	item->moveToThread(ui->threadTreeWidget->thread());

	QString text;

    	if(redacted)
		item->setText(COLUMN_THREAD_TITLE, tr("[ ... Redacted message ... ]"));
	else
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

	// Set later with GxsIdRSTreeWidgetItem::setId
	item->setData(COLUMN_THREAD_DATA, ROLE_THREAD_AUTHOR, QString::fromStdString(msg.mMeta.mAuthorId.toStdString()));
    
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

	item->setData(COLUMN_THREAD_DATA, ROLE_THREAD_MSGID, QString::fromStdString(msg.mMeta.mMsgId.toStdString()));
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

QTreeWidgetItem *GxsForumThreadWidget::generateMissingItem(const RsGxsMessageId &msgId)
{
    GxsIdRSTreeWidgetItem *item = new GxsIdRSTreeWidgetItem(mThreadCompareRole,GxsIdDetails::ICON_TYPE_ALL);
    
	item->setText(COLUMN_THREAD_TITLE, tr("[ ... Missing Message ... ]"));
	item->setData(COLUMN_THREAD_DATA, ROLE_THREAD_MSGID, QString::fromStdString(msgId.toStdString()));
	item->setData(COLUMN_THREAD_DATA, ROLE_THREAD_MISSING, true);
        
	item->setId(RsGxsId(), COLUMN_THREAD_AUTHOR, false); // fixed up columnId()

	return item;
}

void GxsForumThreadWidget::insertThreads()
{
#ifdef DEBUG_FORUMS
	/* get the current Forum */
	std::cerr << "GxsForumThreadWidget::insertThreads()" << std::endl;
#endif

	mNavigatePendingMsgId.clear();
	ui->progressBar->reset();

	if (mFillThread) {
#ifdef DEBUG_FORUMS
		std::cerr << "GxsForumThreadWidget::insertThreads() stop current fill thread" << std::endl;
#endif
		// stop current fill thread
		GxsForumsFillThread *thread = mFillThread;
		mFillThread = NULL;
		thread->stop();
		delete(thread);

		mStateHelper->setLoading(mTokenTypeInsertThreads, false);
	}

	if (groupId().isNull())
	{
		/* not an actual forum - clear */
		mStateHelper->setActive(mTokenTypeInsertThreads, false);
		mStateHelper->clear(mTokenTypeInsertThreads);

		/* clear last stored forumID */
		mLastForumID.clear();

#ifdef DEBUG_FORUMS
		std::cerr << "GxsForumThreadWidget::insertThreads() Current Thread Invalid" << std::endl;
#endif

		return;
	}

#ifdef DEBUG_FORUMS
	std::cerr << "GxsForumThreadWidget::insertThreads() Start filling Forum threads" << std::endl;
#endif

	mStateHelper->setLoading(mTokenTypeInsertThreads, true);

	// create fill thread
	mFillThread = new GxsForumsFillThread(this);

	// set data
	mFillThread->mCompareRole = mThreadCompareRole;
	mFillThread->mForumId = groupId();
	mFillThread->mFilterColumn = ui->filterLineEdit->currentFilter();
	mFillThread->mExpandNewMessages = Settings->getForumExpandNewMessages();
	mFillThread->mViewType = ui->viewBox->currentIndex();
	if (mLastViewType != mFillThread->mViewType || mLastForumID != groupId()) {
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
	emit groupChanged(this);
}

static void copyItem(QTreeWidgetItem *item, const QTreeWidgetItem *newItem)
{
	int i;
	for (i = 0; i < COLUMN_THREAD_COUNT; ++i) {
		if (i != COLUMN_THREAD_AUTHOR) {
			/* Copy text */
			item->setText(i, newItem->text(i));
		}
	}
	for (i = 0; i < ROLE_THREAD_COUNT; ++i) {
		item->setData(COLUMN_THREAD_DATA, Qt::UserRole + i, newItem->data(COLUMN_THREAD_DATA, Qt::UserRole + i));
	}
}

void GxsForumThreadWidget::fillThreads(QList<QTreeWidgetItem *> &threadList, bool expandNewMessages, QList<QTreeWidgetItem*> &itemToExpand)
{
#ifdef DEBUG_FORUMS
	std::cerr << "GxsForumThreadWidget::fillThreads()" << std::endl;
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
			++index;
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
			copyItem(threadItem, *newThread);

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
	std::cerr << "GxsForumThreadWidget::fillThreads() done" << std::endl;
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
			++index;
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
			copyItem(childItem, newChildItem);

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

void GxsForumThreadWidget::insertMessage()
{
	if (groupId().isNull())
	{
		mStateHelper->setActive(mTokenTypeMessageData, false);
		mStateHelper->clear(mTokenTypeMessageData);

		ui->postText->clear();
        //ui->threadTitle->clear();
		return;
	}

	if (mThreadId.isNull())
	{
		mStateHelper->setActive(mTokenTypeMessageData, false);
		mStateHelper->clear(mTokenTypeMessageData);

        //ui->threadTitle->setText(tr("Forum Description"));
		ui->postText->setText(mForumDescription);
		return;
	}

	mStateHelper->setActive(mTokenTypeMessageData, true);

	QTreeWidgetItem *item = ui->threadTreeWidget->currentItem();
	if (item) {
		QTreeWidgetItem *parentItem = item->parent();
		int index = parentItem ? parentItem->indexOfChild(item) : ui->threadTreeWidget->indexOfTopLevelItem(item);
		int count = parentItem ? parentItem->childCount() : ui->threadTreeWidget->topLevelItemCount();
		mStateHelper->setWidgetEnabled(ui->previousButton, (index > 0));
		mStateHelper->setWidgetEnabled(ui->nextButton, (index < count - 1));
	} else {
		// there is something wrong
		mStateHelper->setWidgetEnabled(ui->previousButton, false);
		mStateHelper->setWidgetEnabled(ui->nextButton, false);
		return;
	}

	mStateHelper->setWidgetEnabled(ui->newmessageButton, (IS_GROUP_SUBSCRIBED(mSubscribeFlags) && mThreadId.isNull() == false));

	/* blank text, incase we get nothing */
	ui->postText->clear();
    ui->by_label->setId(RsGxsId());
    ui->time_label->clear() ;
    ui->line->hide() ;
    ui->line_2->hide() ;
    ui->by_text_label->hide() ;
    ui->by_label->hide() ;

	/* request Post */
	RsGxsGrpMsgIdPair msgId = std::make_pair(groupId(), mThreadId);
	requestMessageData(msgId);
}

void GxsForumThreadWidget::insertMessageData(const RsGxsForumMsg &msg)
{
	/* As some time has elapsed since request - check that this is still the current msg.
	 * otherwise, another request will fill the data
     */

	if ((msg.mMeta.mGroupId != groupId()) || (msg.mMeta.mMsgId != mThreadId))
	{
		std::cerr << "GxsForumThreadWidget::insertPostData() Ignoring Invalid Data....";
		std::cerr << std::endl;
		std::cerr << "\t CurrForumId: " << groupId() << " != msg.GroupId: " << msg.mMeta.mGroupId;
		std::cerr << std::endl;
		std::cerr << "\t or CurrThdId: " << mThreadId << " != msg.MsgId: " << msg.mMeta.mMsgId;
		std::cerr << std::endl;
		std::cerr << std::endl;

		mStateHelper->setActive(mTokenTypeMessageData, false);
		mStateHelper->clear(mTokenTypeMessageData);

		return;
	}

    bool redacted = rsReputations->isIdentityBanned(msg.mMeta.mAuthorId) ;
    
	mStateHelper->setActive(mTokenTypeMessageData, true);

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
    ui->by_label->setId(msg.mMeta.mAuthorId) ;

    ui->line->show() ;
    ui->line_2->show() ;
    ui->by_text_label->show() ;
    ui->by_label->show() ;

    if(redacted)
    {
	QString extraTxt = tr("<p><font color=\"#ff0000\"><b>The author of this message (with ID %1) is banned.</b>").arg(QString::fromStdString(msg.mMeta.mAuthorId.toStdString())) ;
	extraTxt += tr("<UL><li><b><font color=\"#ff0000\">Messages from this author are not forwarded. </font></b></li>") ;
	extraTxt += tr("<li><b><font color=\"#ff0000\">Messages from this author are replaced by this text. </font></b></li></ul>") ;
    	extraTxt += tr("<p><b><font color=\"#ff0000\">You can force the visibility and forwarding of messages by setting a different opinion for that Id in People's tab.</font></b></p>") ;
        
	ui->postText->setHtml(extraTxt);
    }
    else
    {
	QString extraTxt = RsHtml().formatText(ui->postText->document(), QString::fromUtf8(msg.mMsg.c_str()), RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS);
	ui->postText->setHtml(extraTxt);
    }
    //ui->threadTitle->setText(QString::fromUtf8(msg.mMeta.mMsgName.c_str()));
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

	while (true) {
		QTreeWidgetItemIterator itemIterator = currentItem ? QTreeWidgetItemIterator(currentItem, QTreeWidgetItemIterator::NotHidden) : QTreeWidgetItemIterator(ui->threadTreeWidget, QTreeWidgetItemIterator::NotHidden);

		QTreeWidgetItem *item;
		while ((item = *itemIterator) != NULL) {
			++itemIterator;

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

/* get selected messages
   the messages tree is single selected, but who knows ... */
int GxsForumThreadWidget::getSelectedMsgCount(QList<QTreeWidgetItem*> *rows, QList<QTreeWidgetItem*> *rowsRead, QList<QTreeWidgetItem*> *rowsUnread)
{
	if (rowsRead) rowsRead->clear();
	if (rowsUnread) rowsUnread->clear();

	QList<QTreeWidgetItem*> selectedItems = ui->threadTreeWidget->selectedItems();
	for(QList<QTreeWidgetItem*>::iterator it = selectedItems.begin(); it != selectedItems.end(); ++it) {
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

		uint32_t statusNew = (status & ~(GXS_SERV::GXS_MSG_STATUS_GUI_NEW | GXS_SERV::GXS_MSG_STATUS_GUI_UNREAD)); // orig status, without NEW AND UNREAD
		if (!read) {
			statusNew |= GXS_SERV::GXS_MSG_STATUS_GUI_UNREAD;
		}

		if (status != statusNew) // is it different?
		{
			std::string msgId = (*row)->data(COLUMN_THREAD_DATA, ROLE_THREAD_MSGID).toString().toStdString();

			// NB: MUST BE PART OF ACTIVE THREAD--- OR ELSE WE MUST STORE GROUPID SOMEWHERE!.
			// LIKE THIS BELOW...
			//std::string grpId = (*Row)->data(COLUMN_THREAD_DATA, ROLE_THREAD_GROUPID).toString().toStdString();

            RsGxsGrpMsgIdPair msgPair = std::make_pair(groupId(), RsGxsMessageId(msgId));

			uint32_t token;
			rsGxsForums->setMessageReadStatus(token, msgPair, read);

			/* Add message id to ignore list for the next updateDisplay */
			mIgnoredMsgId.push_back(RsGxsMessageId(msgId));

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
		for (std::list<QTreeWidgetItem*>::iterator it = changedItems.begin(); it != changedItems.end(); ++it) {
			calculateIconsAndFonts(*it);
		}
		calculateUnreadCount();
	}
}

void GxsForumThreadWidget::markMsgAsReadUnread (bool read, bool children, bool forum)
{
	if (groupId().isNull() || !IS_GROUP_SUBSCRIBED(mSubscribeFlags)) {
		return;
	}

	/* get selected messages */
	QList<QTreeWidgetItem*> rows;
	if (forum) {
		int itemCount = ui->threadTreeWidget->topLevelItemCount();
		for (int item = 0; item < itemCount; ++item) {
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

			for (int i = 0; i < row->childCount(); ++i) {
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

void GxsForumThreadWidget::setAllMessagesReadDo(bool read, uint32_t &/*token*/)
{
	markMsgAsReadUnread(read, true, true);
}

bool GxsForumThreadWidget::navigate(const RsGxsMessageId &msgId)
{
	if (mStateHelper->isLoading(mTokenTypeInsertThreads)) {
		mNavigatePendingMsgId = msgId;

		/* No information if message is available */
		return true;
	}

	QString msgIdString = QString::fromStdString(msgId.toStdString());

	/* Search exisiting item */
	QTreeWidgetItemIterator itemIterator(ui->threadTreeWidget);
	QTreeWidgetItem *item = NULL;
	while ((item = *itemIterator) != NULL) {
		++itemIterator;

		if (item->data(COLUMN_THREAD_DATA, ROLE_THREAD_MSGID).toString() == msgIdString) {
			ui->threadTreeWidget->setCurrentItem(item);
			ui->threadTreeWidget->setFocus();
			return true;
		}
	}

	return false;
}

bool GxsForumThreadWidget::isLoading()
{
	if (mStateHelper->isLoading(mTokenTypeGroupData) || mFillThread) {
		return true;
	}

	return GxsMessageFrameWidget::isLoading();
}

void GxsForumThreadWidget::copyMessageLink()
{
	if (groupId().isNull() || mThreadId.isNull()) {
		return;
	}

	RetroShareLink link;
    QTreeWidgetItem *item = ui->threadTreeWidget->currentItem();

    QString thread_title = (item != NULL)?item->text(COLUMN_THREAD_TITLE):QString() ;

    if (link.createGxsMessageLink(RetroShareLink::TYPE_FORUM, groupId(), mThreadId, thread_title)) {
		QList<RetroShareLink> urls;
		urls.push_back(link);
		RSLinkClipboard::copyLinks(urls);
	}
}

void GxsForumThreadWidget::subscribeGroup(bool subscribe)
{
	if (groupId().isNull()) {
		return;
	}

	uint32_t token;
	rsGxsForums->subscribeToGroup(token, groupId(), subscribe);
//	mTokenQueue->queueRequest(token, 0, RS_TOKREQ_ANSTYPE_ACK, TOKEN_TYPE_SUBSCRIBE_CHANGE);
}

void GxsForumThreadWidget::createmessage()
{
	if (groupId().isNull () || !IS_GROUP_SUBSCRIBED(mSubscribeFlags)) {
		return;
	}

	CreateGxsForumMsg *cfm = new CreateGxsForumMsg(groupId(), mThreadId);
	cfm->show();

	/* window will destroy itself! */
}

void GxsForumThreadWidget::createthread()
{
	if (groupId().isNull ()) {
		QMessageBox::information(this, tr("RetroShare"), tr("No Forum Selected!"));
		return;
	}

	CreateGxsForumMsg *cfm = new CreateGxsForumMsg(groupId(), RsGxsMessageId());
	cfm->show();

	/* window will destroy itself! */
}

static QString buildReplyHeader(const RsMsgMetaData &meta)
{
	RetroShareLink link;
	link.createMessage(meta.mAuthorId, "");
	QString from = link.toHtml();

	QString header = QString("<span>-----%1-----").arg(QApplication::translate("GxsForumThreadWidget", "Original Message"));
	header += QString("<br><font size='3'><strong>%1: </strong>%2</font><br>").arg(QApplication::translate("GxsForumThreadWidget", "From"), from);

	header += QString("<br><font size='3'><strong>%1: </strong>%2</font><br>").arg(QApplication::translate("GxsForumThreadWidget", "Sent"), DateTime::formatLongDateTime(meta.mPublishTs));
	header += QString("<font size='3'><strong>%1: </strong>%2</font></span><br>").arg(QApplication::translate("GxsForumThreadWidget", "Subject"), QString::fromUtf8(meta.mMsgName.c_str()));
	header += "<br>";

	header += QApplication::translate("GxsForumThreadWidget", "On %1, %2 wrote:").arg(DateTime::formatDateTime(meta.mPublishTs), from);

	return header;
}

void GxsForumThreadWidget::flagpersonasbad()
{
    // no need to use the token system for that, since we just need to find out the author's name, which is in the item.
    
	if (groupId().isNull() || mThreadId.isNull()) {
		QMessageBox::information(this, tr("RetroShare"),tr("You cant reply to a non-existant Message"));
		return;
	}

	// Get Message ... then complete replyMessageData().
	RsGxsGrpMsgIdPair postId = std::make_pair(groupId(), mThreadId);
    
    	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;

#ifdef DEBUG_FORUMS
    std::cerr << "GxsForumThreadWidget::requestMsgData_BanAuthor(" << postId.first << "," << postId.second << ")";
    std::cerr << std::endl;
#endif

	GxsMsgReq msgIds;
	std::vector<RsGxsMessageId> &vect = msgIds[postId.first];
	vect.push_back(postId.second);

	uint32_t token;
	mTokenQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, msgIds, mTokenTypeBanAuthor);
}

void GxsForumThreadWidget::replytomessage()
{
	if (groupId().isNull() || mThreadId.isNull()) {
		QMessageBox::information(this, tr("RetroShare"),tr("You cant reply to a non-existant Message"));
		return;
	}

	// Get Message ... then complete replyMessageData().
	RsGxsGrpMsgIdPair postId = std::make_pair(groupId(), mThreadId);
	requestMsgData_ReplyMessage(postId);
}

void GxsForumThreadWidget::replytoforummessage()
{
	if (groupId().isNull() || mThreadId.isNull()) {
		QMessageBox::information(this, tr("RetroShare"),tr("You cant reply to a non-existant Message"));
		return;
	}

	// Get Message ... then complete replyMessageData().
	RsGxsGrpMsgIdPair postId = std::make_pair(groupId(), mThreadId);
	requestMsgData_ReplyForumMessage(postId);
}

void GxsForumThreadWidget::replyMessageData(const RsGxsForumMsg &msg)
{
	if ((msg.mMeta.mGroupId != groupId()) || (msg.mMeta.mMsgId != mThreadId))
	{
		std::cerr << "GxsForumThreadWidget::replyMessageData() ERROR Message Ids have changed!";
		std::cerr << std::endl;
		return;
	}

	if (!msg.mMeta.mAuthorId.isNull())
	{
		MessageComposer *msgDialog = MessageComposer::newMsg();
		msgDialog->setTitleText(QString::fromUtf8(msg.mMeta.mMsgName.c_str()), MessageComposer::REPLY);

		msgDialog->setQuotedMsg(QString::fromUtf8(msg.mMsg.c_str()), buildReplyHeader(msg.mMeta));

		msgDialog->addRecipient(MessageComposer::TO, RsGxsId(msg.mMeta.mAuthorId));
		msgDialog->show();
		msgDialog->activateWindow();

		/* window will destroy itself! */
	}
	else
	{
		QMessageBox::information(this, tr("RetroShare"),tr("You cant reply to an Anonymous Author"));
	}
}

void GxsForumThreadWidget::replyForumMessageData(const RsGxsForumMsg &msg)
{
	if ((msg.mMeta.mGroupId != groupId()) || (msg.mMeta.mMsgId != mThreadId))
	{
		std::cerr << "GxsForumThreadWidget::replyMessageData() ERROR Message Ids have changed!";
		std::cerr << std::endl;
		return;
	}

	if (!msg.mMeta.mAuthorId.isNull())
	{
	CreateGxsForumMsg *cfm = new CreateGxsForumMsg(groupId(), mThreadId);
	QTextDocument doc ;
//	doc.setHtml(QString::fromUtf8(msg.mMsg.c_str()) );
//	std::string cited_text(doc.toPlainText().toStdString()) ;
	RsHtml::makeQuotedText(ui->postText);

	cfm->insertPastedText(RsHtml::makeQuotedText(ui->postText)) ;
	cfm->show();

		/* window will destroy itself! */
	}
	else
	{
		QMessageBox::information(this, tr("RetroShare"),tr("You cant reply to an Anonymous Author"));
	}
}

void GxsForumThreadWidget::saveImage()
{
	QPoint point = ui->actionSave_image->data().toPoint();
	QTextCursor cursor = ui->postText->cursorForPosition(point);
	ImageUtil::extractImage(window(), cursor);
}

void GxsForumThreadWidget::changedViewBox()
{
	if (mInProcessSettings) {
		return;
	}

	// save index
	Settings->setValueToGroup("ForumThreadWidget", "viewBox", ui->viewBox->currentIndex());

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
	Settings->setValueToGroup("ForumThreadWidget", "filterColumn", column);
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
			++visibleChildCount;
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

void GxsForumThreadWidget::requestGroupData()
{
	mSubscribeFlags = 0;
	mSignFlags = 0;
	mForumDescription.clear();

	mTokenQueue->cancelActiveRequestTokens(mTokenTypeGroupData);

	if (groupId().isNull()) {
		mStateHelper->setActive(mTokenTypeGroupData, false);
		mStateHelper->setLoading(mTokenTypeGroupData, false);
		mStateHelper->clear(mTokenTypeGroupData);

		emit groupChanged(this);

		return;
	}

	mStateHelper->setLoading(mTokenTypeGroupData, true);
	emit groupChanged(this);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

	std::list<RsGxsGroupId> grpIds;
	grpIds.push_back(groupId());

#ifdef DEBUG_FORUMS
	std::cerr << "GxsForumThreadWidget::requestGroupData(" << groupId() << ")";
	std::cerr << std::endl;
#endif

	uint32_t token;
	mTokenQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, grpIds, mTokenTypeGroupData);
}

void GxsForumThreadWidget::loadGroupData(const uint32_t &token)
{
#ifdef DEBUG_FORUMS
	std::cerr << "GxsForumThreadWidget::loadGroup_CurrentForum()";
	std::cerr << std::endl;
#endif

	std::vector<RsGxsForumGroup> groups;
	rsGxsForums->getGroupData(token, groups);

	mStateHelper->setLoading(mTokenTypeGroupData, false);

	if (groups.size() == 1)
	{
        mForumGroup = groups[0];
        insertGroupData();

        mStateHelper->setActive(mTokenTypeGroupData, true);

        ui->subscribeToolButton->setHidden(IS_GROUP_SUBSCRIBED(mSubscribeFlags)) ;
    }
	else
	{
		std::cerr << "GxsForumThreadWidget::loadGroupSummary_CurrentForum() ERROR Invalid Number of Groups...";
		std::cerr << std::endl;

		mStateHelper->setActive(mTokenTypeGroupData, false);
		mStateHelper->clear(mTokenTypeGroupData);
	}

	emit groupChanged(this);
}

/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/

void GxsForumThreadWidget::requestMessageData(const RsGxsGrpMsgIdPair &msgId)
{
	mStateHelper->setLoading(mTokenTypeMessageData, true);

	mTokenQueue->cancelActiveRequestTokens(mTokenTypeMessageData);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;

#ifdef DEBUG_FORUMS
	std::cerr << "GxsForumThreadWidget::requestMessage(" << msgId.first << "," << msgId.second << ")";
	std::cerr << std::endl;
#endif

	GxsMsgReq msgIds;
	std::vector<RsGxsMessageId> &vect = msgIds[msgId.first];
	vect.push_back(msgId.second);

	uint32_t token;
	mTokenQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, msgIds, mTokenTypeMessageData);
}

void GxsForumThreadWidget::loadMessageData(const uint32_t &token)
{
	mStateHelper->setLoading(mTokenTypeMessageData, false);

#ifdef DEBUG_FORUMS
	std::cerr << "GxsForumThreadWidget::loadMessage()";
	std::cerr << std::endl;
#endif

	std::vector<RsGxsForumMsg> msgs;
	if (rsGxsForums->getMsgData(token, msgs)) {
		if (msgs.size() != 1) {
			std::cerr << "GxsForumThreadWidget::loadMessage() ERROR Wrong number of answers";
			std::cerr << std::endl;

			mStateHelper->setActive(mTokenTypeMessageData, false);
			mStateHelper->clear(mTokenTypeMessageData);
			return;
		}
		insertMessageData(msgs[0]);
	} else {
		std::cerr << "GxsForumThreadWidget::loadMessage() ERROR Missing Message Data...";
		std::cerr << std::endl;

		mStateHelper->setActive(mTokenTypeMessageData, false);
		mStateHelper->clear(mTokenTypeMessageData);
    }
}

/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/

void GxsForumThreadWidget::requestMsgData_ReplyMessage(const RsGxsGrpMsgIdPair &msgId)
{
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;

#ifdef DEBUG_FORUMS
    std::cerr << "GxsForumThreadWidget::requestMsgData_ReplyMessage(" << msgId.first << "," << msgId.second << ")";
    std::cerr << std::endl;
#endif

	GxsMsgReq msgIds;
	std::vector<RsGxsMessageId> &vect = msgIds[msgId.first];
	vect.push_back(msgId.second);

	uint32_t token;
	mTokenQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, msgIds, mTokenTypeReplyMessage);
}

void GxsForumThreadWidget::requestMsgData_ReplyForumMessage(const RsGxsGrpMsgIdPair &msgId)
{
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;

#ifdef DEBUG_FORUMS
    std::cerr << "GxsForumThreadWidget::requestMsgData_ReplyMessage(" << msgId.first << "," << msgId.second << ")";
    std::cerr << std::endl;
#endif

	GxsMsgReq msgIds;
	std::vector<RsGxsMessageId> &vect = msgIds[msgId.first];
	vect.push_back(msgId.second);

	uint32_t token;
	mTokenQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, msgIds, mTokenTypeReplyForumMessage);
}

void GxsForumThreadWidget::loadMsgData_ReplyMessage(const uint32_t &token)
{
#ifdef DEBUG_FORUMS
    std::cerr << "GxsForumThreadWidget::loadMsgData_ReplyMessage()";
    std::cerr << std::endl;
#endif

	std::vector<RsGxsForumMsg> msgs;
	if (rsGxsForums->getMsgData(token, msgs))
	{
		if (msgs.size() != 1)
		{
			std::cerr << "GxsForumThreadWidget::loadMsgData_ReplyMessage() ERROR Wrong number of answers";
			std::cerr << std::endl;
			return;
		}
		replyMessageData(msgs[0]);
	}
	else
	{
		std::cerr << "GxsForumThreadWidget::loadMsgData_ReplyMessage() ERROR Missing Message Data...";
		std::cerr << std::endl;
	}
}

void GxsForumThreadWidget::loadMsgData_ReplyForumMessage(const uint32_t &token)
{
#ifdef DEBUG_FORUMS
    std::cerr << "GxsForumThreadWidget::loadMsgData_ReplyMessage()";
    std::cerr << std::endl;
#endif

	std::vector<RsGxsForumMsg> msgs;
	if (rsGxsForums->getMsgData(token, msgs))
	{
		if (msgs.size() != 1)
		{
			std::cerr << "GxsForumThreadWidget::loadMsgData_ReplyMessage() ERROR Wrong number of answers";
			std::cerr << std::endl;
			return;
		}

		replyForumMessageData(msgs[0]);
	}
	else
	{
		std::cerr << "GxsForumThreadWidget::loadMsgData_ReplyMessage() ERROR Missing Message Data...";
		std::cerr << std::endl;
	}
}

void GxsForumThreadWidget::loadMsgData_BanAuthor(const uint32_t &token)
{
#ifdef DEBUG_FORUMS
    std::cerr << "GxsForumThreadWidget::loadMsgData_BanAuthor()";
    std::cerr << std::endl;
#endif

	std::vector<RsGxsForumMsg> msgs;
	if (rsGxsForums->getMsgData(token, msgs))
	{
		if (msgs.size() != 1)
		{
			std::cerr << "GxsForumThreadWidget::loadMsgData_ReplyMessage() ERROR Wrong number of answers";
			std::cerr << std::endl;
			return;
		}

		std::cerr << "  banning author id " << msgs[0].mMeta.mAuthorId << std::endl;
	rsReputations->setOwnOpinion(msgs[0].mMeta.mAuthorId,RsReputations::OPINION_NEGATIVE) ;
	}
	else
	{
		std::cerr << "GxsForumThreadWidget::loadMsgData_ReplyMessage() ERROR Missing Message Data...";
		std::cerr << std::endl;
	}
    updateDisplay(true) ;
    
    // we should also update the icons so that they changed to the icon for banned peers.
    
    std::cerr << __PRETTY_FUNCTION__ << ": need to implement the update of GxsTreeWidgetItems icons too." << std::endl;
}
/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/

void GxsForumThreadWidget::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
#ifdef DEBUG_FORUMS
    std::cerr << "GxsForumThreadWidget::loadRequest() UserType: " << req.mUserType;
    std::cerr << std::endl;
#endif

	if (queue == mTokenQueue)
	{
		/* now switch on req */
		if (req.mUserType == mTokenTypeGroupData) {
			loadGroupData(req.mToken);
			return;
		}

		if (req.mUserType == mTokenTypeMessageData) {
			loadMessageData(req.mToken);
			return;
		}

		if (req.mUserType == mTokenTypeReplyMessage) {
			loadMsgData_ReplyMessage(req.mToken);
			return;
		}
		
		if (req.mUserType == mTokenTypeReplyForumMessage) {
			loadMsgData_ReplyForumMessage(req.mToken);
			return;
		}
        
		if (req.mUserType == mTokenTypeBanAuthor) {
			loadMsgData_BanAuthor(req.mToken);
			return;
		}
	}

	GxsMessageFrameWidget::loadRequest(queue, req);
}
