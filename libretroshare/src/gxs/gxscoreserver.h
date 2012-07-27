/*
 * gxscoreserver.h
 *
 *  Created on: 24 Jul 2012
 *      Author: crispy
 */

#ifndef GXSCORESERVER_H_
#define GXSCORESERVER_H_

#include "util/rsthreads.h"
#include "gxs/rsgxs.h"

class GxsCoreServer : RsThread {
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
