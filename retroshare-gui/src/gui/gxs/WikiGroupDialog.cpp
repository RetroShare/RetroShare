/*******************************************************************************
 * retroshare-gui/src/gui/gxs/RsGxsUpdateBroadcastWidget.cpp                   *
 *                                                                             *
 * Copyright 2012-2013 by Robert Fernie     <retroshare.project@gmail.com>     *
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

#include "WikiGroupDialog.h"
#include "gui/common/FilesDefs.h"
#include "gui/gxs/GxsIdChooser.h"
#include "util/qtthreadsutils.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QVBoxLayout>

#include <ctime>

#include <retroshare/rsidentity.h>
#include <retroshare/rswiki.h>
#include <retroshare/rsgxsflags.h>
#include <iostream>

const uint32_t WikiCreateEnabledFlags = ( 
			GXS_GROUP_FLAGS_NAME            |
			// GXS_GROUP_FLAGS_ICON         |
			GXS_GROUP_FLAGS_DESCRIPTION     |
			GXS_GROUP_FLAGS_DISTRIBUTION    |
			// GXS_GROUP_FLAGS_PUBLISHSIGN  |
			GXS_GROUP_FLAGS_EXTRA           |
			// GXS_GROUP_FLAGS_PERSONALSIGN |
			// GXS_GROUP_FLAGS_COMMENTS     |
			0);

uint32_t WikiCreateDefaultsFlags = ( GXS_GROUP_DEFAULTS_DISTRIB_PUBLIC    |
			//GXS_GROUP_DEFAULTS_DISTRIB_GROUP        |
			//GXS_GROUP_DEFAULTS_DISTRIB_LOCAL        |

			GXS_GROUP_DEFAULTS_PUBLISH_OPEN           |
			//GXS_GROUP_DEFAULTS_PUBLISH_THREADS      |
			//GXS_GROUP_DEFAULTS_PUBLISH_REQUIRED     |
			//GXS_GROUP_DEFAULTS_PUBLISH_ENCRYPTED    |

			//GXS_GROUP_DEFAULTS_PERSONAL_GPG         |
			GXS_GROUP_DEFAULTS_PERSONAL_REQUIRED      |
			//GXS_GROUP_DEFAULTS_PERSONAL_IFNOPUB     |

			//GXS_GROUP_DEFAULTS_COMMENTS_YES         |
			GXS_GROUP_DEFAULTS_COMMENTS_NO            |
			0);

uint32_t WikiEditDefaultsFlags = WikiCreateDefaultsFlags;
uint32_t WikiEditEnabledFlags = WikiCreateEnabledFlags;

WikiGroupDialog::WikiGroupDialog(QWidget *parent)
	:GxsGroupDialog(WikiCreateEnabledFlags, WikiCreateDefaultsFlags, parent)
{
}

WikiGroupDialog::WikiGroupDialog(Mode mode, RsGxsGroupId groupId, QWidget *parent)
:GxsGroupDialog(mode, groupId, WikiEditEnabledFlags, WikiEditDefaultsFlags, parent)
{
}

void WikiGroupDialog::initUi()
{
	switch (mode())
	{
	case MODE_CREATE:
		setUiText(UITYPE_SERVICE_HEADER, tr("Create New Wiki Group"));
		setUiText(UITYPE_BUTTONBOX_OK, tr("Create Group"));
		break;
	case MODE_SHOW:
		setUiText(UITYPE_SERVICE_HEADER, tr("Wiki Group"));
		break;
	case MODE_EDIT:
		setUiText(UITYPE_SERVICE_HEADER, tr("Edit Wiki Group"));
		setUiText(UITYPE_BUTTONBOX_OK, tr("Update Group"));
		break;
	}

	setUiText(UITYPE_KEY_SHARE_CHECKBOX, tr("Add Wiki Moderators"));
	setUiText(UITYPE_CONTACTS_DOCK, tr("Select Wiki Moderators"));

	if (!mModeratorsWidget)
	{
		mModeratorsWidget = new QWidget(this);
		auto *layout = new QVBoxLayout(mModeratorsWidget);

		mModeratorsGroup = new QGroupBox(tr("Moderators"), mModeratorsWidget);
		auto *groupLayout = new QVBoxLayout(mModeratorsGroup);

		mModeratorsList = new QListWidget(mModeratorsGroup);
		mModeratorsList->setToolTip(tr("List of users who can moderate this wiki"));
		groupLayout->addWidget(mModeratorsList);

		auto *buttonLayout = new QHBoxLayout();
		mAddModeratorButton = new QPushButton(tr("Add Moderator"), mModeratorsGroup);
		mRemoveModeratorButton = new QPushButton(tr("Remove Moderator"), mModeratorsGroup);
		buttonLayout->addWidget(mAddModeratorButton);
		buttonLayout->addWidget(mRemoveModeratorButton);
		buttonLayout->addStretch();
		groupLayout->addLayout(buttonLayout);

		layout->addWidget(mModeratorsGroup);
		injectExtraWidget(mModeratorsWidget);

		connect(mAddModeratorButton, SIGNAL(clicked()), this, SLOT(addModerator()));
		connect(mRemoveModeratorButton, SIGNAL(clicked()), this, SLOT(removeModerator()));
	}

	updateModeratorControls();
}

QPixmap WikiGroupDialog::serviceImage()
{
    return FilesDefs::getPixmapFromQtResourcePath(":/icons/png/wiki.png");
}


bool WikiGroupDialog::service_createGroup(RsGroupMetaData &meta)
{
	RsWikiCollection grp;
	grp.mMeta = meta;
	grp.mDescription = getDescription().toStdString();

	std::cerr << "WikiGroupDialog::service_CreateGroup()";
	std::cerr << std::endl;

	bool success = rsWiki->createCollection(grp);
	// createCollection should refresh groupId or data
	return success;
}

bool WikiGroupDialog::service_updateGroup(const RsGroupMetaData &editedMeta)
{
	RsWikiCollection grp;
	grp.mMeta = editedMeta;
	grp.mDescription = getDescription().toStdString();

	std::cerr << "WikiGroupDialog::service_updateGroup() submitting changes.";
	std::cerr << std::endl;

	bool success = rsWiki->updateCollection(grp);
	// updateCollection should refresh groupId or data
	return success;
}

bool WikiGroupDialog::service_loadGroup(const RsGxsGenericGroupData *data, Mode mode, QString &description)
{
	std::cerr << "WikiGroupDialog::service_loadGroup()";
	std::cerr << std::endl;

	const RsWikiCollection *pgroup = dynamic_cast<const RsWikiCollection *>(data);
	if (pgroup == nullptr)
	{
		std::cerr << "WikiGroupDialog::service_loadGroup() Error not a RsWikiCollection";
		std::cerr << std::endl;
		return false;
	}

	const RsWikiCollection &group = *pgroup;
	description = QString::fromUtf8(group.mDescription.c_str());
	mCurrentGroupId = group.mMeta.mGroupId;
	mGroupMeta = group.mMeta;
	updateModeratorControls();
	loadModerators(mCurrentGroupId);

	return true;
}

bool WikiGroupDialog::service_getGroupData(const RsGxsGroupId &groupId, RsGxsGenericGroupData *&data)
{
	std::cerr << "WikiGroupDialog::service_getGroupData(" << groupId << ")";
	std::cerr << std::endl;

	std::list<RsGxsGroupId> groupIds({groupId});
	std::vector<RsWikiCollection> groups;
	if (!rsWiki->getCollections(groupIds, groups))
	{
		std::cerr << "WikiGroupDialog::service_loadGroup() Error getting GroupData";
		std::cerr << std::endl;
		return false;
	}

	if (groups.size() != 1)
	{
		std::cerr << "WikiGroupDialog::service_loadGroup() Error Group.size() != 1";
		std::cerr << std::endl;
		return false;
	}

	data = new RsWikiCollection(groups[0]);
	return true;
}

void WikiGroupDialog::loadModerators(const RsGxsGroupId &groupId)
{
	if (!mModeratorsList || groupId.isNull())
	{
		return;
	}

	mModeratorsList->clear();

	RsThread::async([this, groupId]()
	{
		std::list<RsGxsId> moderators;
		if (!rsWiki->getModerators(groupId, moderators))
		{
			return;
		}

		RsQThreadUtils::postToObject([this, moderators]()
		{
			for (const auto &modId : moderators)
			{
				addModeratorToList(modId);
			}
		}, this);
	});
}

void WikiGroupDialog::addModeratorToList(const RsGxsId &gxsId)
{
	if (!mModeratorsList)
	{
		return;
	}

	QString name = QString::fromStdString(gxsId.toStdString());
	RsIdentityDetails details;
	if (rsIdentity->getIdDetails(gxsId, details))
	{
		name = QString::fromUtf8(details.mNickname.c_str());
	}

	QListWidgetItem *item = new QListWidgetItem(name);
	item->setData(Qt::UserRole, QString::fromStdString(gxsId.toStdString()));
	const bool isActive = rsWiki->isActiveModerator(mCurrentGroupId, gxsId, time(nullptr));
	if (!isActive)
	{
		item->setForeground(Qt::gray);
		item->setText(name + tr(" (Inactive)"));
	}

	mModeratorsList->addItem(item);
}

void WikiGroupDialog::updateModeratorControls()
{
	if (!mAddModeratorButton || !mRemoveModeratorButton)
	{
		return;
	}

	const bool isAdmin = IS_GROUP_ADMIN(mGroupMeta.mSubscribeFlags);
	const bool hasGroup = !mCurrentGroupId.isNull();
	const bool enabled = isAdmin && hasGroup;

	mAddModeratorButton->setEnabled(enabled);
	mRemoveModeratorButton->setEnabled(enabled);

	if (enabled)
	{
		/* Clear any previous tooltip when the buttons become enabled */
		mAddModeratorButton->setToolTip(QString());
		mRemoveModeratorButton->setToolTip(QString());
	}
	else
	{
		QString tooltip;

		if (!hasGroup)
		{
			tooltip = tr("Moderators can be managed after the group is created.");
		}
		else if (!isAdmin)
		{
			tooltip = tr("Only group administrators can manage moderators");
		}
		else
		{
			tooltip = tr("Moderator management is not available");
		}

		mAddModeratorButton->setToolTip(tooltip);
		mRemoveModeratorButton->setToolTip(tooltip);
	}
}

