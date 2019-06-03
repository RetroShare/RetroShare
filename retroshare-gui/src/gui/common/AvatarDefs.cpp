/*******************************************************************************
 * gui/common/AvatarDefs.cpp                                                   *
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

#include <QPixmap>

#include <retroshare/rsmsgs.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsidentity.h>
#include <gui/gxs/GxsIdDetails.h>

#include "AvatarDefs.h"

void AvatarDefs::getOwnAvatar(QPixmap &avatar, const QString& defaultImage)
{
	unsigned char *data = NULL;
	int size = 0;

	/* get avatar */
	rsMsgs->getOwnAvatarData(data, size);

	if (size == 0) {
		avatar = QPixmap(defaultImage);
		return;
	}

	/* load image */
	GxsIdDetails::loadPixmapFromData(data, size, avatar) ;

	free(data);
}
void AvatarDefs::getAvatarFromSslId(const RsPeerId& sslId, QPixmap &avatar, const QString& defaultImage)
{
    unsigned char *data = NULL;
    int size = 0;

    /* get avatar */
    rsMsgs->getAvatarData(RsPeerId(sslId), data, size);
    if (size == 0) {
        avatar = QPixmap(defaultImage);
        return;
    }

    /* load image */
    GxsIdDetails::loadPixmapFromData(data, size, avatar) ;

    free(data);
}
void AvatarDefs::getAvatarFromGxsId(const RsGxsId& gxsId, QPixmap &avatar, const QString& defaultImage)
{
    //int size = 0;

    /* get avatar */
    RsIdentityDetails details ;

    if(!rsIdentity->getIdDetails(gxsId, details))
    {
        avatar = QPixmap(defaultImage);
        return ;
    }

    /* load image */

        if(details.mAvatar.mSize == 0 || !GxsIdDetails::loadPixmapFromData(details.mAvatar.mData, details.mAvatar.mSize, avatar))
            avatar = GxsIdDetails::makeDefaultIcon(gxsId);
}

void AvatarDefs::getAvatarFromGpgId(const RsPgpId& gpgId, QPixmap &avatar, const QString& defaultImage)
{
	unsigned char *data = NULL;
	int size = 0;

    if (gpgId == rsPeers->getGPGOwnId()) {
		/* Its me */
		rsMsgs->getOwnAvatarData(data,size);
	} else {
		/* get the first available avatar of one of the ssl ids */
        std::list<RsPeerId> sslIds;
        if (rsPeers->getAssociatedSSLIds(gpgId, sslIds)) {
            std::list<RsPeerId>::iterator sslId;
			for (sslId = sslIds.begin(); sslId != sslIds.end(); ++sslId) {
				rsMsgs->getAvatarData(*sslId, data, size);
				if (size) {
					break;
				}
			}
		}
	}

	if (size == 0) {
		avatar = QPixmap(defaultImage);
		return;
	}

	/* load image */
	GxsIdDetails::loadPixmapFromData(data, size, avatar);

	free(data);
}
