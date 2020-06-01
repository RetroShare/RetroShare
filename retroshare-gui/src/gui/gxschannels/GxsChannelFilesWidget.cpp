/*******************************************************************************
 * retroshare-gui/src/gui/gxschannels/GxsChannelFilesWidget.cpp                *
 *                                                                             *
 * Copyright 2014 by Retroshare Team   <retroshare.project@gmail.com>          *
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

#include "GxsChannelFilesWidget.h"
#include "ui_GxsChannelFilesWidget.h"
#include "GxsChannelFilesStatusWidget.h"
#include "GxsChannelPostsWidget.h"
#include "gui/feeds/GxsChannelPostItem.h"
#include "gui/common/RSTreeWidgetItem.h"
#include "util/misc.h"
#include "util/DateTime.h"
#include "gui/gxs/GxsFeedItem.h"

#include "retroshare/rsgxschannels.h"

#define COLUMN_FILENAME  0
#define COLUMN_SIZE      1
#define COLUMN_STATUS    2
#define COLUMN_TITLE     3
#define COLUMN_PUBLISHED 4
#define COLUMN_COUNT     5
#define COLUMN_DATA      0

#define ROLE_SORT       Qt::UserRole
#define ROLE_GROUP_ID   Qt::UserRole + 1
#define ROLE_MESSAGE_ID Qt::UserRole + 2
#define ROLE_FILE_HASH  Qt::UserRole + 3
#define ROLE_MSG        Qt::UserRole + 4

Q_DECLARE_METATYPE(Sha1CheckSum)

GxsChannelFilesWidget::GxsChannelFilesWidget(QWidget *parent) :
    QWidget(parent), ui(new Ui::GxsChannelFilesWidget)
{
	ui->setupUi(this);

	mFeedItem = NULL;

	/* Connect signals */
	connect(ui->treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));

	/* Sort */
	mCompareRole = new RSTreeWidgetItemCompareRole;
	mCompareRole->setRole(COLUMN_SIZE, ROLE_SORT);
	mCompareRole->setRole(COLUMN_PUBLISHED, ROLE_SORT);

	/* Filter */
	mFilterType = 0;

	/* Initialize file tree */
	ui->treeWidget->setColumnCount(COLUMN_COUNT);
	QTreeWidgetItem *headerItem = ui->treeWidget->headerItem();
	headerItem->setText(COLUMN_FILENAME, tr("Filename"));
	headerItem->setText(COLUMN_SIZE, tr("Size"));
	headerItem->setText(COLUMN_TITLE, tr("Title"));
	headerItem->setText(COLUMN_PUBLISHED, tr("Published"));
	headerItem->setText(COLUMN_STATUS, tr("Status"));

	ui->treeWidget->setColumnWidth(COLUMN_FILENAME, 400);
	ui->treeWidget->setColumnWidth(COLUMN_SIZE, 80);
	ui->treeWidget->setColumnWidth(COLUMN_PUBLISHED, 150);
}

GxsChannelFilesWidget::paintEvent()
{
    QWidget::paintEvent();

    if(!mLoaded)
    {
    }
}

GxsChannelFilesWidget::~GxsChannelFilesWidget()
{
	delete(mCompareRole);
	delete ui;
}

void GxsChannelFilesWidget::addFiles(const RsGxsChannelPost& post, bool related)
{
	if (related) {
		removeItems(post.mMeta.mGroupId, post.mMeta.mMsgId);
	}

	std::list<RsGxsFile>::const_iterator fileIt;
	for (fileIt = post.mFiles.begin(); fileIt != post.mFiles.end(); ++fileIt) {
		const RsGxsFile &file = *fileIt;

		QTreeWidgetItem *treeItem = new RSTreeWidgetItem(mCompareRole);

		treeItem->setText(COLUMN_FILENAME, QString::fromUtf8(file.mName.c_str()));
		treeItem->setText(COLUMN_SIZE, misc::friendlyUnit(file.mSize));
		treeItem->setData(COLUMN_SIZE, ROLE_SORT, (qulonglong)file.mSize);
		treeItem->setText(COLUMN_TITLE, QString::fromUtf8(post.mMeta.mMsgName.c_str()));
		treeItem->setText(COLUMN_PUBLISHED, DateTime::formatDateTime(post.mMeta.mPublishTs));
		treeItem->setData(COLUMN_PUBLISHED, ROLE_SORT, QDateTime::fromTime_t(post.mMeta.mPublishTs));

		treeItem->setData(COLUMN_DATA, ROLE_GROUP_ID, qVariantFromValue(post.mMeta.mGroupId));
		treeItem->setData(COLUMN_DATA, ROLE_MESSAGE_ID, qVariantFromValue(post.mMeta.mMsgId));
		treeItem->setData(COLUMN_DATA, ROLE_FILE_HASH, qVariantFromValue(file.mHash));
		treeItem->setData(COLUMN_DATA, ROLE_MSG, QString::fromUtf8(post.mMsg.c_str()));
		treeItem->setTextAlignment(COLUMN_SIZE, Qt::AlignRight) ;

		ui->treeWidget->addTopLevelItem(treeItem);
		
		QWidget *statusWidget = new GxsChannelFilesStatusWidget(post.mMeta.mGroupId, post.mMeta.mMsgId, file);
		ui->treeWidget->setItemWidget(treeItem, COLUMN_STATUS, statusWidget);

		filterItem(treeItem);
	}
}

