/*******************************************************************************
 * gui/common/FilesDefs.h                                                      *
 *                                                                             *
 * Copyright (C) 2011, Retroshare Team <retroshare.project@gmail.com>          *
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

#ifndef _FILESDEFS_H
#define _FILESDEFS_H

#include <QString>
#include <QIcon>

#include "retroshare/rsids.h"

class FilesDefs
{
public:
	static QString getImageFromFilename(const QString& filename, bool anyForUnknown);

    // Theses methods is here to fix a Qt design flow that makes QIcon loaded from filename (e.g. :/images/icon.png) to not use the cache.
    // As a result, icons created by Qt in this way (mostly from GUI) do not use data sharing.
    // The method below has its own cache.

	static QIcon   getIconFromQtResourcePath(const QString& resource_path);
	static QPixmap getPixmapFromQtResourcePath(const QString& resource_path);
	static QIcon getIconFromGxsIdCache(const RsGxsId& id, const QIcon& setIcon, bool& exist);

    // This method returns a QIcon that is suitable to represent a file of a particular type (image, movie, etc.)

	static QIcon getIconFromFileType(const QString& filename);

	static QString getNameFromFilename(const QString& filename);
};

#endif

