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
#include <qiterator.h>

class QWidget;
class QTextEdit;
class QByteArray;

class ImageUtil
{
public:
	ImageUtil();

	static bool checkImage(const QTextEdit *edit, const QPoint &pos, QRect *cursorRectStartOut = NULL, QRect *cursorRectLeftOut = NULL, QRect *cursorRectRightOut = NULL, QRect *cursorRectEndOut = NULL);
	static bool checkImage(const QTextEdit *edit, const QPoint &pos, QString &imageStr, QRect *cursorRectStartOut = NULL, QRect *cursorRectLeftOut = NULL, QRect *cursorRectRightOut = NULL, QRect *cursorRectEndOut = NULL);
	static void extractImage(QWidget *window, QTextCursor cursor, QString file = "");
	static void copyImage(QWidget *window, QTextCursor cursor);
	static bool saveImage(QWidget *window, const QImage &image, QString file = "");
    static bool optimizeSizeHtml(QString &html, const QImage& original, QImage &optimized, int maxPixels = -1, int maxBytes = -1);
    static bool optimizeSizeBytes(QByteArray &bytearray, const QImage &original, QImage &optimized, const char *format, int maxPixels, int maxBytes);
    static bool hasAlphaContent(const QImage& image);

	private:
        static int checkSize(QByteArray& embeddedImage, const QImage& img, const char *format);
		static void quantization(const QImage& img, QVector<QRgb>& palette);
		static void quantization(QList<QRgb>::iterator begin, QList<QRgb>::iterator end, int depth, QVector<QRgb>& palette);
		static void avgbucket(QList<QRgb>::iterator begin, QList<QRgb>::iterator end, QVector<QRgb>& palette);
};

#endif // IMAGEUTIL_H
