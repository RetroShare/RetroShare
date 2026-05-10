/*******************************************************************************
 * retroshare-gui/src/gui/common/AvatarDefs.cpp                                *
 *                                                                             *
 * Copyright (C) 2012, Robert Fernie <retroshare.project@gmail.com>            *
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

#include <QDateTime>
#include <QFileInfo>
#include <QPixmap>
#include <QDir>
#include <QFile>
#include <thread>

#include <retroshare/rschats.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsidentity.h>
#include <gui/gxs/GxsIdDetails.h>

#include "AvatarDefs.h"
#include "gui/common/FilesDefs.h"

#define AVATAR_CACHE_DIR "avatars"
#define AVATAR_CACHE_STALENESS_SECS 600

QString AvatarDefs::getAvatarCacheDir()
{
    QString cacheDir = QDir::homePath() + "/.cache/RetroShare/";
    if (!QDir(cacheDir).exists()) {
        QDir().mkpath(cacheDir);
    }
    return cacheDir + AVATAR_CACHE_DIR + "/";
}

bool AvatarDefs::loadAvatarFromDiskCache(const RsPeerId& sslId, QPixmap &avatar)
{
    QString cacheDir = getAvatarCacheDir();
    if (!QDir(cacheDir).exists()) {
        QDir().mkpath(cacheDir);
    }

    QString filePath = cacheDir + QString::fromStdString(sslId.toStdString()) + ".png";
    QFile file(filePath);

    if (file.exists()) {
        if (avatar.load(filePath)) {
            return true;
        }
    }
    return false;
}

bool AvatarDefs::saveAvatarToDiskCache(const RsPeerId& sslId, const QPixmap &avatar)
{
    QString cacheDir = getAvatarCacheDir();
    if (!QDir(cacheDir).exists()) {
        QDir().mkpath(cacheDir);
    }

    QString filePath = cacheDir + QString::fromStdString(sslId.toStdString()) + ".png";
    return avatar.save(filePath, "PNG");
}

void AvatarDefs::cleanupAvatarDiskCache()
{
    QString cacheDir = getAvatarCacheDir();
    QDir dir(cacheDir);

    if (!dir.exists()) {
        return;
    }

    // Remove avatars older than 30 days
    QDateTime threshold = QDateTime::currentDateTime().addDays(-30);
    QFileInfoList files = dir.entryInfoList(QStringList() << "*.png", QDir::Files);

    for (const QFileInfo &file : files) {
        if (file.lastModified() < threshold) {
            dir.remove(file.fileName());
        }
    }
}

void AvatarDefs::getOwnAvatar(QPixmap &avatar, const QString& defaultImage)
{
	unsigned char *data = NULL;
	int size = 0;

	/* get avatar */
    rsChats->getOwnNodeAvatarData(data, size);

	if (size == 0) {
        avatar = FilesDefs::getPixmapFromQtResourcePath(defaultImage);
		return;
	}

	/* load image */
	GxsIdDetails::loadPixmapFromData(data, size, avatar,GxsIdDetails::ORIGINAL) ;

	free(data);
}

bool AvatarDefs::getAvatarFromSslId(const RsPeerId& sslId, QPixmap &avatar, const QString& defaultImage)
{
    unsigned char *data = NULL;
    int size = 0;

    if (loadAvatarFromDiskCache(sslId, avatar)) {
        QString filePath = getAvatarCacheDir() + QString::fromStdString(sslId.toStdString()) + ".png";
        qint64 ageSecs = QFileInfo(filePath).lastModified().secsTo(QDateTime::currentDateTime());
        if (ageSecs <= AVATAR_CACHE_STALENESS_SECS)
            return true;

        // Cache is stale — refresh in background by writing raw bytes directly to disk.
        // QPixmap must not be used off the main thread, so we write the raw avatar data
        // returned by the network layer straight to a temp file and rename atomically.
        std::thread([sslId, filePath]() {
            unsigned char *bgData = nullptr;
            int bgSize = 0;
            rsChats->getAvatarData(sslId, bgData, bgSize);
            if (bgSize > 0) {
                QString tmp = filePath + ".tmp";
                QFile f(tmp);
                if (f.open(QIODevice::WriteOnly)) {
                    f.write(reinterpret_cast<const char *>(bgData), bgSize);
                    f.close();
                    QFile::rename(tmp, filePath);
                }
                free(bgData);
            }
        }).detach();

        return true;
    }

    /* No cache — fetch synchronously on first load */
    rsChats->getAvatarData(sslId, data, size);
    if (size == 0) {
        if (!defaultImage.isEmpty())
            avatar = GxsIdDetails::makeDefaultGroupIconFromString(
                    QString::fromStdString(sslId.toStdString()), ":icons/person.png", GxsIdDetails::LARGE);
        return false;
    }

    GxsIdDetails::loadPixmapFromData(data, size, avatar, GxsIdDetails::LARGE);
    saveAvatarToDiskCache(sslId, avatar);
    free(data);
    return true;
}

bool AvatarDefs::getAvatarFromGxsId(const RsGxsId& gxsId, QPixmap &avatar, const QString& defaultImage)
{
    //int size = 0;

    /* get avatar */
    RsIdentityDetails details ;

    if(!rsIdentity->getIdDetails(gxsId, details))
    {
        avatar = FilesDefs::getPixmapFromQtResourcePath(defaultImage);
        return false;
    }

    /* load image */

        if(details.mAvatar.mSize == 0 || !GxsIdDetails::loadPixmapFromData(details.mAvatar.mData, details.mAvatar.mSize, avatar,GxsIdDetails::LARGE))
            avatar = GxsIdDetails::makeDefaultIcon(gxsId,GxsIdDetails::LARGE);

        return true;
}

bool AvatarDefs::getAvatarFromGpgId(const RsPgpId& gpgId, QPixmap &avatar, const QString& defaultImage)
{
	unsigned char *data = NULL;
	int size = 0;

    if (gpgId == rsPeers->getGPGOwnId()) {
		/* Its me */
        rsChats->getOwnNodeAvatarData(data,size);
	} else {
		/* get the first available avatar of one of the ssl ids */
        std::list<RsPeerId> sslIds;
        if (rsPeers->getAssociatedSSLIds(gpgId, sslIds)) {
            std::list<RsPeerId>::iterator sslId;
			for (sslId = sslIds.begin(); sslId != sslIds.end(); ++sslId) {
                rsChats->getAvatarData(*sslId, data, size);
				if (size) {
					break;
				}
			}
		}
	}

    if (size == 0) {
        if (!defaultImage.isEmpty()) {
            avatar = GxsIdDetails::makeDefaultGroupIconFromString(QString::fromStdString(gpgId.toStdString()), ":icons/person.png", GxsIdDetails::LARGE);
        }
        return false;
    }

	/* load image */
	GxsIdDetails::loadPixmapFromData(data, size, avatar, GxsIdDetails::LARGE);

	free(data);

    return true;
}
