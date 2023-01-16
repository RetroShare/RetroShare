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
#include "gui/common/FilesDefs.h"

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

// Album Requirements:
// - All Photos require Publish signature (PUBLISH THREADS).
// - Comments are in the threads - so these need Author signatures.
// - Author signature required for all groups.
uint32_t AlbumCreateDefaultsFlags = ( GXS_GROUP_DEFAULTS_DISTRIB_PUBLIC    |
                           //GXS_GROUP_DEFAULTS_DISTRIB_GROUP        |
                           //GXS_GROUP_DEFAULTS_DISTRIB_LOCAL        |

                           //GXS_GROUP_DEFAULTS_PUBLISH_OPEN         |
                           GXS_GROUP_DEFAULTS_PUBLISH_THREADS      |
                           //GXS_GROUP_DEFAULTS_PUBLISH_REQUIRED     |
                           //GXS_GROUP_DEFAULTS_PUBLISH_ENCRYPTED    |

                           //GXS_GROUP_DEFAULTS_PERSONAL_GPG         |
                           //GXS_GROUP_DEFAULTS_PERSONAL_REQUIRED    |
                           GXS_GROUP_DEFAULTS_PERSONAL_IFNOPUB     |
                           GXS_GROUP_DEFAULTS_PERSONAL_GROUP       |

                           GXS_GROUP_DEFAULTS_COMMENTS_YES         |
                           //GXS_GROUP_DEFAULTS_COMMENTS_NO          |
                           0);

uint32_t AlbumEditEnabledFlags = AlbumCreateEnabledFlags;
uint32_t AlbumEditDefaultsFlags = AlbumCreateDefaultsFlags;

AlbumGroupDialog::AlbumGroupDialog(QWidget *parent)
    : GxsGroupDialog(AlbumCreateEnabledFlags, AlbumCreateDefaultsFlags, parent)
{
}

AlbumGroupDialog::AlbumGroupDialog(Mode mode, RsGxsGroupId groupId, QWidget *parent)
    : GxsGroupDialog(mode, groupId, AlbumEditEnabledFlags, AlbumEditDefaultsFlags, parent)
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
    mAlbumExtra = new AlbumExtra(this);
    injectExtraWidget(mAlbumExtra);
}

QPixmap AlbumGroupDialog::serviceImage()
{
    return FilesDefs::getPixmapFromQtResourcePath(":/images/album_create_64.png");
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
        pixmap.save(&buffer, "JPG"); // writes image into ba in PNG format

        group.mThumbnail.copy((uint8_t *) ba.data(), ba.size());
    } else {
        group.mThumbnail.clear();
    }

    // Additional Fields.
    group.mShareMode = mAlbumExtra->getShareMode();
    group.mCaption = mAlbumExtra->getCaption();
    group.mPhotographer = mAlbumExtra->getPhotographer();
    group.mWhere = mAlbumExtra->getWhere();
    group.mWhen = mAlbumExtra->getWhen();
}

bool AlbumGroupDialog::service_createGroup(RsGroupMetaData &meta)
{
    // Specific Function.
    RsPhotoAlbum grp;
    prepareAlbumGroup(grp, meta);

    bool success = rsPhoto->createAlbum(grp);
    // TODO createAlbum should refresh groupId or GroupObj.
    return success;
}

bool AlbumGroupDialog::service_updateGroup(const RsGroupMetaData &editedMeta)
{
    RsPhotoAlbum grp;
    prepareAlbumGroup(grp, editedMeta);

    std::cerr << "AlbumGroupDialog::service_updateGroup() submitting changes";
    std::cerr << std::endl;

    bool success = rsPhoto->updateAlbum(grp);
    // TODO updateAlbum should refresh groupId or GroupObj.
    return success;
}

bool AlbumGroupDialog::service_loadGroup(const RsGxsGenericGroupData *data, Mode /*mode*/, QString &description)
{
    std::cerr << "AlbumGroupDialog::service_loadGroup()";
    std::cerr << std::endl;

    const RsPhotoAlbum *pgroup = dynamic_cast<const RsPhotoAlbum*>(data);

    if(pgroup == nullptr)
    {
        std::cerr << "AlbumGroupDialog::service_loadGroup() Error not a RsPhotoAlbum" << std::endl;
        return false;
    }

    const RsPhotoAlbum& group = *pgroup;
    description = QString::fromUtf8(group.mDescription.c_str());
    
    if (group.mThumbnail.mData) {
        QPixmap pixmap;
        if (GxsIdDetails::loadPixmapFromData(group.mThumbnail.mData, group.mThumbnail.mSize, pixmap,GxsIdDetails::ORIGINAL)) {
            setLogo(pixmap);
        }
    } else {
            setLogo(FilesDefs::getPixmapFromQtResourcePath(":/images/album_create_64.png"));
    }

    // Load additional data....
    mAlbumExtra->setShareMode(group.mShareMode);
    mAlbumExtra->setCaption(group.mCaption);
    mAlbumExtra->setPhotographer(group.mPhotographer);
    mAlbumExtra->setWhere(group.mWhere);
    mAlbumExtra->setWhen(group.mWhen);

    return true;
}

bool AlbumGroupDialog::service_getGroupData(const RsGxsGroupId& grpId,RsGxsGenericGroupData *& data)
{
    std::cerr << "AlbumGroupDialog::service_getGroupData(" << grpId << ")";
    std::cerr << std::endl;

    std::list<RsGxsGroupId> groupIds({grpId});
    std::vector<RsPhotoAlbum> groups;
    if (!rsPhoto->getAlbums(groupIds, groups))
    {
        std::cerr << "AlbumGroupDialog::service_getGroupData() Error getting GroupData";
        std::cerr << std::endl;
        return false;
    }

    if (groups.size() != 1)
    {
        std::cerr << "AlbumGroupDialog::service_getGroupData() Error Group.size() != 1";
        std::cerr << std::endl;
        return false;
    }

    data = new RsPhotoAlbum(groups[0]);
    return true;
}

