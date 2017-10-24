#ifndef IMAGEUTIL_H
#define IMAGEUTIL_H

#include <QTextCursor>
#include <QWidget>
#include <qiterator.h>

class ImageUtil
{
public:
	ImageUtil();

	static void extractImage(QWidget *window, QTextCursor cursor);
	static bool optimizeSize(QString &html, const QImage& original, QImage &optimized, int maxPixels = -1, int maxBytes = -1);

	private:
		static int checkSize(QString& embeddedImage, const QImage& img, int maxBytes = -1);
		static void quantization(const QImage& img, QVector<QRgb>& palette);
		static void quantization(QList<QRgb>::iterator begin, QList<QRgb>::iterator end, int depth, QVector<QRgb>& palette);
		static void avgbucket(QList<QRgb>::iterator begin, QList<QRgb>::iterator end, QVector<QRgb>& palette);
};

#endif // IMAGEUTIL_H
