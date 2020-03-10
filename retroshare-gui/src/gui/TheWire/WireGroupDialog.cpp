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

#include "WireGroupDialog.h"
#include "gui/gxs/GxsIdDetails.h"

#include <iostream>

const uint32_t WireCreateEnabledFlags = ( 
							GXS_GROUP_FLAGS_NAME          |
							GXS_GROUP_FLAGS_ICON          |
							GXS_GROUP_FLAGS_DESCRIPTION   |
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

							// GXS_GROUP_DEFAULTS_COMMENTS_YES		  |
							GXS_GROUP_DEFAULTS_COMMENTS_NO          |
							0);

uint32_t WireEditEnabledFlags = WireCreateEnabledFlags;
uint32_t WireEditDefaultsFlags = WireCreateDefaultsFlags;

WireGroupDialog::WireGroupDialog(TokenQueue *tokenQueue, QWidget *parent)
	: GxsGroupDialog(tokenQueue, WireCreateEnabledFlags, WireCreateDefaultsFlags, parent)
{
}

WireGroupDialog::WireGroupDialog(TokenQueue *tokenExternalQueue, RsTokenService *tokenService, Mode mode, RsGxsGroupId groupId, QWidget *parent)
	: GxsGroupDialog(tokenExternalQueue, tokenService, mode, groupId, WireEditEnabledFlags, WireEditDefaultsFlags, parent)
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
}

QPixmap WireGroupDialog::serviceImage()
{
	return QPixmap(":/icons/wire-circle.png");
}

void WireGroupDialog::prepareWireGroup(RsWireGroup &group, const RsGroupMetaData &meta)
{
	group.mMeta = meta;
	group.mDescription = getDescription().toUtf8().constData();

#if 0
	QPixmap pixmap = getLogo();

	if (!pixmap.isNull()) {
		QByteArray ba;
		QBuffer buffer(&ba);

		buffer.open(QIODevice::WriteOnly);
		pixmap.save(&buffer, "PNG"); // writes image into ba in PNG format

		group.mThumbnail.copy((uint8_t *) ba.data(), ba.size());
	} else {
		group.mThumbnail.clear();
	}
#endif

}

bool WireGroupDialog::service_CreateGroup(uint32_t &token, const RsGroupMetaData &meta)
{
	// Specific Function.
	RsWireGroup grp;
	prepareWireGroup(grp, meta);

	rsWire->createGroup(token, grp);
	return true;
}

bool WireGroupDialog::service_EditGroup(uint32_t &token, RsGroupMetaData &editedMeta)
{
	RsWireGroup grp;
	prepareWireGroup(grp, editedMeta);

	std::cerr << "WireGroupDialog::service_EditGroup() submitting changes";
	std::cerr << std::endl;

	// TODO: no interface here, yet.
	// rsWire->updateGroup(token, grp);
	return true;
}

bool WireGroupDialog::service_loadGroup(uint32_t token, Mode /*mode*/, RsGroupMetaData& groupMetaData, QString &description)
{
	std::cerr << "WireGroupDialog::service_loadGroup(" << token << ")";
	std::cerr << std::endl;

	std::vector<RsWireGroup> groups;
	if (!rsWire->getGroupData(token, groups))
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

	std::cerr << "WireGroupDialog::service_loadGroup() Unfinished Loading";
	std::cerr << std::endl;

	const RsWireGroup &group = groups[0];
	groupMetaData = group.mMeta;
	description = QString::fromUtf8(group.mDescription.c_str());

#if 0
	if (group.mThumbnail.mData) {
		QPixmap pixmap;
		if (GxsIdDetails::loadPixmapFromData(group.mThumbnail.mData, group.mThumbnail.mSize, pixmap,GxsIdDetails::ORIGINAL)) {
			setLogo(pixmap);
		}
	} else {
			setLogo(QPixmap(":/images/album_create_64.png"));
	}
#endif

	return true;
}