void WikiGroupDialog::addModerator()
{
	if (mCurrentGroupId.isNull())
	{
		return;
	}

	QDialog chooserDialog(this);
	chooserDialog.setWindowTitle(tr("Select Moderator"));
	QVBoxLayout *layout = new QVBoxLayout(&chooserDialog);
	GxsIdChooser *chooser = new GxsIdChooser(&chooserDialog);
	chooser->setFlags(IDCHOOSER_ID_REQUIRED);
	layout->addWidget(chooser);
	QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
		&chooserDialog);
	connect(buttons, &QDialogButtonBox::accepted, &chooserDialog, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, &chooserDialog, &QDialog::reject);
	layout->addWidget(buttons);

	if (chooserDialog.exec() != QDialog::Accepted)
	{
		return;
	}

	RsGxsId selectedId;
	const GxsIdChooser::ChosenId_Ret choice = chooser->getChosenId(selectedId);
	if (choice != GxsIdChooser::KnowId && choice != GxsIdChooser::UnKnowId)
	{
		return;
	}

	const RsGxsGroupId groupId = mCurrentGroupId;
	RsThread::async([this, groupId, selectedId]()
	{
		const bool success = rsWiki->addModerator(groupId, selectedId);
		RsQThreadUtils::postToObject([this, selectedId, success]()
		{
			if (success)
			{
				addModeratorToList(selectedId);
				QMessageBox::information(this, tr("Success"),
					tr("Moderator added successfully."));
			}
			else
			{
				QMessageBox::warning(this, tr("Error"),
					tr("Failed to add moderator."));
			}
		}, this);
	});
}

