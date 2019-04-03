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
#include "pqi/p3cfgmgr.h"
#include "pqi/pqihandler.h"


#define RS_CONFIG_ADVANCED_STRING       "AdvMode"



class p3ServerConfig: public RsServerConfig
{
	public:

	p3ServerConfig(p3PeerMgr *peerMgr, p3LinkMgr *linkMgr, p3NetMgr *netMgr, pqihandler *pqih, p3GeneralConfig *genCfg);
virtual ~p3ServerConfig();

	void load_config();

	/* From RsIface::RsConfig */

virtual int 	getConfigNetStatus(RsConfigNetStatus &status);
virtual int 	getConfigStartup(RsConfigStartup &params);
//virtual int 	getConfigDataRates(RsConfigDataRates &params);

                /***** for RsConfig -> p3BandwidthControl ****/

virtual int getTotalBandwidthRates(RsConfigDataRates &rates);
virtual int getAllBandwidthRates(std::map<RsPeerId, RsConfigDataRates> &ratemap);
    virtual int getTrafficInfo(std::list<RSTrafficClue>& out_lst, std::list<RSTrafficClue> &in_lst) ;

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

virtual bool getConfigurationOption(uint32_t key, std::string &opt);
virtual bool setConfigurationOption(uint32_t key, const std::string &opt);

	/* Operating Mode */
virtual uint32_t getOperatingMode();
virtual bool     setOperatingMode(uint32_t opMode);
virtual bool     setOperatingMode(const std::string &opModeStr);

virtual int SetMaxDataRates( int downKb, int upKb );
virtual int GetMaxDataRates( int &downKb, int &upKb );
virtual int GetCurrentDataRates( float &inKb, float &outKb );
virtual int GetTrafficSum( uint64_t &inb, uint64_t &outb );

/********************* ABOVE is RsConfig Interface *******/

	private:

bool switchToOperatingMode(uint32_t opMode);

bool findConfigurationOption(uint32_t key, std::string &keystr);

	p3PeerMgr *mPeerMgr;
	p3LinkMgr *mLinkMgr;
	p3NetMgr  *mNetMgr;
	pqihandler *mPqiHandler;
	p3GeneralConfig *mGeneralConfig;

	RsMutex configMtx;
	uint32_t mUserLevel; // store last one... will later be a config Item too.
	float mRateDownload;
	float mRateUpload;

	uint32_t mOpMode;
};

#endif
