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
#include <QDebug>

#include <QtAndroidExtras>
#include <QFileInfo>
#include <QDateTime>

#include "retroshare/rsinit.h"

int main(int argc, char *argv[])
{
	QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QGuiApplication app(argc, argv);

	QQmlApplicationEngine engine;
	engine.load(QUrl(QLatin1String("qrc:/main.qml")));

	QString sockPath = QString::fromStdString(RsAccounts::ConfigDirectory());
	sockPath.append("/libresapi.sock");

	QFileInfo fileInfo(sockPath);

	qDebug() << "Is service.cpp running as a service?" << QtAndroid::androidService().isValid();
	qDebug() << "Is service.cpp running as an activity?" << QtAndroid::androidActivity().isValid();
	qDebug() << "QML APP:" << sockPath << fileInfo.exists() << fileInfo.lastModified().toString();

	return app.exec();
}
