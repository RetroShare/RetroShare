/*******************************************************************************
 * libretroshare/src/zeroconf: p3zcnatassist.cc                                *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2011-2012 by Robert Fernie <retroshare@lunamutt.com>              *
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
#include "zeroconf/p3zeroconf.h"
#include "zeroconf/p3zcnatassist.h"

#include <openssl/sha.h>
#include <iostream>

//#define DEBUG_ZCNATASSIST	1


p3zcNatAssist::p3zcNatAssist()
	:mZcMtx("p3zcNatAssist")
{

#ifdef DEBUG_ZCNATASSIST
	std::cerr << "p3zcNatAssist::p3zcNatAssist()" << std::endl;
	std::cerr << std::endl;
#endif

	mMappingStatus = ZC_SERVICE_STOPPED;
	mMappingStatusTS = time(NULL);

	mLocalPort = 0;
	mLocalPortSet = false;

	mExternalPort = 0;
	mExternalPortSet = false;

	mMapped = false;

}

p3zcNatAssist::~p3zcNatAssist()
{
	shutdown();
}

	/* pqiNetAssist - external interface functions */
void    p3zcNatAssist::enable(bool on)
{
#ifdef DEBUG_ZCNATASSIST
	std::cerr << "p3zcNatAssist::enable(" << on << ")";
	std::cerr << std::endl;
#endif

	RsStackMutex stack(mZcMtx); /****** STACK LOCK MUTEX *******/

	mEnabled = on;

	if ((!mEnabled) && (mMappingStatus == ZC_SERVICE_ACTIVE))
	{
		locked_stopMapping();
	}
		
}
	
void    p3zcNatAssist::shutdown() /* blocking call */
{

#ifdef DEBUG_ZCNATASSIST
	std::cerr << "p3zcNatAssist::shutdown()";
	std::cerr << std::endl;
#endif

	enable(false);	
}

void	p3zcNatAssist::restart()
{
#ifdef DEBUG_ZCNATASSIST
	std::cerr << "p3zcNatAssist::restart()";
	std::cerr << std::endl;
#endif

	RsStackMutex stack(mZcMtx); /****** STACK LOCK MUTEX *******/

	if (mMappingStatus == ZC_SERVICE_ACTIVE)
	{
		locked_stopMapping();
	}
}

bool    p3zcNatAssist::getEnabled()
{
	RsStackMutex stack(mZcMtx); /****** STACK LOCK MUTEX *******/

#ifdef DEBUG_ZCNATASSIST
	std::cerr << "p3zcNatAssist::getEnabled() : " << mEnabled;
	std::cerr << std::endl;
#endif

	return mEnabled;
}

bool    p3zcNatAssist::getActive()
{
	RsStackMutex stack(mZcMtx); /****** STACK LOCK MUTEX *******/

#ifdef DEBUG_ZCNATASSIST
	std::cerr << "p3zcNatAssist::getActive() : " << (mEnabled && mMapped);
	std::cerr << std::endl;
#endif

	return (mEnabled && mMapped);
}

void    p3zcNatAssist::setInternalPort(unsigned short iport_in)
{

#ifdef DEBUG_ZCNATASSIST
	std::cerr << "p3zcNatAssist::setInternalPort() : " << iport_in;
	std::cerr << std::endl;
#endif

	bool changed = false;
	{
		RsStackMutex stack(mZcMtx); /****** STACK LOCK MUTEX *******/

		changed = (mEnabled) && (mLocalPort != iport_in);
		mLocalPort = iport_in;
		mLocalPortSet = true;

	}

	if (changed)
	{
		restart();
	}
}

void    p3zcNatAssist::setExternalPort(unsigned short eport_in)
{

#ifdef DEBUG_ZCNATASSIST
	std::cerr << "p3zcNatAssist::setExternalPort() : " << eport_in;
	std::cerr << std::endl;
#endif

	bool changed = false;
	{
		RsStackMutex stack(mZcMtx); /****** STACK LOCK MUTEX *******/

		changed = (mEnabled) && (mExternalPort != eport_in);
		mExternalPort = eport_in;
		mExternalPortSet = true;

	}

	if (changed)
	{
		restart();
	}
}

