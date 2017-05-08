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

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QMovie>
#include <QToolButton>

#include "retroshare/rsgxsflags.h"

#include "PopularityDefs.h"
#include "RSElidedItemDelegate.h"
#include "RSTreeWidgetItem.h"
#include "gui/common/ElidedLabel.h"
#include "gui/settings/rsharesettings.h"
#include "util/QtVersion.h"
#include "util/DateTime.h"

#include <stdint.h>

#define COLUMN_NAME        0
#define COLUMN_UNREAD      1
#define COLUMN_POPULARITY  2
#define COLUMN_COUNT       3
#define COLUMN_DATA        COLUMN_NAME

#define ROLE_ID              Qt::UserRole
#define ROLE_NAME            Qt::UserRole + 1
#define ROLE_DESCRIPTION     Qt::UserRole + 2
#define ROLE_POPULARITY      Qt::UserRole + 3
#define ROLE_LASTPOST        Qt::UserRole + 4
#define ROLE_POSTS           Qt::UserRole + 5
#define ROLE_UNREAD          Qt::UserRole + 6
#define ROLE_SEARCH_SCORE    Qt::UserRole + 7
#define ROLE_SUBSCRIBE_FLAGS Qt::UserRole + 8
#define ROLE_COLOR           Qt::UserRole + 9
#define ROLE_SAVED_ICON      Qt::UserRole + 10

#define FILTER_NAME_INDEX  0
#define FILTER_DESC_INDEX  1

Q_DECLARE_METATYPE(ElidedLabel*)
Q_DECLARE_METATYPE(QLabel*)

GroupTreeWidget::GroupTreeWidget(QWidget *parent) :
		QWidget(parent), ui(new Ui::GroupTreeWidget)
{
	ui->setupUi(this);

	displayMenu = NULL;
	actionSortAscending = NULL;
//	actionSortDescending = NULL;
	actionSortByName = NULL;
	actionSortByPopularity = NULL;
	actionSortByLastPost = NULL;
	actionSortByPosts = NULL;
	actionSortByUnread = NULL;

	compareRole = new RSTreeWidgetItemCompareRole;
	compareRole->setRole(COLUMN_DATA, ROLE_NAME);

	/* Connect signals */
	connect(ui->filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterChanged()));
	connect(ui->filterLineEdit, SIGNAL(filterChanged(int)), this, SLOT(filterChanged()));

	connect(ui->treeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customContextMenuRequested(QPoint)));
	connect(ui->treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
	connect(ui->treeWidget, SIGNAL(itemActivated(QTreeWidgetItem*,int)), this, SLOT(itemActivated(QTreeWidgetItem*,int)));
	if (!style()->styleHint(QStyle::SH_ItemView_ActivateItemOnSingleClick, NULL, this)) {
		// need signal itemClicked too
		connect(ui->treeWidget, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(itemActivated(QTreeWidgetItem*,int)));
	}

	/* Add own item delegate */
	RSElidedItemDelegate *itemDelegate = new RSElidedItemDelegate(this);
	itemDelegate->setSpacing(QSize(0, 2));
	ui->treeWidget->setItemDelegate(itemDelegate);

	/* Initialize tree widget */
	ui->treeWidget->setColumnCount(COLUMN_COUNT);

	int S = QFontMetricsF(font()).height() ;
	int W = QFontMetricsF(font()).width("999") ;

	/* Set header resize modes and initial section sizes */
	QHeaderView *header = ui->treeWidget->header ();
	header->setStretchLastSection(false);
	QHeaderView_setSectionResizeModeColumn(header, COLUMN_NAME, QHeaderView::Stretch);
	header->resizeSection(COLUMN_NAME, 10*S) ;
	QHeaderView_setSectionResizeModeColumn(header, COLUMN_UNREAD, QHeaderView::Fixed);
	header->resizeSection(COLUMN_UNREAD, W+4) ;
	QHeaderView_setSectionResizeModeColumn(header, COLUMN_POPULARITY, QHeaderView::Fixed);
	header->resizeSection(COLUMN_POPULARITY, 2*S) ;

	/* add filter actions */
	ui->filterLineEdit->addFilter(QIcon(), tr("Title"), FILTER_NAME_INDEX , tr("Search Title"));
	ui->filterLineEdit->addFilter(QIcon(), tr("Description"), FILTER_DESC_INDEX , tr("Search Description"));
	ui->filterLineEdit->setCurrentFilter(FILTER_NAME_INDEX);

	/* Initialize display button */
	initDisplayMenu(ui->displayButton);

	ui->treeWidget->setIconSize(QSize(S*1.6,S*1.6));
}

