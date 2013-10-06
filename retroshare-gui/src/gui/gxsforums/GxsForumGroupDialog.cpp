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

#include "GxsForumGroupDialog.h"

#include <retroshare/rsgxsforums.h>
#include <iostream>

// To start with we only have open forums - with distribution controls.
			
const uint32_t ForumCreateEnabledFlags = ( GXS_GROUP_FLAGS_ICON        |
			GXS_GROUP_FLAGS_DESCRIPTION   |
			GXS_GROUP_FLAGS_DISTRIBUTION  |
			// GXS_GROUP_FLAGS_PUBLISHSIGN   |
			GXS_GROUP_FLAGS_SHAREKEYS     |
			// GXS_GROUP_FLAGS_PERSONALSIGN  |
			// GXS_GROUP_FLAGS_COMMENTS      |
			0);
			
const uint32_t ForumCreateDefaultsFlags = ( GXS_GROUP_DEFAULTS_DISTRIB_PUBLIC    |
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


const uint32_t ForumEditEnabledFlags = ( GXS_GROUP_FLAGS_ICON        |
                        GXS_GROUP_FLAGS_DESCRIPTION   |
                        0);

const uint32_t ForumEditDefaultsFlags = 0;

GxsForumGroupDialog::GxsForumGroupDialog(TokenQueue *tokenQueue, QWidget *parent)
	:GxsGroupDialog(tokenQueue, ForumCreateEnabledFlags, ForumCreateDefaultsFlags, parent)
{
}

GxsForumGroupDialog::GxsForumGroupDialog(const RsGxsForumGroup &group, Mode mode, QWidget *parent)
	:GxsGroupDialog(group.mMeta, mode, parent)
{
}

void GxsForumGroupDialog::initUi()
{
	switch (mode())
	{
	case MODE_CREATE:
		setUiText(UITYPE_SERVICE_HEADER, tr("Create New Forum"));
		break;
	case MODE_SHOW:
		setUiText(UITYPE_SERVICE_HEADER, tr("Forum"));
		break;
	case MODE_EDIT:
		setUiText(UITYPE_SERVICE_HEADER, tr("Edit Forum"));
		break;
	}
}

QPixmap GxsForumGroupDialog::serviceImage()
{
	return QPixmap(":/images/konversation64.png");
}

bool GxsForumGroupDialog::service_CreateGroup(uint32_t &token, const RsGroupMetaData &meta)
{
	// Specific Function.
	RsGxsForumGroup grp;
	grp.mMeta = meta;
        grp.mDescription = std::string(ui.groupDesc->toPlainText().toUtf8());

	rsGxsForums->createGroup(token, grp);
	return true;
}

bool GxsForumGroupDialog::service_EditGroup(uint32_t &token, RsGxsGroupUpdateMeta &updateMeta)
{
    RsGxsForumGroup grp;
    grp.mDescription = std::string(ui.groupDesc->toPlainText().toUtf8());

    rsGxsForums->updateGroup(token, updateMeta, grp);
    return true;
}
