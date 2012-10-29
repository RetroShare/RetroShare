/****************************************************************
 *
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

#include "FriendSelectionWidget.h"
#include "ui_FriendSelectionWidget.h"
#include "gui/notifyqt.h"
#include "gui/common/RSTreeWidgetItem.h"
#include "gui/common/StatusDefs.h"
#include "gui/common/PeerDefs.h"
#include "gui/common/GroupDefs.h"

#include <retroshare/rspeers.h>
#include <retroshare/rsstatus.h>

#include <algorithm>

#define COLUMN_NAME   0
#define COLUMN_CHECK  0
#define COLUMN_DATA   0

#define ROLE_ID       Qt::UserRole
#define ROLE_SORT     Qt::UserRole + 1

#define COLOR_CONNECT Qt::blue

#define IMAGE_GROUP16    ":/images/user/group16.png"
#define IMAGE_FRIENDINFO ":/images/peerdetails_16x16.png"

static bool isSelected(FriendSelectionWidget::Modus modus, QTreeWidgetItem *item)
{
	switch (modus) {
	case FriendSelectionWidget::MODUS_SINGLE:
	case FriendSelectionWidget::MODUS_MULTI:
		return item->isSelected();
	case FriendSelectionWidget::MODUS_CHECK:
		return (item->checkState(COLUMN_CHECK) == Qt::Checked);
	}

	return false;
}

static void setSelected(FriendSelectionWidget::Modus modus, QTreeWidgetItem *item, bool select)
{
	switch (modus) {
	case FriendSelectionWidget::MODUS_SINGLE:
	case FriendSelectionWidget::MODUS_MULTI:
		item->setSelected(select);
		break;
	case FriendSelectionWidget::MODUS_CHECK:
		item->setCheckState(COLUMN_CHECK, select ? Qt::Checked : Qt::Unchecked);
		break;
	}
}

FriendSelectionWidget::FriendSelectionWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::FriendSelectionWidget)
{
	ui->setupUi(this);

	started = false;
	listModus = MODUS_SINGLE;
	showGroups = true;
	inItemChanged = false;

	connect(ui->friendList, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuRequested(QPoint)));
	connect(ui->friendList, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(itemDoubleClicked(QTreeWidgetItem*,int)));
	connect(ui->friendList, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(itemChanged(QTreeWidgetItem*,int)));
	connect(ui->filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterItems(QString)));

	connect(NotifyQt::getInstance(), SIGNAL(groupsChanged(int)), this, SLOT(fillList()));
	connect(NotifyQt::getInstance(), SIGNAL(peerStatusChanged(const QString&,int)), this, SLOT(peerStatusChanged(const QString&,int)));

	compareRole = new RSTreeWidgetItemCompareRole;
	compareRole->setRole(COLUMN_NAME, ROLE_SORT);

	// sort list by name ascending
	ui->friendList->sortItems(COLUMN_NAME, Qt::AscendingOrder);

	ui->filterLineEdit->setPlaceholderText(tr("Search Friends"));
}

FriendSelectionWidget::~FriendSelectionWidget()
{
	delete ui;
}

void FriendSelectionWidget::setHeaderText(const QString &text)
{
	ui->friendList->headerItem()->setText(COLUMN_NAME, text);
}

void FriendSelectionWidget::setModus(Modus modus)
{
	listModus = modus;

	switch (listModus) {
	case MODUS_SINGLE:
	case MODUS_CHECK:
		ui->friendList->setSelectionMode(QAbstractItemView::SingleSelection);
		break;
	case MODUS_MULTI:
		ui->friendList->setSelectionMode(QAbstractItemView::ExtendedSelection);
		break;
	}

	fillList();
}

void FriendSelectionWidget::setShowGroups(bool show)
{
	showGroups = show;

	fillList();
}

void FriendSelectionWidget::start()
{
	started = true;
	fillList();
}

void FriendSelectionWidget::fillList()
{
	if (!started) {
		return;
	}

	// get selected items
	std::list<std::string> sslIdsSelected;
	selectedSslIds(sslIdsSelected, true);

	std::list<std::string> groupIdsSelected;
	selectedGroupIds(groupIdsSelected);

	// remove old items
	ui->friendList->clear();

	// get existing groups
	std::list<RsGroupInfo> groupInfoList;
	std::list<RsGroupInfo>::iterator groupIt;
	rsPeers->getGroupInfoList(groupInfoList);

	std::list<std::string> sslIds;
	std::list<std::string>::iterator sslIt;
	rsPeers->getFriendList(sslIds);

	std::list<StatusInfo> statusInfo;
	rsStatus->getStatusList(statusInfo);

	std::list<std::string> filledSslIds;

	// start with groups
	groupIt = groupInfoList.begin();
	while (true) {
		QTreeWidgetItem *groupItem = NULL;
		RsGroupInfo *groupInfo = NULL;

		if (showGroups && groupIt != groupInfoList.end()) {
			groupInfo = &(*groupIt);

			if (groupInfo->peerIds.size() == 0) {
				// don't show empty groups
				groupIt++;
				continue;
			}

			// add group item
			groupItem = new RSTreeWidgetItem(compareRole, IDTYPE_GROUP);

			// Add item to the list
			ui->friendList->addTopLevelItem(groupItem);

			groupItem->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
			groupItem->setTextAlignment(COLUMN_NAME, Qt::AlignLeft | Qt::AlignVCenter);
			groupItem->setIcon(COLUMN_NAME, QIcon(IMAGE_GROUP16));

			groupItem->setData(COLUMN_DATA, ROLE_ID, QString::fromStdString(groupInfo->id));

			groupItem->setExpanded(true);

			QString groupName = GroupDefs::name(*groupInfo);
			groupItem->setText(COLUMN_NAME, groupName);
			groupItem->setData(COLUMN_DATA, ROLE_SORT, ((groupInfo->flag & RS_GROUP_FLAG_STANDARD) ? "0 " : "1 ") + groupName);

			if (listModus == MODUS_CHECK) {
				groupItem->setFlags(Qt::ItemIsUserCheckable | groupItem->flags());
				groupItem->setCheckState(0, Qt::Unchecked);
			}

			if (std::find(groupIdsSelected.begin(), groupIdsSelected.end(), groupInfo->id) != groupIdsSelected.end()) {
				setSelected(listModus, groupItem, true);
			}
		}

		// iterate through ssl ids
		for (sslIt = sslIds.begin(); sslIt != sslIds.end(); sslIt++) {
			RsPeerDetails detail;
			if (!rsPeers->getPeerDetails(*sslIt, detail)) {
				continue; /* BAD */
			}

			if (groupInfo) {
				// we fill a group, check if gpg id is assigned
				if (std::find(groupInfo->peerIds.begin(), groupInfo->peerIds.end(), detail.gpg_id) == groupInfo->peerIds.end()) {
					continue;
				}
			} else {
				// we fill the not assigned ssl ids
				if (std::find(filledSslIds.begin(), filledSslIds.end(), *sslIt) != filledSslIds.end()) {
					continue;
				}
			}

			// add equal too, its no problem
			filledSslIds.push_back(detail.id);

			// make a widget per friend
			QTreeWidgetItem *item = new RSTreeWidgetItem(compareRole, IDTYPE_SSL);

			QString name = PeerDefs::nameWithLocation(detail);
			item->setText(COLUMN_NAME, name);

			int state = RS_STATUS_OFFLINE;
			if (detail.state & RS_PEER_STATE_CONNECTED) {
				std::list<StatusInfo>::iterator it;
				for (it = statusInfo.begin(); it != statusInfo.end() ; it++) {
					if (it->id == detail.id) {
						state = it->status;
						break;
					}
				}
			}

			if (state != (int) RS_STATUS_OFFLINE) {
				item->setTextColor(COLUMN_NAME, COLOR_CONNECT);
			}

			item->setIcon(COLUMN_NAME, QIcon(StatusDefs::imageUser(state)));
			item->setData(COLUMN_DATA, ROLE_ID, QString::fromStdString(detail.id));
			item->setData(COLUMN_DATA, ROLE_SORT, "2 " + name);

			if (listModus == MODUS_CHECK) {
				item->setFlags(Qt::ItemIsUserCheckable | item->flags());
				item->setCheckState(0, Qt::Unchecked);
			}

			// add to the list
			if (groupItem) {
				groupItem->addChild(item);
			} else {
				ui->friendList->addTopLevelItem(item);
			}

			if (std::find(sslIdsSelected.begin(), sslIdsSelected.end(), detail.id) != sslIdsSelected.end()) {
				setSelected(listModus, item, true);
			}
		}

		if (groupIt != groupInfoList.end()) {
			groupIt++;
		} else {
			// all done
			break;
		}
	}

	if (ui->filterLineEdit->text().isEmpty() == false) {
		filterItems(ui->filterLineEdit->text());
	}

	ui->friendList->update(); /* update display */

	emit contentChanged();
}

