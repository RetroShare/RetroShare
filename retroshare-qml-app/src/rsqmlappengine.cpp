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

#include "rsqmlappengine.h"

#include <QQuickWindow>
#include <QDebug>


/*static*/ RsQmlAppEngine* RsQmlAppEngine::mMainInstance = nullptr;

void RsQmlAppEngine::handleUri(QString uri)
{
	QObject* rootObj = rootObjects()[0];
	QQuickWindow* mainWindow = qobject_cast<QQuickWindow*>(rootObj);

	if(mainWindow)
	{
		QMetaObject::invokeMethod(mainWindow, "handleIntentUri",
		                          Qt::AutoConnection,
		                          Q_ARG(QVariant, uri));
	}
	else qCritical() << __PRETTY_FUNCTION__
	                 << "Root object is not a window!";
}
