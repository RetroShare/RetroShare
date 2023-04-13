/*******************************************************************************
 * plugins/FeedReader/gui/FeedReaderDialog.cpp                                 *
 *                                                                             *
 * Copyright (C) 2012 by Thunder                                               *
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

#include <QKeyEvent>
#include <QMenu>
#include <QInputDialog>
#include <QPainter>
#include <QMessageBox>

#include "FeedReaderDialog.h"
#include "FeedReaderMessageWidget.h"
#include "ui_FeedReaderDialog.h"
#include "FeedReaderNotify.h"
#include "FeedReaderConfig.h"
#include "AddFeedDialog.h"
#include "FeedReaderStringDefs.h"
#include "gui/common/RSTreeWidgetItem.h"
#include "gui/settings/rsharesettings.h"
#include "gui/notifyqt.h"
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
#define ROLE_FEED_NEW         Qt::UserRole + 4
#define ROLE_FEED_NAME        Qt::UserRole + 5
#define ROLE_FEED_WORKSTATE   Qt::UserRole + 6
#define ROLE_FEED_LOADING     Qt::UserRole + 7
#define ROLE_FEED_ICON        Qt::UserRole + 8
#define ROLE_FEED_ERROR       Qt::UserRole + 9
#define ROLE_FEED_DEACTIVATED Qt::UserRole + 10

FeedReaderDialog::FeedReaderDialog(RsFeedReader *feedReader, FeedReaderNotify *notify, QWidget *parent)
	: MainPage(parent), mFeedReader(feedReader), mNotify(notify), ui(new Ui::FeedReaderDialog)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui->setupUi(this);

	mProcessSettings = false;
	mOpenFeedIds = NULL;
	mMessageWidget = NULL;

	connect(mNotify, &FeedReaderNotify::feedChanged, this, &FeedReaderDialog::feedChanged, Qt::QueuedConnection);

	connect(NotifyQt::getInstance(), SIGNAL(settingsChanged()), this, SLOT(settingsChanged()));

	/* connect signals */
	connect(ui->feedTreeWidget, SIGNAL(itemActivated(QTreeWidgetItem*,int)), this, SLOT(feedTreeItemActivated(QTreeWidgetItem*)));
	if (!style()->styleHint(QStyle::SH_ItemView_ActivateItemOnSingleClick, NULL, this)) {
		// need signal itemClicked too
		connect(ui->feedTreeWidget, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(feedTreeItemActivated(QTreeWidgetItem*)));
	}
	connect(ui->feedTreeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(feedTreeCustomPopupMenu(QPoint)));
	connect(ui->feedTreeWidget, SIGNAL(signalMouseMiddleButtonClicked(QTreeWidgetItem*)), this, SLOT(feedTreeMiddleButtonClicked(QTreeWidgetItem*)));

	connect(ui->messageTabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(messageTabCloseRequested(int)));
	connect(ui->messageTabWidget, SIGNAL(currentChanged(int)), this, SLOT(messageTabChanged(int)));

	connect(ui->feedAddButton, SIGNAL(clicked()), this, SLOT(newFeed()));
	connect(ui->feedProcessButton, SIGNAL(clicked()), this, SLOT(processFeed()));

	connect(ui->feedTreeWidget, SIGNAL(feedReparent(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(feedTreeReparent(QTreeWidgetItem*,QTreeWidgetItem*)));

	mFeedCompareRole = new RSTreeWidgetItemCompareRole;
	mFeedCompareRole->setRole(COLUMN_FEED_NAME, ROLE_FEED_SORT);

	/* enable drag and drop */
	ui->feedTreeWidget->setAcceptDrops(true);
	ui->feedTreeWidget->setDragEnabled(true);
	ui->feedTreeWidget->setDragDropMode(QAbstractItemView::InternalMove);

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
	ui->splitter->setStretchFactor(0, 0);
	ui->splitter->setStretchFactor(1, 1);

	QList<int> sizes;
	sizes << 300 << width(); // Qt calculates the right sizes
	ui->splitter->setSizes(sizes);

	/* load settings */
	processSettings(true);

	/* initialize feed list */
	ui->feedTreeWidget->sortItems(COLUMN_FEED_NAME, Qt::AscendingOrder);

	ui->feedTreeWidget->installEventFilter(this);

	settingsChanged();

	feedTreeItemActivated(NULL);
}

FeedReaderDialog::~FeedReaderDialog()
{
	/* save settings */
	processSettings(false);

	delete(mFeedCompareRole);
	delete(ui);

	if (mOpenFeedIds) {
		delete mOpenFeedIds;
		mOpenFeedIds = NULL;
	}
}

UserNotify *FeedReaderDialog::createUserNotify(QObject *parent)
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
			addFeedToExpand(Settings->value("open").toUInt());
		}
		Settings->endArray();
	} else {
		// save settings

		// state of splitter
		Settings->setValue("Splitter", ui->splitter->saveState());

		// open groups
		Settings->beginWriteArray("Feeds");
		int arrayIndex = 0;
		QList<uint32_t> expandedFeedIds;
		getExpandedFeedIds(expandedFeedIds);
		foreach (uint32_t feedId, expandedFeedIds) {
			Settings->setArrayIndex(arrayIndex++);
			Settings->setValue("open", feedId);
		}
		Settings->endArray();
	}

	Settings->endGroup();
	mProcessSettings = false;
}

