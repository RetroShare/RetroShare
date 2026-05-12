/*******************************************************************************
 * retroshare-gui/src/gui/common/AvatarDefs.h                                  *
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

#ifndef _AVATARDEFS_H
#define _AVATARDEFS_H

#include <string>
#include <QString>
#include <retroshare/rstypes.h>
#include <retroshare/rsgxsifacetypes.h>

#define AVATAR_DEFAULT_IMAGE ":/icons/no_avatar.png"
#define AVATAR_DEFAULT_IMAGE_SQUARE ":/icons/no_avatar_square.png"

class QPixmap;

class AvatarDefs
{
public:
    static void getOwnAvatar(QPixmap &avatar, const QString& defaultImage = AVATAR_DEFAULT_IMAGE_SQUARE);

    static bool getAvatarFromSslId(const RsPeerId& sslId, QPixmap &avatar, const QString& defaultImage = AVATAR_DEFAULT_IMAGE);
    static bool getAvatarFromGpgId(const RsPgpId & gpgId, QPixmap &avatar, const QString& defaultImage = AVATAR_DEFAULT_IMAGE);
    static bool getAvatarFromGxsId(const RsGxsId & gxsId, QPixmap &avatar, const QString& defaultImage = AVATAR_DEFAULT_IMAGE);
};

#endif

