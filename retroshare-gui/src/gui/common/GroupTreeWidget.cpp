/*******************************************************************************
 * gui/common/GroupTreeWidget.cpp                                              *
 *                                                                             *
 * Copyright (C) 2010, Retroshare Team <retroshare.project@gmail.com>          *
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

#include "GroupTreeWidget.h"
#include "ui_GroupTreeWidget.h"

#include "gui/common/FilesDefs.h"
#include "retroshare/rsgxsflags.h"

#include "PopularityDefs.h"
#include "RSElidedItemDelegate.h"
#include "RSTreeWidgetItem.h"
#include "gui/settings/rsharesettings.h"
#include "util/QtVersion.h"
#include "util/DateTime.h"

#include <QMenu>

#include <stdint.h>

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
#define ROLE_SEARCH_STRING   Qt::UserRole + 11
#define ROLE_REQUEST_ID      Qt::UserRole + 12

#define FILTER_NAME_INDEX  0
#define FILTER_DESC_INDEX  1

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
	compareRole->setRole(GTW_COLUMN_DATA, ROLE_NAME);

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
	ui->treeWidget->setColumnCount(GTW_COLUMN_COUNT);
	ui->treeWidget->enableColumnCustomize(true);
	ui->treeWidget->setColumnCustomizable(GTW_COLUMN_NAME, false);

	int S = QFontMetricsF(font()).height() ;
	int W = QFontMetricsF(font()).width("_") ;
	int D = QFontMetricsF(font()).width("9999-99-99[]") ;

	QTreeWidgetItem *headerItem = ui->treeWidget->headerItem();
	headerItem->setText(GTW_COLUMN_NAME, tr("Name"));
	headerItem->setText(GTW_COLUMN_UNREAD, "");
	headerItem->setText(GTW_COLUMN_POSTS, "");
	headerItem->setText(GTW_COLUMN_POPULARITY, "");
	headerItem->setText(GTW_COLUMN_LAST_POST, "");
	headerItem->setToolTip(GTW_COLUMN_NAME, tr("Group Name"));
	headerItem->setToolTip(GTW_COLUMN_UNREAD, tr("Number of Unread message"));
	headerItem->setToolTip(GTW_COLUMN_POSTS, tr("Friend's Posts"));
	headerItem->setToolTip(GTW_COLUMN_POPULARITY, tr("Popularity"));
	headerItem->setToolTip(GTW_COLUMN_LAST_POST, tr("Last Post"));

	/* Set header resize modes and initial section sizes */
	QHeaderView *header = ui->treeWidget->header ();
	header->setStretchLastSection(true);
	QHeaderView_setSectionResizeModeColumn(header, GTW_COLUMN_NAME, QHeaderView::Interactive);
	header->resizeSection(GTW_COLUMN_NAME, 40*W) ;
	QHeaderView_setSectionResizeModeColumn(header, GTW_COLUMN_UNREAD, QHeaderView::Interactive);
	header->resizeSection(GTW_COLUMN_UNREAD, 3*W+4) ;
	QHeaderView_setSectionResizeModeColumn(header, GTW_COLUMN_POSTS, QHeaderView::Interactive);
	header->resizeSection(GTW_COLUMN_POSTS, 3*W+4) ;
	header->setSectionHidden(GTW_COLUMN_POSTS, true);
	QHeaderView_setSectionResizeModeColumn(header, GTW_COLUMN_POPULARITY, QHeaderView::Interactive);
	header->resizeSection(GTW_COLUMN_POPULARITY, 3*W) ;
	QHeaderView_setSectionResizeModeColumn(header, GTW_COLUMN_LAST_POST, QHeaderView::Interactive);
	header->resizeSection(GTW_COLUMN_LAST_POST, D+4) ;
	header->setSectionHidden(GTW_COLUMN_LAST_POST, true);

	/* add filter actions */
	ui->filterLineEdit->addFilter(QIcon(), tr("Title"), FILTER_NAME_INDEX , tr("Search Title"));
	ui->filterLineEdit->addFilter(QIcon(), tr("Description"), FILTER_DESC_INDEX , tr("Search Description"));
	ui->filterLineEdit->setCurrentFilter(FILTER_NAME_INDEX);

	ui->distantSearchLineEdit->setPlaceholderText(tr("Search entire network...")) ;

	connect(ui->distantSearchLineEdit,SIGNAL(returnPressed()),this,SLOT(distantSearch())) ;

	/* Initialize display button */
	initDisplayMenu(ui->displayButton);

	ui->treeWidget->setIconSize(QSize(S*1.8,S*1.8));
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

	ui->toolBarFrame->layout()->addWidget(toolButton);
}

