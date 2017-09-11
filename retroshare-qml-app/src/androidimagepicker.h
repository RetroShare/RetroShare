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

		// Read the image
		QImageReader reader;
		reader.setFileName(localPath);
		QImage image = reader.read();

		image = image.scaled(96,96,Qt::KeepAspectRatio,Qt::SmoothTransformation);

		// Transform image into PNG format
		QByteArray ba;
		QBuffer    buffer( &ba );
		buffer.open( QIODevice::WriteOnly );
		image.save( &buffer, "png" );

		// Get Based 64 image string
		QString encoded = QString(ba.toBase64());

		qDebug() << "imageToBase64() encoded" ;

		return encoded;
	}

	static QString faceImage (QVariantList onloads, int size)
	{
		QImage result(size, size, QImage::Format_ARGB32_Premultiplied);
		QPainter painter(&result);


		int counter = 0;
		for (QVariantList::iterator j = onloads.begin(); j != onloads.end(); j++)
		{
			QString path = ":/"+(*j).toString();
			QImageReader reader;
			reader.setFileName(path);
			QImage image = reader.read();
			painter.drawImage(0, 0, image); // xi, yi is the position for imagei
//			if (counter == 0)
//			{
//				base = QImage(bg.size(), QImage::Format_ARGB32_Premultiplied);
//				qDebug() << "FIIIRST ";

//			}
//			else
//			{

//			}
			qDebug() << "iterating through QVariantList ";
			qDebug() << (*j).toString(); // Print QVariant
			counter++;
		}
		painter.end();

		// Transform image into PNG format
		QByteArray ba;
		QBuffer    buffer( &ba );
		buffer.open( QIODevice::WriteOnly );
		result.save( &buffer, "png" );

		// Get Based 64 image string
		QString encoded = QString(ba.toBase64());

		qDebug() << "@@@@@ encoded avatar " << encoded ;

		return encoded;

	}

	QImage getImageFromPath (QString localPath)
	{
		qDebug() << "getImageFromPath() local path:" << localPath ;

		// Read the image
		QImageReader reader;
		reader.setFileName(localPath);
		QImage image = reader.read();
		return image;

	}


};
