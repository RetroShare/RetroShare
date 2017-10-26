#pragma once

#include <QObject>
#include <QDebug>

#include <QFile>
#include <QUrl>
#include <QImage>
#include <QImageReader>
#include <QBuffer>

#include "qpainter.h"


#ifdef __ANDROID__
#	include <QtAndroid>
#	include <QtAndroidExtras/QAndroidJniObject>
#endif // __ANDROID__

struct AndroidImagePicker : QObject
{
	Q_OBJECT

public slots:

	static void openPicker()
	{
		qDebug() << "Starting image picker intent";

#ifdef __ANDROID__
		QtAndroid::androidActivity().callMethod<void>(
		                            "openImagePicker",
		                            "()V" );
#endif // __ANDROID__

	}

	// Used to convert a given image path into a png base64 string
	static QString imageToBase64 (QString  const& path)
	{
		// Get local path from uri
		QUrl url (path);
		QString localPath = url.toLocalFile();

		qDebug() << "imageToBase64() local path:" << localPath ;
		QImage image= getImage (localPath);

		image = image.scaled(96,96,Qt::KeepAspectRatio,Qt::SmoothTransformation);

		qDebug() << "imageToBase64() encoding" ;

		return imageToB64(image);
	}

	static QString b64AvatarGen (QVariantList onloads, int size)
	{
		qDebug() << "b64AvatarGen(): Generating face Avatar from";

		QImage result(size, size, QImage::Format_ARGB32_Premultiplied);
		QPainter painter(&result);


		int counter = 0;
		for (QVariantList::iterator j = onloads.begin(); j != onloads.end(); j++)
		{
			QImage image = getImage (":/"+(*j).toString());
			painter.drawImage(0, 0, image); // xi, yi is the position for imagei
			counter++;
		}
		painter.end();

		return imageToB64(result);
	}

	static QImage getImage (QString  const& path)
	{
		QImageReader reader;
		reader.setFileName(path);
		return reader.read();
	}

	static QString imageToB64 (QImage image)
	{
		// Transform image into PNG format
		QByteArray ba;
		QBuffer    buffer( &ba );
		buffer.open( QIODevice::WriteOnly );
		image.save( &buffer, "png" );

		// Get Based 64 image string
		return QString(ba.toBase64());
	}


};
