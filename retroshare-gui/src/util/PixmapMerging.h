/*******************************************************************************
 * util/PixmapMerging.h                                                        *
 *                                                                             *
 * Copyright (c) 2012 Crypton           <retroshare.project@gmail.com>         *
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

#ifndef PIXMAPMERGING_H
#define PIXMAPMERGING_H

#include <util/rsqtutildll.h>

#include <util/NonCopyable.h>

#include <string>

class QPixmap;

/**
 * Merges 2 pixmaps together.
 *
 * 
 */
class PixmapMerging : NonCopyable {
public:

	/**
	 * Merges 2 pixmaps together.
	 *
	 * Used to have a background picture (a cadre) and a picture inside it.
	 * Currently used to a the avatar of a contact inside a nice background.
	 *
	 * @param foregroundPixmapData an 'array' with the foreground picture data
	 * @param backgroundPixmapFilename background pixmap filename e.g ":/pics/avatar_background.png"
	 */
	RSQTUTIL_API static QPixmap merge(const std::string & foregroundPixmapData, const std::string & backgroundPixmapFilename);
};

#endif	//PIXMAPMERGING_H
