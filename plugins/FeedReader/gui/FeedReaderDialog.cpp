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

#include <QKeyEvent>
#include <QMenu>
#include <QInputDialog>
#include <QPainter>
#include <QMessageBox>

#include "FeedReaderDialog.h"
#include "FeedReaderMessageWidget.h"
#include "ui_FeedReaderDialog.h"
#include "FeedReaderNotify.h"
#include "AddFeedDialog.h"
#include "FeedReaderStringDefs.h"
#include "gui/common/RSTreeWidgetItem.h"
#include "gui/settings/rsharesettings.h"
#include "FeedReaderUserNotify.h"

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

FeedReaderDialog::FeedReaderDialog(RsFeedReader *feedReader, QWidget *parent)
	: MainPage(parent), mFeedReader(feedReader), ui(new Ui::FeedReaderDialog)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui->setupUi(this);

	mProcessSettings = false;
	mOpenFeedIds = NULL;

	mNotify = new FeedReaderNotify();
	mFeedReader->setNotify(mNotify);
	connect(mNotify, SIGNAL(notifyFeedChanged(QString,int)), this, SLOT(feedChanged(QString,int)));
	connect(mNotify, SIGNAL(notifyMsgChanged(QString,QString,int)), this, SLOT(msgChanged(QString,QString,int)));

	/* connect signals */
	connect(ui->feedTreeWidget, SIGNAL(itemActivated(QTreeWidgetItem*,int)), this, SLOT(feedTreeItemActivated(QTreeWidgetItem*)));
	if (!style()->styleHint(QStyle::SH_ItemView_ActivateItemOnSingleClick, NULL, this)) {
		// need signal itemClicked too
		connect(ui->feedTreeWidget, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(feedTreeItemActivated(QTreeWidgetItem*)));
	}
	connect(ui->feedTreeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(feedTreeCustomPopupMenu(QPoint)));

	connect(ui->messageTabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(messageTabCloseRequested(int)));
	connect(ui->messageTabWidget, SIGNAL(currentChanged(int)), this, SLOT(messageTabChanged(int)));

	connect(ui->feedAddButton, SIGNAL(clicked()), this, SLOT(newFeed()));
	connect(ui->feedProcessButton, SIGNAL(clicked()), this, SLOT(processFeed()));

	mFeedCompareRole = new RSTreeWidgetItemCompareRole;
	mFeedCompareRole->setRole(COLUMN_FEED_NAME, ROLE_FEED_SORT);

	/* initialize root item */
	mRootItem = new QTreeWidgetItem(ui->feedTreeWidget);
	QString name = tr("Message Folders");
	mRootItem->setText(COLUMN_FEED_NAME, name);
	mRootItem->setIcon(COLUMN_FEED_NAME, QIcon(":/images/Root.png"));
	mRootItem->setData(COLUMN_FEED_DATA, ROLE_FEED_NAME, name);
	mRootItem->setData(COLUMN_FEED_DATA, ROLE_FEED_FOLDER, true);
	mRootItem->setData(COLUMN_FEED_DATA, ROLE_FEED_ICON, QIcon(":/images/Root.png"));
	mRootItem->setExpanded(true);

    /* set initial size the splitter */
    QList<int> sizes;
    sizes << 300 << width(); // Qt calculates the right sizes
    ui->splitter->setSizes(sizes);

	/* load settings */
	processSettings(true);

	/* initialize feed list */
	ui->feedTreeWidget->sortItems(COLUMN_FEED_NAME, Qt::AscendingOrder);

	ui->feedTreeWidget->installEventFilter(this);

	mMessageWidget = createMessageWidget("");
	// remove close button of the the first tab
	ui->messageTabWidget->hideCloseButton(ui->messageTabWidget->indexOf(mMessageWidget));

	feedTreeItemActivated(NULL);
}

FeedReaderDialog::~FeedReaderDialog()
{
	/* save settings */
	processSettings(false);

	delete(mFeedCompareRole);
	delete(ui);

	mFeedReader->setNotify(NULL);
	delete(mNotify);

	if (mOpenFeedIds) {
		delete mOpenFeedIds;
		mOpenFeedIds = NULL;
	}
}

