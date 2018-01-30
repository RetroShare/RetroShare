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

using namespace resource_api;

int main(int argc, char *argv[])
{
#ifdef __ANDROID__
	AndroidStdIOCatcher dbg; (void) dbg;
#endif

	QCoreApplication app(argc, argv);

	signal(SIGINT, &QCoreApplication::exit);
	signal(SIGTERM, &QCoreApplication::exit);
#ifdef SIGBREAK
	signal(SIGBREAK, &QCoreApplication::exit);
#endif // def SIGBREAK

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
	QObject::connect( &shouldExitTimer, &QTimer::timeout, [&](){
		if(ctrl_mod.processShouldExit()) app.quit(); } );
	shouldExitTimer.start();

	return app.exec();
}
