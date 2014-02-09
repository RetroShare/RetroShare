/*
 * Retroshare Gxs Support
 *
 * Copyright 2012-2013 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

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

WikiGroupDialog::WikiGroupDialog(TokenQueue *tokenQueue, QWidget *parent)
	:GxsGroupDialog(tokenQueue, WikiCreateEnabledFlags, WikiCreateDefaultsFlags, parent)
{
}

WikiGroupDialog::WikiGroupDialog(TokenQueue *tokenExternalQueue, RsTokenService *tokenService, Mode mode, RsGxsGroupId groupId, QWidget *parent)
:GxsGroupDialog(tokenExternalQueue, tokenService, mode, groupId, WikiEditEnabledFlags, WikiEditDefaultsFlags, parent)
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
	return QPixmap(":/images/resource-group-new_64.png");
}

bool WikiGroupDialog::service_CreateGroup(uint32_t &token, const RsGroupMetaData &meta)
{
	// Specific Function.
	RsWikiCollection grp;
	grp.mMeta = meta;
	//grp.mDescription = std::string(desc.toUtf8());
	std::cerr << "WikiGroupDialog::service_CreateGroup() storing to Queue";
	std::cerr << std::endl;

	rsWiki->submitCollection(token, grp);

	return true;
}

bool WikiGroupDialog::service_EditGroup(uint32_t &token, 
			RsGxsGroupUpdateMeta &updateMeta,
			RsGroupMetaData &editedMeta)
{
	RsWikiCollection grp;
	grp.mMeta = editedMeta;

	std::cerr << "WikiGroupDialog::service_EditGroup() submitting changes.";
	std::cerr << std::endl;

	rsWiki->updateCollection(token, updateMeta, grp);
	return true;
}


bool WikiGroupDialog::service_loadGroup(uint32_t token, Mode mode, RsGroupMetaData& groupMetaData)
{
        std::cerr << "WikiGroupDialog::service_loadGroup(" << token << ")";
        std::cerr << std::endl;

        std::vector<RsWikiCollection> groups;
        if (!rsWiki->getCollections(token, groups))
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

        std::cerr << "WikisGroupDialog::service_loadGroup() Unfinished Loading";
        std::cerr << std::endl;

	groupMetaData = groups[0].mMeta;
	return true;
}

