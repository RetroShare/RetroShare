/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2010, RetroShare Team
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

#include "GroupTreeWidget.h"
#include "ui_GroupTreeWidget.h"

#include "RSItemDelegate.h"
#include "PopularityDefs.h"

#define COLUMN_NAME        0
#define COLUMN_POPULARITY  1
#define COLUMN_COUNT       2
#define COLUMN_DATA        COLUMN_NAME

#define ROLE_ID           Qt::UserRole
#define ROLE_NAME         Qt::UserRole + 1
#define ROLE_DESCRIPTION  Qt::UserRole + 2
#define ROLE_LASTPOST     Qt::UserRole + 3
#define ROLE_SEARCH_SCORE Qt::UserRole + 4

#define COMBO_NAME_INDEX  0
#define COMBO_DESC_INDEX  1

GroupTreeWidget::GroupTreeWidget(QWidget *parent) :
		QWidget(parent), ui(new Ui::GroupTreeWidget)
{
	ui->setupUi(this);

	/* Connect signals */
	connect(ui->clearFilter, SIGNAL(clicked()), this, SLOT(clearFilter()));
	connect(ui->filterText, SIGNAL(textChanged(const QString &)), this, SLOT(filterChanged()));
	connect(ui->filterCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(filterChanged()));

	connect(ui->treeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customContextMenuRequested(QPoint)));
	connect(ui->treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)));

	/* Add own item delegate */
	RSItemDelegate *itemDelegate = new RSItemDelegate(this);
	itemDelegate->removeFocusRect(COLUMN_POPULARITY);
	itemDelegate->setSpacing(QSize(0, 2));
	ui->treeWidget->setItemDelegate(itemDelegate);

	/* Initialize tree widget */
	ui->treeWidget->setColumnCount(COLUMN_COUNT);

	/* Set header resize modes and initial section sizes */
	QHeaderView *header = ui->treeWidget->header ();
	header->setResizeMode(COLUMN_NAME, QHeaderView::Stretch);
	header->resizeSection(COLUMN_NAME, 170);
	header->setResizeMode(COLUMN_POPULARITY, QHeaderView::Fixed);
	header->resizeSection(COLUMN_POPULARITY, 25);

	/* Initialize filter */
	ui->clearFilter->hide();
}

GroupTreeWidget::~GroupTreeWidget()
{
	delete ui;
}

void GroupTreeWidget::changeEvent(QEvent *e)
{
	QWidget::changeEvent(e);
	switch (e->type()) {
	case QEvent::LanguageChange:
		ui->retranslateUi(this);
		break;
	default:
		break;
	}
}

void GroupTreeWidget::customContextMenuRequested(const QPoint &pos)
{
	emit treeCustomContextMenuRequested(pos);
}

void GroupTreeWidget::currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
	QString id;

	if (current) {
		id = current->data(COLUMN_DATA, ROLE_ID).toString();
	}

	emit treeCurrentItemChanged(id);
}

QTreeWidgetItem *GroupTreeWidget::addCategoryItem(const QString &name, const QIcon &icon, bool expand)
{
	QFont font = QFont("ARIAL", 10);
	font.setBold(true);

	QTreeWidgetItem *item = new QTreeWidgetItem();
	item->setText(COLUMN_NAME, name);
	item->setData(COLUMN_DATA, ROLE_NAME, name);
	item->setFont(COLUMN_NAME, font);
	item->setIcon(COLUMN_NAME, icon);
	item->setSizeHint(COLUMN_NAME, QSize(18, 18));
	item->setForeground(COLUMN_NAME, QBrush(QColor(79, 79, 79)));

	ui->treeWidget->addTopLevelItem(item);

	item->setExpanded(expand);

	return item;
}

QString GroupTreeWidget::itemId(QTreeWidgetItem *item)
{
	if (item == NULL) {
		return "";
	}

	return item->data(COLUMN_DATA, ROLE_ID).toString();
}

void GroupTreeWidget::fillGroupItems(QTreeWidgetItem *categoryItem, const QList<GroupItemInfo> &itemList)
{
	if (categoryItem == NULL) {
		return;
	}

	/* Iterate all items */
	QList<GroupItemInfo>::const_iterator it;
	for (it = itemList.begin(); it != itemList.end(); it++) {
		const GroupItemInfo &itemInfo = *it;

		QTreeWidgetItem *item = NULL;

		/* Search exisiting item */
		int childCount = categoryItem->childCount();
		for (int child = 0; child < childCount; child++) {
			QTreeWidgetItem *childItem = categoryItem->child(child);
			if (childItem->data(COLUMN_DATA, ROLE_ID).toString() == itemInfo.id) {
				/* Found child */
				item = childItem;
				break;
			}
		}

		if (item == NULL) {
			item = new GroupTreeWidgetItem();
			item->setData(COLUMN_DATA, ROLE_ID, itemInfo.id);
			categoryItem->addChild(item);
		}

		item->setText(COLUMN_NAME, itemInfo.name);
		item->setData(COLUMN_DATA, ROLE_NAME, itemInfo.name);
		item->setData(COLUMN_DATA, ROLE_DESCRIPTION, itemInfo.description);

		/* Set last post */
		item->setData(COLUMN_DATA, ROLE_LASTPOST, itemInfo.lastpost);

		/* Set icon */
		item->setIcon(COLUMN_NAME, itemInfo.icon);

		/* Set popularity */
		QString tooltip = PopularityDefs::tooltip(itemInfo.popularity);
		item->setIcon(COLUMN_POPULARITY, PopularityDefs::icon(itemInfo.popularity));

		/* Set tooltip */
		if (itemInfo.privatekey) {
			tooltip += "\n" + tr("Private Key Available");
		}
		item->setToolTip(COLUMN_NAME, tooltip);
		item->setToolTip(COLUMN_POPULARITY, tooltip);

		/* Set color */
		QBrush brush;
		if (itemInfo.privatekey) {
			brush = QBrush(Qt::blue);
		}
		item->setForeground(COLUMN_NAME, brush);

		/* Calculate score */
		calculateScore(item);
	}

	/* Remove all items not in list */
	int child = 0;
	int childCount = categoryItem->childCount();
	while (child < childCount) {
		QString id = categoryItem->child(child)->data(COLUMN_DATA, ROLE_ID).toString();

		for (it = itemList.begin(); it != itemList.end(); it++) {
			if (it->id == id) {
				break;
			}
		}

		if (it == itemList.end()) {
			delete(categoryItem->takeChild(child));
			childCount = categoryItem->childCount();
		} else {
			child++;
		}
	}

	categoryItem->sortChildren(COLUMN_NAME, Qt::DescendingOrder);
}