void GxsChannelFilesWidget::clear()
{
	ui->treeWidget->clear();
	closeFeedItem();
}

void GxsChannelFilesWidget::setFilter(const QString &text, int type)
{
	if (mFilterText == text && mFilterType == type) {
		return;
	}

	mFilterText = text;
	mFilterType = type;

	filterItems();
}

void GxsChannelFilesWidget::setFilterText(const QString &text)
{
	setFilter(text, mFilterType);
}

void GxsChannelFilesWidget::setFilterType(int type)
{
	setFilter(mFilterText, type);
}

void GxsChannelFilesWidget::filterItems()
{
	QTreeWidgetItemIterator it(ui->treeWidget);
	QTreeWidgetItem *treeItem;
	while ((treeItem = *it) != NULL) {
		++it;
		filterItem(treeItem);
	}
}

void GxsChannelFilesWidget::filterItem(QTreeWidgetItem *treeItem)
{
	bool visible = mFilterText.isEmpty();

	switch (mFilterType) {
	case GxsChannelPostsWidget::FILTER_TITLE:
		visible = treeItem->text(COLUMN_TITLE).contains(mFilterText, Qt::CaseInsensitive);
		break;
	case GxsChannelPostsWidget::FILTER_MSG:
		visible = treeItem->data(COLUMN_DATA, ROLE_MSG).toString().contains(mFilterText, Qt::CaseInsensitive);
		break;
	case GxsChannelPostsWidget::FILTER_FILE_NAME:
		visible = treeItem->text(COLUMN_FILENAME).contains(mFilterText, Qt::CaseInsensitive);
		break;
	}

	treeItem->setHidden(!visible);
}

//QTreeWidgetItem *GxsChannelFilesWidget::findFile(const RsGxsGroupId &groupId, const RsGxsMessageId &messageId, const RsFileHash &fileHash)
//{
//	QTreeWidgetItemIterator it(ui->treeWidget);
//	QTreeWidgetItem *treeItem;
//	while ((treeItem = *it) != NULL) {
//		++it;

//		if (fileHash != treeItem->data(COLUMN_DATA, ROLE_FILE_HASH).value<RsFileHash>()) {
//			continue;
//		}
//		if (messageId != treeItem->data(COLUMN_DATA, ROLE_MESSAGE_ID).value<RsGxsMessageId>()) {
//			continue;
//		}
//		if (groupId != treeItem->data(COLUMN_DATA, ROLE_GROUP_ID).value<RsGxsGroupId>()) {
//			continue;
//		}

//		return treeItem;
//	}

//	return NULL;
//}

void GxsChannelFilesWidget::removeItems(const RsGxsGroupId &groupId, const RsGxsMessageId &messageId)
{
	QTreeWidgetItemIterator it(ui->treeWidget);
	QTreeWidgetItem *treeItem;
	while ((treeItem = *it) != NULL) {
		++it;

		if (messageId != treeItem->data(COLUMN_DATA, ROLE_MESSAGE_ID).value<RsGxsMessageId>()) {
			continue;
		}
		if (groupId != treeItem->data(COLUMN_DATA, ROLE_GROUP_ID).value<RsGxsGroupId>()) {
			continue;
		}

		delete(treeItem);
	}
}

void GxsChannelFilesWidget::closeFeedItem()
{
	if (mFeedItem) {
		delete(mFeedItem);
		mFeedItem = NULL;
	}

	ui->feedItemFrame->hide();
}

void GxsChannelFilesWidget::currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem */*previous*/)
{
	if (!current) {
		closeFeedItem();
		return;
	}

	RsGxsGroupId groupId = current->data(COLUMN_DATA, ROLE_GROUP_ID).value<RsGxsGroupId>();
	RsGxsMessageId messageId = current->data(COLUMN_DATA, ROLE_MESSAGE_ID).value<RsGxsMessageId>();

	if (mFeedItem) {
		if (mFeedItem->groupId() == groupId && mFeedItem->messageId() == messageId) {
			return;
		}
		closeFeedItem();
	}

	mFeedItem = new GxsChannelPostItem(NULL, 0, groupId, messageId, true, true);
	ui->feedItemFrame->show();
	ui->feedItemFrame->layout()->addWidget(mFeedItem);
}
