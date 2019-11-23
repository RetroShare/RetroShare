/*******************************************************************************
 * libretroshare/src/jsonapi/: restbedservice.cc                               *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2019-2019 Cyril Soler                                             *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include "util/rsthreads.h"
#include "util/rsdebug.h"
#include "restbedservice.h"

RestbedService::RestbedService()
{
	mService = std::make_shared<restbed::Service>();	// this is a place holder, in case we request some internal values.
	mListeningPort = 9092;
	mBindingAddress = "127.0.0.1";
}

bool RestbedService::stop()
{
	mService->stop();

	RsThread::ask_for_stop();

	while(isRunning())
	{
		std::cerr << "(II) shutting down restbed service." << std::endl;
		rstime::rs_usleep(1000*1000);
	}
    return true;
}

uint16_t RestbedService::listeningPort() const { return mListeningPort ; }
void RestbedService::setListeningPort(uint16_t p) { mListeningPort = p ; }
void RestbedService::setBindAddress(const std::string& bindAddress) { mBindingAddress = bindAddress ; }

void RestbedService::runloop()
{
	auto settings = std::make_shared< restbed::Settings >( );
	settings->set_port( mListeningPort );
	settings->set_bind_address( mBindingAddress );
	settings->set_default_header( "Connection", "close" );

	if(mService->is_up())
	{
		std::cerr << "(II) WebUI is already running. Killing it." << std::endl;
		mService->stop();
	}

	mService = std::make_shared<restbed::Service>();	// re-allocating mService is important because it deletes the existing service and therefore leaves the listening port open

	for(auto& r:getResources())
		mService->publish(r);

	try
	{
		std::cerr << "(II) Starting restbed service on port " << std::dec << mListeningPort << " and binding address \"" << mBindingAddress << "\"" << std::endl;
		mService->start( settings );
	}
	catch(std::exception& e)
	{
		RsErr() << "Could  not start web interface: " << e.what() << std::endl;
		return;
	}

	std::cerr << "(II) restbed service stopped." << std::endl;
}
