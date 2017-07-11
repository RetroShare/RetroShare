#pragma once

#include <QObject>
#include <QDebug>

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
};