void FriendSelectionWidget::peerStatusChanged(const QString& peerId, int status)
{
	QTreeWidgetItemIterator itemIterator(ui->friendList);
	QTreeWidgetItem *item;
	while ((item = *itemIterator) != NULL) {
		itemIterator++;

		if (item->data(COLUMN_DATA, ROLE_ID).toString() == peerId) {
			QColor color;
			if (status != (int) RS_STATUS_OFFLINE) {
				color = COLOR_CONNECT;
			}

			item->setTextColor(COLUMN_NAME, color);
			item->setIcon(COLUMN_NAME, QIcon(StatusDefs::imageUser(status)));
			//break; no break, friend can assigned to groups more than one
		}
	}
}

void FriendSelectionWidget::contextMenuRequested(const QPoint &pos)
{
	emit customContextMenuRequested(pos);
}

void FriendSelectionWidget::itemDoubleClicked(QTreeWidgetItem *item, int /*column*/)
{
	if (!item) {
		return;
	}

	emit doubleClicked((IdType) item->type(), item->data(COLUMN_DATA, ROLE_ID).toString());
}

void FriendSelectionWidget::itemChanged(QTreeWidgetItem *item, int column)
{
	if (inItemChanged) {
		return;
	}

	if (listModus != MODUS_CHECK) {
		return;
	}

	if (column != COLUMN_CHECK) {
		return;
	}

	if (!showGroups) {
		return;
	}

	inItemChanged = true;

	switch ((IdType) item->type()) {
	case IDTYPE_NONE:
		break;
	case IDTYPE_GROUP:
		{
			bool selected = isSelected(listModus, item);

			int childCount = item->childCount();
			for (int i = 0; i < childCount; ++i) {
				setSelected(listModus, item->child(i), selected);
			}
		}
		break;
	case IDTYPE_SSL:
		{
			QTreeWidgetItem *itemParent = item->parent();
			if (itemParent == NULL) {
				break;
			}

			int childCount = itemParent->childCount();
			bool foundUnselected = false;
			for (int index = 0; index < childCount; ++index) {
				if (!isSelected(listModus, itemParent->child(index))) {
					foundUnselected = true;
					break;
				}
			}
			setSelected(listModus, itemParent, !foundUnselected);
		}
		break;
	}

	inItemChanged = false;
}

