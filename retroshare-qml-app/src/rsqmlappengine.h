#pragma once
/*
 * RetroShare Qml App
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
#include <QQmlApplicationEngine>


class RsQmlAppEngine : public QQmlApplicationEngine
{
	Q_OBJECT

public:

	RsQmlAppEngine(bool isMainInstance = false, QObject* parent = nullptr) :
	    QQmlApplicationEngine(parent)
	{ if(isMainInstance) mMainInstance = this; }

	~RsQmlAppEngine() { if(mMainInstance == this) mMainInstance = nullptr; }

	static RsQmlAppEngine* mainInstance() { return mMainInstance; }

public slots:
	void handleUri(QString uri);

private:

	/**
	 * Using a static variable as QQmlApplicationEngine singleton caused crash
	 * on application termination, while using a dinamically allocated one
	 * causes deadlock on termination in QML WorkerScript destructor object
	 * with the following stacktrace
	 * #0  0x00007f0a8c3e4c67 in sched_yield () from /lib64/libc.so.6
	 * #1  0x00007f0a8db5191c in QQuickWorkerScriptEngine::~QQuickWorkerScriptEngine() () from /usr/lib64/libQt5Qml.so.5
	 * #2  0x00007f0a8db51949 in QQuickWorkerScriptEngine::~QQuickWorkerScriptEngine() () from /usr/lib64/libQt5Qml.so.5
	 * #3  0x00007f0a8d61359c in QObjectPrivate::deleteChildren() () from /usr/lib64/libQt5Core.so.5
	 * #4  0x00007f0a8d61ab83 in QObject::~QObject() () from /usr/lib64/libQt5Core.so.5
	 * #5  0x00007f0a8da7d27d in QQmlEngine::~QQmlEngine() () from /usr/lib64/libQt5Qml.so.5
	 * #6  0x00007f0a8dafeb39 in QQmlApplicationEngine::~QQmlApplicationEngine() () from /usr/lib64/libQt5Qml.so.5
	 * #7  0x00007f0a8d61359c in QObjectPrivate::deleteChildren() () from /usr/lib64/libQt5Core.so.5
	 * #8  0x00007f0a8d61ab83 in QObject::~QObject() () from /usr/lib64/libQt5Core.so.5
	 * #9  0x00007f0a8d5ede58 in QCoreApplication::~QCoreApplication() () from /usr/lib64/libQt5Core.so.5
	 * #10 0x00007f0a8dd2d97b in QGuiApplication::~QGuiApplication() () from /usr/lib64/libQt5Gui.so.5
	 * #11 0x000000000041b6b4 in main (argc=1, argv=0x7fff218dd1a8) at ../../../../Development/rs-develop/retroshare-qml-app/src/main-app.cpp:58
	 *
	 * To avoid this we leave the creation of the instance to the user (main)
	 * and to store the static pointer to that, the pointer can be null at early
	 * stage of execution (or if the user forget to initialize it properly) so
	 * early user (JNI intent handler) should take this in account
	 */
	static RsQmlAppEngine* mMainInstance;
};