// Load and save settings (group must be started from the caller)
void GroupTreeWidget::processSettings(bool load)
{
	if (Settings == NULL) {
		return;
	}

	const int SORTBY_NAME = 1;
	const int SORTBY_POPULARITY = 2;
	const int SORTBY_LASTPOST = 3;
	const int SORTBY_POSTS = 4;
	const int SORTBY_UNREAD = 5;

	ui->treeWidget->setSettingsVersion(1);//Change it when modifing column properties
	ui->treeWidget->processSettings(load);

	if (load) {
		// load Settings

		// state of order
		bool ascSort = Settings->value("GroupAscSort", true).toBool();
		actionSortAscending->setChecked(ascSort);
		actionSortDescending->setChecked(!ascSort);

		// state of sort
		int sortby = Settings->value("GroupSortBy").toInt();
		switch (sortby) {
		case SORTBY_NAME:
			if (actionSortByName) {
				actionSortByName->setChecked(true);
			}
			break;
		case SORTBY_POPULARITY:
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
		// save Settings

		// state of order
		Settings->setValue("GroupAscSort", !(actionSortDescending && actionSortDescending->isChecked())); //True by default

		// state of sort
		int sortby = SORTBY_NAME;
		if (actionSortByName && actionSortByName->isChecked()) {
			sortby = SORTBY_NAME;
		} else if (actionSortByPopularity && actionSortByPopularity->isChecked()) {
			sortby = SORTBY_POPULARITY;
		} else if (actionSortByLastPost && actionSortByLastPost->isChecked()) {
			sortby = SORTBY_LASTPOST;
		} else if (actionSortByPosts && actionSortByPosts->isChecked()) {
			sortby = SORTBY_POSTS;
		} else if (actionSortByUnread && actionSortByUnread->isChecked()) {
			sortby = SORTBY_UNREAD;
		}
		Settings->setValue("GroupSortBy", sortby);
	}
}

void GroupTreeWidget::initDisplayMenu(QToolButton *toolButton)
{
	displayMenu = new QMenu();
	QActionGroup *actionGroupAsc = new QActionGroup(displayMenu);

	actionSortDescending = displayMenu->addAction(FilesDefs::getIconFromQtResourcePath(":/images/sort_decrease.png"), tr("Sort Descending Order"), this, SLOT(sort()));
	actionSortDescending->setCheckable(true);
	actionSortDescending->setActionGroup(actionGroupAsc);

	actionSortAscending = displayMenu->addAction(FilesDefs::getIconFromQtResourcePath(":/images/sort_incr.png"), tr("Sort Ascending Order"), this, SLOT(sort()));
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

	actionSortByPosts = displayMenu->addAction(QIcon(), tr("Sort by Number of Friend's Posts"), this, SLOT(sort()));
	actionSortByPosts->setCheckable(true);
	actionSortByPosts->setActionGroup(actionGroup);

	actionSortByUnread = displayMenu->addAction(QIcon(), tr("Sort by Unread"), this, SLOT(sort()));
	actionSortByUnread->setCheckable(true);
	actionSortByUnread->setActionGroup(actionGroup);

	toolButton->setMenu(displayMenu);
}

void GroupTreeWidget::updateColors()
{
	QTreeWidgetItemIterator itemIterator(ui->treeWidget);
	QTreeWidgetItem *item;
	while ((item = *itemIterator) != NULL) {
		++itemIterator;

		int color = item->data(GTW_COLUMN_DATA, ROLE_COLOR).toInt();
		if (color >= 0) {
			item->setData(GTW_COLUMN_NAME, Qt::TextColorRole, mTextColor[color]);
		} else {
			item->setData(GTW_COLUMN_NAME, Qt::TextColorRole, QVariant());
		}

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
		id = current->data(GTW_COLUMN_DATA, ROLE_ID).toString();
	}

	emit treeCurrentItemChanged(id);
}

void GroupTreeWidget::itemActivated(QTreeWidgetItem *item, int column)
{
	Q_UNUSED(column);

	QString id;

	if (item) {
		id = item->data(GTW_COLUMN_DATA, ROLE_ID).toString();
	}

	emit treeItemActivated(id);
}

QTreeWidgetItem *GroupTreeWidget::addCategoryItem(const QString &name, const QIcon &icon, bool expand)
{
	QFont font;
	QTreeWidgetItem *item = new QTreeWidgetItem();
	ui->treeWidget->addTopLevelItem(item);
	// To get StyleSheet for Items
	ui->treeWidget->style()->unpolish(ui->treeWidget);
	ui->treeWidget->style()->polish(ui->treeWidget);

	item->setText(GTW_COLUMN_NAME, name);
	item->setData(GTW_COLUMN_DATA, ROLE_NAME, name);
	font = item->font(GTW_COLUMN_NAME);
	font.setBold(true);
	item->setFont(GTW_COLUMN_NAME, font);
	item->setIcon(GTW_COLUMN_NAME, icon);

	int S = QFontMetricsF(font).height();

	item->setSizeHint(GTW_COLUMN_NAME, QSize(S*1.9, S*1.9));
	item->setData(GTW_COLUMN_NAME, Qt::TextColorRole, textColorCategory());
	item->setData(GTW_COLUMN_DATA, ROLE_COLOR, GROUPTREEWIDGET_COLOR_CATEGORY);

	item->setExpanded(expand);

	return item;
}

void GroupTreeWidget::removeSearchItem(QTreeWidgetItem *item)
{
    ui->treeWidget->takeTopLevelItem(ui->treeWidget->indexOfTopLevelItem(item)) ;
}

QTreeWidgetItem *GroupTreeWidget::addSearchItem(const QString& search_string, uint32_t id, const QIcon& icon)
{
    QTreeWidgetItem *item = addCategoryItem(search_string,icon,true);

    item->setData(GTW_COLUMN_DATA,ROLE_SEARCH_STRING,search_string) ;
    item->setData(GTW_COLUMN_DATA,ROLE_REQUEST_ID   ,id) ;

    return item;
}

void GroupTreeWidget::setDistSearchVisible(bool visible)
{
    ui->distantSearchLineEdit->setVisible(visible);
}

bool GroupTreeWidget::isSearchRequestResult(QPoint &point,QString& group_id,uint32_t& search_req_id)
{
	QTreeWidgetItem *item = ui->treeWidget->itemAt(point);
	if (item == NULL)
		return false;

	QTreeWidgetItem *parent = item->parent();

	if(parent == NULL)
		return false ;

	search_req_id = parent->data(GTW_COLUMN_DATA, ROLE_REQUEST_ID).toUInt();
	group_id = itemId(item) ;

	return search_req_id > 0;
}

bool GroupTreeWidget::isSearchRequestResultItem(QTreeWidgetItem *item,QString& group_id,uint32_t& search_req_id)
{
	QTreeWidgetItem *parent = item->parent();

	if(parent == NULL)
		return false ;

	search_req_id = parent->data(GTW_COLUMN_DATA, ROLE_REQUEST_ID).toUInt();
	group_id = itemId(item) ;

	return search_req_id > 0;
}

bool GroupTreeWidget::isSearchRequestItem(QPoint &point,uint32_t& search_req_id)
{
    QTreeWidgetItem *item = ui->treeWidget->itemAt(point);
	if (item == NULL)
		return false;

	search_req_id = item->data(GTW_COLUMN_DATA, ROLE_REQUEST_ID).toUInt();

    return search_req_id > 0;
}

QString GroupTreeWidget::itemId(QTreeWidgetItem *item)
{
	if (item == NULL) {
		return "";
	}

	return item->data(GTW_COLUMN_DATA, ROLE_ID).toString();
}

QString GroupTreeWidget::itemIdAt(QPoint &point)
{
	QTreeWidgetItem *item = ui->treeWidget->itemAt(point);
	if (item == NULL) {
		return "";
	}

	return item->data(GTW_COLUMN_DATA, ROLE_ID).toString();
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
			if (childItem->data(GTW_COLUMN_DATA, ROLE_ID).toString() == itemInfo.id) {
				/* Found child */
				item = childItem;
				break;
			}
		}

		if (item == NULL) {
			item = new RSTreeWidgetItem(compareRole);
			item->setData(GTW_COLUMN_DATA, ROLE_ID, itemInfo.id);
			categoryItem->addChild(item);
		}

		item->setText(GTW_COLUMN_NAME, itemInfo.name);
		item->setData(GTW_COLUMN_DATA, ROLE_NAME, itemInfo.name);
		item->setData(GTW_COLUMN_DATA, ROLE_DESCRIPTION, itemInfo.description);

		// Add children for context strings. This happens in the search.
		while(nullptr != item->takeChild(0));

		for(auto& str:itemInfo.context_strings)
			if(!str.empty())
			{
				QTreeWidgetItem *wit = new QTreeWidgetItem(QStringList(QString::fromUtf8(str.c_str())));
				wit->setData(GTW_COLUMN_DATA,ROLE_ID,itemInfo.id);
				item->addChild(wit);
			}

		/* Set last post */
		qlonglong lastPost = itemInfo.lastpost.toTime_t();
		item->setData(GTW_COLUMN_DATA, ROLE_LASTPOST, -lastPost); // negative for correct sorting
		if(itemInfo.lastpost == QDateTime::fromTime_t(0))
			item->setText(GTW_COLUMN_LAST_POST, tr("Never"));
		else
			item->setText(GTW_COLUMN_LAST_POST, itemInfo.lastpost.toString(Qt::ISODate).replace("T"," "));


		/* Set visible posts */
		item->setText(GTW_COLUMN_POSTS, QString::number(itemInfo.max_visible_posts));
		item->setData(GTW_COLUMN_DATA, ROLE_POSTS, -itemInfo.max_visible_posts);// negative for correct sorting

		/* Set icon */
		item->setIcon(GTW_COLUMN_NAME, itemInfo.icon);

		/* Set popularity */
		QString tooltip = PopularityDefs::tooltip(itemInfo.popularity);

		item->setIcon(GTW_COLUMN_POPULARITY, PopularityDefs::icon(itemInfo.popularity));
		item->setData(GTW_COLUMN_DATA, ROLE_POPULARITY, -itemInfo.popularity); // negative for correct sorting

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

		QString desc = itemInfo.description.left(30);
		desc.replace("\n"," ");
		desc.replace("\t"," ");

		if(itemInfo.description.length() > 30)
			desc += "…";

		tooltip += "\n" + tr("Description") + ": " + desc;

		tooltip += "\n" + tr("Id") + ": " + itemInfo.id;

		item->setToolTip(GTW_COLUMN_NAME, tooltip);
		item->setToolTip(GTW_COLUMN_UNREAD, tooltip);
		item->setToolTip(GTW_COLUMN_POSTS, tooltip);
		item->setToolTip(GTW_COLUMN_POPULARITY, tooltip);

		item->setData(GTW_COLUMN_DATA, ROLE_SUBSCRIBE_FLAGS, itemInfo.subscribeFlags);

		/* Set color */
		if (itemInfo.publishKey) {
			item->setData(GTW_COLUMN_DATA, ROLE_COLOR, GROUPTREEWIDGET_COLOR_PRIVATEKEY);
			item->setData(GTW_COLUMN_NAME, Qt::ForegroundRole, QBrush(textColorPrivateKey()));
		} else {
			// Let StyleSheet color
			item->setData(GTW_COLUMN_DATA, ROLE_COLOR, GROUPTREEWIDGET_COLOR_STANDARD);
			item->setData(GTW_COLUMN_NAME, Qt::BackgroundRole, QVariant());
		}

		/* Calculate score */
		calculateScore(item, filterText);
	}

	/* Remove all items not in list */
	int child = 0;
	int childCount = categoryItem->childCount();
	while (child < childCount) {
		QString id = categoryItem->child(child)->data(GTW_COLUMN_DATA, ROLE_ID).toString();

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

	QFont font = item->font(GTW_COLUMN_NAME);

	if (unreadCount) {
		item->setData(GTW_COLUMN_DATA, ROLE_UNREAD, unreadCount);
		item->setText(GTW_COLUMN_UNREAD, QString::number(unreadCount));
		font.setBold(true);
	} else {
		item->setText(GTW_COLUMN_UNREAD, "");
		font.setBold(false);
	}

	item->setFont(GTW_COLUMN_NAME, font);
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
		if (item->data(GTW_COLUMN_DATA, ROLE_ID).toString() == id) {
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

	item->setData(GTW_COLUMN_NAME, Qt::StatusTipRole, wait ? "waiting" : "");
	return true;
}

RSTreeWidget *GroupTreeWidget::treeWidget()
{
	return ui->treeWidget;
}

bool GroupTreeWidget::getGroupName(const QString& id, QString& name)
{
	QTreeWidgetItem *item = getItemFromId(id);
	if (item == NULL) {
		return false;
	}

	name = item->data(GTW_COLUMN_DATA, ROLE_NAME).toString();

	return true;
}

int GroupTreeWidget::subscribeFlags(const QString &id)
{
	QTreeWidgetItem *item = getItemFromId(id);
	if (item == NULL) {
		return 0;
	}

	return item->data(GTW_COLUMN_DATA, ROLE_SUBSCRIBE_FLAGS).toInt();
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
				scoreString = item->data(GTW_COLUMN_DATA, ROLE_NAME).toString();
				break;
			case FILTER_DESC_INDEX:
				scoreString = item->data(GTW_COLUMN_DATA, ROLE_DESCRIPTION).toString();
				break;
			}

			score = scoreString.count(filterText, Qt::CaseInsensitive);

			if (score) {
				item->setHidden(false);
			} else {
				item->setHidden(true);
			}
		}

		item->setData(GTW_COLUMN_DATA, ROLE_SEARCH_SCORE, -score); // negative for correct sorting

		return;
	}

	/* Find out which has given word in it */
	QTreeWidgetItemIterator itemIterator(ui->treeWidget);
	QTreeWidgetItem *tmpItem;
	while ((tmpItem = *itemIterator) != NULL) {
		++itemIterator;

		if (tmpItem->data(GTW_COLUMN_DATA, ROLE_ID).toString().isEmpty()) {
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
		compareRole->setRole(GTW_COLUMN_DATA, ROLE_SEARCH_SCORE);
		compareRole->addRole(GTW_COLUMN_DATA, ROLE_LASTPOST);
	} else if (actionSortByName && actionSortByName->isChecked()) {
		compareRole->setRole(GTW_COLUMN_DATA, ROLE_NAME);
	} else if (actionSortByPopularity && actionSortByPopularity->isChecked()) {
		compareRole->setRole(GTW_COLUMN_DATA, ROLE_POPULARITY);
	} else if (actionSortByLastPost && actionSortByLastPost->isChecked()) {
		compareRole->setRole(GTW_COLUMN_DATA, ROLE_LASTPOST);
	} else if (actionSortByPosts && actionSortByPosts->isChecked()) {
		compareRole->setRole(GTW_COLUMN_DATA, ROLE_POSTS);
	} else if (actionSortByUnread && actionSortByUnread->isChecked()) {
		compareRole->setRole(GTW_COLUMN_DATA, ROLE_UNREAD);
	}

	if (categoryItem) {
		categoryItem->sortChildren(GTW_COLUMN_DATA, order);
	} else {
		int count = ui->treeWidget->topLevelItemCount();
		for (int child = 0; child < count; ++child) {
			ui->treeWidget->topLevelItem(child)->sortChildren(GTW_COLUMN_DATA, order);
		}
	}
}

void GroupTreeWidget::distantSearch()
{
    emit distantSearchRequested(ui->distantSearchLineEdit->text());

    ui->distantSearchLineEdit->clear();
}

void GroupTreeWidget::sort()
{
	resort(NULL);
}
