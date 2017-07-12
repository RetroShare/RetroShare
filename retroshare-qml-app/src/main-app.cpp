/*
 * RetroShare Android QML App
 * Copyright (C) 2016-2017  Gioacchino Mazzurco <gio@eigenlab.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QtGlobal>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlComponent>
#include <QDebug>
#include <QDir>
#include <QThread>
#include <QVariant>

#ifdef Q_OS_ANDROID
#	include <QtAndroid>
#	include <QtAndroidExtras/QAndroidJniObject>
#	include <atomic>
#	include "androidplatforminteracions.h"
#else
#	include "defaultplatforminteracions.h"
#endif // Q_OS_ANDROID


#include "libresapilocalclient.h"
#include "rsqmlappengine.h"
#include "androidimagepicker.h"
#include "platforminteracions.h"

int main(int argc, char *argv[])
{
	QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QGuiApplication app(argc, argv);

	/** When possible it is better to use +rsApi+ object directly instead of
	 * multiple instances of +LibresapiLocalClient+ in Qml */
	qmlRegisterType<LibresapiLocalClient>(
	            "org.retroshare.qml_components.LibresapiLocalClient", 1, 0,
	            "LibresapiLocalClient");


	QString sockPath = QDir::homePath() + "/.retroshare";
	sockPath.append("/libresapi.sock");

	LibresapiLocalClient rsApi;
	rsApi.openConnection(sockPath);

	RsQmlAppEngine engine(true);
	QQmlContext& rootContext = *engine.rootContext();

	qmlRegisterType<AndroidImagePicker>(
	            "org.retroshare.qml_components.AndroidImagePicker", 1, 0,
	            "AndroidImagePicker");

	AndroidImagePicker androidImagePicker;
	engine.rootContext()->setContextProperty("androidImagePicker",
	                                  &androidImagePicker);

	QStringList mainArgs = app.arguments();

#ifdef Q_OS_ANDROID
	rootContext.setContextProperty("Q_OS_ANDROID", QVariant(true));

	AndroidPlatformInteracions platformGW(&app);

	/* Add Activity Intent data to args, because onNewIntent is called only if
	 * the Intet was triggered when the Activity was already created, so only in
	 * case onCreate is not called.
	 * The solution exposed in http://stackoverflow.com/a/36942185 is not
	 * adaptable to our case, because when onCreate is called the RsQmlAppEngine
	 * is not ready yet.
	 */
	uint waitCount = 0;
	std::atomic<bool> waitIntent(true);
	QString uriStr;
	do
	{
		QtAndroid::runOnAndroidThread(
		            [&waitIntent, &uriStr]()
		{
			QAndroidJniObject activity = QtAndroid::androidActivity();
			if(!activity.isValid())
			{
				qDebug() << "QtAndroid::runOnAndroidThread(...)"
				         << "activity not ready yet";
				return;
			}

			QAndroidJniObject intent = activity.callObjectMethod(
			            "getIntent", "()Landroid/content/Intent;");
			if(!intent.isValid())
			{
				qDebug() << "QtAndroid::runOnAndroidThread(...)"
				         << "intent not ready yet";
				return;
			}

			QAndroidJniObject intentData = intent.callObjectMethod(
			            "getDataString", "()Ljava/lang/String;");
			if(intentData.isValid()) uriStr = intentData.toString();

			waitIntent = false;
		});

		if(waitIntent)
		{
			qWarning() << "uriStr not ready yet after waiting"
			           << waitCount << "times";
			app.processEvents();
			++waitCount;
			QThread::msleep(10);
		}
	}
	while (waitIntent);

	qDebug() << "Got uriStr:" << uriStr;

	if(!uriStr.isEmpty()) mainArgs.append(uriStr);
#else
	DefaultPlatformInteracions platformGW(&app);
	rootContext.setContextProperty("Q_OS_ANDROID", QVariant(false));
#endif

	rootContext.setContextProperty("mainArgs", mainArgs);
	rootContext.setContextProperty("platformGW", &platformGW);

#ifdef QT_DEBUG
	rootContext.setContextProperty("QT_DEBUG", QVariant(true));
	rsApi.setDebug(false);
#else
	rootContext.setContextProperty("QT_DEBUG", QVariant(false));
#endif // QT_DEBUG

	rootContext.setContextProperty("apiSocketPath", sockPath);
	rootContext.setContextProperty("rsApi", &rsApi);
	engine.load(QUrl(QLatin1String("qrc:/main-app.qml")));

	return app.exec();
}
