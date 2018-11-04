/*******************************************************************************
 * plugins/FeedReader/gui/FeedReaderMessageWidget.cpp                          *
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

#include <QDateTime>
#include <QMenu>
#include <QKeyEvent>
#include <QClipboard>
#include <QDesktopServices>
#include <QTimer>
#include <QPainter>

#include "FeedReaderMessageWidget.h"
#include "ui_FeedReaderMessageWidget.h"
#include "FeedReaderNotify.h"
#include "FeedReaderConfig.h"
#include "FeedReaderDialog.h"

#include "gui/common/RSTreeWidgetItem.h"
#include "gui/settings/rsharesettings.h"
#include "util/HandleRichText.h"
#include "util/QtVersion.h"

#include "retroshare/rsiface.h"

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

FeedReaderMessageWidget::FeedReaderMessageWidget(const std::string &feedId, RsFeedReader *feedReader, FeedReaderNotify *notify, QWidget *parent) :
	QWidget(parent), mFeedReader(feedReader), mNotify(notify), ui(new Ui::FeedReaderMessageWidget)
{
	ui->setupUi(this);

	mProcessSettings = false;
	mUnreadCount = 0;
	mNewCount = 0;

	/* connect signals */
	connect(mNotify, SIGNAL(feedChanged(QString,int)), this, SLOT(feedChanged(QString,int)));
	connect(mNotify, SIGNAL(msgChanged(QString,QString,int)), this, SLOT(msgChanged(QString,QString,int)));

	connect(ui->msgTreeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(msgItemChanged()));
	connect(ui->msgTreeWidget, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(msgItemClicked(QTreeWidgetItem*,int)));

	connect(ui->msgTreeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(msgTreeCustomPopupMenu(QPoint)));

	connect(ui->filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterItems(QString)));
	connect(ui->filterLineEdit, SIGNAL(filterChanged(int)), this, SLOT(filterColumnChanged(int)));

	connect(ui->linkButton, SIGNAL(clicked()), this, SLOT(openLinkMsg()));
	connect(ui->expandButton, SIGNAL(clicked()), this, SLOT(toggleMsgText()));

	connect(ui->msgReadButton, SIGNAL(clicked()), this, SLOT(markAsReadMsg()));
	connect(ui->msgUnreadButton, SIGNAL(clicked()), this, SLOT(markAsUnreadMsg()));
	connect(ui->msgReadAllButton, SIGNAL(clicked()), this, SLOT(markAllAsReadMsg()));
	connect(ui->msgRemoveButton, SIGNAL(clicked()), this, SLOT(removeMsg()));
	connect(ui->feedProcessButton, SIGNAL(clicked()), this, SLOT(processFeed()));

	// create timer for navigation
	mTimer = new QTimer(this);
	mTimer->setInterval(300);
	mTimer->setSingleShot(true);
	connect(mTimer, SIGNAL(timeout()), this, SLOT(updateCurrentMessage()));

	mMsgCompareRole = new RSTreeWidgetItemCompareRole;
	mMsgCompareRole->setRole(COLUMN_MSG_TITLE, ROLE_MSG_SORT);
	mMsgCompareRole->setRole(COLUMN_MSG_READ, ROLE_MSG_SORT);
	mMsgCompareRole->setRole(COLUMN_MSG_PUBDATE, ROLE_MSG_SORT);
	mMsgCompareRole->setRole(COLUMN_MSG_AUTHOR, ROLE_MSG_SORT);

	/* initialize msg list */
	ui->msgTreeWidget->sortItems(COLUMN_MSG_PUBDATE, Qt::DescendingOrder);

	/* set header resize modes and initial section sizes */
	QHeaderView *header = ui->msgTreeWidget->header();
	QHeaderView_setSectionResizeModeColumn(header, COLUMN_MSG_TITLE, QHeaderView::Interactive);
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
	QHeaderView_setSectionResizeModeColumn(header, COLUMN_MSG_READ, QHeaderView::Fixed);

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

	setFeedId(feedId);
}

FeedReaderMessageWidget::~FeedReaderMessageWidget()
{
	// stop and delete timer
	mTimer->stop();
	delete(mTimer);

	/* save settings */
	processSettings(false);

	delete(mMsgCompareRole);
	delete ui;
}

void FeedReaderMessageWidget::processSettings(bool load)
{
	mProcessSettings = true;
	Settings->beginGroup(QString("FeedReaderDialog"));

	QHeaderView *header = ui->msgTreeWidget->header ();

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
		ui->msgSplitter->restoreState(Settings->value("msgSplitter").toByteArray());
	} else {
		// save settings

		// state of thread tree
		Settings->setValue("msgTree", header->saveState());

		// state of splitter
		Settings->setValue("msgSplitter", ui->msgSplitter->saveState());
	}

	Settings->endGroup();
	mProcessSettings = false;
}

