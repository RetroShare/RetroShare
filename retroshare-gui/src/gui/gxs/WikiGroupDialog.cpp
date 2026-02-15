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
#include "gui/gxs/GxsIdTreeWidgetItem.h"
#include "gui/common/FilesDefs.h"
#include "gui/RetroShareLink.h"
#include "gui/common/FriendSelectionWidget.h"
#include "util/qtthreadsutils.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>

#include <ctime>
#include <set>
#include <vector>

#include <retroshare/rsidentity.h>
#include <retroshare/rswiki.h>
#include <retroshare/rsgxsflags.h>
#include <iostream>

const uint32_t WikiCreateEnabledFlags = ( 
			GXS_GROUP_FLAGS_NAME            |
			// GXS_GROUP_FLAGS_ICON         |
			GXS_GROUP_FLAGS_DESCRIPTION     |
			GXS_GROUP_FLAGS_DISTRIBUTION    |
			GXS_GROUP_FLAGS_ADDADMINS       |
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
	:GxsGroupDialog(WikiCreateEnabledFlags, WikiCreateDefaultsFlags, parent),
	 mEventHandlerId(0)
{
	// Register for Wiki events to catch moderator changes
	RsEventType wikiEventType = (RsEventType)rsEvents->getDynamicEventType("GXS_WIKI");
	rsEvents->registerEventsHandler(
		[this](std::shared_ptr<const RsEvent> event) {
			RsQThreadUtils::postToObject([=]() { 
				handleEvent_main_thread(event); 
			}, this );
		},
		mEventHandlerId, wikiEventType);
}

WikiGroupDialog::WikiGroupDialog(Mode mode, RsGxsGroupId groupId, QWidget *parent)
:GxsGroupDialog(mode, groupId, WikiEditEnabledFlags, WikiEditDefaultsFlags, parent),
 mEventHandlerId(0)
{
	// Register for Wiki events to catch moderator changes
	RsEventType wikiEventType = (RsEventType)rsEvents->getDynamicEventType("GXS_WIKI");
	rsEvents->registerEventsHandler(
		[this](std::shared_ptr<const RsEvent> event) {
			RsQThreadUtils::postToObject([=]() { 
				handleEvent_main_thread(event); 
			}, this );
		},
		mEventHandlerId, wikiEventType);
}

WikiGroupDialog::~WikiGroupDialog()
{
	rsEvents->unregisterEventsHandler(mEventHandlerId);
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

	if (ui.addAdmins_cb)
	{
		ui.addAdmins_cb->hide();
		ui.adminsList->hide();
		ui.filtercomboBox->hide();
	}

	if (!mModeratorsWidget)
	{
		mModeratorsWidget = new QWidget(this);
		auto *layout = new QVBoxLayout(mModeratorsWidget);

		mModeratorsGroup = new QGroupBox(tr("Moderators"), mModeratorsWidget);
		auto *groupLayout = new QVBoxLayout(mModeratorsGroup);

		mModeratorsList = new QTreeWidget(mModeratorsGroup);
		mModeratorsList->setToolTip(tr("List of users who can moderate this wiki"));
		mModeratorsList->setColumnCount(2);
		mModeratorsList->setHeaderLabels({tr("Moderator"), tr("Id")});
		mModeratorsList->setRootIsDecorated(false);
		mModeratorsList->setSelectionMode(QAbstractItemView::SingleSelection);
		mModeratorsList->setUniformRowHeights(true);
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
	std::vector<RsWikiCollection> existingGroups;
	if (rsWiki->getCollections({editedMeta.mGroupId}, existingGroups) && !existingGroups.empty())
	{
		grp = existingGroups.front();
	}

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
			updateModeratorsLabel(moderators);
			for (const auto &modId : moderators)
			{
				addModeratorToList(modId);
			}
		}, this);
	});
}

