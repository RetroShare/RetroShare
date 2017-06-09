#include "imageutil.h"
#include "util/misc.h"
#include "util/rsscopetimer.h"

#include <QApplication>
#include <QByteArray>
#include <QImage>
#include <QMessageBox>
#include <QString>
#include <QTextCursor>
#include <QTextDocumentFragment>
#include <QBuffer>
#include <QtGlobal>
#include <QtMath>
#include <QSet>
#include <iostream>

ImageUtil::ImageUtil() {}

void ImageUtil::extractImage(QWidget *window, QTextCursor cursor)
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
			QString file;
			success = true;
			if(misc::getSaveFileName(window, RshareSettings::LASTDIR_IMAGES, "Save Picture File", "Pictures (*.png *.xpm *.jpg)", file))
			{
				if(!image.save(file, 0, 100))
					if(!image.save(file + ".png", 0, 100))
						QMessageBox::warning(window, QApplication::translate("ImageUtil", "Save image"), QApplication::translate("ImageUtil", "Cannot save the image, invalid filename"));
			}
		}
	}
	if(!success)
	{
		QMessageBox::warning(window, QApplication::translate("ImageUtil", "Save image"), QApplication::translate("ImageUtil", "Not an image"));
	}
}

void ImageUtil::optimizeSize(QString &html, const QImage& original, QImage &optimized, int maxPixels, int maxBytes)
{
	//nothing to do if it fits into the limits
	optimized = original;
	if ((maxPixels <= 0) || (optimized.width()*optimized.height() <= maxPixels)) {
		if(checkSize(html, optimized, maxBytes)) {
			return;
		}
	}

	QVector<QRgb> ct;
	quantization(original, ct);

	//Downscale the image to fit into maxPixels
	qreal scale = qSqrt((qreal)(maxPixels) / original.width() / original.height());
	if(scale > 1.0) scale = 1.0;

	//Half the resolution until image fits into maxBytes, or width becomes 0
	bool success = false;
	do {
		optimized = original.scaledToWidth((int)(original.width() * scale), Qt::SmoothTransformation).convertToFormat(QImage::Format_Indexed8, ct);
		success = checkSize(html, optimized, maxBytes);
		scale = scale / 2.0;
	} while((!optimized.isNull()) && !success);
}

bool ImageUtil::checkSize(QString &embeddedImage, const QImage &img, int maxBytes)
{
	RsScopeTimer st("Check size");
	//0 means no limit
	if(maxBytes > 500)
		maxBytes -= 500;	//make place for html stuff
	else if(maxBytes > 0) {
		std::cerr << QString("Limit is too small nothing will fit in, limit: %1 bytes\n").arg(maxBytes).toStdString();
		return false;
	}

	QByteArray bytearray;
	QBuffer buffer(&bytearray);
	bool success = false;

	std::cout << QString("Trying image: format PNG, size %1x%2, colors %3\n").arg(img.width()).arg(img.height()).arg(img.colorCount()).toStdString();
	if (buffer.open(QIODevice::WriteOnly)) {
		if (img.save(&buffer, "PNG", 0)) {
			if((maxBytes > 0) && (bytearray.length() * 4/3 > maxBytes))	// *4/3 for base64
			{
				std::cout << QString("\tToo large, size: %1, limit: %2 bytes\n").arg(bytearray.length() * 4/3).arg(maxBytes).toStdString();
			}else{
				std::cout << QString("\tOK, size: %1, limit: %2 bytes\n").arg(bytearray.length() * 4/3).arg(maxBytes).toStdString();
				success = true;
				QByteArray encodedByteArray = bytearray.toBase64();
				embeddedImage = "<img src=\"data:image/png;base64,";
				embeddedImage.append(encodedByteArray);
				embeddedImage.append("\">");
			}
		} else {
			std::cerr << "ImageUtil: image can't be saved to buffer" << std::endl;
		}
		buffer.close();
		bytearray.clear();
	} else {
		std::cerr << "ImageUtil: buffer can't be opened" << std::endl;
	}
	return success;
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
	int bits = 4;	// bits/pixel
	int samplesize = 100000;	//only take this many color samples

	RsScopeTimer st("Quantization");
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
