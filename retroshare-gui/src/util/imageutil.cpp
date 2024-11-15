/*******************************************************************************
 * util/imageutil.cpp                                                          *
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

#include "imageutil.h"
#include "util/misc.h"
#include "util/rstime.h"

#include <QApplication>
#include <QWidget>
#include <QTextEdit>
#include <QByteArray>
#include <QImage>
#include <QClipboard>
#include <QMimeData>
#include <QMessageBox>
#include <QString>
#include <QTextCursor>
#include <QTextDocumentFragment>
#include <QBuffer>
#include <QtGlobal>
#include <QSet>
#include <cmath>
#include <iostream>

ImageUtil::ImageUtil() {}

bool ImageUtil::checkImage(const QTextEdit *edit, const QPoint &pos, QRect *cursorRectStartOut, QRect *cursorRectLeftOut, QRect *cursorRectRightOut, QRect *cursorRectEndOut)
{
	QString imageStr;
	return checkImage(edit, pos, imageStr, cursorRectStartOut, cursorRectLeftOut, cursorRectRightOut, cursorRectEndOut);
}

bool ImageUtil::checkImage(const QTextEdit *edit, const QPoint &pos, QString &imageStr, QRect *cursorRectStartOut, QRect *cursorRectLeftOut, QRect *cursorRectRightOut, QRect *cursorRectEndOut)
{
	//Get text cursor under pos. But if pos is under text browser end line this return last cursor.
	QTextCursor cursor = edit->cursorForPosition(pos);
	//First get rect of cursor (could be at left or right of image)
	QRect cursorRectStart = edit->cursorRect(cursor);
	//Second get text
	cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 1);//To get character just before
	QRect cursorRectLeft = edit->cursorRect(cursor);
	cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 2);
	QRect cursorRectRight = edit->cursorRect(cursor);
	imageStr = cursor.selection().toHtml();

	if (cursorRectStartOut) {
		*cursorRectStartOut = cursorRectStart;
	}
	if (cursorRectLeftOut) {
		*cursorRectLeftOut = cursorRectLeft;
	}
	if (cursorRectRightOut) {
		*cursorRectRightOut = cursorRectRight;
	}

	QRect cursorRectEnd = cursorRectStart;
	//Finally set left with right of precedent character.
	if (cursorRectEnd.top() < cursorRectLeft.bottom())
	{
		cursorRectEnd.setLeft(cursorRectLeft.right());
	} else {
		//Image on new line
		cursorRectEnd.setLeft(0);
	}
	//And set Right with left of next character.
	if (cursorRectEnd.bottom() > cursorRectRight.top())
	{
		cursorRectEnd.setRight(cursorRectRight.left());
	} else {
		//New line after Image.
	}

	if (cursorRectEndOut) {
		*cursorRectEndOut = cursorRectEnd;
	}

	//If pos is on text rect
	if (cursorRectEnd.contains(pos))
	{
		return imageStr.indexOf("base64,") != -1;
	}
	return false;
}

void ImageUtil::extractImage(QWidget *window, QTextCursor cursor, QString file)
{
	cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 1);
	cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 2);
	QString imagestr = cursor.selection().toHtml();
	bool success = false;
	int start = imagestr.indexOf("base64,") + 7;
	int stop = imagestr.indexOf("\"", start);
	int length = stop - start;
	if((start >= 0) && (length > 0))
	{
		QByteArray ba = QByteArray::fromBase64(imagestr.mid(start, length).toLatin1());
		QImage image = QImage::fromData(ba);
		if(!image.isNull())
		{
			success = true;
			saveImage(window, image, file);
		}
	}
	if(!success)
	{
		QMessageBox::warning(window, QApplication::translate("ImageUtil", "Save image"), QApplication::translate("ImageUtil", "Not an image"));
	}
}

bool ImageUtil::saveImage(QWidget *window, const QImage &image, QString file)
{
	bool result = false;

	if (!file.isEmpty() || misc::getSaveFileName(window, RshareSettings::LASTDIR_IMAGES, QApplication::translate("ImageUtil", "Save Picture File"), QApplication::translate("ImageUtil", "Pictures (*.png *.xpm *.jpg)"), file)) {
		if (image.save(file, nullptr, 100)) {
			result = true;
		} else {
			if (image.save(file + ".png", nullptr, 100)) {
				result = true;
			} else {
				QMessageBox::warning(window, QApplication::translate("ImageUtil", "Save image"), QApplication::translate("ImageUtil", "Cannot save the image, invalid filename")
									 + "\n" + file);
			}
		}
	}

	return result;
}

void ImageUtil::copyImage(QWidget *window, QTextCursor cursor)
{
	cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 1);
	cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 2);
	QString imagestr = cursor.selection().toHtml();
	bool success = false;
	int start = imagestr.indexOf("base64,") + 7;
	int stop = imagestr.indexOf("\"", start);
	int length = stop - start;
	if((start >= 0) && (length > 0))
	{
		QByteArray ba = QByteArray::fromBase64(imagestr.mid(start, length).toLatin1());
		QImage image = QImage::fromData(ba);
		if(!image.isNull())
		{
			success = true;
			QClipboard *clipboard = QApplication::clipboard();
			QMimeData *data = new QMimeData;
			data->setImageData(image);
			clipboard->setMimeData(data, QClipboard::Clipboard);
		}
	}
	if(!success)
	{
		QMessageBox::warning(window, QApplication::translate("ImageUtil", "Copy image"), QApplication::translate("ImageUtil", "Not an image"));
	}
}

bool ImageUtil::optimizeSizeBytes(QByteArray &bytearray, const QImage &original, QImage &optimized, const char *format, int maxPixels, int maxBytes)
{
	//nothing to do if it fits into the limits
	optimized = original;
	if ((maxPixels <= 0) || (optimized.width()*optimized.height() <= maxPixels)) {
        int s = checkSize(bytearray, optimized,format);
		if((maxBytes <= 0) || (s <= maxBytes)) {
			return true;
		}
	}

	//Downscale the image to fit into maxPixels
	double whratio = (qreal)original.width() / (qreal)original.height();
	int maxwidth;
	if(maxPixels > 0) {
		int maxwidth2 = (int)sqrt((double)(maxPixels) * whratio);
		maxwidth = (original.width() > maxwidth2) ? maxwidth2 : original.width();
	} else
		maxwidth = original.width();

	int minwidth = (int)sqrt(100.0 * whratio);

	//if maxBytes not defined, do not reduce color space, just downscale
	if(maxBytes <= 0) {
        checkSize(bytearray, optimized = original.scaledToWidth(maxwidth, Qt::SmoothTransformation),format);
		return true;
	}

	QVector<QRgb> ct;
	quantization(original, ct);

	//Use binary search to find a suitable image size + linear regression to guess the file size
    double maxsize = (double)checkSize(bytearray, optimized = original.scaledToWidth(maxwidth, Qt::SmoothTransformation).convertToFormat(QImage::Format_Indexed8, ct, Qt::ThresholdDither),format);
	if(maxsize <= maxBytes) return true;	//success
    double minsize = (double)checkSize(bytearray, optimized = original.scaledToWidth(minwidth, Qt::SmoothTransformation).convertToFormat(QImage::Format_Indexed8, ct, Qt::ThresholdDither),format);
	if(minsize > maxBytes) return false; //impossible

//	std::cout << "maxS: " << maxsize << " minS: " << minsize << std::endl;
//	std::cout << "maxW: " << maxwidth << " minW: " << minwidth << std::endl;
	int region = 500;
	bool success = false;
	int latestgood = 0;
	do {
		double m = (maxsize - minsize) / ((double)maxwidth * (double)maxwidth / whratio - (double)minwidth * (double)minwidth / whratio);
		double b = maxsize - m * ((double)maxwidth * (double)maxwidth / whratio);
		double a = ((double)(maxBytes - region/2) - b) / m; //maxBytes - region/2 target the center of the accepted region
		int nextwidth = (int)sqrt(a * whratio);
        int nextsize = checkSize(bytearray, optimized = original.scaledToWidth(nextwidth, Qt::SmoothTransformation).convertToFormat(QImage::Format_Indexed8, ct, Qt::ThresholdDither),format);
		if(nextsize <= maxBytes) {
			minsize = nextsize;
			minwidth = nextwidth;
			if(nextsize >= (maxBytes - region) || //the file size is close enough to the limit
			latestgood >= nextsize) { //The algorithm does not converge anymore
				success = true;
			} else {
				latestgood = nextsize;
			}
		} else {
			maxsize = nextsize;
			maxwidth = nextwidth;
		}
//		std::cout << "maxS: " << maxsize << " minS: " << minsize << std::endl;
//		std::cout << "maxW: " << maxwidth << " minW: " << minwidth << std::endl;
	} while(!success);
	return true;
	//html = html.arg(original.width());
	//std::cout << html.toStdString() << std::endl;
}

bool ImageUtil::hasAlphaContent(const QImage& image)
{
    if(!image.hasAlphaChannel())
    {
        std::cerr << "Image of size " << image.width() << " x " << image.height() << ": No transparency content detected." << std::endl;
        return false;
    }

    for(int i=0;i<image.width();++i)
        for(int j=0;j<image.height();++j)
            if(qAlpha(image.pixel(i,j)) < 255)
            {
                std::cerr << "Image of size " << image.width() << " x " << image.height() << ": Transparency content detected." << std::endl;
                return true;
            }

    std::cerr << "Image of size " << image.width() << " x " << image.height() << ": No transparency content detected." << std::endl;
    return false;
}
bool ImageUtil::optimizeSizeHtml(QString &html, const QImage& original, QImage &optimized, int maxPixels, int maxBytes)
{
	QByteArray bytearray;
	if(maxBytes > 0){
		maxBytes = maxBytes * 3/4 - 50; //base64 and html stuff
		if(maxBytes < 1) maxBytes = 1;
	}

    // check for transparency
    bool has_transparency = hasAlphaContent(original);

    if(optimizeSizeBytes(bytearray, original, optimized,has_transparency?"PNG":"JPG",maxPixels, maxBytes))
	{
		QByteArray encodedByteArray = bytearray.toBase64();
		html = "<img src=\"data:image/";
		html.append(has_transparency ? "png" : "jpeg");
		html.append(";base64,");
		html.append(encodedByteArray);
		html.append("\">");
		return true;
	}
	return false;
}

int ImageUtil::checkSize(QByteArray &bytearray, const QImage &img,const char *format)
{
	rstime::RsScopeTimer st("Check size");

	bytearray.clear();
	QBuffer buffer(&bytearray);
	int size = 0;

	//std::cout << QString("Trying image: format PNG, size %1x%2, colors %3\n").arg(img.width()).arg(img.height()).arg(img.colorCount()).toStdString();
	if (buffer.open(QIODevice::WriteOnly)) {
        if (img.save(&buffer, format, 85)) {
			size = bytearray.length();
		} else {
			std::cerr << "ImageUtil: image can't be saved to buffer" << std::endl;
		}
		buffer.close();
	} else {
		std::cerr << "ImageUtil: buffer can't be opened" << std::endl;
	}
	return size;
}

bool redLessThan(const QRgb &c1, const QRgb &c2)
{
	return qRed(c1) < qRed(c2);
}

bool greenLessThan(const QRgb &c1, const QRgb &c2)
{
	return qGreen(c1) < qGreen(c2);
}

bool blueLessThan(const QRgb &c1, const QRgb &c2)
{
	return qBlue(c1) < qBlue(c2);
}

//median cut algoritmh
void ImageUtil::quantization(const QImage &img, QVector<QRgb> &palette)
{
	int bits = 8;	// bits/pixel
	int samplesize = 100000;	//only take this many color samples

	rstime::RsScopeTimer st("Quantization");
	QSet<QRgb> colors;

	//collect color information
	int imgsize = img.width()*img.height();
	int width = img.width();
	samplesize = qMin(samplesize, imgsize);
	double sampledist = (double)imgsize / (double)samplesize;
	for (double i = 0; i < imgsize; i += sampledist) {
		QRgb pixel = img.pixel((int)i % width, (int)i / width);
		colors.insert(pixel);
	}

	QList<QRgb> colorlist = colors.toList();
	//don't do the algoritmh if we have less than 16 different colors
	if(colorlist.size() <= (1 << bits)) {
		for(int i = 0; i < colors.count(); ++i)
			palette.append(colorlist[i]);
	} else {
		quantization(colorlist.begin(), colorlist.end(), bits, palette);
	}
}

void ImageUtil::quantization(QList<QRgb>::iterator begin, QList<QRgb>::iterator end, int depth, QVector<QRgb> &palette)
{
	//the buckets are ready
	if(depth == 0) {
		avgbucket(begin, end, palette);
		return;
	}

	//nothing to do
	int count = end - begin;
	if(count == 1) {
		palette.append(*begin);
		return;
	}

	//widest color channel
	int rl = 255;
	int gl = 255;
	int bl = 255;
	int rh = 0;
	int gh = 0;
	int bh = 0;
	for(QList<QRgb>::iterator it = begin; it < end; ++it) {
		rl = qMin(rl, qRed(*it));
		gl = qMin(gl, qGreen(*it));
		bl = qMin(bl, qBlue(*it));
		rh = qMax(rh, qRed(*it));
		gh = qMax(gh, qGreen(*it));
		bh = qMax(bh, qBlue(*it));
	}
	int red = rh - rl;
	int green = gh - gl;
	int blue = bh - bl;

	//order by the widest channel
	if(red > green)
		if(red > blue)
			qSort(begin, end, redLessThan);
		else
			qSort(begin, end, blueLessThan);
	else
		if(green > blue)
			qSort(begin, end, greenLessThan);
		else
			qSort(begin, end, blueLessThan);

	//split into two buckets
	QList<QRgb>::iterator split = begin + count / 2;
	quantization(begin, split, depth - 1, palette);
	quantization(split, end, depth - 1, palette);
}

void ImageUtil::avgbucket(QList<QRgb>::iterator begin, QList<QRgb>::iterator end, QVector<QRgb> &palette)
{
	int red = 0;
	int green = 0;
	int blue = 0;
	int count = end - begin;

	for(QList<QRgb>::iterator it = begin; it < end; ++it) {
		red += qRed(*it);
		green += qGreen(*it);
		blue += qBlue(*it);
	}

	QRgb color = qRgb(red/count, green/count, blue/count);
	palette.append(color);
}
