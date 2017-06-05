/*
 * RetroShare Android Service
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

#include <QCoreApplication>
#include <QDebug>
#include <QMetaObject>
#include <QDir>

#ifdef __ANDROID__
#	include "util/androiddebug.h"
#endif

#include "api/ApiServer.h"
#include "api/ApiServerLocal.h"
#include "api/RsControlModule.h"

using namespace resource_api;

int main(int argc, char *argv[])
{
#ifdef __ANDROID__
	AndroidStdIOCatcher dbg; (void) dbg;
#endif

	QCoreApplication app(argc, argv);
	ApiServer api;
	RsControlModule ctrl_mod(argc, argv, api.getStateTokenServer(), &api, true);
	api.addResourceHandler(
	            "control",
	            dynamic_cast<resource_api::ResourceRouter*>(&ctrl_mod),
	            &resource_api::RsControlModule::handleRequest);


	QString sockPath = QDir::homePath() + "/.retroshare";
	sockPath.append("/libresapi.sock");
	qDebug() << "Listening on:" << sockPath;

	ApiServerLocal apiServerLocal(&api, sockPath); (void) apiServerLocal;

	while (!ctrl_mod.processShouldExit())
	{
		app.processEvents();
		usleep(20000);
	}

	/* Since QCoreApplication::quit() is a no-op until the event loop has been
	 * started, we need to defer the call until it starts. Thus, we queue a
	 * deferred method call to quit() */
	QMetaObject::invokeMethod(&app, "quit", Qt::QueuedConnection);

	return app.exec();
}
