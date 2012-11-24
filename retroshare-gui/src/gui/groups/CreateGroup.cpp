/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006 - 2010 RetroShare Team
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

#include <QPushButton>

#include <retroshare/rspeers.h>

#include "CreateGroup.h"
#include "gui/common/GroupDefs.h"
#include "gui/settings/rsharesettings.h"
#include "util/misc.h"

/** Default constructor */
CreateGroup::CreateGroup(const std::string &groupId, QWidget *parent)
  : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
{
	/* Invoke Qt Designer generated QObject setup routine */
	ui.setupUi(this);

	Settings->loadWidgetInformation(this);

	mIsStandard = false;

	ui.headerFrame->setHeaderImage(QPixmap(":/images/user/add_group256.png"));

	mGroupId = groupId;

	/* Initialize friends list */
	ui.friendList->setHeaderText(tr("Friends"));
	ui.friendList->setModus(FriendSelectionWidget::MODUS_CHECK);
	ui.friendList->setShowType(FriendSelectionWidget::SHOW_GROUP | FriendSelectionWidget::SHOW_GPG);
	ui.friendList->start();

	if (mGroupId.empty() == false) {
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
			ui.headerFrame->setHeaderText(tr("Edit Group"));

			ui.groupName->setDisabled(mIsStandard);

			ui.friendList->setSelectedGpgIds(groupInfo.peerIds, false);
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
	for (groupIt = groupInfoList.begin(); groupIt != groupInfoList.end(); groupIt++) {
		if (mGroupId.empty() || groupIt->id != mGroupId) {
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

	if (mGroupId.empty()) {
		// add new group
		groupInfo.name = misc::removeNewLine(ui.groupName->text()).toUtf8().constData();
		if (!rsPeers->addGroup(groupInfo)) {
			return;
		}
	} else {
		if (rsPeers->getGroupInfo(mGroupId, groupInfo) == true) {
			if (!mIsStandard) {
				groupInfo.name = misc::removeNewLine(ui.groupName->text()).toUtf8().constData();
				if (!rsPeers->editGroup(mGroupId, groupInfo)) {
					return;
				}
			}
		}
	}

	std::list<std::string> gpgIds;
	ui.friendList->selectedGpgIds(gpgIds, true);

	std::list<std::string>::iterator it;
	for (it = groupInfo.peerIds.begin(); it != groupInfo.peerIds.end(); ++it) {
		std::list<std::string>::iterator gpgIt = std::find(gpgIds.begin(), gpgIds.end(), *it);
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
