/*
 * RetroShare Android QML App
 * Copyright (C) 2016  Gioacchino Mazzurco <gio@eigenlab.org>
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

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlComponent>
#include <QDebug>

#ifdef __ANDROID__
#	include <QtAndroidExtras>
#endif

#include <QFileInfo>
#include <QDateTime>

#include "libresapilocalclient.h"
#include "retroshare/rsinit.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
	qmlRegisterType<LibresapiLocalClient>(
	            "org.retroshare.qml_components.LibresapiLocalClient", 1, 0,
	            "LibresapiLocalClient");

    QString sockPath = QString::fromStdString(RsAccounts::ConfigDirectory());
    sockPath.append("/libresapi.sock");

	engine.rootContext()->setContextProperty("apiSocketPath", sockPath);
	engine.load(QUrl(QLatin1String("qrc:/qml/main.qml")));

	QFileInfo fileInfo(sockPath);

#ifdef __ANDROID__
    qDebug() << "Is main.cpp running as a service?" << QtAndroid::androidService().isValid();
    qDebug() << "Is main.cpp running as an activity?" << QtAndroid::androidActivity().isValid();
#endif

	qDebug() << "QML APP:" << sockPath << fileInfo.exists() << fileInfo.lastModified().toString();

    return app.exec();
}
