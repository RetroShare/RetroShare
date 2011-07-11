#ifndef LIBRETROSHARE_CONFIG_IMPLEMENTATION_H
#define LIBRETROSHARE_CONFIG_IMPLEMENTATION_H

/*
 * libretroshare/src/rsserver: p3serverconfig.h
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2011-2011 by Robert Fernie.
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


#include "retroshare/rsconfig.h"
#include "pqi/p3peermgr.h"
#include "pqi/p3linkmgr.h"
#include "pqi/p3netmgr.h"

class p3ServerConfig: public RsServerConfig
{
	public:

	p3ServerConfig(p3PeerMgr *peerMgr, p3LinkMgr *linkMgr, p3NetMgr *netMgr);
virtual ~p3ServerConfig();

	/* From RsIface::RsConfig */

virtual int 	getConfigNetStatus(RsConfigNetStatus &status);
virtual int 	getConfigStartup(RsConfigStartup &params);
virtual int 	getConfigDataRates(RsConfigDataRates &params);

	/* From RsInit */

virtual std::string      RsConfigDirectory();
virtual std::string      RsConfigKeysDirectory();

virtual std::string  RsProfileConfigDirectory();
virtual bool         getStartMinimised();
virtual std::string  getRetroShareLink();

virtual bool getAutoLogin();
virtual void setAutoLogin(bool autoLogin);
virtual bool RsClearAutoLogin();

virtual std::string getRetroshareDataDirectory();

	/* New Stuff */

virtual uint32_t getUserLevel();

virtual uint32_t getNetState();
virtual uint32_t getNetworkMode();
virtual uint32_t getNatTypeMode();
virtual uint32_t getNatHoleMode();
virtual uint32_t getConnectModes();

/********************* ABOVE is RsConfig Interface *******/

	private:

	p3PeerMgr *mPeerMgr;
	p3LinkMgr *mLinkMgr;
	p3NetMgr  *mNetMgr;

	RsMutex configMtx;
	uint32_t mUserLevel; // store last one... will later be a config Item too.

};

#endif
