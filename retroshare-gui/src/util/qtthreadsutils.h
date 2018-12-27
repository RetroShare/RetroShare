/*******************************************************************************
 * util/qthreadutils.h                                                         *
 *                                                                             *
 * Copyright (C) 2018  Gioacchino Mazzurco <gio@eigenlab.org>                  *
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
	QObject src;
	auto type = obj->metaObject();
	QObject::connect( &src, &QObject::destroyed, obj,
	                  [fun, type, obj]
	{
		// ensure that the object is not being destructed
		if (obj->metaObject()->inherits(type)) fun();
	}, Qt::QueuedConnection );
}

/**
 * @brief execute given function in the given QThread
 */
template <typename F>
void postToThread(F &&fun, QThread *thread = qApp->thread())
{
	QObject * obj = QAbstractEventDispatcher::instance(thread);
	Q_ASSERT(obj);
	QObject src;
	auto type = obj->metaObject();
	QObject::connect( &src, &QObject::destroyed, obj,
	                  [fun, type, obj]
	{
		// ensure that the object is not being destructed
		if (obj->metaObject()->inherits(type)) fun();
	}, Qt::QueuedConnection );
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

