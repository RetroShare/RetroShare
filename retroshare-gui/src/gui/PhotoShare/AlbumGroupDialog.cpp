/*******************************************************************************
 * retroshare-gui/src/gui/PhotoShare/AlbumGroupDialog.cpp                         *
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

#include "AlbumGroupDialog.h"
#include "AlbumExtra.h"
#include "gui/gxs/GxsIdDetails.h"

#include <iostream>

const uint32_t AlbumCreateEnabledFlags = ( 
              GXS_GROUP_FLAGS_NAME        |
               GXS_GROUP_FLAGS_ICON        |
                          GXS_GROUP_FLAGS_DESCRIPTION   |
                          GXS_GROUP_FLAGS_DISTRIBUTION  |
                          // GXS_GROUP_FLAGS_PUBLISHSIGN   |
                          // GXS_GROUP_FLAGS_SHAREKEYS     |    // disabled because the UI doesn't handle it yet.
                          // GXS_GROUP_FLAGS_PERSONALSIGN  |
                          // GXS_GROUP_FLAGS_COMMENTS      |
                          GXS_GROUP_FLAGS_EXTRA         |
                          0);

uint32_t AlbumCreateDefaultsFlags = ( GXS_GROUP_DEFAULTS_DISTRIB_PUBLIC    |
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

uint32_t AlbumEditEnabledFlags = AlbumCreateEnabledFlags;
uint32_t AlbumEditDefaultsFlags = AlbumCreateDefaultsFlags;

AlbumGroupDialog::AlbumGroupDialog(TokenQueue *tokenQueue, QWidget *parent)
    : GxsGroupDialog(tokenQueue, AlbumCreateEnabledFlags, AlbumCreateDefaultsFlags, parent)
{
}

AlbumGroupDialog::AlbumGroupDialog(TokenQueue *tokenExternalQueue, RsTokenService *tokenService, Mode mode, RsGxsGroupId groupId, QWidget *parent)
    : GxsGroupDialog(tokenExternalQueue, tokenService, mode, groupId, AlbumEditEnabledFlags, AlbumEditDefaultsFlags, parent)
{
}

void AlbumGroupDialog::initUi()
{
    switch (mode())
    {
    case MODE_CREATE:
        setUiText(UITYPE_SERVICE_HEADER, tr("Create New Album"));
        setUiText(UITYPE_BUTTONBOX_OK, tr("Create"));
        break;
    case MODE_SHOW:
        setUiText(UITYPE_SERVICE_HEADER, tr("Album"));
        break;
    case MODE_EDIT:
        setUiText(UITYPE_SERVICE_HEADER, tr("Edit ALbum"));
        setUiText(UITYPE_BUTTONBOX_OK, tr("Update Album"));
        break;
    }

    setUiText(UITYPE_ADD_ADMINS_CHECKBOX, tr("Add Album Admins"));
    setUiText(UITYPE_CONTACTS_DOCK, tr("Select Album Admins"));

    // Inject Extra Widgets.
    injectExtraWidget(new AlbumExtra(this));
}

QPixmap AlbumGroupDialog::serviceImage()
{
    return QPixmap(":/images/album_create_64.png");
}

void AlbumGroupDialog::prepareAlbumGroup(RsPhotoAlbum &group, const RsGroupMetaData &meta)
{
    group.mMeta = meta;
    group.mDescription = getDescription().toUtf8().constData();

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

    // Additional Fields that we need to fill in.
    group.mCaption = "Caption goes here";
    group.mPhotographer = "photographer";
    group.mWhere = "Where?";

}

bool AlbumGroupDialog::service_CreateGroup(uint32_t &token, const RsGroupMetaData &meta)
{
    // Specific Function.
    RsPhotoAlbum grp;
    prepareAlbumGroup(grp, meta);

    rsPhoto->submitAlbumDetails(token, grp);
    return true;
}

bool AlbumGroupDialog::service_EditGroup(uint32_t &token, RsGroupMetaData &editedMeta)
{
    RsPhotoAlbum grp;
    prepareAlbumGroup(grp, editedMeta);

    std::cerr << "AlbumGroupDialog::service_EditGroup() submitting changes";
    std::cerr << std::endl;

    // TODO: no interface here, yet.
    // rsPhoto->updateGroup(token, grp);
    return true;
}

bool AlbumGroupDialog::service_loadGroup(uint32_t token, Mode /*mode*/, RsGroupMetaData& groupMetaData, QString &description)
{
    std::cerr << "AlbumGroupDialog::service_loadGroup(" << token << ")";
    std::cerr << std::endl;

    std::vector<RsPhotoAlbum> groups;
    if (!rsPhoto->getAlbum(token, groups))
    {
        std::cerr << "AlbumGroupDialog::service_loadGroup() Error getting GroupData";
        std::cerr << std::endl;
        return false;
    }

    if (groups.size() != 1)
    {
        std::cerr << "AlbumGroupDialog::service_loadGroup() Error Group.size() != 1";
        std::cerr << std::endl;
        return false;
    }

    std::cerr << "AlbumGroupDialog::service_loadGroup() Unfinished Loading";
    std::cerr << std::endl;

    const RsPhotoAlbum &group = groups[0];
    groupMetaData = group.mMeta;
    description = QString::fromUtf8(group.mDescription.c_str());
    
    if (group.mThumbnail.mData) {
        QPixmap pixmap;
        if (GxsIdDetails::loadPixmapFromData(group.mThumbnail.mData, group.mThumbnail.mSize, pixmap,GxsIdDetails::ORIGINAL)) {
            setLogo(pixmap);
        }
    } else {
            setLogo(QPixmap(":/images/album_create_64.png"));
    }

    // NEED TO Load additional data....
    return true;
}
