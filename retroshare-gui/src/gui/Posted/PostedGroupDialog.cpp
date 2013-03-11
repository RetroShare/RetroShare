/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008 Robert Fernie
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

#include "PostedGroupDialog.h"

#include <retroshare/rswiki.h>
#include <iostream>

const uint32_t PostedCreateEnabledFlags = ( // GXS_GROUP_FLAGS_ICON        |
                          GXS_GROUP_FLAGS_DESCRIPTION   |
                          GXS_GROUP_FLAGS_DISTRIBUTION  |
                          // GXS_GROUP_FLAGS_PUBLISHSIGN   |
                          GXS_GROUP_FLAGS_SHAREKEYS     |
                          // GXS_GROUP_FLAGS_PERSONALSIGN  |
                          // GXS_GROUP_FLAGS_COMMENTS      |
                          0);

uint32_t PostedCreateDefaultsFlags = ( GXS_GROUP_DEFAULTS_DISTRIB_PUBLIC    |
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


PostedGroupDialog::PostedGroupDialog(TokenQueue *tokenQueue, QWidget *parent)
	:GxsGroupDialog(tokenQueue, PostedCreateEnabledFlags, PostedCreateDefaultsFlags, parent)
{


}

PostedGroupDialog::PostedGroupDialog(const RsPostedGroup &group, QWidget *parent)
		:GxsGroupDialog(group.mMeta, MODE_SHOW, parent)
{


}

void PostedGroupDialog::initUi()
{
	switch (mode())
	{
	case MODE_CREATE:
		setUiText(UITYPE_SERVICE_HEADER, tr("Create New Posted Topic"));
		break;
	case MODE_SHOW:
		setUiText(UITYPE_SERVICE_HEADER, tr("Posted Topic"));
		break;
	case MODE_EDIT:
		setUiText(UITYPE_SERVICE_HEADER, tr("Edit Posted Topic"));
		break;
	}

	setUiText(UITYPE_KEY_SHARE_CHECKBOX, tr("Add Topic Admins"));
	setUiText(UITYPE_CONTACTS_DOCK, tr("Select Topic Admins"));
}

QPixmap PostedGroupDialog::serviceImage()
{
	return QPixmap(":/images/resource-group_64.png");
}

bool PostedGroupDialog::service_CreateGroup(uint32_t &token, const RsGroupMetaData &meta)
{
	// Specific Function.
	RsPostedGroup grp;
	grp.mMeta = meta;
        grp.mDescription = getDescription().toStdString();
	std::cerr << "PostedGroupDialog::service_CreateGroup() storing to Queue";
	std::cerr << std::endl;

	rsPosted->createGroup(token, grp);

	return true;
}
