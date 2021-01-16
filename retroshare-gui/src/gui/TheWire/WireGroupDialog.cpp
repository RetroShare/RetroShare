/*******************************************************************************
 * gui/TheWire/WireGroupDialog.cpp                                             *
 *                                                                             *
 * Copyright (C) 2020 by Robert Fernie       <retroshare.project@gmail.com>    *
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

#include "WireGroupExtra.h"

#include "WireGroupDialog.h"
#include "gui/common/FilesDefs.h"
#include "gui/gxs/GxsIdDetails.h"

#include <iostream>

const uint32_t WireCreateEnabledFlags = ( 
							GXS_GROUP_FLAGS_NAME          |
							GXS_GROUP_FLAGS_ICON          |
							// GXS_GROUP_FLAGS_DESCRIPTION   |
							GXS_GROUP_FLAGS_DISTRIBUTION  |
							// GXS_GROUP_FLAGS_PUBLISHSIGN   |
							// GXS_GROUP_FLAGS_SHAREKEYS     |	// disabled because the UI doesn't handle it yet.
							// GXS_GROUP_FLAGS_PERSONALSIGN  |
							// GXS_GROUP_FLAGS_COMMENTS      |
							GXS_GROUP_FLAGS_EXTRA         |
						  0);

uint32_t WireCreateDefaultsFlags = ( GXS_GROUP_DEFAULTS_DISTRIB_PUBLIC	|
							// GXS_GROUP_DEFAULTS_DISTRIB_GROUP       |
							// GXS_GROUP_DEFAULTS_DISTRIB_LOCAL       |

							GXS_GROUP_DEFAULTS_PUBLISH_OPEN         |
							// GXS_GROUP_DEFAULTS_PUBLISH_THREADS     |
							// GXS_GROUP_DEFAULTS_PUBLISH_REQUIRED    |
							// GXS_GROUP_DEFAULTS_PUBLISH_ENCRYPTED   |

							// GXS_GROUP_DEFAULTS_PERSONAL_GPG        |
							GXS_GROUP_DEFAULTS_PERSONAL_REQUIRED    |
							// GXS_GROUP_DEFAULTS_PERSONAL_IFNOPUB    |
							GXS_GROUP_DEFAULTS_PERSONAL_GROUP       |

							// GXS_GROUP_DEFAULTS_COMMENTS_YES		  |
							GXS_GROUP_DEFAULTS_COMMENTS_NO          |
							0);

uint32_t WireEditEnabledFlags = WireCreateEnabledFlags;
uint32_t WireEditDefaultsFlags = WireCreateDefaultsFlags;

WireGroupDialog::WireGroupDialog(QWidget *parent)
	: GxsGroupDialog(WireCreateEnabledFlags, WireCreateDefaultsFlags, parent)
{
}

WireGroupDialog::WireGroupDialog(Mode mode, RsGxsGroupId groupId, QWidget *parent)
	: GxsGroupDialog(mode, groupId, WireEditEnabledFlags, WireEditDefaultsFlags, parent)
{
}

void WireGroupDialog::initUi()
{
	switch (mode())
	{
	case MODE_CREATE:
		setUiText(UITYPE_SERVICE_HEADER, tr("Create New Wire"));
		setUiText(UITYPE_BUTTONBOX_OK, tr("Create"));
		break;
	case MODE_SHOW:
		setUiText(UITYPE_SERVICE_HEADER, tr("Wire"));
		break;
	case MODE_EDIT:
		setUiText(UITYPE_SERVICE_HEADER, tr("Edit Wire"));
		setUiText(UITYPE_BUTTONBOX_OK, tr("Update Wire"));
		break;
	}

	setUiText(UITYPE_ADD_ADMINS_CHECKBOX, tr("Add Wire Admins"));
	setUiText(UITYPE_CONTACTS_DOCK, tr("Select Wire Admins"));

	mExtra = new WireGroupExtra(this);
	injectExtraWidget(mExtra);
}

QPixmap WireGroupDialog::serviceImage()
{
    return FilesDefs::getPixmapFromQtResourcePath(":/icons/wire-circle.png");
}

void WireGroupDialog::prepareWireGroup(RsWireGroup &group, const RsGroupMetaData &meta)
{
	group.mMeta = meta;
	QPixmap pixmap = getLogo();

	if (!pixmap.isNull()) {
		QByteArray ba;
		QBuffer buffer(&ba);

		buffer.open(QIODevice::WriteOnly);
		pixmap.save(&buffer, "PNG"); // writes image into ba in PNG format

		group.mHeadshot.copy((uint8_t *) ba.data(), ba.size());
	} else {
		group.mHeadshot.clear();
	}

	// from Extra Widget.
	group.mTagline = mExtra->getTagline();
	group.mLocation = mExtra->getLocation();
	pixmap = mExtra->getMasthead();

	if (!pixmap.isNull()) {
		QByteArray ba;
		QBuffer buffer(&ba);

		buffer.open(QIODevice::WriteOnly);
		pixmap.save(&buffer, "JPG");

		group.mMasthead.copy((uint8_t *) ba.data(), ba.size());
	} else {
		group.mMasthead.clear();
	}
}

bool WireGroupDialog::service_createGroup(RsGroupMetaData &meta)
{
	RsWireGroup grp;
	prepareWireGroup(grp, meta);

	//bool success = rsWire->createGroup(grp);
	// TODO createGroup should refresh groupId or Data
	//return success;
	rsWire->createWire(grp);

	meta = grp.mMeta;
	return true;
}

bool WireGroupDialog::service_updateGroup(const RsGroupMetaData &editedMeta)
{
	RsWireGroup grp;
	prepareWireGroup(grp, editedMeta);

	std::cerr << "WireGroupDialog::service_updateGroup() submitting changes";
	std::cerr << std::endl;

	//bool success = rsWire->updateGroup(grp);
	// TODO updateGroup should refresh groupId or Data
	//return success;

	return rsWire->editWire(grp);
}

bool WireGroupDialog::service_loadGroup(const RsGxsGenericGroupData *data, Mode mode, QString &description)
{
	std::cerr << "WireGroupDialog::service_loadGroup()";
	std::cerr << std::endl;

	const RsWireGroup *pgroup = dynamic_cast<const RsWireGroup*>(data);
	if (pgroup == nullptr)
	{
		std::cerr << "WireGroupDialog::service_loadGroup() Error not a RsWireGroup";
		std::cerr << std::endl;
		return false;
	}

	const RsWireGroup &group = *pgroup;
	// description = QString::fromUtf8(group.mDescription.c_str());

	if (group.mHeadshot.mData) {
		QPixmap pixmap;
		if (GxsIdDetails::loadPixmapFromData(group.mHeadshot.mData, group.mHeadshot.mSize, pixmap,GxsIdDetails::ORIGINAL)) {
			setLogo(pixmap);
		}
	} else {
			setLogo(FilesDefs::getPixmapFromQtResourcePath(":/images/album_create_64.png"));
	}

	// from Extra Widget.
	mExtra->setTagline(group.mTagline);
	mExtra->setLocation(group.mLocation);

	if (group.mMasthead.mData){
		QPixmap pixmap;
		if (GxsIdDetails::loadPixmapFromData(group.mMasthead.mData, group.mMasthead.mSize, pixmap,GxsIdDetails::ORIGINAL))
		{
			mExtra->setMasthead(pixmap);
		}
	}

	return true;
}

bool WireGroupDialog::service_getGroupData(const RsGxsGroupId &grpId, RsGxsGenericGroupData *&data)
{
	std::cerr << "WireGroupDialog::service_getGroupData(" << grpId << ")";
	std::cerr << std::endl;

	std::list<RsGxsGroupId> groupIds({grpId});
	std::vector<RsWireGroup> groups;
	if (!rsWire->getGroups(groupIds, groups))
	{
		std::cerr << "WireGroupDialog::service_loadGroup() Error getting GroupData";
		std::cerr << std::endl;
		return false;
	}

	if (groups.size() != 1)
	{
		std::cerr << "WireGroupDialog::service_loadGroup() Error Group.size() != 1";
		std::cerr << std::endl;
		return false;
	}

	data = new RsWireGroup(groups[0]);
	return true;
}