UserNotify *FeedReaderDialog::getUserNotify(QObject *parent)
{
	return new FeedReaderUserNotify(this, mFeedReader, mNotify, parent);
}

void FeedReaderDialog::processSettings(bool load)
{
	mProcessSettings = true;
	Settings->beginGroup(QString("FeedReaderDialog"));

	if (load) {
		// load settings

		// state of splitter
		ui->splitter->restoreState(Settings->value("Splitter").toByteArray());

		// open feeds
		int arrayIndex = Settings->beginReadArray("Feeds");
		for (int index = 0; index < arrayIndex; index++) {
			Settings->setArrayIndex(index);
			addFeedToExpand(Settings->value("open").toString().toStdString());
		}
		Settings->endArray();
	} else {
		// save settings

		// state of splitter
		Settings->setValue("Splitter", ui->splitter->saveState());

		// open groups
		Settings->beginWriteArray("Feeds");
		int arrayIndex = 0;
		QList<std::string> expandedFeedIds;
		getExpandedFeedIds(expandedFeedIds);
		foreach (std::string feedId, expandedFeedIds) {
			Settings->setArrayIndex(arrayIndex++);
			Settings->setValue("open", QString::fromStdString(feedId));
		}
		Settings->endArray();
	}

	Settings->endGroup();
	mProcessSettings = false;
}

void FeedReaderDialog::addFeedToExpand(const std::string &feedId)
{
	if (mOpenFeedIds == NULL) {
		mOpenFeedIds = new QList<std::string>;
	}
	if (mOpenFeedIds->contains(feedId)) {
		return;
	}
	mOpenFeedIds->push_back(feedId);
}

void FeedReaderDialog::getExpandedFeedIds(QList<std::string> &feedIds)
{
	QTreeWidgetItemIterator it(ui->feedTreeWidget);
	QTreeWidgetItem *item;
	while ((item = *it) != NULL) {
		++it;
		if (!item->isExpanded()) {
			continue;
		}
		if (!item->data(COLUMN_FEED_DATA, ROLE_FEED_FOLDER).toBool()) {
			continue;
		}
		std::string feedId = item->data(COLUMN_FEED_DATA, ROLE_FEED_ID).toString().toStdString();
		if (feedId.empty()) {
			continue;
		}
		feedIds.push_back(feedId);
	}
}

void FeedReaderDialog::showEvent(QShowEvent */*event*/)
{
	updateFeeds("", mRootItem);
}

