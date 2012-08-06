
/*
 * libretroshare/src/gxs: gxscoreserver.cc
 *
 * General Data service, interface for RetroShare.
 *
 * Copyright 2011-2011 by Evi-Parker Christopher
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */


#include "gxscoreserver.h"

GxsCoreServer::GxsCoreServer()
: mGxsMutex("GxsCoreServer")
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