void WikiGroupDialog::removeModerator()
{
	if (!mModeratorsList || mCurrentGroupId.isNull())
	{
		return;
	}

	QListWidgetItem *item = mModeratorsList->currentItem();
	if (!item)
	{
		QMessageBox::information(this, tr("Remove Moderator"),
			tr("Please select a moderator to remove."));
		return;
	}

	const RsGxsId modId(item->data(Qt::UserRole).toString().toStdString());
	const RsGxsGroupId groupId = mCurrentGroupId;

	const int ret = QMessageBox::question(this, tr("Remove Moderator"),
		tr("Are you sure you want to remove this moderator?"),
		QMessageBox::Yes | QMessageBox::No);

	if (ret != QMessageBox::Yes)
	{
		return;
	}

	RsThread::async([this, groupId, modId]()
	{
		const bool success = rsWiki->removeModerator(groupId, modId);
		RsQThreadUtils::postToObject([this, modId, success]()
		{
			if (success)
			{
				if (mModeratorsList)
				{
					for (int i = 0; i < mModeratorsList->count(); ++i)
					{
						QListWidgetItem *listItem = mModeratorsList->item(i);
						if (!listItem)
							continue;

						const RsGxsId currentId(listItem->data(Qt::UserRole).toString().toStdString());
						if (currentId == modId)
						{
							delete mModeratorsList->takeItem(i);
							break;
						}
					}
				}
				QMessageBox::information(this, tr("Success"),
					tr("Moderator removed successfully."));
			}
			else
			{
				QMessageBox::warning(this, tr("Error"),
					tr("Failed to remove moderator."));
			}
		}, this);
	});
}
