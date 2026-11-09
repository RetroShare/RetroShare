/*******************************************************************************
 * util/qthreadutils.h                                                         *
 *                                                                             *
 * Copyright (C) 2018-2020  Gioacchino Mazzurco <gio@eigenlab.org>             *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#pragma once

/* Thanks to KubaO which realeased original C++14 versions of this functions
 *   https://github.com/KubaO/stackoverflown/blob/master/questions/metacall-21646467/main.cpp
 *   https://github.com/KubaO/stackoverflown/blob/master/LICENSE
 */

#include <QtGlobal>
#include <QtCore>

#include <type_traits>
#include <utility>

namespace RsQThreadUtils {

#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)

/**
 * @brief execute given function in the QThread where given QObject belongs
 */
template <typename F>
void postToObject(F &&fun, QObject *obj = qApp)
{
	if (qobject_cast<QThread*>(obj))
		qWarning() << "posting a call to a thread object - consider using postToThread";
	// Post the functor to obj's thread WITHOUT creating a temporary QObject on the
	// calling thread. Creating a QObject here adopts Qt on a non-Qt worker thread
	// (e.g. std::thread): it allocates per-thread QThreadData and registers a
	// thread_local cleanup, which crashes when that worker thread exits on MinGW
	// (libgcc emutls frees the thread_local storage before the __cxa_thread_atexit
	// destructors run -> NULL deref in Qt's Cleanup::~Cleanup, qthread_win.cpp).
	// invokeMethod posts to obj's thread without touching the caller's Qt state,
	// and Qt cancels the call automatically if obj is destroyed first.
	QMetaObject::invokeMethod(obj, std::forward<F>(fun), Qt::QueuedConnection);
}

/**
 * @brief execute given function in the given QThread
 */
template <typename F>
void postToThread(F &&fun, QThread *thread = qApp->thread())
{
	QObject * obj = QAbstractEventDispatcher::instance(thread);
	Q_ASSERT(obj);
	// Same rationale as postToObject: never create a QObject on the calling thread.
	QMetaObject::invokeMethod(obj, std::forward<F>(fun), Qt::QueuedConnection);
}

#else // QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)

template <typename F>
struct FEvent : QEvent
{
	using Fun = typename std::decay<F>::type;
	const QObject *const obj;
	const QMetaObject *const type = obj->metaObject();
	Fun fun;
	template <typename Fun>
	FEvent(const QObject *obj, Fun &&fun) :
	    QEvent(QEvent::None), obj(obj), fun(std::forward<Fun>(fun)) {}
	~FEvent()
	{
#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
		// ensure that the object is not being destructed
		if (obj->metaObject()->inherits(type)) fun();
#else // QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
		fun();
#endif // QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
	}
};

/**
 * @brief execute given function in the QThread where given QObject belongs
 */
template <typename F>
static void postToObject(F &&fun, QObject *obj = qApp)
{
	if (qobject_cast<QThread*>(obj))
		qWarning() << "posting a call to a thread object - consider using postToThread";
	QCoreApplication::postEvent(obj, new FEvent<F>(obj, std::forward<F>(fun)));
}

/**
 * @brief execute given function in the given QThread
 */
template <typename F>
static void postToThread(F &&fun, QThread *thread = qApp->thread())
{
	QObject * obj = QAbstractEventDispatcher::instance(thread);
	Q_ASSERT(obj);
	QCoreApplication::postEvent(obj, new FEvent<F>(obj, std::forward<F>(fun)));
}
#endif // QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)

}