GroupTreeWidget::~GroupTreeWidget()
{
	delete ui;
}

static void getNameWidget(QTreeWidget *treeWidget, QTreeWidgetItem *item, ElidedLabel *&nameLabel, QLabel *&waitLabel)
{
	QWidget *widget = treeWidget->itemWidget(item, COLUMN_NAME);

	if (!widget) {
		widget = new QWidget;
		widget->setAttribute(Qt::WA_TranslucentBackground);
		nameLabel = new ElidedLabel(widget);
		waitLabel = new QLabel(widget);
		QMovie *movie = new QMovie(":/images/loader/circleball-16.gif");
		waitLabel->setMovie(movie);
		waitLabel->setHidden(true);

		widget->setProperty("nameLabel", qVariantFromValue(nameLabel));
		widget->setProperty("waitLabel", qVariantFromValue(waitLabel));

		QHBoxLayout *layout = new QHBoxLayout;
		layout->setSpacing(0);
		layout->setContentsMargins(0, 0, 0, 0);

		layout->addWidget(nameLabel);
		layout->addWidget(waitLabel);

		widget->setLayout(layout);

		treeWidget->setItemWidget(item, COLUMN_NAME, widget);
	} else {
		nameLabel = widget->property("nameLabel").value<ElidedLabel*>();
		waitLabel = widget->property("waitLabel").value<QLabel*>();
	}
}

void GroupTreeWidget::changeEvent(QEvent *e)
{
	QWidget::changeEvent(e);
	switch (e->type()) {
	case QEvent::LanguageChange:
		ui->retranslateUi(this);
		break;
	case QEvent::StyleChange:
		updateColors();
		break;
	default:
		// remove compiler warnings
		break;
	}
}

void GroupTreeWidget::addToolButton(QToolButton *toolButton)
{
	if (!toolButton) {
		return;
	}

	/* Initialize button */
	toolButton->setAutoRaise(true);
	toolButton->setIconSize(ui->displayButton->iconSize());
	toolButton->setFocusPolicy(ui->displayButton->focusPolicy());

	ui->titleBarFrame->layout()->addWidget(toolButton);
}

void GroupTreeWidget::processSettings(RshareSettings *settings, bool load)
{
	if (settings == NULL) {
		return;
	}

	const int SORTBY_NAME = 1;
	const int SORTBY_POPULRITY = 2;
	const int SORTBY_LASTPOST = 3;
	const int SORTBY_POSTS = 4;
	const int SORTBY_UNREAD = 5;

	if (load) {
		// load settings

		// state of order
		bool ascSort = settings->value("GroupAscSort", true).toBool();
		actionSortAscending->setChecked(ascSort);
		actionSortDescending->setChecked(!ascSort);

		// state of sort
		int sortby = settings->value("GroupSortBy").toInt();
		switch (sortby) {
		case SORTBY_NAME:
			if (actionSortByName) {
				actionSortByName->setChecked(true);
			}
			break;
		case SORTBY_POPULRITY:
			if (actionSortByPopularity) {
				actionSortByPopularity->setChecked(true);
			}
			break;
		case SORTBY_LASTPOST:
			if (actionSortByLastPost) {
				actionSortByLastPost->setChecked(true);
			}
			break;
		case SORTBY_POSTS:
			if (actionSortByPosts) {
				actionSortByPosts->setChecked(true);
			}
			break;
		case SORTBY_UNREAD:
			if (actionSortByUnread) {
				actionSortByUnread->setChecked(true);
			}
			break;
		}
	} else {
		// save settings

		// state of order
		settings->setValue("GroupAscSort", !(actionSortDescending && actionSortDescending->isChecked())); //True by default

		// state of sort
		int sortby = SORTBY_NAME;
		if (actionSortByName && actionSortByName->isChecked()) {
			sortby = SORTBY_NAME;
		} else if (actionSortByPopularity && actionSortByPopularity->isChecked()) {
			sortby = SORTBY_POPULRITY;
		} else if (actionSortByLastPost && actionSortByLastPost->isChecked()) {
			sortby = SORTBY_LASTPOST;
		} else if (actionSortByPosts && actionSortByPosts->isChecked()) {
			sortby = SORTBY_POSTS;
		} else if (actionSortByUnread && actionSortByUnread->isChecked()) {
			sortby = SORTBY_UNREAD;
		}
		settings->setValue("GroupSortBy", sortby);
	}
}

