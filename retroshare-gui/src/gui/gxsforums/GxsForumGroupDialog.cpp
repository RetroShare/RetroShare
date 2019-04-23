/*******************************************************************************
 * retroshare-gui/src/gui/gxsforums/CreateGxsForumGroupDialog.cpp              *
 *                                                                             *
 * Copyright 2013 Robert Fernie        <retroshare.project@gmail.com>          *
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

#include "GxsForumGroupDialog.h"

#include <retroshare/rsgxsforums.h>
#include <iostream>

// To start with we only have open forums - with distribution controls.
			
const uint32_t ForumCreateEnabledFlags = ( 
			GXS_GROUP_FLAGS_NAME          |
//			GXS_GROUP_FLAGS_ICON          |
//			GXS_GROUP_FLAGS_COLOR         |
			GXS_GROUP_FLAGS_DESCRIPTION   |
			GXS_GROUP_FLAGS_DISTRIBUTION  |
			// GXS_GROUP_FLAGS_PUBLISHSIGN|
			// GXS_GROUP_FLAGS_SHAREKEYS  |
			GXS_GROUP_FLAGS_ADDADMINS     |
			GXS_GROUP_FLAGS_ANTI_SPAM     |
			// GXS_GROUP_FLAGS_PERSONALSIGN  |
			// GXS_GROUP_FLAGS_COMMENTS      |
			0);
			
const uint32_t ForumCreateDefaultsFlags = ( GXS_GROUP_DEFAULTS_DISTRIB_PUBLIC    |
			//GXS_GROUP_DEFAULTS_DISTRIB_GROUP        |
			//GXS_GROUP_DEFAULTS_DISTRIB_LOCAL        |
			
			GXS_GROUP_DEFAULTS_PUBLISH_OPEN           |
			//GXS_GROUP_DEFAULTS_PUBLISH_THREADS      |
			//GXS_GROUP_DEFAULTS_PUBLISH_REQUIRED     |
			//GXS_GROUP_DEFAULTS_PUBLISH_ENCRYPTED    |
			
			//GXS_GROUP_DEFAULTS_PERSONAL_PGP         |
			GXS_GROUP_DEFAULTS_PERSONAL_REQUIRED      |
			//GXS_GROUP_DEFAULTS_PERSONAL_IFNOPUB     |
			
			//GXS_GROUP_DEFAULTS_COMMENTS_YES         |
			GXS_GROUP_DEFAULTS_COMMENTS_NO            |
			0);

const uint32_t ForumEditEnabledFlags = ForumCreateEnabledFlags;
const uint32_t ForumEditDefaultsFlags = ForumCreateDefaultsFlags;

GxsForumGroupDialog::GxsForumGroupDialog(TokenQueue *tokenQueue, QWidget *parent)
    : GxsGroupDialog(tokenQueue, ForumCreateEnabledFlags, ForumCreateDefaultsFlags, parent)
{
    ui.pubKeyShare_cb->setEnabled(true) ;
    ui.label_2->setToolTip(tr("<p>Put one of your identities here to allow others to send feedback and also have moderator rights on the forum. You may as well leave that field blank and keep the forum anonymously administrated.</p>"));
}

GxsForumGroupDialog::GxsForumGroupDialog(TokenQueue *tokenExternalQueue, RsTokenService *tokenService, Mode mode, RsGxsGroupId groupId, QWidget *parent)
    : GxsGroupDialog(tokenExternalQueue, tokenService, mode, groupId, ForumEditEnabledFlags, ForumEditDefaultsFlags, parent)
{
    ui.pubKeyShare_cb->setEnabled(true) ;
}

void GxsForumGroupDialog::initUi()
{
	switch (mode())
	{
	case MODE_CREATE:
		setUiText(UITYPE_SERVICE_HEADER, tr("Create New Forum"));
		setUiText(UITYPE_BUTTONBOX_OK, tr("Create"));
		break;
	case MODE_SHOW:
		setUiText(UITYPE_SERVICE_HEADER, tr("Forum"));
		break;
	case MODE_EDIT:
		setUiText(UITYPE_SERVICE_HEADER, tr("Edit Forum"));
		setUiText(UITYPE_BUTTONBOX_OK, tr("Update Forum"));
		break;
	}

    setUiToolTip(UITYPE_ADD_ADMINS_CHECKBOX,tr("Forum moderators can edit/delete/pinup others posts"));

	//setUiText(UITYPE_KEY_SHARE_CHECKBOX, tr("Add Forum Admins"));
    //setUiText(UITYPE_CONTACTS_DOCK, tr("Select Forum Admins"));
}

QPixmap GxsForumGroupDialog::serviceImage()
{
	return QPixmap(":/icons/png/forums.png");
}

bool GxsForumGroupDialog::service_CreateGroup(uint32_t &token, const RsGroupMetaData &meta)
{
	// Specific Function.
	RsGxsForumGroup grp;
	grp.mMeta = meta;
	grp.mDescription = getDescription().toUtf8().constData();
	getSelectedModerators(grp.mAdminList.ids);

	rsGxsForums->createGroup(token, grp);
	return true;
}

bool GxsForumGroupDialog::service_EditGroup(uint32_t &token, RsGroupMetaData &editedMeta)
{
	RsGxsForumGroup grp(mGroupData);	// start again from cached information. That allows to keep the pinned posts for instance.

    // now replace data by locally edited/changed information

	grp.mMeta = editedMeta;
	grp.mDescription = getDescription().toUtf8().constData();

	getSelectedModerators(grp.mAdminList.ids);

	std::cerr << "GxsForumGroupDialog::service_EditGroup() submitting changes";
	std::cerr << std::endl;

	rsGxsForums->updateGroup(token, grp);
	return true;
}

bool GxsForumGroupDialog::service_loadGroup(uint32_t token, Mode /*mode*/, RsGroupMetaData& groupMetaData, QString &description , QString &colorstring)
{
	std::cerr << "GxsForumGroupDialog::service_loadGroup(" << token << ")";
	std::cerr << std::endl;

	std::vector<RsGxsForumGroup> groups;

	if (!rsGxsForums->getGroupData(token, groups))
	{
		std::cerr << "GxsForumGroupDialog::service_loadGroup() Error getting GroupData";
		std::cerr << std::endl;
		return false;
	}

	if (groups.size() != 1)
	{
		std::cerr << "GxsForumGroupDialog::service_loadGroup() Error Group.size() != 1";
		std::cerr << std::endl;
		return false;
	}

	std::cerr << "GxsForumsGroupDialog::service_loadGroup() Unfinished Loading";
	std::cerr << std::endl;

    // Information handled by GxsGroupDialog. description should rather be handled here in the service part!

	groupMetaData = groups[0].mMeta;
	description = QString::fromUtf8(groups[0].mDescription.c_str());

    // Local information. Description should be handled here.

    setSelectedModerators(groups[0].mAdminList.ids);

    mGroupData = groups[0]; // keeps the private information

	return true;
}
