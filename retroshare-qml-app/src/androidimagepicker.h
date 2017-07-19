#pragma once

#include <QObject>
#include <QDebug>

#include <QFile>
#include <QUrl>
#include <QImage>
#include <QImageReader>
#include <QBuffer>


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
};
