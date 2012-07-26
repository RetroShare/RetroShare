/*
 * gxscoreserver.cpp
 *
 *  Created on: 24 Jul 2012
 *      Author: crispy
 */

#include "gxscoreserver.h"

GxsCoreServer::GxsCoreServer()
{

}

GxsCoreServer::~GxsCoreServer()
{

	std::set<RsGxsService*>::iterator sit;

	for(sit = mGxsServices.begin(); sit != mGxsServices.end(); sit++)
		delete *sit;

}


void GxsCoreServer::run()
{
	std::set<RsGxsService*>::iterator sit;

	double timeDelta = 0.2;

	while(isRunning())
	{

#ifndef WINDOWS_SYS
        usleep((int) (timeDelta * 1000000));
#else
        Sleep((int) (timeDelta * 1000));
#endif

		for(sit = mGxsServices.begin(); sit != mGxsServices.end(); sit++)
				(*sit)->tick();

	}
}

void GxsCoreServer::addService(RsGxsService* service)
{
	mGxsServices.insert(service);
}

bool GxsCoreServer::removeService(RsGxsService* service)
{
	return (mGxsServices.erase(service) > 0);
}

