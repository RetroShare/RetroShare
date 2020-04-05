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

#include <retroshare/rswiki.h>
#include <iostream>

const uint32_t WikiCreateEnabledFlags = ( 
			  GXS_GROUP_FLAGS_NAME        |
			  // GXS_GROUP_FLAGS_ICON        |
                          GXS_GROUP_FLAGS_DESCRIPTION   |
                          GXS_GROUP_FLAGS_DISTRIBUTION  |
                          // GXS_GROUP_FLAGS_PUBLISHSIGN   |
                          GXS_GROUP_FLAGS_SHAREKEYS     |
                          // GXS_GROUP_FLAGS_PERSONALSIGN  |
                          // GXS_GROUP_FLAGS_COMMENTS      |
                          0);

uint32_t WikiCreateDefaultsFlags = ( GXS_GROUP_DEFAULTS_DISTRIB_PUBLIC    |
                           //GXS_GROUP_DEFAULTS_DISTRIB_GROUP        |
                           //GXS_GROUP_DEFAULTS_DISTRIB_LOCAL        |

                           GXS_GROUP_DEFAULTS_PUBLISH_OPEN         |
                           //GXS_GROUP_DEFAULTS_PUBLISH_THREADS      |
                           //GXS_GROUP_DEFAULTS_PUBLISH_REQUIRED     |
                           //GXS_GROUP_DEFAULTS_PUBLISH_ENCRYPTED    |

                           //GXS_GROUP_DEFAULTS_PERSONAL_GPG         |
                           GXS_GROUP_DEFAULTS_PERSONAL_REQUIRED    |
                           //GXS_GROUP_DEFAULTS_PERSONAL_IFNOPUB     |

                           //GXS_GROUP_DEFAULTS_COMMENTS_YES         |
                           GXS_GROUP_DEFAULTS_COMMENTS_NO          |
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
}

QPixmap WikiGroupDialog::serviceImage()
{
	return QPixmap(":/icons/png/wiki.png");
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

