/*
 * RetroShare Android Service
 * Copyright (C) 2016-2018  Gioacchino Mazzurco <gio@eigenlab.org>
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
#include <QDir>
#include <QTimer>
#include <csignal>

#ifdef __ANDROID__
#	include "util/androiddebug.h"
#endif

#include "api/ApiServer.h"
#include "api/ApiServerLocal.h"
#include "api/RsControlModule.h"

#ifdef RS_JSONAPI
#	include "jsonapi/jsonapi.h"
#	include "retroshare/rsiface.h"

JsonApiServer jas(9092, [](int ec)
{
	RsControl::instance()->rsGlobalShutDown();
	QCoreApplication::exit(ec);
});

void exitGracefully(int ec) { jas.shutdown(ec); }

#else // ifdef RS_JSONAPI
void exitGracefully(int ec) { QCoreApplication::exit(ec); }
#endif // ifdef RS_JSONAPI

using namespace resource_api;

int main(int argc, char *argv[])
{
#ifdef __ANDROID__
	AndroidStdIOCatcher dbg; (void) dbg;
#endif

	QCoreApplication app(argc, argv);

	signal(SIGINT, exitGracefully);
	signal(SIGTERM, exitGracefully);
#ifdef SIGBREAK
	signal(SIGBREAK, exitGracefully);
#endif // ifdef SIGBREAK

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

	// This ugly but RsControlModule has no other way to callback for stop
	QTimer shouldExitTimer;
	shouldExitTimer.setTimerType(Qt::VeryCoarseTimer);
	shouldExitTimer.setInterval(1000);
	QObject::connect( &shouldExitTimer, &QTimer::timeout, [&]()
	{ if(ctrl_mod.processShouldExit()) exitGracefully(0); } );
	shouldExitTimer.start();

#ifdef RS_JSONAPI
	jas.start();
#endif

	return app.exec();
}
