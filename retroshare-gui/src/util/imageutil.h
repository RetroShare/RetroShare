/*******************************************************************************
 * util/imageutil.h                                                            *
 *                                                                             *
 * Copyright (c) 2010 Retroshare Team  <retroshare.project@gmail.com>          *
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

#ifndef IMAGEUTIL_H
#define IMAGEUTIL_H

#include <QTextCursor>
#include <QWidget>
#include <QByteArray>
#include <qiterator.h>

class ImageUtil
{
public:
	ImageUtil();

	static void extractImage(QWidget *window, QTextCursor cursor, QString file = "");
	static bool optimizeSizeHtml(QString &html, const QImage& original, QImage &optimized, int maxPixels = -1, int maxBytes = -1);
	static bool optimizeSizeBytes(QByteArray &bytearray, const QImage& original, QImage &optimized, int maxPixels = -1, int maxBytes = -1);

	private:
		static int checkSize(QByteArray& embeddedImage, const QImage& img);
		static void quantization(const QImage& img, QVector<QRgb>& palette);
		static void quantization(QList<QRgb>::iterator begin, QList<QRgb>::iterator end, int depth, QVector<QRgb>& palette);
		static void avgbucket(QList<QRgb>::iterator begin, QList<QRgb>::iterator end, QVector<QRgb>& palette);
};

#endif // IMAGEUTIL_H
