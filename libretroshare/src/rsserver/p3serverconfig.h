/*******************************************************************************
 * libretroshare/src/rsserver: p3serverconfig.h                                *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2011-2011 by Robert Fernie <retroshare@lunamutt.com>              *
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
#ifndef LIBRETROSHARE_CONFIG_IMPLEMENTATION_H
#define LIBRETROSHARE_CONFIG_IMPLEMENTATION_H

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

virtual RsConfigUserLvl getUserLevel();

virtual RsNetState getNetState();
virtual RsNetworkMode getNetworkMode();
virtual RsNatTypeMode getNatTypeMode();
virtual RsNatHoleMode getNatHoleMode();
virtual RsConnectModes getConnectModes();

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
        RsConfigUserLvl mUserLevel; // store last one... will later be a config Item too.
	float mRateDownload;
	float mRateUpload;

	uint32_t mOpMode;
};

#endif
