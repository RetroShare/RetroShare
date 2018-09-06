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
#include <csignal>

#ifdef __ANDROID__
#	include "util/androiddebug.h"
#endif

#ifdef LIBRESAPI_LOCAL_SERVER
#	include <QDir>
#	include <QTimer>
#	include <QDebug>

#	include "api/ApiServer.h"
#	include "api/ApiServerLocal.h"
#	include "api/RsControlModule.h"
#else // ifdef LIBRESAPI_LOCAL_SERVER
#	include <QObject>

#	include "retroshare/rsinit.h"
#	include "retroshare/rsiface.h"
#endif // ifdef LIBRESAPI_LOCAL_SERVER

#ifdef RS_JSONAPI
#	include "jsonapi/jsonapi.h"
#	include "util/rsnet.h"
#	include "util/rsurl.h"

#	include <cstdint>
#	include <QCommandLineParser>
#	include <QString>
#	include <iostream>
#endif // def RS_JSONAPI

int main(int argc, char *argv[])
{
#ifdef __ANDROID__
	AndroidStdIOCatcher dbg; (void) dbg;
#endif

	QCoreApplication app(argc, argv);

	signal(SIGINT,   QCoreApplication::exit);
	signal(SIGTERM,  QCoreApplication::exit);
#ifdef SIGBREAK
	signal(SIGBREAK, QCoreApplication::exit);
#endif // ifdef SIGBREAK

#ifdef LIBRESAPI_LOCAL_SERVER
	using namespace resource_api;

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
	{ if(ctrl_mod.processShouldExit()) QCoreApplication::exit(0); } );
	shouldExitTimer.start();
#else // ifdef LIBRESAPI_LOCAL_SERVER
	RsInit::InitRsConfig();
	RsInit::InitRetroShare(argc, argv, true);
	RsControl::earlyInitNotificationSystem();
	QObject::connect(
	            &app, &QCoreApplication::aboutToQuit,
	            [](){
		if(RsControl::instance()->isReady())
			RsControl::instance()->rsGlobalShutDown(); } );
#endif // ifdef LIBRESAPI_LOCAL_SERVER

#ifdef RS_JSONAPI
	uint16_t jsonApiPort = 9092;
	std::string jsonApiBindAddress = "127.0.0.1";

	{
		QCommandLineOption jsonApiPortOpt(
		            "jsonApiPort", "JSON API listening port.", "port", "9092");
		QCommandLineOption jsonApiBindAddressOpt(
		            "jsonApiBindAddress", "JSON API Bind Address.",
		            "IP Address", "127.0.0.1");

		QCommandLineParser cmdParser;
		cmdParser.addHelpOption();
		cmdParser.addOption(jsonApiPortOpt);
		cmdParser.addOption(jsonApiBindAddressOpt);

		cmdParser.parse(app.arguments());

		if(cmdParser.isSet(jsonApiPortOpt))
		{
			QString jsonApiPortStr = cmdParser.value(jsonApiPortOpt);
			bool portOk;
			jsonApiPort = jsonApiPortStr.toUShort(&portOk);
			if(!portOk)
			{
				std::cerr << "ERROR: jsonApiPort option value must be a valid "
				          << "TCP port!" << std::endl;
				cmdParser.showHelp();
				QCoreApplication::exit(EINVAL);
			}
		}

		if(cmdParser.isSet(jsonApiBindAddressOpt))
		{
			sockaddr_storage tmp;
			jsonApiBindAddress =
			        cmdParser.value(jsonApiBindAddressOpt).toStdString();
			if(!sockaddr_storage_inet_pton(tmp, jsonApiBindAddress))
			{
				std::cerr << "ERROR: jsonApiBindAddress option value must "
				          << "be a valid IP address!" << std::endl;
				cmdParser.showHelp();
				QCoreApplication::exit(EINVAL);
			}
		}
	}

	JsonApiServer jas( jsonApiPort, jsonApiBindAddress,
	                   [](int ec) { QCoreApplication::exit(ec); } );
	jas.start();

	{
		sockaddr_storage tmp;
		sockaddr_storage_inet_pton(tmp, jsonApiBindAddress);
		sockaddr_storage_setport(tmp, jsonApiPort);
		sockaddr_storage_ipv6_to_ipv4(tmp);
		RsUrl tmpUrl(sockaddr_storage_tostring(tmp));
		tmpUrl.setScheme("http");

		std::cerr << "JSON API listening on "
		          << tmpUrl.toString()
		          << std::endl;
	}

#endif // ifdef RS_JSONAPI

	return app.exec();
}
