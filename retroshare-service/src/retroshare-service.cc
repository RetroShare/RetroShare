/*
 * RetroShare Service
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

#include "util/stacktrace.h"

CrashStackTrace gCrashStackTrace;

#include <QCoreApplication>
#include <csignal>
#include <QObject>
#include <QStringList>

#ifdef __ANDROID__
#	include <QAndroidService>
#endif // def __ANDROID__

#include "retroshare/rsinit.h"
#include "retroshare/rsiface.h"

#ifdef __ANDROID__
#	include "util/androiddebug.h"
#endif

#ifndef RS_JSONAPI
#	error Inconsistent build configuration retroshare_service needs rs_jsonapi
#endif

int main(int argc, char* argv[])
{
#ifdef __ANDROID__
	AndroidStdIOCatcher dbg; (void) dbg;
	QAndroidService app(argc, argv);
#else // def __ANDROID__
	QCoreApplication app(argc, argv);
#endif // def __ANDROID__

	signal(SIGINT,   QCoreApplication::exit);
	signal(SIGTERM,  QCoreApplication::exit);
#ifdef SIGBREAK
	signal(SIGBREAK, QCoreApplication::exit);
#endif // ifdef SIGBREAK

	RsInit::InitRsConfig();

	// clumsy way to enable JSON API by default
	if(!QCoreApplication::arguments().contains("--jsonApiPort"))
	{
		int argc2 = argc + 2;
		char* argv2[argc2]; for (int i = 0; i < argc; ++i ) argv2[i] = argv[i];
		char opt[] = "--jsonApiPort";
		char val[] = "9092";
		argv2[argc] = opt;
		argv2[argc+1] = val;
		RsInit::InitRetroShare(argc2, argv2, true);
	}
	else RsInit::InitRetroShare(argc, argv, true);

	RsControl::earlyInitNotificationSystem();
	rsControl->setShutdownCallback(QCoreApplication::exit);
	QObject::connect(
	            &app, &QCoreApplication::aboutToQuit,
	            [](){
		if(RsControl::instance()->isReady())
			RsControl::instance()->rsGlobalShutDown(); } );

	return app.exec();
}