bool    p3zcNatAssist::getInternalAddress(struct sockaddr_storage &addr)
{

#ifdef DEBUG_ZCNATASSIST
	std::cerr << "p3zcNatAssist::getInternalAddress() always returns false";
	std::cerr << std::endl;
#endif

	return false;
}


bool    p3zcNatAssist::getExternalAddress(struct sockaddr_storage &addr)
{
	RsStackMutex stack(mZcMtx); /****** STACK LOCK MUTEX *******/

	if (mMapped)
	{
#ifdef DEBUG_ZCNATASSIST
		std::cerr << "p3zcNatAssist::getExternalAddress() mMapped => true";
		std::cerr << std::endl;
#endif
		addr = mExternalAddress;
		return true;
	}


#ifdef DEBUG_ZCNATASSIST
	std::cerr << "p3zcNatAssist::getExternalAddress() !mMapped => false";
	std::cerr << std::endl;
#endif

	return false;
}


/***********************************************************************************/

int p3zcNatAssist::tick()
{
	{
		RsStackMutex stack(mZcMtx); /****** STACK LOCK MUTEX *******/

		locked_startMapping();
	}


	checkServiceFDs(); // will cause callbacks - if data is ready.
	
	return 0;
}

void p3zcNatAssist::checkServiceFDs()
{
	RsStackMutex stack(mZcMtx); /****** STACK LOCK MUTEX *******/

	if (mMappingStatus == ZC_SERVICE_ACTIVE)
	{
		locked_checkFD(mMappingRef);
	}
}


void p3zcNatAssist::locked_checkFD(DNSServiceRef ref)
{
	//std::cerr << "p3zcNatAssist::locked_checkFD() Start";
	//std::cerr << std::endl;

	int sockfd = DNSServiceRefSockFD(ref);

	fd_set ReadFDs, WriteFDs, ExceptFDs;
	FD_ZERO(&ReadFDs);
	FD_ZERO(&WriteFDs);
	FD_ZERO(&ExceptFDs);
	
	FD_SET(sockfd, &ReadFDs);

	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	
	if (select(sockfd + 1, &ReadFDs, &WriteFDs, &ExceptFDs, &timeout) < 0)
	{
		std::cerr << "p3zeroconf::checkFD() Select ERROR";
		std::cerr << std::endl;
		return;
	}
	
	if (FD_ISSET(sockfd, &ReadFDs))
	{
		DNSServiceErrorType err = DNSServiceProcessResult(ref);
		switch(err)
		{
			case 0:
				/* ok */
				break;
			default:
				std::cerr << "DNSServiceProcessResult() ERROR: ";
				std::cerr << displayDNSServiceError(err);
				std::cerr << std::endl;
				break;
		}
	}

	//std::cerr << "p3zcNatAssist::locked_checkFD() End";
	//std::cerr << std::endl;
	return;

}


void p3zcNatAssist_CallbackMapping( DNSServiceRef sdRef, DNSServiceFlags flags,
    					uint32_t interfaceIndex, DNSServiceErrorType errorCode, 
    					uint32_t externalAddress, DNSServiceProtocol protocol, 
					uint16_t internalPort, uint16_t externalPort, 
    					uint32_t ttl, void *context )
{
	p3zcNatAssist *zc = (p3zcNatAssist *) context;
	zc->callbackMapping(sdRef, flags, interfaceIndex, errorCode, externalAddress, protocol, internalPort, externalPort, ttl);
}


