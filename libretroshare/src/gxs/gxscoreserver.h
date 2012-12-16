#ifndef GXSCORESERVER_H_
#define GXSCORESERVER_H_

/*
 * libretroshare/src/gxs: gxscoreserver.h
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


#include "util/rsthreads.h"
#include "gxs/rsgxs.h"

class GxsCoreServer : public RsThread
{
public:
	GxsCoreServer();
	~GxsCoreServer();

	void run();

	void addService(RsGxsService* service);
	bool removeService(RsGxsService* service);

private:

	std::set<RsGxsService*> mGxsServices;
	RsMutex mGxsMutex;
};

#endif /* GXSCORESERVER_H_ */
