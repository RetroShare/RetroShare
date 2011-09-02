/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2010, RetroShare Team
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#include <QPixmap>

#include <retroshare/rsmsgs.h>
#include <retroshare/rspeers.h>

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
	avatar.loadFromData(data, size, "PNG") ;

	delete[] data;
}

void AvatarDefs::getAvatarFromSslId(const std::string& sslId, QPixmap &avatar, const QString& defaultImage)
{
	unsigned char *data = NULL;
	int size = 0;

	/* get avatar */
	rsMsgs->getAvatarData(sslId, data, size);
	if (size == 0) {
		avatar = QPixmap(defaultImage);
		return;
	}

	/* load image */
	avatar.loadFromData(data, size, "PNG") ;

	delete[] data;
}

void AvatarDefs::getAvatarFromGpgId(const std::string& gpgId, QPixmap &avatar, const QString& defaultImage)
{
	unsigned char *data = NULL;
	int size = 0;

	if (gpgId == rsPeers->getGPGOwnId()) {
		/* Its me */
		rsMsgs->getOwnAvatarData(data,size);
	} else {
		/* get the first available avatar of one of the ssl ids */
		std::list<std::string> sslIds;
		if (rsPeers->getAssociatedSSLIds(gpgId, sslIds)) {
			std::list<std::string>::iterator sslId;
			for (sslId = sslIds.begin(); sslId != sslIds.end(); sslId++) {
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
	avatar.loadFromData(data, size, "PNG") ;

	delete[] data;
}