void FeedReaderDialog::settingsChanged()
{
	if (FeedReaderSetting_OpenAllInNewTab()) {
		if (mMessageWidget) {
			delete(mMessageWidget);
			mMessageWidget = NULL;
		}
	} else {
		if (!mMessageWidget) {
			mMessageWidget = createMessageWidget(0);
			// remove close button of the the first tab
			ui->messageTabWidget->hideCloseButton(ui->messageTabWidget->indexOf(mMessageWidget));
		}
	}
}

void FeedReaderDialog::addFeedToExpand(uint32_t feedId)
{
	if (mOpenFeedIds == NULL) {
		mOpenFeedIds = new QList<uint32_t>;
	}
	if (mOpenFeedIds->contains(feedId)) {
		return;
	}
	mOpenFeedIds->push_back(feedId);
}

void FeedReaderDialog::getExpandedFeedIds(QList<uint32_t> &feedIds)
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
		uint32_t feedId = item->data(COLUMN_FEED_DATA, ROLE_FEED_ID).toUInt();
		if (feedId == 0) {
			continue;
		}
		feedIds.push_back(feedId);
	}
}

void FeedReaderDialog::showEvent(QShowEvent */*event*/)
{
	updateFeeds(0, mRootItem);
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

uint32_t FeedReaderDialog::currentFeedId()
{
	QTreeWidgetItem *item = ui->feedTreeWidget->currentItem();
	if (!item) {
		return 0;
	}

	return item->data(COLUMN_FEED_DATA, ROLE_FEED_ID).toUInt();
}

void FeedReaderDialog::setCurrentFeedId(uint32_t feedId)
{
	if (feedId == 0) {
		return;
	}

	QTreeWidgetItemIterator it(ui->feedTreeWidget);
	QTreeWidgetItem *item;
	while ((item = *it) != NULL) {
		if (item->data(COLUMN_FEED_DATA, ROLE_FEED_ID).toUInt() == feedId) {
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
	uint32_t feedId;
	QTreeWidgetItem *item = ui->feedTreeWidget->currentItem();
	if (item) {
		folder = item->data(COLUMN_FEED_DATA, ROLE_FEED_FOLDER).toBool();
		feedId = item->data(COLUMN_FEED_DATA, ROLE_FEED_ID).toUInt();
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

	if (!FeedReaderSetting_OpenAllInNewTab()) {
		contextMnu.addSeparator();

		action = contextMnu.addAction(QIcon(""), tr("Open in new tab"), this, SLOT(openInNewTab()));
		if (!item || folder || feedMessageWidget(feedId)) {
			action->setEnabled(false);
		}
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

void FeedReaderDialog::updateFeeds(uint32_t parentId, QTreeWidgetItem *parentItem)
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
		uint32_t feedId = item->data(COLUMN_FEED_DATA, ROLE_FEED_ID).toUInt();

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
		} else {
			/* disable drop */
			item->setFlags(item->flags() & ~Qt::ItemIsDropEnabled);
		}
	}

	if (mOpenFeedIds && mOpenFeedIds->empty()) {
		delete mOpenFeedIds;
		mOpenFeedIds = NULL;
	}

	calculateFeedItems();
}

void FeedReaderDialog::calculateFeedItem(QTreeWidgetItem *item, uint32_t &unreadCount, uint32_t &newCount, bool &loading)
{
	uint32_t unreadCountItem = 0;
	uint32_t newCountItem = 0;
	bool loadingItem = false;

	if (item->data(COLUMN_FEED_DATA, ROLE_FEED_FOLDER).toBool()) {
		int childCount = item->childCount();
		for (int index = 0; index < childCount; ++index) {
			calculateFeedItem(item->child(index), unreadCountItem, newCountItem, loadingItem);
		}
	} else {
		unreadCountItem = item->data(COLUMN_FEED_DATA, ROLE_FEED_UNREAD).toUInt();
		newCountItem = item->data(COLUMN_FEED_DATA, ROLE_FEED_NEW).toUInt();
		loadingItem = item->data(COLUMN_FEED_DATA, ROLE_FEED_LOADING).toBool();
	}

	unreadCount += unreadCountItem;
	newCount += newCountItem;
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

	QColor colorActivated = ui->feedTreeWidget->palette().color(QPalette::Active, QPalette::Text);
	QColor color = ui->feedTreeWidget->palette().color(QPalette::Active, QPalette::Base);
	QColor colorDeactivated;
	colorDeactivated.setRgbF((color.redF() + colorActivated.redF()) / 2, (color.greenF() + colorActivated.greenF()) / 2, (color.blueF() + colorActivated.blueF()) / 2);

	for (int i = 0; i < COLUMN_FEED_COUNT; i++) {
		QFont font = item->font(i);
		font.setBold(unreadCountItem != 0);
		item->setFont(i, font);

		item->setData(COLUMN_FEED_NAME, Qt::ForegroundRole, deactivated ? colorDeactivated : colorActivated);
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
	} else if (newCountItem) {
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

	item->setIcon(COLUMN_FEED_NAME, icon);
}

void FeedReaderDialog::calculateFeedItems()
{
	uint32_t unreadCount = 0;
	uint32_t newCount = 0;
	bool loading = false;

	calculateFeedItem(mRootItem, unreadCount, newCount, loading);
	ui->feedTreeWidget->sortItems(COLUMN_FEED_NAME, Qt::AscendingOrder);
}

QIcon FeedReaderDialog::iconFromFeed(const FeedInfo &feedInfo)
{
	QIcon icon;
	if (feedInfo.flag.folder) {
		/* use folder icon */
		icon = QIcon(":/images/Folder.png");
	} else {
		if (feedInfo.icon.empty()) {
			/* use standard icon */
			icon = QIcon(":/images/Feed.png");
		} else {
			/* use icon from feed */
			QPixmap pixmap;
			if (pixmap.loadFromData(QByteArray::fromBase64(feedInfo.icon.c_str()))) {
				icon = pixmap.scaled(16, 16, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
			}
		}
	}

	return icon;
}

void FeedReaderDialog::updateFeedItem(QTreeWidgetItem *item, const FeedInfo &feedInfo)
{
	item->setData(COLUMN_FEED_DATA, ROLE_FEED_ICON, iconFromFeed(feedInfo));

	QString name = QString::fromUtf8(feedInfo.name.c_str());
	item->setData(COLUMN_FEED_DATA, ROLE_FEED_NAME, name.isEmpty() ? tr("No name") : name);
	item->setData(COLUMN_FEED_DATA, ROLE_FEED_WORKSTATE, FeedReaderStringDefs::workState(feedInfo.workstate));

	uint32_t unreadCount;
	uint32_t newCount;
	mFeedReader->getMessageCount(feedInfo.feedId, NULL, &newCount, &unreadCount);

	item->setData(COLUMN_FEED_NAME, ROLE_FEED_SORT, QString("%1_%2").arg(QString(feedInfo.flag.folder ? "0" : "1"), name));
	item->setData(COLUMN_FEED_DATA, ROLE_FEED_UNREAD, unreadCount);
	item->setData(COLUMN_FEED_DATA, ROLE_FEED_NEW, newCount);
	item->setData(COLUMN_FEED_DATA, ROLE_FEED_LOADING, feedInfo.workstate != FeedInfo::WAITING);
	item->setData(COLUMN_FEED_DATA, ROLE_FEED_ID, feedInfo.feedId);
	item->setData(COLUMN_FEED_DATA, ROLE_FEED_FOLDER, feedInfo.flag.folder);
	item->setData(COLUMN_FEED_DATA, ROLE_FEED_DEACTIVATED, feedInfo.flag.deactivated);
	item->setData(COLUMN_FEED_DATA, ROLE_FEED_ERROR, (bool) (feedInfo.errorState != RS_FEED_ERRORSTATE_OK));
	item->setToolTip(COLUMN_FEED_NAME, (feedInfo.errorState != RS_FEED_ERRORSTATE_OK) ? FeedReaderStringDefs::errorString(feedInfo) : "");
}

void FeedReaderDialog::feedChanged(uint32_t feedId, int type)
{
	if (!isVisible()) {
		/* complete update in showEvent */
		return;
	}

	if (feedId == 0) {
		return;
	}

	FeedInfo feedInfo;
	if (type != NOTIFY_TYPE_DEL) {
		if (!mFeedReader->getFeedInfo(feedId, feedInfo)) {
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
			if (item->data(COLUMN_FEED_DATA, ROLE_FEED_ID).toUInt() == feedId) {
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
		QTreeWidgetItemIterator it(ui->feedTreeWidget);
		QTreeWidgetItem *itemParent;
		while ((itemParent = *it) != NULL) {
			if (itemParent->data(COLUMN_FEED_DATA, ROLE_FEED_ID).toUInt() == feedInfo.parentId) {
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

FeedReaderMessageWidget *FeedReaderDialog::feedMessageWidget(uint32_t id)
{
	int tabCount = ui->messageTabWidget->count();
	for (int index = 0; index < tabCount; ++index) {
		FeedReaderMessageWidget *childWidget = dynamic_cast<FeedReaderMessageWidget*>(ui->messageTabWidget->widget(index));
		if (mMessageWidget && childWidget == mMessageWidget) {
			continue;
		}
		if (childWidget && childWidget->feedId() == id) {
			return childWidget;
		}
	}

	return NULL;
}

FeedReaderMessageWidget *FeedReaderDialog::createMessageWidget(uint32_t feedId)
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

	uint32_t feedId = item->data(COLUMN_FEED_DATA, ROLE_FEED_ID).toUInt();
	/* search exisiting tab */
	FeedReaderMessageWidget *messageWidget = feedMessageWidget(feedId);
	if (!messageWidget) {
		if (mMessageWidget) {
			/* not found, use standard tab */
			messageWidget = mMessageWidget;
			messageWidget->setFeedId(feedId);
		} else {
			/* create new tab */
			messageWidget = createMessageWidget(feedId);
		}
	}

	ui->messageTabWidget->setCurrentWidget(messageWidget);
}

void FeedReaderDialog::feedTreeMiddleButtonClicked(QTreeWidgetItem *item)
{
	if (!item) {
		return;
	}

	openFeedInNewTab(item->data(COLUMN_FEED_DATA, ROLE_FEED_ID).toUInt());
}

void FeedReaderDialog::openInNewTab()
{
	openFeedInNewTab(currentFeedId());
}

void FeedReaderDialog::openFeedInNewTab(uint32_t feedId)
{
	if (feedId == 0) {
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

	if (messageWidget != mMessageWidget && messageWidget->feedId() == 0) {
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
		uint32_t feedId;
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
	uint32_t feedId = currentFeedId();
	if (feedId == 0) {
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
	uint32_t feedId = currentFeedId();
	if (feedId == 0) {
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
	uint32_t feedId = currentFeedId();
	if (feedId == 0) {
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
	uint32_t feedId = currentFeedId();
	/* empty feed id process all feeds */

	mFeedReader->processFeed(feedId);
}

void FeedReaderDialog::feedTreeReparent(QTreeWidgetItem *item, QTreeWidgetItem *newParent)
{
	if (!item || ! newParent) {
		return;
	}

	uint32_t feedId = item->data(COLUMN_FEED_DATA, ROLE_FEED_ID).toUInt();
	uint32_t parentId = newParent->data(COLUMN_FEED_DATA, ROLE_FEED_ID).toUInt();

	if (feedId == 0) {
		return;
	}

	RsFeedAddResult result = mFeedReader->setParent(feedId, parentId);
	if (FeedReaderStringDefs::showError(this, result, tr("Move feed"), tr("Cannot move feed."))) {
		return;
	}

	bool expanded = item->isExpanded();
	item->parent()->removeChild(item);
	newParent->addChild(item);
	item->setExpanded(expanded);
	newParent->setExpanded(true);

	calculateFeedItems();
}
