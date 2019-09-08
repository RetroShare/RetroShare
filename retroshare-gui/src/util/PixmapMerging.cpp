/*******************************************************************************
 * util/PixmapMerging.cpp                                                      *
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

#ifdef UNUSED_CODE
#include <gui/gxs/GxsIdDetails.h>
#include <util/PixmapMerging.h>

#include <QPixmap>
#include <QPainter>

QPixmap PixmapMerging::merge(const std::string & foregroundPixmapData, const std::string & backgroundPixmapFilename)
{
	QPixmap foregroundImage;
	GxsIdDetails::loadPixmapFromData((uchar *) foregroundPixmapData.c_str(), foregroundPixmapData.size(),foregroundImage);

	QPixmap backgroundPixmap = QPixmap(QString::fromStdString(backgroundPixmapFilename));

	if (!foregroundImage.isNull()) {
		QPainter painter(&backgroundPixmap);
		painter.drawPixmap(0, 0, foregroundImage.scaled(backgroundPixmap.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
		painter.end();
	}

	return backgroundPixmap;
}

#endif
