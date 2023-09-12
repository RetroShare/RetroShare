/*******************************************************************************
 * retroshare-gui/src/gui/Groups/CreateGroup.cpp                               *
 *                                                                             *
 * Copyright 2006-2010 by Retroshare Team <retroshare.project@gmail.com>       *
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

#include <QPushButton>

#include <retroshare/rspeers.h>

#include "CreateGroup.h"
#include "gui/common/GroupDefs.h"
#include "gui/settings/rsharesettings.h"
#include "util/misc.h"
#include "gui/common/FilesDefs.h"

#include <algorithm>

/** Default constructor */
CreateGroup::CreateGroup(const RsNodeGroupId &groupId, QWidget *parent)
  : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
{
	/* Invoke Qt Designer generated QObject setup routine */
	ui.setupUi(this);

	Settings->loadWidgetInformation(this);

	mIsStandard = false;

    ui.headerFrame->setHeaderImage(FilesDefs::getPixmapFromQtResourcePath(":/icons/groups/colored.svg"));

	mGroupId = groupId;

	if (!mGroupId.isNull())
		ui.groupId_LE->setText(QString::fromStdString(mGroupId.toStdString()));
	else
		ui.groupId_LE->setText(tr("To be defined"));

	/* Initialize friends list */
	ui.friendList->setHeaderText(tr("Friends"));
	ui.friendList->setModus(FriendSelectionWidget::MODUS_CHECK);
	ui.friendList->setShowType(FriendSelectionWidget::SHOW_GROUP | FriendSelectionWidget::SHOW_GPG);
	ui.friendList->start();

    if (!mGroupId.isNull()) {
		/* edit exisiting group */
		RsGroupInfo groupInfo;
		if (rsPeers->getGroupInfo(mGroupId, groupInfo)) {
			mIsStandard = (groupInfo.flag & RS_GROUP_FLAG_STANDARD);

			if (mIsStandard) {
				ui.groupName->setText(GroupDefs::name(groupInfo));
			} else {
				ui.groupName->setText(misc::removeNewLine(groupInfo.name));
			}

			setWindowTitle(tr("Edit Group"));
            ui.headerFrame->setHeaderImage(FilesDefs::getPixmapFromQtResourcePath(":/icons/groups/colored.svg"));
			ui.headerFrame->setHeaderText(tr("Edit Group"));

			ui.groupName->setDisabled(mIsStandard);

            ui.friendList->setSelectedIds<RsPgpId,FriendSelectionWidget::IDTYPE_GPG>(groupInfo.peerIds, false);
		} else {
			/* Group not found, create new */
			mGroupId.clear();
		}
	} else {
		ui.headerFrame->setHeaderText(tr("Create a Group"));
	}

	std::list<RsGroupInfo> groupInfoList;
	rsPeers->getGroupInfoList(groupInfoList);

	std::list<RsGroupInfo>::iterator groupIt;
	for (groupIt = groupInfoList.begin(); groupIt != groupInfoList.end(); ++groupIt) {
        if (mGroupId.isNull() || groupIt->id != mGroupId) {
			mUsedGroupNames.append(GroupDefs::name(*groupIt));
		}
	}

	connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(changeGroup()));
	connect(ui.buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
	connect(ui.groupName, SIGNAL(textChanged(QString)), this, SLOT(groupNameChanged(QString)));

	groupNameChanged(ui.groupName->text());
}

/** Destructor. */
CreateGroup::~CreateGroup()
{
	Settings->saveWidgetInformation(this);
}

void CreateGroup::groupNameChanged(QString text)
{
	if (text.isEmpty() || mUsedGroupNames.contains(misc::removeNewLine(text))) {
        ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	} else {
        ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
	}
}

void CreateGroup::changeGroup()
{
	RsGroupInfo groupInfo;

    if (mGroupId.isNull())
    {
		// add new group
		groupInfo.name = misc::removeNewLine(ui.groupName->text()).toUtf8().constData();

        if (!rsPeers->addGroup(groupInfo))
			return;

    }
    else
    {
        if (rsPeers->getGroupInfo(mGroupId, groupInfo))
        {
			if (!mIsStandard) {
				groupInfo.name = misc::removeNewLine(ui.groupName->text()).toUtf8().constData();
				if (!rsPeers->editGroup(mGroupId, groupInfo)) {
					return;
				}
			}
		}
	}

    std::set<RsPgpId> gpgIds;
    ui.friendList->selectedIds<RsPgpId,FriendSelectionWidget::IDTYPE_GPG>(gpgIds, true);

    std::set<RsPgpId>::iterator it;
	for (it = groupInfo.peerIds.begin(); it != groupInfo.peerIds.end(); ++it) {
        std::set<RsPgpId>::iterator gpgIt = std::find(gpgIds.begin(), gpgIds.end(), *it);
		if (gpgIt == gpgIds.end()) {
			rsPeers->assignPeerToGroup(groupInfo.id, *it, false);
			continue;
		}

		gpgIds.erase(gpgIt);
	}

	for (it = gpgIds.begin(); it != gpgIds.end(); ++it) {
		rsPeers->assignPeerToGroup(groupInfo.id, *it, true);
	}

	accept();
}
