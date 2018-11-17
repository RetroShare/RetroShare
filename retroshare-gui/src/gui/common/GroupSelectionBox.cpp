/*******************************************************************************
 * gui/common/GroupSelectionBox.cpp                                            *
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

#include <QLayout>
#include <QDialogButtonBox>
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
    std::list<RsNodeGroupId> selectedIds;
	selectedGroupIds(selectedIds);

	clear();

	std::list<RsGroupInfo> groupIds;
	rsPeers->getGroupInfoList(groupIds);

	for (std::list<RsGroupInfo>::const_iterator it(groupIds.begin()); it != groupIds.end(); ++it) {
		QListWidgetItem *item = new QListWidgetItem(GroupDefs::name(*it));
        item->setData(ROLE_ID, QString::fromStdString(it->id.toStdString()));
		item->setBackgroundColor(QColor(183,236,181));
		addItem(item);
	}

	setSelectedGroupIds(selectedIds);
}

void GroupSelectionBox::selectedGroupIds(std::list<RsNodeGroupId> &groupIds) const
{
	int itemCount = count();

	for (int i = 0; i < itemCount; ++i) {
		QListWidgetItem *listItem = item(i);
		if (listItem->checkState() == Qt::Checked) {
            groupIds.push_back(RsNodeGroupId(item(i)->data(ROLE_ID).toString().toStdString()));
			std::cerr << "Adding selected item " << groupIds.back() << std::endl;
		}
	}
}

void GroupSelectionBox::setSelectedGroupIds(const std::list<RsNodeGroupId>& groupIds)
{
	int itemCount = count();

	for (int i = 0; i < itemCount; ++i) {
		QListWidgetItem *listItem = item(i);

        if (std::find(groupIds.begin(), groupIds.end(), RsNodeGroupId(listItem->data(ROLE_ID).toString().toStdString())) != groupIds.end()) {
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
			std::cerr << "Adding selected item " << groupNames.back().toUtf8().constData() << std::endl;
		}
	}
}

std::list<RsNodeGroupId> GroupSelectionDialog::selectGroups(const std::list<RsNodeGroupId>& default_groups)
{
    GroupSelectionDialog gsd(NULL) ;

    gsd.mBox->setSelectedGroupIds(default_groups) ;

    gsd.exec();

    std::list<RsNodeGroupId> selected_groups ;
    gsd.mBox->selectedGroupIds(selected_groups);

    return selected_groups ;
}

GroupSelectionDialog::~GroupSelectionDialog()
{
    delete mBox ;
}
GroupSelectionDialog::GroupSelectionDialog(QWidget* /*parent*/)
{
    mBox = new GroupSelectionBox(this) ;

    QLayout *l = new QVBoxLayout ;
    setLayout(l) ;

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    l->addWidget(mBox) ;
    l->addWidget(buttonBox) ;
    l->update() ;
}

