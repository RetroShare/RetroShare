/*
 * RetroShare Android QML App
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

#include "NativeCalls.h"
#include "rsqmlappengine.h"

#include <QMetaObject>
#include <QDebug>

JNIEXPORT void JNICALL
Java_org_retroshare_android_qml_1app_jni_NativeCalls_notifyIntentUri
(JNIEnv* env, jclass, jstring uri)
{
	qDebug() << __PRETTY_FUNCTION__;

	const char *uriBytes = env->GetStringUTFChars(uri, NULL);
	QString uriStr(uriBytes);
	env->ReleaseStringUTFChars(uri, uriBytes);

	RsQmlAppEngine* engine = RsQmlAppEngine::mainInstance();
	if(engine)
		QMetaObject::invokeMethod(
		            engine, "handleUri",
		            Qt::QueuedConnection, // BlockingQueuedConnection, AutoConnection
		            Q_ARG(QString, uriStr));
	else qCritical() << __PRETTY_FUNCTION__ << "RsQmlAppEngine::mainInstance()"
	                 << "not initialized yet!";
}