void GroupTreeWidget::setUnreadCount(QTreeWidgetItem *item, int unreadCount)
{
	if (item == NULL) {
		return;
	}

	QString name = item->data(COLUMN_DATA, ROLE_NAME).toString();
	QFont font = item->font(COLUMN_NAME);

	if (unreadCount) {
		name += QString(" (%1)").arg(unreadCount);
		font.setBold(true);
	} else {
		font.setBold(false);
	}

	item->setText(COLUMN_NAME, name);
	item->setFont(COLUMN_NAME, font);

}

void GroupTreeWidget::calculateScore(QTreeWidgetItem *item)
{
	if (item) {
		/* Calculate one item */
		QString filterText = ui->filterText->text();
		int score;
		if (filterText.isEmpty()) {
			score = 0;
			item->setHidden(false);
		} else {
			QString scoreString;

			switch (ui->filterCombo->currentIndex()) {
			case COMBO_NAME_INDEX:
				scoreString = item->data(COLUMN_DATA, ROLE_NAME).toString();
				break;
			case COMBO_DESC_INDEX:
				scoreString = item->data(COLUMN_DATA, ROLE_DESCRIPTION).toString();
				break;
			}

			score = scoreString.count(filterText, Qt::CaseInsensitive);

			if (score) {
				item->setHidden(false);
			} else {
				item->setHidden(true);
			}
		}

		item->setData(COLUMN_DATA, ROLE_SEARCH_SCORE, score);

		return;
	}

	/* Find out which has given word in it */
	QTreeWidgetItemIterator itemIterator(ui->treeWidget);
	QTreeWidgetItem *item;
	while ((item = *itemIterator) != NULL) {
		itemIterator++;

		if (item->data(COLUMN_DATA, ROLE_ID).toString().isEmpty()) {
			continue;
		}

		calculateScore(item);
	}
}

void GroupTreeWidget::filterChanged()
{
	if (ui->filterText->text().isEmpty()) {
		clearFilter();
		return;
	}

	ui->clearFilter->setEnabled(true);
	ui->clearFilter->show();

	/* Recalculate score */
	calculateScore(NULL);

	int count = ui->treeWidget->topLevelItemCount();
	for (int child = 0; child < count; child++) {
		ui->treeWidget->topLevelItem(child)->sortChildren(COLUMN_NAME, Qt::DescendingOrder);
	}
}

void GroupTreeWidget::clearFilter()
{
	ui->filterText->clear();
	ui->clearFilter->hide();
	ui->filterText->setFocus();

	/* Recalculate score */
	calculateScore(NULL);

	int count = ui->treeWidget->topLevelItemCount();
	for (int child = 0; child < count; child++) {
		ui->treeWidget->topLevelItem(child)->sortChildren(COLUMN_NAME, Qt::DescendingOrder);
	}
}

GroupTreeWidgetItem::GroupTreeWidgetItem() : QTreeWidgetItem()
{
}

bool GroupTreeWidgetItem::operator<(const QTreeWidgetItem& other) const
{
	QDateTime otherChanTs = other.data(COLUMN_DATA, ROLE_LASTPOST).toDateTime();
	QDateTime thisChanTs = this->data(COLUMN_DATA, ROLE_LASTPOST).toDateTime();

	uint32_t otherCount = other.data(COLUMN_DATA, ROLE_SEARCH_SCORE).toUInt();
	uint32_t thisCount = this->data(COLUMN_DATA, ROLE_SEARCH_SCORE).toUInt();

	/* If counts are equal then determine by who has the most recent post */
	if (otherCount == thisCount){
		if (thisChanTs < otherChanTs) {
			return true;
		}
	}

	/* Choose the item where the string occurs the most */
	if (thisCount < otherCount) {
		return true;
	}

	if (thisChanTs < otherChanTs) {
		return true;
	}

	/* Compare name */
    return text(COLUMN_NAME) < other.text(COLUMN_NAME);
}