void GroupTreeWidget::initDisplayMenu(QToolButton *toolButton)
{
	displayMenu = new QMenu();
	QActionGroup *actionGroupAsc = new QActionGroup(displayMenu);

	actionSortDescending = displayMenu->addAction(QIcon(":/images/sort_decrease.png"), tr("Sort Descending Order"), this, SLOT(sort()));
	actionSortDescending->setCheckable(true);
	actionSortDescending->setActionGroup(actionGroupAsc);

	actionSortAscending = displayMenu->addAction(QIcon(":/images/sort_incr.png"), tr("Sort Ascending Order"), this, SLOT(sort()));
	actionSortAscending->setCheckable(true);
	actionSortAscending->setActionGroup(actionGroupAsc);

	displayMenu->addSeparator();

	QActionGroup *actionGroup = new QActionGroup(displayMenu);
	actionSortByName = displayMenu->addAction(QIcon(), tr("Sort by Name"), this, SLOT(sort()));
	actionSortByName->setCheckable(true);
	actionSortByName->setChecked(true); // set standard to sort by name
	actionSortByName->setActionGroup(actionGroup);

	actionSortByPopularity = displayMenu->addAction(QIcon(), tr("Sort by Popularity"), this, SLOT(sort()));
	actionSortByPopularity->setCheckable(true);
	actionSortByPopularity->setActionGroup(actionGroup);

	actionSortByLastPost = displayMenu->addAction(QIcon(), tr("Sort by Last Post"), this, SLOT(sort()));
	actionSortByLastPost->setCheckable(true);
	actionSortByLastPost->setActionGroup(actionGroup);

	actionSortByPosts = displayMenu->addAction(QIcon(), tr("Sort by Number of Posts"), this, SLOT(sort()));
	actionSortByPosts->setCheckable(true);
	actionSortByPosts->setActionGroup(actionGroup);

	actionSortByUnread = displayMenu->addAction(QIcon(), tr("Sort by Unread"), this, SLOT(sort()));
	actionSortByUnread->setCheckable(true);
	actionSortByUnread->setActionGroup(actionGroup);

	toolButton->setMenu(displayMenu);
}

void GroupTreeWidget::updateColors()
{
	QBrush brush;
	QBrush standardBrush = ui->treeWidget->palette().color(QPalette::Text);

	QTreeWidgetItemIterator itemIterator(ui->treeWidget);
	QTreeWidgetItem *item;
	while ((item = *itemIterator) != NULL) {
		++itemIterator;

		int color = item->data(COLUMN_DATA, ROLE_COLOR).toInt();
		if (color >= 0) {
			brush = QBrush(mTextColor[color]);
		} else {
			brush = standardBrush;
		}

		item->setForeground(COLUMN_NAME, brush);

		ElidedLabel *nameLabel = NULL;
		QLabel *waitLabel = NULL;
		getNameWidget(ui->treeWidget, item, nameLabel, waitLabel);
		nameLabel->setTextColor(brush.color());
	}
}

void GroupTreeWidget::customContextMenuRequested(const QPoint &pos)
{
	emit treeCustomContextMenuRequested(pos);
}

void GroupTreeWidget::currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
	Q_UNUSED(previous);

	QString id;

	if (current) {
		id = current->data(COLUMN_DATA, ROLE_ID).toString();
	}

	emit treeCurrentItemChanged(id);
}

void GroupTreeWidget::itemActivated(QTreeWidgetItem *item, int column)
{
	Q_UNUSED(column);

	QString id;

	if (item) {
		id = item->data(COLUMN_DATA, ROLE_ID).toString();
	}

	emit treeItemActivated(id);
}