int p3zcNatAssist::locked_startMapping()
{
	if (!mEnabled)
	{
		std::cerr << "p3zcNatAssist::locked_startMapping()";
		std::cerr << "NatAssist not Enabled";
		std::cerr << std::endl;
		return 0;
	}

	if (!mLocalPortSet)
	{
		std::cerr << "p3zcNatAssist::locked_startMapping()";
		std::cerr << "Port Not Set";
		std::cerr << std::endl;
		return 0;
	}

	if (mMappingStatus == ZC_SERVICE_ACTIVE)
	{
		return 0;
	}
	

	std::cerr << "p3zcNatAssist::locked_startMapping() Mapping!";
	std::cerr << std::endl;
	std::cerr << "p3zcNatAssist::locked_startMapping() Local Port: " << mLocalPort;
	std::cerr << std::endl;

	DNSServiceRef *sdRef = &mMappingRef;
	DNSServiceFlags flags = 0; /* no flags, currently reserved */
	uint32_t interfaceIndex = 0; /* primary interface */

	DNSServiceProtocol protocol = (kDNSServiceProtocol_UDP | kDNSServiceProtocol_TCP);
	uint16_t internalPort = htons(mLocalPort);
	uint16_t externalPort = htons(mLocalPort);
	if (mExternalPortSet)
	{
		externalPort = htons(mExternalPort);
	}
	
	std::cerr << "p3zcNatAssist::locked_startMapping() External Port: " << ntohs(externalPort);
	std::cerr << std::endl;
	
	uint32_t ttl = 0; /* default ttl */

    	DNSServiceNATPortMappingReply callBack = p3zcNatAssist_CallbackMapping; 
	void *context = this;

	DNSServiceErrorType errcode = DNSServiceNATPortMappingCreate(sdRef, flags, interfaceIndex, protocol, 
    				internalPort, externalPort, ttl, callBack, context);  

	if (errcode != kDNSServiceErr_NoError)
	{
		std::cerr <<  "p3zcNatAssist::locked_startMapping() ERROR: ";
		std::cerr << displayDNSServiceError(errcode);
		std::cerr << std::endl;
	}
	else
	{
		mMappingStatus = ZC_SERVICE_ACTIVE;
		mMappingStatusTS = time(NULL);
	}

	return 1;
}



void p3zcNatAssist::callbackMapping(DNSServiceRef sdRef, DNSServiceFlags flags,
    					uint32_t interfaceIndex, DNSServiceErrorType errorCode, 
    					uint32_t externalAddress, DNSServiceProtocol protocol, 
					uint16_t internalPort, uint16_t externalPort, uint32_t ttl)
{
	std::cerr << "p3zcNatAssist::callbackMapping()";
	std::cerr << std::endl;

	/* handle queryIp */
	if (errorCode != kDNSServiceErr_NoError)
	{
		std::cerr <<  "p3zcNatAssist::callbackMapping() FAILED ERROR: ";
		std::cerr << displayDNSServiceError(errorCode);
		std::cerr << std::endl;
		return;
	}

	if ((externalAddress == 0) && (externalPort == 0))
	{
		/* failed :( */
		mMapped = false;

		std::cerr <<  "p3zcNatAssist::callbackMapping() ZeroAddress ... Mapping not possible";
		std::cerr << std::endl;
		return;
	}


	mMapped = true;
	
	// THIS APPEARS TO BE IPv4 ONLY!.
	
	sockaddr_storage_clear(mExternalAddress);
	struct sockaddr_in *addr = (struct sockaddr_in *) &mExternalAddress;
	
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = externalAddress;
	addr->sin_port = externalPort;
	
	mTTL = ttl;

	std::cerr <<  "p3zcNatAssist::callbackMapping() Success";
	std::cerr << std::endl;

	std::cerr <<  "interfaceIndex: " << interfaceIndex;
	std::cerr << std::endl;

	std::cerr <<  "internalPort: " << ntohs(internalPort);
	std::cerr << std::endl;

	std::cerr <<  "externalAddress: " << sockaddr_storage_tostring(mExternalAddress);
	std::cerr << std::endl;

	std::cerr <<  "protocol: " << protocol;
	std::cerr << std::endl;

	std::cerr <<  "ttl: " << ttl;
	std::cerr << std::endl;

}


void p3zcNatAssist::locked_stopMapping()
{
	std::cerr << "p3zcNatAssist::locked_stopMapping()";
	std::cerr << std::endl;

	if (mMappingStatus != ZC_SERVICE_ACTIVE)
	{
		return;
	}

	DNSServiceRefDeallocate(mMappingRef);

	mMappingStatus = ZC_SERVICE_STOPPED;
	mMappingStatusTS = time(NULL);
	mMapped = false;
}