void FriendSelectionWidget::filterItems(const QString& text)
{
	int count = ui->friendList->topLevelItemCount();
	for (int index = 0; index < count; index++) {
		filterItem(ui->friendList->topLevelItem(index), text);
	}
}

bool FriendSelectionWidget::filterItem(QTreeWidgetItem *item, const QString &text)
{
	bool visible = true;

	if (text.isEmpty() == false) {
		if (item->text(0).contains(text, Qt::CaseInsensitive) == false) {
			visible = false;
		}
	}

	int visibleChildCount = 0;
	int count = item->childCount();
	for (int index = 0; index < count; index++) {
		if (filterItem(item->child(index), text)) {
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

int FriendSelectionWidget::selectedItemCount()
{
	return ui->friendList->selectedItems().count();
}

QString FriendSelectionWidget::selectedId(IdType &idType)
{
	QTreeWidgetItem *item = ui->friendList->currentItem();
	if (!item) {
		idType = IDTYPE_NONE;
		return "";
	}

	idType = (IdType) item->type();
	return item->data(COLUMN_DATA, ROLE_ID).toString();
}

void FriendSelectionWidget::selectedIds(IdType idType, std::list<std::string> &ids, bool onlyDirectSelected)
{
	QTreeWidgetItemIterator itemIterator(ui->friendList);
	QTreeWidgetItem *item;
	while ((item = *itemIterator) != NULL) {
		itemIterator++;

		std::string id;

		switch ((IdType) item->type()) {
		case IDTYPE_NONE:
			break;
		case IDTYPE_GROUP:
			if (idType == IDTYPE_GROUP) {
				if (isSelected(listModus, item)) {
					id = item->data(COLUMN_DATA, ROLE_ID).toString().toStdString();
				}
			}
			break;
		case IDTYPE_SSL:
			if (idType == IDTYPE_SSL) {
				if (isSelected(listModus, item)) {
					id = item->data(COLUMN_DATA, ROLE_ID).toString().toStdString();
				} else {
					if (!onlyDirectSelected) {
						QTreeWidgetItem *itemParent = item->parent();
						if (itemParent && isSelected(listModus, itemParent)) {
							id = item->data(COLUMN_DATA, ROLE_ID).toString().toStdString();
						}
					}
				}
			}
			break;
		}
		if (!id.empty() && std::find(ids.begin(), ids.end(), id) == ids.end()) {
			ids.push_back(id);
		}
	}
}

void FriendSelectionWidget::setSelectedIds(IdType idType, const std::list<std::string> &ids, bool add)
{
	QTreeWidgetItemIterator itemIterator(ui->friendList);
	QTreeWidgetItem *item;
	while ((item = *itemIterator) != NULL) {
		itemIterator++;

		std::string id = item->data(COLUMN_DATA, ROLE_ID).toString().toStdString();
		IdType itemType = (IdType) item->type();

		switch (itemType) {
		case IDTYPE_NONE:
			break;
		case IDTYPE_GROUP:
		case IDTYPE_SSL:
			if (idType == itemType) {
				if (std::find(ids.begin(), ids.end(), id) != ids.end()) {
					setSelected(listModus, item, true);
					break;
				}
			}
			if (!add) {
				setSelected(listModus, item, false);
			}
			break;
		}
	}
}