void FeedReaderMessageWidget::showEvent(QShowEvent */*event*/)
{
	updateMsgs();
}

bool FeedReaderMessageWidget::eventFilter(QObject *obj, QEvent *event)
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
	/* pass the event on to the parent class */
	return QWidget::eventFilter(obj, event);
}

void FeedReaderMessageWidget::setFeedId(const std::string &feedId)
{
	if (mFeedId == feedId) {
		if (!feedId.empty()) {
			return;
		}
	}

	mFeedId = feedId;

	ui->feedProcessButton->setEnabled(!mFeedId.empty());

	if (!mFeedId.empty()) {
		if (mFeedReader->getFeedInfo(mFeedId, mFeedInfo)) {
			mFeedReader->getMessageCount(mFeedId, NULL, &mNewCount, &mUnreadCount);
		} else {
			mFeedId.clear();
			mFeedInfo = FeedInfo();
		}
	} else {
		mFeedInfo = FeedInfo();
	}

	if (mFeedId.empty()) {
		ui->msgReadAllButton->setEnabled(false);
		ui->msgTreeWidget->setPlaceholderText("");
	} else {
		if (mFeedInfo.flag.forum) {
			ui->msgReadAllButton->setEnabled(false);
			ui->msgTreeWidget->setPlaceholderText(tr("The messages will be added to the forum"));
		} else {
			ui->msgReadAllButton->setEnabled(true);
			ui->msgTreeWidget->setPlaceholderText("");
		}
	}

	updateMsgs();
	updateCurrentMessage();

	emit feedMessageChanged(this);
}

QString FeedReaderMessageWidget::feedName(bool withUnreadCount)
{
	QString name = mFeedInfo.name.empty() ? tr("No name") : QString::fromUtf8(mFeedInfo.name.c_str());

	if (withUnreadCount && mUnreadCount) {
		name += QString(" (%1)").arg(mUnreadCount);
	}

	return name;
}

