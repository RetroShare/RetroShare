/*******************************************************************************
 * retroshare-gui/src/gui/Posted/PostedGroupDialog.cpp                         *
 *                                                                             *
 * Copyright (C) 2013 by Robert Fernie       <retroshare.project@gmail.com>    *
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
#include <QBuffer>

#include "PostedGroupDialog.h"
#include "gui/gxs/GxsIdDetails.h"
#include "gui/common/FilesDefs.h"

#include <retroshare/rswiki.h>
#include <iostream>

const uint32_t PostedCreateEnabledFlags = ( 
			  GXS_GROUP_FLAGS_NAME        |
			   GXS_GROUP_FLAGS_ICON        |
                          GXS_GROUP_FLAGS_DESCRIPTION   |
                          GXS_GROUP_FLAGS_DISTRIBUTION  |
                          // GXS_GROUP_FLAGS_PUBLISHSIGN   |
                          // GXS_GROUP_FLAGS_SHAREKEYS     |	// disabled because the UI doesn't handle it yet.
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

uint32_t PostedEditEnabledFlags = PostedCreateEnabledFlags;
uint32_t PostedEditDefaultsFlags = PostedCreateDefaultsFlags;

PostedGroupDialog::PostedGroupDialog(QWidget *parent)
    : GxsGroupDialog(PostedCreateEnabledFlags, PostedCreateDefaultsFlags, parent)
{
}

PostedGroupDialog::PostedGroupDialog(Mode mode, RsGxsGroupId groupId, QWidget *parent)
    : GxsGroupDialog(mode, groupId, PostedEditEnabledFlags, PostedEditDefaultsFlags, parent)
{
}

void PostedGroupDialog::initUi()
{
	switch (mode())
	{
	case MODE_CREATE:
		setUiText(UITYPE_SERVICE_HEADER, tr("Create New Board"));
		setUiText(UITYPE_BUTTONBOX_OK, tr("Create"));
		break;
	case MODE_SHOW:
		setUiText(UITYPE_SERVICE_HEADER, tr("Board"));
		break;
	case MODE_EDIT:
		setUiText(UITYPE_SERVICE_HEADER, tr("Edit Board"));
		setUiText(UITYPE_BUTTONBOX_OK, tr("Update Board"));
		break;
	}

	setUiText(UITYPE_ADD_ADMINS_CHECKBOX, tr("Add Board Admins"));
	setUiText(UITYPE_CONTACTS_DOCK, tr("Select Board Admins"));
}

QPixmap PostedGroupDialog::serviceImage()
{
    return FilesDefs::getPixmapFromQtResourcePath(":/icons/png/posted.png");
}

void PostedGroupDialog::preparePostedGroup(RsPostedGroup &group, const RsGroupMetaData &meta)
{
	group.mMeta = meta;
	group.mDescription = getDescription().toUtf8().constData();

	QPixmap pixmap = getLogo();

	if (!pixmap.isNull()) {
		QByteArray ba;
		QBuffer buffer(&ba);

		buffer.open(QIODevice::WriteOnly);
		pixmap.save(&buffer, "PNG"); // writes image into ba in PNG format

		group.mGroupImage.copy((uint8_t *) ba.data(), ba.size());
	} else {
		group.mGroupImage.clear();
	}
}

bool PostedGroupDialog::service_createGroup(RsGroupMetaData& meta)
{
	// Specific Function.
	RsPostedGroup grp;
	preparePostedGroup(grp, meta);

	if(rsPosted->createBoard(grp))
        meta = grp.mMeta;

	return true;
}

bool PostedGroupDialog::service_updateGroup(const RsGroupMetaData& editedMeta)
{
	RsPostedGroup grp;
	preparePostedGroup(grp, editedMeta);

	std::cerr << "PostedGroupDialog::service_EditGroup() submitting changes";
	std::cerr << std::endl;

	return rsPosted->editBoard(grp);
}

bool PostedGroupDialog::service_loadGroup(const RsGxsGenericGroupData *data, Mode mode, QString& description)
{
	const RsPostedGroup *pgroup = dynamic_cast<const RsPostedGroup*>(data);

	if(pgroup == nullptr)
	{
		std::cerr << "PostedGroupDialog::service_loadGroup() Error not a RsPostedGroup" << std::endl;
		return false;
	}

	std::cerr << "PostedGroupDialog::service_loadGroup() Unfinished Loading";
	std::cerr << std::endl;

	const RsPostedGroup& group = *pgroup;
	description = QString::fromUtf8(group.mDescription.c_str());
	
	if (group.mGroupImage.mData)
    {
		QPixmap pixmap;
		if (GxsIdDetails::loadPixmapFromData(group.mGroupImage.mData, group.mGroupImage.mSize, pixmap,GxsIdDetails::ORIGINAL))
			setLogo(pixmap);

	}
    else
        setLogo(FilesDefs::getPixmapFromQtResourcePath(":/icons/png/posted.png"));

	return true;
}

bool PostedGroupDialog::service_getGroupData(const RsGxsGroupId& grpId,RsGxsGenericGroupData *& data)
{
	std::vector<RsPostedGroup> boardsInfo ;

    if( rsPosted->getBoardsInfo(std::list<RsGxsGroupId>({grpId}),boardsInfo) && boardsInfo.size() == 1)
    {
        data = new RsPostedGroup(boardsInfo[0]);
        return true;
    }
    else
        return false;

}
