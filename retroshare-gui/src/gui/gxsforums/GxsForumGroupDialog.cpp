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
#include "gui/common/FilesDefs.h"

#include <retroshare/rsgxsforums.h>
#include <iostream>

// To start with we only have open forums - with distribution controls.
			
const uint32_t ForumCreateEnabledFlags = ( 
			GXS_GROUP_FLAGS_NAME          |
//			GXS_GROUP_FLAGS_ICON          |
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

GxsForumGroupDialog::GxsForumGroupDialog(QWidget *parent)
    : GxsGroupDialog(ForumCreateEnabledFlags, ForumCreateDefaultsFlags, parent)
{
    ui.pubKeyShare_cb->setEnabled(true) ;
    ui.idChooserLabel->setToolTip(tr("<p>Put one of your identities here to allow others to send feedback and also have moderator rights on the forum. You may as well leave that field blank and keep the forum anonymously administrated.</p>"));
}

GxsForumGroupDialog::GxsForumGroupDialog(Mode mode, RsGxsGroupId groupId, QWidget *parent)
    : GxsGroupDialog(mode, groupId, ForumEditEnabledFlags, ForumEditDefaultsFlags, parent)
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
    return FilesDefs::getPixmapFromQtResourcePath(":/icons/png/forums.png");
}

bool GxsForumGroupDialog::service_createGroup(RsGroupMetaData& meta)
{
	// Specific Function.
	RsGxsForumGroup grp;
	grp.mMeta = meta;
	grp.mDescription = getDescription().toUtf8().constData();
	getSelectedModerators(grp.mAdminList.ids);

	if(rsGxsForums->createForum(grp))
    {
        meta = grp.mMeta;
        return true;
    }
    else
		return false;
}

bool GxsForumGroupDialog::service_updateGroup(const RsGroupMetaData& editedMeta)
{
	RsGxsForumGroup grp(mGroupData);	// start again from cached information. That allows to keep the pinned posts for instance.

    // now replace data by locally edited/changed information

	grp.mMeta = editedMeta;
	grp.mDescription = getDescription().toUtf8().constData();

	getSelectedModerators(grp.mAdminList.ids);

	std::cerr << "GxsForumGroupDialog::service_EditGroup() submitting changes";
	std::cerr << std::endl;

    return rsGxsForums->editForum(grp);
}

bool GxsForumGroupDialog::service_loadGroup(const RsGxsGenericGroupData *data, Mode /*mode*/, QString &description)
{
	const RsGxsForumGroup *pgroup = dynamic_cast<const RsGxsForumGroup*>(data);

	if (!pgroup)
	{
		RsErr() << "GxsForumGroupDialog::service_loadGroup() supplied generic group is not a RsGxsForumGroup"<< std::endl;
		return false;
	}

    // Information handled by GxsGroupDialog. description should rather be handled here in the service part!

	description = QString::fromUtf8(pgroup->mDescription.c_str());

    // Local information. Description should be handled here.

    setSelectedModerators(pgroup->mAdminList.ids);

    mGroupData = *pgroup; // keeps the private information

	return true;
}

bool GxsForumGroupDialog::service_getGroupData(const RsGxsGroupId& grpId,RsGxsGenericGroupData *& data)
{
	std::vector<RsGxsForumGroup> forumsInfo ;

    if( rsGxsForums->getForumsInfo(std::list<RsGxsGroupId>({grpId}),forumsInfo) && forumsInfo.size() == 1)
    {
        data = new RsGxsForumGroup(forumsInfo[0]);
        return true;
    }
    else
        return false;

}