QIcon FeedReaderMessageWidget::feedIcon()
{
	QIcon icon = FeedReaderDialog::iconFromFeed(mFeedInfo);

	if (mFeedInfo.flag.deactivated) {
		/* create disabled icon */
		icon = icon.pixmap(QSize(16, 16), QIcon::Disabled);
	}

	if (!mFeedId.empty()) {
		QImage overlayIcon;

		if (mFeedInfo.workstate != FeedInfo::WAITING) {
			/* overlaying icon */
			overlayIcon = QImage(":/images/FeedProcessOverlay.png");
		} else if (mFeedInfo.errorState != RS_FEED_ERRORSTATE_OK) {
			overlayIcon = QImage(":/images/FeedErrorOverlay.png");
		} else if (mNewCount) {
			overlayIcon = QImage(":/images/FeedNewOverlay.png");
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
	}

	return icon;
}

std::string FeedReaderMessageWidget::currentMsgId()
{
	QTreeWidgetItem *item = ui->msgTreeWidget->currentItem();
	if (!item) {
		return "";
	}

	return item->data(COLUMN_MSG_DATA, ROLE_MSG_ID).toString().toStdString();
}

void FeedReaderMessageWidget::msgTreeCustomPopupMenu(QPoint /*point*/)
{
	QMenu contextMnu(this);

	QList<QTreeWidgetItem*> selectedItems = ui->msgTreeWidget->selectedItems();

	QAction *action = contextMnu.addAction(QIcon(""), tr("Mark as read"), this, SLOT(markAsReadMsg()));
	action->setEnabled(!selectedItems.empty());

	action = contextMnu.addAction(QIcon(""), tr("Mark as unread"), this, SLOT(markAsUnreadMsg()));
	action->setEnabled(!selectedItems.empty());

	action = contextMnu.addAction(QIcon(""), tr("Mark all as read"), this, SLOT(markAllAsReadMsg()));
	action->setEnabled(!mFeedId.empty());

	contextMnu.addSeparator();

	action = contextMnu.addAction(QIcon(""), tr("Copy link"), this, SLOT(copySelectedLinksMsg()));
	action->setEnabled(!selectedItems.empty());

	action = contextMnu.addAction(QIcon(""), tr("Remove"), this, SLOT(removeMsg()));
	action->setEnabled(!selectedItems.empty());

	contextMnu.addSeparator();

	action = contextMnu.addAction(QIcon(""), tr("Retransform"), this, SLOT(retransformMsg()));
	action->setEnabled((mFeedInfo.transformationType != RS_FEED_TRANSFORMATION_TYPE_NONE) && !selectedItems.empty());

	contextMnu.exec(QCursor::pos());
}

void FeedReaderMessageWidget::updateMsgs()
{
	if (mFeedId.empty()) {
		ui->msgTreeWidget->clear();
		return;
	}

	std::list<FeedMsgInfo> msgInfos;
	if (!mFeedReader->getFeedMsgList(mFeedId, msgInfos)) {
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

void FeedReaderMessageWidget::calculateMsgIconsAndFonts(QTreeWidgetItem *item)
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

	for (int i = 0; i < COLUMN_MSG_COUNT; i++) {
		QFont font = item->font(i);
		font.setBold(isnew || !read);
		item->setFont(i, font);
	}

	item->setData(COLUMN_MSG_READ, ROLE_MSG_SORT, QString("%1_%2_%3").arg(QString(isnew ? "1" : "0"), QString(read ? "0" : "1"), item->data(COLUMN_MSG_TITLE, ROLE_MSG_SORT).toString()));
}

void FeedReaderMessageWidget::updateMsgItem(QTreeWidgetItem *item, FeedMsgInfo &info)
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

void FeedReaderMessageWidget::feedChanged(const QString &feedId, int type)
{
	if (feedId.isEmpty()) {
		return;
	}

	if (feedId.toStdString() != mFeedId) {
		return;
	}

	if (type == NOTIFY_TYPE_DEL) {
		setFeedId("");
		return;
	}

	if (type == NOTIFY_TYPE_MOD) {
		if (!mFeedReader->getFeedInfo(mFeedId, mFeedInfo)) {
			setFeedId("");
			return;
		}

		emit feedMessageChanged(this);
	}
}

void FeedReaderMessageWidget::msgChanged(const QString &feedId, const QString &msgId, int type)
{
	if (feedId.isEmpty() || msgId.isEmpty()) {
		return;
	}

	if (feedId.toStdString() != mFeedId) {
		return;
	}

	uint32_t unreadCount;
	uint32_t newCount;
	mFeedReader->getMessageCount(mFeedId, NULL, &newCount, &unreadCount);
	if (unreadCount != mUnreadCount || newCount || mNewCount) {
		mUnreadCount = unreadCount;
		mNewCount = newCount;
		emit feedMessageChanged(this);
	}

	if (!isVisible()) {
		/* complete update in showEvent */
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

	if (type == NOTIFY_TYPE_MOD) {
		if (msgId.toStdString() == currentMsgId()) {
			updateCurrentMessage();
		}
	}

	if (type == NOTIFY_TYPE_ADD) {
		QTreeWidgetItem *item = new RSTreeWidgetItem(mMsgCompareRole);
		updateMsgItem(item, msgInfo);
		ui->msgTreeWidget->addTopLevelItem(item);
		filterItem(item);
	}
}

void FeedReaderMessageWidget::msgItemClicked(QTreeWidgetItem *item, int column)
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

	// show current message directly
	updateCurrentMessage();
}

void FeedReaderMessageWidget::msgItemChanged()
{
	mTimer->stop();
	mTimer->start();
}

void FeedReaderMessageWidget::updateCurrentMessage()
{
	mTimer->stop();

#warning FeedReaderMessageWidget.cpp TODO thunder2: show link somewhere

	std::string msgId = currentMsgId();

	if (mFeedId.empty() || msgId.empty()) {
		ui->msgTitle->clear();
//		ui->msgLink->clear();
		ui->msgText->clear();
		ui->linkButton->setEnabled(false);

		ui->msgReadButton->setEnabled(false);
		ui->msgUnreadButton->setEnabled(false);
		ui->msgRemoveButton->setEnabled(false);
		return;
	}

	QTreeWidgetItem *item = ui->msgTreeWidget->currentItem();
	if (!item) {
		/* there is something wrong */
		ui->msgTitle->clear();
//		ui->msgLink->clear();
		ui->msgText->clear();
		ui->linkButton->setEnabled(false);

		ui->msgReadButton->setEnabled(false);
		ui->msgUnreadButton->setEnabled(false);
		ui->msgRemoveButton->setEnabled(false);
		return;
	}

	ui->msgReadButton->setEnabled(true);
	ui->msgUnreadButton->setEnabled(true);
	ui->msgRemoveButton->setEnabled(true);

	/* get msg */
	FeedMsgInfo msgInfo;
	if (!mFeedReader->getMsgInfo(mFeedId, msgId, msgInfo)) {
		ui->msgTitle->clear();
//		ui->msgLink->clear();
		ui->msgText->clear();
		ui->linkButton->setEnabled(false);
		return;
	}

	bool setToReadOnActive = FeedReaderSetting_SetMsgToReadOnActivate();
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

	QString msgTxt = RsHtml().formatText(ui->msgText->document(), QString::fromUtf8((msgInfo.descriptionTransformed.empty() ? msgInfo.description : msgInfo.descriptionTransformed).c_str()), RSHTML_FORMATTEXT_EMBED_LINKS);

	ui->msgText->setHtml(msgTxt);
	ui->msgTitle->setText(QString::fromUtf8(msgInfo.title.c_str()));

	ui->linkButton->setEnabled(!msgInfo.link.empty());
}

void FeedReaderMessageWidget::setMsgAsReadUnread(QList<QTreeWidgetItem *> &rows, bool read)
{
	QList<QTreeWidgetItem*>::iterator rowIt;

	if (mFeedId.empty()) {
		return;
	}

	for (rowIt = rows.begin(); rowIt != rows.end(); rowIt++) {
		QTreeWidgetItem *item = *rowIt;
		bool rowRead = item->data(COLUMN_MSG_DATA, ROLE_MSG_READ).toBool();
		bool rowNew = item->data(COLUMN_MSG_DATA, ROLE_MSG_NEW).toBool();

		if (rowNew || read != rowRead) {
			std::string msgId = item->data(COLUMN_MSG_DATA, ROLE_MSG_ID).toString().toStdString();
			mFeedReader->setMessageRead(mFeedId, msgId, read);
		}
	}
}

void FeedReaderMessageWidget::filterColumnChanged(int column)
{
	if (mProcessSettings) {
		return;
	}

	filterItems(ui->filterLineEdit->text());

	// save index
	Settings->setValueToGroup("FeedReaderDialog", "filterColumn", column);
}

void FeedReaderMessageWidget::filterItems(const QString& text)
{
	int filterColumn = ui->filterLineEdit->currentFilter();

	int count = ui->msgTreeWidget->topLevelItemCount();
	for (int index = 0; index < count; ++index) {
		filterItem(ui->msgTreeWidget->topLevelItem(index), text, filterColumn);
	}
}

void FeedReaderMessageWidget::filterItem(QTreeWidgetItem *item, const QString &text, int filterColumn)
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

void FeedReaderMessageWidget::filterItem(QTreeWidgetItem *item)
{
	filterItem(item, ui->filterLineEdit->text(), ui->filterLineEdit->currentFilter());
}

void FeedReaderMessageWidget::toggleMsgText()
{
	// save state of button
	Settings->setValueToGroup("FeedReaderDialog", "expandButton", ui->expandButton->isChecked());

	toggleMsgText_internal();
}

void FeedReaderMessageWidget::toggleMsgText_internal()
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

void FeedReaderMessageWidget::markAsReadMsg()
{
	QList<QTreeWidgetItem*> selectedItems = ui->msgTreeWidget->selectedItems();
	setMsgAsReadUnread(selectedItems, true);
}

void FeedReaderMessageWidget::markAsUnreadMsg()
{
	QList<QTreeWidgetItem*> selectedItems = ui->msgTreeWidget->selectedItems();
	setMsgAsReadUnread(selectedItems, false);
}

void FeedReaderMessageWidget::markAllAsReadMsg()
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

void FeedReaderMessageWidget::copySelectedLinksMsg()
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

void FeedReaderMessageWidget::removeMsg()
{
	if (mFeedId.empty()) {
		return;
	}

	QList<QTreeWidgetItem*> selectedItems = ui->msgTreeWidget->selectedItems();
	QList<QTreeWidgetItem*>::iterator it;
	std::list<std::string> msgIds;

	for (it = selectedItems.begin(); it != selectedItems.end(); ++it) {
		msgIds.push_back((*it)->data(COLUMN_MSG_DATA, ROLE_MSG_ID).toString().toStdString());
	}
	mFeedReader->removeMsgs(mFeedId, msgIds);
}

void FeedReaderMessageWidget::retransformMsg()
{
	if (mFeedId.empty()) {
		return;
	}

	QList<QTreeWidgetItem*> selectedItems = ui->msgTreeWidget->selectedItems();
	QList<QTreeWidgetItem*>::iterator it;

	for (it = selectedItems.begin(); it != selectedItems.end(); ++it) {
		mFeedReader->retransformMsg(mFeedId, (*it)->data(COLUMN_MSG_DATA, ROLE_MSG_ID).toString().toStdString());
	}
}

void FeedReaderMessageWidget::processFeed()
{
	if (mFeedId.empty()) {
		return;
	}

	mFeedReader->processFeed(mFeedId);
}

void FeedReaderMessageWidget::copyLinkMsg()
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

void FeedReaderMessageWidget::openLinkMsg()
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
