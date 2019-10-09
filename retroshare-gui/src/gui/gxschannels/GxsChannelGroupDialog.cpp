/*******************************************************************************
 * retroshare-gui/src/gui/gxschannels/GxsChannelGroupDialog.cpp                *
 *                                                                             *
 * Copyright 2013 by Robert Fernie     <retroshare.project@gmail.com>          *
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

#include "gui/gxs/GxsIdDetails.h"
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
			// GXS_GROUP_FLAGS_SHAREKEYS     |	// disabled because the UI doesn't handle it, so no need to show the disabled button. The user can do it in a second step from the channel menu.
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
    : GxsGroupDialog(tokenQueue, ChannelCreateEnabledFlags, ChannelCreateDefaultsFlags, parent)
{
    ui.commentGroupBox->setEnabled(false);	// These are here because comments_allowed are actually not used yet, so the group will not be changed by the setting and when
    ui.comments_allowed->setChecked(true);	// the group info is displayed it will therefore be set to "disabled" in all cases although it is enabled.
}

GxsChannelGroupDialog::GxsChannelGroupDialog(TokenQueue *tokenExternalQueue, RsTokenService *tokenService, Mode mode, RsGxsGroupId groupId, QWidget *parent)
    : GxsGroupDialog(tokenExternalQueue, tokenService, mode, groupId, ChannelEditEnabledFlags, ChannelEditDefaultsFlags, parent)
{
    ui.commentGroupBox->setEnabled(false);	// These are here because comments_allowed are actually not used yet, so the group will not be changed by the setting and when
    ui.comments_allowed->setChecked(true);	// the group info is displayed it will therefore be set to "disabled" in all cases although it is enabled.
}

void GxsChannelGroupDialog::initUi()
{
	switch (mode())
	{
	case MODE_CREATE:
		setUiText(UITYPE_SERVICE_HEADER, tr("Create New Channel"));
		setUiText(UITYPE_BUTTONBOX_OK,   tr("Create"));
		break;
	case MODE_SHOW:
		setUiText(UITYPE_SERVICE_HEADER, tr("Channel"));
		break;
	case MODE_EDIT:
		setUiText(UITYPE_SERVICE_HEADER, tr("Edit Channel"));
		setUiText(UITYPE_BUTTONBOX_OK,   tr("Update Channel"));
		break;
	}
	
	setUiText(UITYPE_KEY_SHARE_CHECKBOX, tr("Add Channel Admins"));
	setUiText(UITYPE_CONTACTS_DOCK,      tr("Select Channel Admins"));
}

QPixmap GxsChannelGroupDialog::serviceImage()
{
	switch (mode())
	{
	case MODE_CREATE:
		return QPixmap(":/icons/png/channel.png");
	case MODE_SHOW:
		return QPixmap(":/icons/png/channel.png");
	case MODE_EDIT:
		return QPixmap(":/icons/png/channel.png");
	}

	return QPixmap();
}

void GxsChannelGroupDialog::prepareChannelGroup(RsGxsChannelGroup &group, const RsGroupMetaData &meta)
{
	group.mMeta = meta;
	group.mDescription = getDescription().toUtf8().constData();

	QPixmap pixmap = getLogo();

	if (!pixmap.isNull()) {
		QByteArray ba;
		QBuffer buffer(&ba);

		buffer.open(QIODevice::WriteOnly);
		pixmap.save(&buffer, "PNG"); // writes image into ba in PNG format

		group.mImage.copy((uint8_t *) ba.data(), ba.size());
	} else {
		group.mImage.clear();
	}
}

bool GxsChannelGroupDialog::service_CreateGroup(uint32_t &token, const RsGroupMetaData &meta)
{
	// Specific Function.
	RsGxsChannelGroup grp;
	prepareChannelGroup(grp, meta);

	rsGxsChannels->createGroup(token, grp);
	return true;
}

bool GxsChannelGroupDialog::service_EditGroup(uint32_t &token, RsGroupMetaData &editedMeta)
{
	RsGxsChannelGroup grp;
	prepareChannelGroup(grp, editedMeta);

	std::cerr << "GxsChannelGroupDialog::service_EditGroup() submitting changes";
	std::cerr << std::endl;

	rsGxsChannels->updateGroup(token, grp);
	return true;
}

bool GxsChannelGroupDialog::service_loadGroup(uint32_t token, Mode /*mode*/, RsGroupMetaData& groupMetaData, QString &description)
{
	std::cerr << "GxsChannelGroupDialog::service_loadGroup(" << token << ")";
	std::cerr << std::endl;

	std::vector<RsGxsChannelGroup> groups;
	if (!rsGxsChannels->getGroupData(token, groups))
	{
		std::cerr << "GxsChannelGroupDialog::service_loadGroup() Error getting GroupData";
		std::cerr << std::endl;
		return false;
	}

	if (groups.size() != 1)
	{
		std::cerr << "GxsChannelGroupDialog::service_loadGroup() Error Group.size() != 1";
		std::cerr << std::endl;
		return false;
	}

	std::cerr << "GxsChannelsGroupDialog::service_loadGroup() Unfinished Loading";
	std::cerr << std::endl;

	const RsGxsChannelGroup &group = groups[0];
	groupMetaData = group.mMeta;
	description = QString::fromUtf8(group.mDescription.c_str());

	if (group.mImage.mData) {
		QPixmap pixmap;

		if (GxsIdDetails::loadPixmapFromData(group.mImage.mData, group.mImage.mSize,pixmap,GxsIdDetails::ORIGINAL)) {
			setLogo(pixmap);
		}
	}

	return true;
}
