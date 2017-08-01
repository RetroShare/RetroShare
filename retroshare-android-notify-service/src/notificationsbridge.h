#pragma once
/*
 * RetroShare Android Service
 * Copyright (C) 2017  Gioacchino Mazzurco <gio@eigenlab.org>
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

#include <QObject>
#include <QString>
#include <QDebug>

#ifdef __ANDROID__
#	include <QtAndroid>
#	include <QtAndroidExtras/QAndroidJniObject>
#endif // __ANDROID__

struct NotificationsBridge : QObject
{
	Q_OBJECT

public slots:
	static void notify(const QString& title, const QString& text = "",
	                   const QString& uri = "")
	{
		qDebug() << __PRETTY_FUNCTION__ << title << text << uri;

#ifdef __ANDROID__
		QtAndroid::androidService().callMethod<void>(
		            "notify",
		            "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V",
		            QAndroidJniObject::fromString(title).object(),
		            QAndroidJniObject::fromString(text).object(),
		            QAndroidJniObject::fromString(uri).object()
		            );
#endif // __ANDROID__
	}
};
