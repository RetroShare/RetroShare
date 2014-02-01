/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2013 Robert Fernie
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

#include "GxsChannelGroupDialog.h"

#include <retroshare/rsgxschannels.h>
#include <iostream>

// To start with we only have open forums - with distribution controls.

const uint32_t ChannelCreateEnabledFlags = ( 
			GXS_GROUP_FLAGS_NAME        |
			GXS_GROUP_FLAGS_ICON        |
			GXS_GROUP_FLAGS_DESCRIPTION   |
			GXS_GROUP_FLAGS_DISTRIBUTION  |
			// GXS_GROUP_FLAGS_PUBLISHSIGN   |
			GXS_GROUP_FLAGS_SHAREKEYS     |
			// GXS_GROUP_FLAGS_PERSONALSIGN  |
			GXS_GROUP_FLAGS_COMMENTS      |
			0);
			
const uint32_t ChannelCreateDefaultsFlags = ( GXS_GROUP_DEFAULTS_DISTRIB_PUBLIC    |
			//GXS_GROUP_DEFAULTS_DISTRIB_GROUP        |
			//GXS_GROUP_DEFAULTS_DISTRIB_LOCAL        |

			GXS_GROUP_DEFAULTS_PUBLISH_OPEN         |
			//GXS_GROUP_DEFAULTS_PUBLISH_THREADS      |
			//GXS_GROUP_DEFAULTS_PUBLISH_REQUIRED     |
			//GXS_GROUP_DEFAULTS_PUBLISH_ENCRYPTED    |

			//GXS_GROUP_DEFAULTS_PERSONAL_GPG         |
			GXS_GROUP_DEFAULTS_PERSONAL_REQUIRED    |
			//GXS_GROUP_DEFAULTS_PERSONAL_IFNOPUB     |

			GXS_GROUP_DEFAULTS_COMMENTS_YES         |
			//GXS_GROUP_DEFAULTS_COMMENTS_NO          |
			0);

const uint32_t ChannelEditEnabledFlags = ChannelCreateEnabledFlags;
const uint32_t ChannelEditDefaultsFlags = ChannelCreateDefaultsFlags;

GxsChannelGroupDialog::GxsChannelGroupDialog(TokenQueue *tokenQueue, QWidget *parent)
	:GxsGroupDialog(tokenQueue, ChannelCreateEnabledFlags, ChannelCreateDefaultsFlags, parent)
{
}

GxsChannelGroupDialog::GxsChannelGroupDialog(TokenQueue *tokenExternalQueue, RsTokenService *tokenService, Mode mode, RsGxsGroupId groupId, uint32_t enableFlags, uint32_t defaultFlags, QWidget *parent)
:GxsGroupDialog(tokenExternalQueue, tokenService, mode, groupId, ChannelEditEnabledFlags, ChannelEditDefaultsFlags, parent)
{
}

void GxsChannelGroupDialog::initUi()
{
	switch (mode())
	{
	case MODE_CREATE:
		setUiText(UITYPE_SERVICE_HEADER, tr("Create New Channel"));
		break;
	case MODE_SHOW:
		setUiText(UITYPE_SERVICE_HEADER, tr("Channel"));
		break;
	case MODE_EDIT:
		setUiText(UITYPE_SERVICE_HEADER, tr("Edit Channel"));
		break;
	}
	
	setUiText(UITYPE_KEY_SHARE_CHECKBOX, tr("Add Channel Admins"));
	setUiText(UITYPE_CONTACTS_DOCK, tr("Select Channel Admins"));
	setUiText(UITYPE_BUTTONBOX_OK, tr("Create Channel"));
}

QPixmap GxsChannelGroupDialog::serviceImage()
{
	return QPixmap(":/images/add_channel64.png");
}

bool GxsChannelGroupDialog::service_CreateGroup(uint32_t &token, const RsGroupMetaData &meta)
{
	// Specific Function.
	RsGxsChannelGroup grp;
	grp.mMeta = meta;
	//grp.mDescription = std::string(desc.toUtf8());

	rsGxsChannels->createGroup(token, grp);
	return true;
}



bool GxsChannelGroupDialog::service_EditGroup(uint32_t &token, RsGxsGroupUpdateMeta &updateMeta)
{
	std::cerr << "GxsChannelGroupDialog::service_EditGroup() UNFINISHED";
	std::cerr << std::endl;

	return false;
}