void WikiGroupDialog::updateModeratorsLabel(const std::list<RsGxsId> &moderators)
{
	QString moderatorsListString;
	RsIdentityDetails det;
	RetroShareLink link;

	for (const auto &moderatorId : moderators)
	{
		rsIdentity->getIdDetails(moderatorId, det);

		if (!moderatorsListString.isNull())
		{
			moderatorsListString += ", ";
		}

		const QString displayName = det.mNickname.empty()
			? tr("[Unknown]")
			: QString::fromUtf8(det.mNickname.c_str());

		link = RetroShareLink::createMessage(det.mId, "");
		if (link.valid())
		{
			moderatorsListString += QString("<a href=\"%1\">%2</a>")
				.arg(link.toString().toHtmlEscaped(), displayName.toHtmlEscaped());
		}
		else
		{
			moderatorsListString += displayName;
		}
	}

	if (moderatorsListString.isNull())
	{
		moderatorsListString = tr("[None]");
	}

	ui.moderatorsValueLabel->setText(moderatorsListString);
}

void WikiGroupDialog::addModeratorToList(const RsGxsId &gxsId)
{
	if (!mModeratorsList)
	{
		return;
	}

	for (int i = 0; i < mModeratorsList->topLevelItemCount(); ++i)
	{
		QTreeWidgetItem *existingItem = mModeratorsList->topLevelItem(i);
		if (!existingItem)
		{
			continue;
		}

		GxsIdRSTreeWidgetItem *gxsItem = dynamic_cast<GxsIdRSTreeWidgetItem*>(existingItem);
		if (!gxsItem)
		{
			continue;
		}

		RsGxsId existingId;
		if (!gxsItem->getId(existingId))
		{
			continue;
		}

		if (existingId == gxsId)
		{
			return;
		}
	}

	auto *item = new GxsIdRSTreeWidgetItem(nullptr, GxsIdDetails::ICON_TYPE_AVATAR, true, mModeratorsList);
	item->setId(gxsId, 0, false);
	item->setText(1, QString::fromStdString(gxsId.toStdString()));
	const bool isActive = rsWiki->isActiveModerator(mCurrentGroupId, gxsId, time(nullptr));
	if (!isActive)
	{
		item->setForeground(0, Qt::gray);
		item->setForeground(1, Qt::gray);
		item->setText(0, item->text(0) + tr(" (Inactive)"));
	}

	mModeratorsList->addTopLevelItem(item);
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
	chooserDialog.setWindowTitle(tr("Select Moderators"));
	QVBoxLayout *layout = new QVBoxLayout(&chooserDialog);

	auto *filterLayout = new QHBoxLayout();
	auto *filterLabel = new QLabel(tr("Show:"), &chooserDialog);
	auto *filterCombo = new QComboBox(&chooserDialog);
	filterCombo->addItem(tr("All People"));
	filterCombo->addItem(tr("My Contacts"));
	filterCombo->setCurrentIndex(0);
	filterLayout->addWidget(filterLabel);
	filterLayout->addWidget(filterCombo);
	filterLayout->addStretch();
	layout->addLayout(filterLayout);

	FriendSelectionWidget *chooser = new FriendSelectionWidget(&chooserDialog);
	chooser->setHeaderText(tr("Moderators:"));
	chooser->setModus(FriendSelectionWidget::MODUS_CHECK);
	chooser->setShowType(FriendSelectionWidget::SHOW_GXS);
	chooser->start();
	layout->addWidget(chooser);

	connect(filterCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
		[chooser](int index)
		{
			switch (index)
			{
			default:
			case 0:
				chooser->setShowType(FriendSelectionWidget::SHOW_GXS);
				break;
			case 1:
				chooser->setShowType(FriendSelectionWidget::SHOW_CONTACTS);
				break;
			}
		});

	std::set<RsGxsId> existingModerators;
	if (mModeratorsList)
	{
		for (int i = 0; i < mModeratorsList->topLevelItemCount(); ++i)
		{
			QTreeWidgetItem *item = mModeratorsList->topLevelItem(i);
			if (!item)
			{
				continue;
			}

			GxsIdRSTreeWidgetItem *gxsItem = dynamic_cast<GxsIdRSTreeWidgetItem*>(item);
			if (!gxsItem)
			{
				continue;
			}

			RsGxsId id;
			if (gxsItem->getId(id))
			{
				existingModerators.insert(id);
			}
		}
	}
	chooser->setSelectedIds<RsGxsId,FriendSelectionWidget::IDTYPE_GXS>(existingModerators, false);

	QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
		&chooserDialog);
	connect(buttons, &QDialogButtonBox::accepted, &chooserDialog, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, &chooserDialog, &QDialog::reject);
	layout->addWidget(buttons);

	if (chooserDialog.exec() != QDialog::Accepted)
	{
		return;
	}

	std::set<RsGxsId> selectedIds;
	chooser->selectedIds<RsGxsId,FriendSelectionWidget::IDTYPE_GXS>(selectedIds, true);

	std::set<RsGxsId> newModerators;
	for (const auto &selectedId : selectedIds)
	{
		if (existingModerators.find(selectedId) == existingModerators.end())
		{
			newModerators.insert(selectedId);
		}
	}

	if (newModerators.empty())
	{
		QMessageBox::information(this, tr("Add Moderators"),
			tr("No new moderators selected."));
		return;
	}

	const RsGxsGroupId groupId = mCurrentGroupId;
	RsThread::async([this, groupId, newModerators]()
	{
		std::vector<RsGxsId> addedModerators;
		std::vector<RsGxsId> failedModerators;

		for (const auto &moderatorId : newModerators)
		{
			if (rsWiki->addModerator(groupId, moderatorId))
			{
				addedModerators.push_back(moderatorId);
			}
			else
			{
				failedModerators.push_back(moderatorId);
			}
		}

		RsQThreadUtils::postToObject([this, addedModerators, failedModerators]()
		{
			// The group update happens asynchronously in RsGenExchange, 
			// and p3wiki::notifyChanges() will send an UPDATED_COLLECTION
			// event when the change is actually committed. The moderator list
			// will be automatically refreshed via handleEvent_main_thread().
			
			if (addedModerators.empty())
			{
				QMessageBox::warning(this, tr("Error"),
					tr("Failed to submit moderator add requests."));
			}
			else if (!failedModerators.empty())
			{
				QMessageBox::warning(this, tr("Warning"),
					tr("Some moderator requests could not be submitted."));
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

	QTreeWidgetItem *item = mModeratorsList->currentItem();
	GxsIdRSTreeWidgetItem *gxsItem = dynamic_cast<GxsIdRSTreeWidgetItem*>(item);
	if (!gxsItem)
	{
		QMessageBox::information(this, tr("Remove Moderator"),
			tr("Please select a moderator to remove."));
		return;
	}

	RsGxsId modId;
	if (!gxsItem->getId(modId))
	{
		QMessageBox::information(this, tr("Remove Moderator"),
			tr("Please select a moderator to remove."));
		return;
	}
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
		RsQThreadUtils::postToObject([this, success]()
		{
			// The group update happens asynchronously in RsGenExchange, 
			// and p3wiki::notifyChanges() will send an UPDATED_COLLECTION
			// event when the change is actually committed. The moderator list
			// will be automatically refreshed via handleEvent_main_thread().
			
			if (!success)
			{
				QMessageBox::warning(this, tr("Error"),
					tr("Failed to submit moderator remove request."));
			}
		}, this);
	});
}

void WikiGroupDialog::handleEvent_main_thread(std::shared_ptr<const RsEvent> event)
{
	// Cast to the specific Wiki event
	const RsGxsWikiEvent *e = dynamic_cast<const RsGxsWikiEvent*>(event.get());

	if (e)
	{
		// Check if the event is for the group we're currently editing
		if (e->mWikiGroupId == mCurrentGroupId)
		{
			switch (e->mWikiEventCode)
			{
				case RsWikiEventCode::UPDATED_COLLECTION:
					// Reload moderators when the group is updated
					loadModerators(mCurrentGroupId);
					break;
				default:
					break;
			}
		}
	}
}
