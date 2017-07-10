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
		    QAndroidJniObject::callStaticMethod<void>(
			                            "org/retroshare/android/qml_app/RetroshareImagePicker",
			                            "imagePickerIntent",
			                            "()Landroid/content/Intent;" );
        #endif // __ANDROID__
	}
};