QTreeWidgetItem *GroupTreeWidget::addCategoryItem(const QString &name, const QIcon &icon, bool expand)
{
	QFont font;
	QTreeWidgetItem *item = new QTreeWidgetItem();
	ui->treeWidget->addTopLevelItem(item);

	ElidedLabel *nameLabel = NULL;
	QLabel *waitLabel = NULL;
	getNameWidget(ui->treeWidget, item, nameLabel, waitLabel);

	nameLabel->setText(name);
	item->setData(COLUMN_DATA, ROLE_NAME, name);
	font = item->font(COLUMN_NAME);
	font.setBold(true);
	item->setFont(COLUMN_NAME, font);
	nameLabel->setFont(font);
	item->setIcon(COLUMN_NAME, icon);

	int S = QFontMetricsF(font).height();

	item->setSizeHint(COLUMN_NAME, QSize(S*1.1, S*1.1));
	item->setForeground(COLUMN_NAME, QBrush(textColorCategory()));
	nameLabel->setTextColor(textColorCategory());
	item->setData(COLUMN_DATA, ROLE_COLOR, GROUPTREEWIDGET_COLOR_CATEGORY);

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

QString GroupTreeWidget::itemIdAt(QPoint &point)
{
	QTreeWidgetItem *item = ui->treeWidget->itemAt(point);
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

	QString filterText = ui->filterLineEdit->text();

	/* Iterate all items */
	QList<GroupItemInfo>::const_iterator it;
	for (it = itemList.begin(); it != itemList.end(); ++it) {
		const GroupItemInfo &itemInfo = *it;

		QTreeWidgetItem *item = NULL;

		/* Search exisiting item */
		int childCount = categoryItem->childCount();
		for (int child = 0; child < childCount; ++child) {
			QTreeWidgetItem *childItem = categoryItem->child(child);
			if (childItem->data(COLUMN_DATA, ROLE_ID).toString() == itemInfo.id) {
				/* Found child */
				item = childItem;
				break;
			}
		}

		if (item == NULL) {
			item = new RSTreeWidgetItem(compareRole);
			item->setData(COLUMN_DATA, ROLE_ID, itemInfo.id);
			categoryItem->addChild(item);
		}

		ElidedLabel *nameLabel = NULL;
		QLabel *waitLabel = NULL;
		getNameWidget(ui->treeWidget, item, nameLabel, waitLabel);

		nameLabel->setText(itemInfo.name);
		item->setData(COLUMN_DATA, ROLE_NAME, itemInfo.name);
		item->setData(COLUMN_DATA, ROLE_DESCRIPTION, itemInfo.description);

		/* Set last post */
		qlonglong lastPost = itemInfo.lastpost.toTime_t();
		item->setData(COLUMN_DATA, ROLE_LASTPOST, -lastPost); // negative for correct sorting

		/* Set visible posts */
		item->setData(COLUMN_DATA, ROLE_POSTS, -itemInfo.max_visible_posts);// negative for correct sorting

		/* Set icon */
		if (waitLabel->isVisible()) {
			/* Item is waiting, save icon in role */
			item->setData(COLUMN_DATA, ROLE_SAVED_ICON, itemInfo.icon);
		} else {
			item->setIcon(COLUMN_NAME, itemInfo.icon);
		}

		/* Set popularity */
		QString tooltip = PopularityDefs::tooltip(itemInfo.popularity);

		item->setIcon(COLUMN_POPULARITY, PopularityDefs::icon(itemInfo.popularity));
		item->setData(COLUMN_DATA, ROLE_POPULARITY, -itemInfo.popularity); // negative for correct sorting

		/* Set tooltip */
		if (itemInfo.adminKey)
			tooltip += "\n" + tr("You are admin (modify names and description using Edit menu)");
		else if (itemInfo.publishKey)
			tooltip += "\n" + tr("You have been granted as publisher (you can post here!)");

		if(!IS_GROUP_SUBSCRIBED(itemInfo.subscribeFlags))
			tooltip += "\n" + QString::number(itemInfo.max_visible_posts) + " messages available" ;
		// if(itemInfo.max_visible_posts)  // wtf? this=0 when there are some posts definitely exist - lastpost is recent
		if(itemInfo.lastpost == QDateTime::fromTime_t(0))
		    tooltip += "\n" + tr("Last Post") + ": "  + tr("Never") ;
		else
			tooltip += "\n" + tr("Last Post") + ": "  + DateTime::formatLongDateTime(itemInfo.lastpost) ;
		if(!IS_GROUP_SUBSCRIBED(itemInfo.subscribeFlags))
			tooltip += "\n" + tr("Subscribe to download and read messages") ;

		item->setToolTip(COLUMN_NAME, tooltip);
		item->setToolTip(COLUMN_UNREAD, tooltip);
		item->setToolTip(COLUMN_POPULARITY, tooltip);

		item->setData(COLUMN_DATA, ROLE_SUBSCRIBE_FLAGS, itemInfo.subscribeFlags);

		/* Set color */
		QBrush brush;
		if (itemInfo.publishKey) {
			brush = QBrush(textColorPrivateKey());
			item->setData(COLUMN_DATA, ROLE_COLOR, GROUPTREEWIDGET_COLOR_PRIVATEKEY);
		} else {
			brush = ui->treeWidget->palette().color(QPalette::Text);
			item->setData(COLUMN_DATA, ROLE_COLOR, GROUPTREEWIDGET_COLOR_STANDARD);
		}
		item->setForeground(COLUMN_NAME, brush);
		nameLabel->setTextColor(brush.color());

		/* Calculate score */
		calculateScore(item, filterText);
	}

	/* Remove all items not in list */
	int child = 0;
	int childCount = categoryItem->childCount();
	while (child < childCount) {
		QString id = categoryItem->child(child)->data(COLUMN_DATA, ROLE_ID).toString();

		for (it = itemList.begin(); it != itemList.end(); ++it) {
			if (it->id == id) {
				break;
			}
		}

		if (it == itemList.end()) {
			delete(categoryItem->takeChild(child));
			childCount = categoryItem->childCount();
		} else {
			++child;
		}
	}

	resort(categoryItem);
}

void GroupTreeWidget::setUnreadCount(QTreeWidgetItem *item, int unreadCount)
{
	if (item == NULL) {
		return;
	}
	ElidedLabel *nameLabel = NULL;
	QLabel *waitLabel = NULL;
	getNameWidget(ui->treeWidget, item, nameLabel, waitLabel);

	QFont font = nameLabel->font();

	if (unreadCount) {
		item->setData(COLUMN_DATA, ROLE_UNREAD, unreadCount);
		item->setText(COLUMN_UNREAD, QString::number(unreadCount));
		font.setBold(true);
	} else {
		item->setText(COLUMN_UNREAD, "");
		font.setBold(false);
	}

	nameLabel->setFont(font);
}

QTreeWidgetItem *GroupTreeWidget::getItemFromId(const QString &id)
{
	if (id.isEmpty()) {
		return NULL;
	}

	/* Search exisiting item */
	QTreeWidgetItemIterator itemIterator(ui->treeWidget);
	QTreeWidgetItem *item;
	while ((item = *itemIterator) != NULL) {
		++itemIterator;

		if (item->parent() == NULL) {
			continue;
		}
		if (item->data(COLUMN_DATA, ROLE_ID).toString() == id) {
			return item;
		}
	}
	return NULL ;
}

QTreeWidgetItem *GroupTreeWidget::activateId(const QString &id, bool focus)
{
	QTreeWidgetItem *item = getItemFromId(id);
	if (item == NULL) {
		return NULL;
	}

	ui->treeWidget->setCurrentItem(item);
	if (focus) {
		ui->treeWidget->setFocus();
	}
	return item;
}

bool GroupTreeWidget::setWaiting(const QString &id, bool wait)
{
	QTreeWidgetItem *item = getItemFromId(id);
	if (!item) {
		return false;
	}

	ElidedLabel *nameLabel = NULL;
	QLabel *waitLabel = NULL;
	getNameWidget(ui->treeWidget, item, nameLabel, waitLabel);
	if (wait) {
		if (waitLabel->isVisible()) {
			/* Allready waiting */
		} else {
			/* Save icon in role */
			QIcon icon = item->icon(COLUMN_NAME);
			item->setData(COLUMN_DATA, ROLE_SAVED_ICON, icon);

			/* Create empty icon of the same size */
			QPixmap pixmap(ui->treeWidget->iconSize());
			pixmap.fill(Qt::transparent);
			item->setIcon(COLUMN_NAME, QIcon(pixmap));

			/* Show waitLabel and hide nameLabel */
			nameLabel->setHidden(true);
			waitLabel->setVisible(true);
			waitLabel->movie()->start();
		}
	} else {
		if (waitLabel->isVisible()) {
			/* Show nameLabel and hide waitLabel */
			waitLabel->movie()->stop();
			waitLabel->setHidden(true);
			nameLabel->setVisible(true);

			/* Set icon saved in role */
			item->setIcon(COLUMN_NAME, item->data(COLUMN_DATA, ROLE_SAVED_ICON).value<QIcon>());
			item->setData(COLUMN_DATA, ROLE_SAVED_ICON, QIcon());
		}
	}

	return true;
}

RSTreeWidget *GroupTreeWidget::treeWidget()
{
	return ui->treeWidget;
}

bool GroupTreeWidget::getGroupName(QString id, QString& name)
{
	QTreeWidgetItem *item = getItemFromId(id);
	if (item == NULL) {
		return false;
	}

	name = item->data(COLUMN_DATA, ROLE_NAME).toString();

	return true;
}

int GroupTreeWidget::subscribeFlags(const QString &id)
{
	QTreeWidgetItem *item = getItemFromId(id);
	if (item == NULL) {
		return 0;
	}

	return item->data(COLUMN_DATA, ROLE_SUBSCRIBE_FLAGS).toInt();
}

void GroupTreeWidget::calculateScore(QTreeWidgetItem *item, const QString &filterText)
{
	if (item) {
		/* Calculate one item */
		int score;
		if (filterText.isEmpty()) {
			score = 0;
			item->setHidden(false);
		} else {
			QString scoreString;

			switch (ui->filterLineEdit->currentFilter()) {
			case FILTER_NAME_INDEX:
				scoreString = item->data(COLUMN_DATA, ROLE_NAME).toString();
				break;
			case FILTER_DESC_INDEX:
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

		item->setData(COLUMN_DATA, ROLE_SEARCH_SCORE, -score); // negative for correct sorting

		return;
	}

	/* Find out which has given word in it */
	QTreeWidgetItemIterator itemIterator(ui->treeWidget);
	QTreeWidgetItem *tmpItem;
	while ((tmpItem = *itemIterator) != NULL) {
		++itemIterator;

		if (tmpItem->data(COLUMN_DATA, ROLE_ID).toString().isEmpty()) {
			continue;
		}

		calculateScore(tmpItem, filterText);
	}
}

void GroupTreeWidget::filterChanged()
{
	/* Recalculate score */
	calculateScore(NULL, ui->filterLineEdit->text());

	resort(NULL);
}

void GroupTreeWidget::resort(QTreeWidgetItem *categoryItem)
{
	Qt::SortOrder order = (actionSortAscending == NULL || actionSortAscending->isChecked()) ? Qt::AscendingOrder : Qt::DescendingOrder;

	if (ui->filterLineEdit->text().isEmpty() == false) {
		compareRole->setRole(COLUMN_DATA, ROLE_SEARCH_SCORE);
		compareRole->addRole(COLUMN_DATA, ROLE_LASTPOST);
	} else if (actionSortByName && actionSortByName->isChecked()) {
		compareRole->setRole(COLUMN_DATA, ROLE_NAME);
	} else if (actionSortByPopularity && actionSortByPopularity->isChecked()) {
		compareRole->setRole(COLUMN_DATA, ROLE_POPULARITY);
	} else if (actionSortByLastPost && actionSortByLastPost->isChecked()) {
		compareRole->setRole(COLUMN_DATA, ROLE_LASTPOST);
	} else if (actionSortByPosts && actionSortByPosts->isChecked()) {
		compareRole->setRole(COLUMN_DATA, ROLE_POSTS);
	} else if (actionSortByUnread && actionSortByUnread->isChecked()) {
		compareRole->setRole(COLUMN_DATA, ROLE_UNREAD);
	}

	if (categoryItem) {
		categoryItem->sortChildren(COLUMN_DATA, order);
	} else {
		int count = ui->treeWidget->topLevelItemCount();
		for (int child = 0; child < count; ++child) {
			ui->treeWidget->topLevelItem(child)->sortChildren(COLUMN_DATA, order);
		}
	}
}

void GroupTreeWidget::sort()
{
	resort(NULL);
}
