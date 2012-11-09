#include <retroshare/rspeers.h>
#include "GroupSelectionBox.h"
#include "GroupDefs.h"
#include "gui/notifyqt.h"

#include <algorithm>

#define ROLE_ID Qt::UserRole

GroupSelectionBox::GroupSelectionBox(QWidget *parent)
	: QListWidget(parent)
{
	setSelectionMode(QAbstractItemView::SingleSelection);

	connect(NotifyQt::getInstance(), SIGNAL(groupsChanged(int)), this, SLOT(fillGroups()));

	// Fill with available groups
	fillGroups();
}

void GroupSelectionBox::fillGroups()
{
	std::list<std::string> selectedIds;
	selectedGroupIds(selectedIds);

	clear();

	std::list<RsGroupInfo> groupIds;
	rsPeers->getGroupInfoList(groupIds);

	for (std::list<RsGroupInfo>::const_iterator it(groupIds.begin()); it != groupIds.end(); ++it) {
		QListWidgetItem *item = new QListWidgetItem(GroupDefs::name(*it));
		item->setData(ROLE_ID, QString::fromStdString(it->id));
		item->setBackgroundColor(QColor(183,236,181));
		addItem(item);
	}

	setSelectedGroupIds(selectedIds);
}

void GroupSelectionBox::selectedGroupIds(std::list<std::string> &groupIds) const
{
	int itemCount = count();

	for (int i = 0; i < itemCount; ++i) {
		QListWidgetItem *listItem = item(i);
		if (listItem->checkState() == Qt::Checked) {
			groupIds.push_back(item(i)->data(ROLE_ID).toString().toStdString());
			std::cerr << "Addign selected item " << groupIds.back() << std::endl;
		}
	}
}

void GroupSelectionBox::setSelectedGroupIds(const std::list<std::string>& groupIds)
{
	int itemCount = count();

	for (int i = 0; i < itemCount; ++i) {
		QListWidgetItem *listItem = item(i);

		if (std::find(groupIds.begin(), groupIds.end(), listItem->data(ROLE_ID).toString().toStdString()) != groupIds.end()) {
			listItem->setCheckState(Qt::Checked);
		} else {
			listItem->setCheckState(Qt::Unchecked);
		}
	}
}

void GroupSelectionBox::selectedGroupNames(QList<QString> &groupNames) const
{
	int itemCount = count();

	for (int i = 0; i < itemCount; ++i) {
		QListWidgetItem *listItem = item(i);
		if (listItem->checkState() == Qt::Checked) {
			groupNames.push_back(item(i)->text());
			std::cerr << "Addign selected item " << groupNames.back().toUtf8().constData() << std::endl;
		}
	}
}