bool FeedReaderDialog::eventFilter(QObject *obj, QEvent *event)
{
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

void FeedReaderDialog::setCurrentFeedId(const std::string &feedId)
{
	if (feedId.empty()) {
		return;
	}

	QTreeWidgetItemIterator it(ui->feedTreeWidget);
	QTreeWidgetItem *item;
	while ((item = *it) != NULL) {
		if (item->data(COLUMN_FEED_DATA, ROLE_FEED_ID).toString().toStdString() == feedId) {
			ui->feedTreeWidget->setCurrentItem(item);
			break;
		}
		++it;
	}
}

void FeedReaderDialog::feedTreeCustomPopupMenu(QPoint /*point*/)
{
	QMenu contextMnu(this);

	bool folder = false;
	std::string feedId;
	QTreeWidgetItem *item = ui->feedTreeWidget->currentItem();
	if (item) {
		folder = item->data(COLUMN_FEED_DATA, ROLE_FEED_FOLDER).toBool();
		feedId = item->data(COLUMN_FEED_DATA, ROLE_FEED_ID).toString().toStdString();
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

	action = contextMnu.addAction(QIcon(""), tr("Open in new tab"), this, SLOT(openInNewTab()));
	if (!item || folder || feedMessageWidget(feedId)) {
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

			if (mOpenFeedIds) {
				int index = mOpenFeedIds->indexOf(feedIt->feedId);
				if (index >= 0) {
					item->setExpanded(true);
					mOpenFeedIds->removeAt(index);
				}
			}
		}
	}

	if (mOpenFeedIds && mOpenFeedIds->empty()) {
		delete mOpenFeedIds;
		mOpenFeedIds = NULL;
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

FeedReaderMessageWidget *FeedReaderDialog::feedMessageWidget(const std::string &id)
{
	int tabCount = ui->messageTabWidget->count();
	for (int index = 0; index < tabCount; ++index) {
		FeedReaderMessageWidget *childWidget = dynamic_cast<FeedReaderMessageWidget*>(ui->messageTabWidget->widget(index));
		if (childWidget == mMessageWidget) {
			continue;
		}
		if (childWidget && childWidget->feedId() == id) {
			return childWidget;
		}
	}

	return NULL;
}

FeedReaderMessageWidget *FeedReaderDialog::createMessageWidget(const std::string &feedId)
{
	FeedReaderMessageWidget *messageWidget = new FeedReaderMessageWidget(feedId, mFeedReader, mNotify);
	int index = ui->messageTabWidget->addTab(messageWidget, messageWidget->feedName(true));
	ui->messageTabWidget->setTabIcon(index, messageWidget->feedIcon());
	connect(messageWidget, SIGNAL(feedMessageChanged(QWidget*)), this, SLOT(messageTabInfoChanged(QWidget*)));

	return messageWidget;
}

void FeedReaderDialog::feedTreeItemActivated(QTreeWidgetItem *item)
{
	if (!item) {
		ui->feedAddButton->setEnabled(false);
		ui->feedProcessButton->setEnabled(false);
		return;
	}

	ui->feedProcessButton->setEnabled(true);

	if (item->data(COLUMN_FEED_DATA, ROLE_FEED_FOLDER).toBool()) {
		ui->feedAddButton->setEnabled(true);
		return;
	}

	ui->feedAddButton->setEnabled(false);

	std::string feedId = item->data(COLUMN_FEED_DATA, ROLE_FEED_ID).toString().toStdString();
	/* search exisiting tab */
	FeedReaderMessageWidget *messageWidget = feedMessageWidget(feedId);
	if (!messageWidget) {
		/* not found, use standard tab */
		messageWidget = mMessageWidget;
		messageWidget->setFeedId(feedId);
	}

	ui->messageTabWidget->setCurrentWidget(messageWidget);
}

void FeedReaderDialog::openInNewTab()
{
	std::string feedId = currentFeedId();
	if (feedId.empty()) {
		return;
	}

	/* search exisiting tab */
	FeedReaderMessageWidget *messageWidget = feedMessageWidget(feedId);
	if (!messageWidget) {
		/* not found, create new tab */
		messageWidget = createMessageWidget(feedId);
	}

	ui->messageTabWidget->setCurrentWidget(messageWidget);
}

void FeedReaderDialog::messageTabCloseRequested(int index)
{
	FeedReaderMessageWidget *messageWidget = dynamic_cast<FeedReaderMessageWidget*>(ui->messageTabWidget->widget(index));
	if (!messageWidget) {
		return;
	}

	if (messageWidget == mMessageWidget) {
		return;
	}

	delete(messageWidget);
}

void FeedReaderDialog::messageTabChanged(int index)
{
	FeedReaderMessageWidget *messageWidget = dynamic_cast<FeedReaderMessageWidget*>(ui->messageTabWidget->widget(index));
	if (!messageWidget) {
		return;
	}

	setCurrentFeedId(messageWidget->feedId());
}

void FeedReaderDialog::messageTabInfoChanged(QWidget *widget)
{
	int index = ui->messageTabWidget->indexOf(widget);
	if (index < 0) {
		return;
	}

	FeedReaderMessageWidget *messageWidget = dynamic_cast<FeedReaderMessageWidget*>(ui->messageTabWidget->widget(index));
	if (!messageWidget) {
		return;
	}

	if (messageWidget != mMessageWidget && messageWidget->feedId().empty()) {
		messageWidget->deleteLater();
		return;
	}

	ui->messageTabWidget->setTabText(index, messageWidget->feedName(true));
	ui->messageTabWidget->setTabIcon(index, messageWidget->feedIcon());
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
