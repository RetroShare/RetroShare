/*******************************************************************************
 * libretroshare/src/rsserver: p3serverconfig.cc                               *
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
#include <retroshare/rsturtle.h>
#include "rsserver/p3serverconfig.h"
#include "services/p3bwctrl.h"

#include "pqi/authgpg.h"
#include "pqi/authssl.h"

RsServerConfig *rsConfig = NULL;

static const std::string pqih_ftr("PQIH_FTR");

#define DEFAULT_DOWNLOAD_KB_RATE       (10000.0)
#define DEFAULT_UPLOAD_KB_RATE         (10000.0)

#define MIN_MINIMAL_RATE	(5.0)


p3ServerConfig::p3ServerConfig(p3PeerMgr *peerMgr, p3LinkMgr *linkMgr, p3NetMgr *netMgr, pqihandler *pqih, p3GeneralConfig *genCfg)
:configMtx("p3ServerConfig")
{
	mPeerMgr = peerMgr;
	mLinkMgr = linkMgr;
	mNetMgr = netMgr;
	mPqiHandler = pqih;

	mGeneralConfig = genCfg;

	RsStackMutex stack(configMtx); /******* LOCKED MUTEX *****/

	mUserLevel = RSCONFIG_USER_LEVEL_NEW; /* START LEVEL */
	mRateDownload =  DEFAULT_DOWNLOAD_KB_RATE;
	mRateUpload = DEFAULT_UPLOAD_KB_RATE;

	mOpMode = RS_OPMODE_FULL;

	rsConfig = this;
}


p3ServerConfig::~p3ServerConfig() 
{ 
	return; 
}


void p3ServerConfig::load_config()
{
	/* get the real bandwidth setting from GeneralConfig */
        std::string rates = mGeneralConfig -> getSetting(pqih_ftr);

        float mri, mro;
        if (2 == sscanf(rates.c_str(), "%f %f", &mri, &mro))
        {
		RsStackMutex stack(configMtx); /******* LOCKED MUTEX *****/

		mRateDownload = mri;
		mRateUpload = mro;
        }
        else
        {
		RsStackMutex stack(configMtx); /******* LOCKED MUTEX *****/

		mRateDownload =  DEFAULT_DOWNLOAD_KB_RATE;
		mRateUpload = DEFAULT_UPLOAD_KB_RATE;
        }

	/* enable operating mode */
	uint32_t opMode = getOperatingMode();
	switchToOperatingMode(opMode);
}


#define RS_CONFIG_ADVANCED_STRING 	"AdvMode"

bool p3ServerConfig::findConfigurationOption(uint32_t key, std::string &keystr)
{
	bool found = false;
	switch(key)
	{
		case RS_CONFIG_ADVANCED:
			keystr = RS_CONFIG_ADVANCED_STRING;
			found = true;
			break;
	}
	return found;
}


bool p3ServerConfig::getConfigurationOption(uint32_t key, std::string &opt)
{
	std::string strkey;
	if (!findConfigurationOption(key, strkey))
	{
		std::cerr << "p3ServerConfig::getConfigurationOption() OPTION NOT VALID: " << key;
		std::cerr << std::endl;
		return false;
	}

	opt = mGeneralConfig->getSetting(strkey);
	return true;
}

		
bool p3ServerConfig::setConfigurationOption(uint32_t key, const std::string &opt)
{
	std::string strkey;
	if (!findConfigurationOption(key, strkey))
	{
		std::cerr << "p3ServerConfig::setConfigurationOption() OPTION NOT VALID: " << key;
		std::cerr << std::endl;
		return false;
	}

	mGeneralConfig->setSetting(strkey, opt);
	return true;
}

	/* From RsIface::RsConfig */
int 	p3ServerConfig::getConfigNetStatus(RsConfigNetStatus &status)
{
	status.ownId = AuthSSL::getAuthSSL()->OwnId();
	status.ownName = AuthGPG::getAuthGPG()->getGPGOwnName();

	// Details from PeerMgr.
	peerState pstate;
	mPeerMgr->getOwnNetStatus(pstate);

	status.localAddr = sockaddr_storage_iptostring(pstate.localaddr);
	status.localPort = sockaddr_storage_port(pstate.localaddr);

	status.extAddr = sockaddr_storage_iptostring(pstate.serveraddr);
	status.extPort = sockaddr_storage_port(pstate.serveraddr);
	status.extDynDns = pstate.dyndns;

	status.firewalled = true;
	status.forwardPort  = true;

	/* update network configuration */
	pqiNetStatus nstatus;
	mNetMgr->getNetStatus(nstatus);

	status.netUpnpOk  = nstatus.mUpnpOk;
	status.netStunOk  = false;
	status.netExtAddressOk = nstatus.mExtAddrOk;

	status.netDhtOk   = nstatus.mDhtOk;
	status.netDhtNetSize = nstatus.mDhtNetworkSize;
	status.netDhtRsNetSize = nstatus.mDhtRsNetworkSize;

	/* update DHT/UPnP status */
	status.uPnPState  = mNetMgr->getUPnPState();
	status.uPnPActive = mNetMgr->getUPnPEnabled();
	status.DHTActive  = mNetMgr->getDHTEnabled();

	return 1;
}

int 	p3ServerConfig::getConfigStartup(RsConfigStartup &/*params*/)
{
        //status.promptAtBoot = true; /* popup the password prompt */
	return 0;
}

/***** for RsConfig -> p3BandwidthControl ****/

int p3ServerConfig::getTrafficInfo(std::list<RSTrafficClue>& out_lst,std::list<RSTrafficClue>& in_lst)
{

    if (rsBandwidthControl)
        return rsBandwidthControl->ExtractTrafficInfo(out_lst,in_lst);
    else
        return 0 ;
}

int 	p3ServerConfig::getTotalBandwidthRates(RsConfigDataRates &rates)
{
	if (rsBandwidthControl)
	{
		return rsBandwidthControl->getTotalBandwidthRates(rates);
	}
	return 0;
}

int 	p3ServerConfig::getAllBandwidthRates(std::map<RsPeerId, RsConfigDataRates> &ratemap)
{
	if (rsBandwidthControl)
	{
		return rsBandwidthControl->getAllBandwidthRates(ratemap);
	}
	return 0;
}


	/* From RsInit */

std::string      p3ServerConfig::RsConfigDirectory()
{
	return std::string();
}

std::string      p3ServerConfig::RsConfigKeysDirectory()
{
	return std::string();
}


std::string  p3ServerConfig::RsProfileConfigDirectory()
{
	return std::string();
}

bool         p3ServerConfig::getStartMinimised()
{
	return 0;
}

std::string  p3ServerConfig::getRetroShareLink()
{
	return std::string();
}


bool p3ServerConfig::getAutoLogin()
{
	return 0;
}

void p3ServerConfig::setAutoLogin(bool /*autoLogin*/)
{
	return;
}

bool p3ServerConfig::RsClearAutoLogin()
{
	return 0;
}


std::string p3ServerConfig::getRetroshareDataDirectory()
{
	return std::string();
}

	/* New Stuff */

uint32_t p3ServerConfig::getUserLevel()
{
	uint32_t userLevel = RSCONFIG_USER_LEVEL_NEW;
	{
		RsStackMutex stack(configMtx); /******* LOCKED MUTEX *****/
		userLevel = mUserLevel;
	}

	switch(userLevel)
	{
                case RSCONFIG_USER_LEVEL_OVERRIDE:
			break;

#define MIN_BASIC_FRIENDS 2
			
		// FALL THROUGH EVERYTHING.
		default:
		case RSCONFIG_USER_LEVEL_NEW:
		{

			if (mPeerMgr->getFriendCount(true, false) > MIN_BASIC_FRIENDS)
			{
				userLevel = RSCONFIG_USER_LEVEL_BASIC;
			}
		}
		/* fallthrough */
		case RSCONFIG_USER_LEVEL_BASIC:
		{
			/* check that we have some lastConnect > 0 */
			if (mPeerMgr->haveOnceConnected())
			{
				userLevel = RSCONFIG_USER_LEVEL_CASUAL;
			}
		}
		/* fallthrough */
		case RSCONFIG_USER_LEVEL_CASUAL:
		case RSCONFIG_USER_LEVEL_POWER:

		{
			/* check that the firewall is open */

			uint32_t netMode = mNetMgr->getNetworkMode();
			uint32_t firewallMode = mNetMgr->getNatHoleMode();

			if ((RSNET_NETWORK_EXTERNALIP == netMode) ||
			   ((RSNET_NETWORK_BEHINDNAT == netMode) &&
			  	 	((RSNET_NATHOLE_UPNP == firewallMode) ||
			   		(RSNET_NATHOLE_NATPMP == firewallMode) ||
			   		(RSNET_NATHOLE_FORWARDED == firewallMode))))
			{
				userLevel = RSCONFIG_USER_LEVEL_POWER;
			}
		}
			break; /* for all */
	}

	{
		RsStackMutex stack(configMtx); /******* LOCKED MUTEX *****/
		mUserLevel = userLevel;
	}

	return userLevel;
}


uint32_t p3ServerConfig::getNetState()
{
	return mNetMgr->getNetStateMode();
}

uint32_t p3ServerConfig::getNetworkMode()
{
	return mNetMgr->getNetworkMode();
}

uint32_t p3ServerConfig::getNatTypeMode()
{
	return mNetMgr->getNatTypeMode();
}

uint32_t p3ServerConfig::getNatHoleMode()
{
	return mNetMgr->getNatHoleMode();
}

uint32_t p3ServerConfig::getConnectModes()
{
	return mNetMgr->getConnectModes();
}

        /* Operating Mode */
#define RS_CONFIG_OPERATING_STRING 	"OperatingMode"

uint32_t p3ServerConfig::getOperatingMode()
{
#ifdef SAVE_OPERATING_MODE
	std::string modestr = mGeneralConfig->getSetting(RS_CONFIG_OPERATING_STRING);
	uint32_t mode = RS_OPMODE_FULL;

	if (modestr == "FULL")
	{
		mode = RS_OPMODE_FULL;
	}
	else if (modestr == "NOTURTLE")
	{
		mode = RS_OPMODE_NOTURTLE;
	}
	else if (modestr == "GAMING")
	{
		mode = RS_OPMODE_GAMING;
	}
	else if (modestr == "MINIMAL")
	{
		mode = RS_OPMODE_MINIMAL;
	}
	return mode;
#else
	RsStackMutex stack(configMtx); /******* LOCKED MUTEX *****/
	return mOpMode;
#endif
}


bool p3ServerConfig::setOperatingMode(uint32_t opMode)
{
#ifdef SAVE_OPERATING_MODE
	std::string modestr = "FULL";
	switch(opMode)
	{
		case RS_OPMODE_FULL:
			modestr = "FULL";
		break;
		case RS_OPMODE_NOTURTLE:
			modestr = "NOTURTLE";

		break;
		case RS_OPMODE_GAMING:
			modestr = "GAMING";

		break;
		case RS_OPMODE_MINIMAL:
			modestr = "MINIMAL";
		break;
	}
	mGeneralConfig->setSetting(RS_CONFIG_OPERATING_STRING, modestr);
#else
	{
		RsStackMutex stack(configMtx); /******* LOCKED MUTEX *****/
		mOpMode = opMode;
	}
#endif
	return switchToOperatingMode(opMode);
}

bool p3ServerConfig::setOperatingMode(const std::string &opModeStr)
{
	uint32_t opMode = RS_OPMODE_FULL;
	std::string upper;
	stringToUpperCase(opModeStr, upper);

	if (upper == "NOTURTLE")
	{
		opMode = RS_OPMODE_NOTURTLE;
	}
	else if (upper == "GAMING")
	{
		opMode = RS_OPMODE_GAMING;
	}
	else if (upper == "MINIMAL")
	{
		opMode = RS_OPMODE_MINIMAL;
	}
	else	// "FULL" by default
	{
		opMode = RS_OPMODE_FULL;
	}

	return setOperatingMode(opMode);
}

bool p3ServerConfig::switchToOperatingMode(uint32_t opMode)
{
	float dl_rate = 0;
	float ul_rate = 0;
	bool turtle_enabled = true;

	{
		RsStackMutex stack(configMtx); /******* LOCKED MUTEX *****/
		dl_rate = mRateDownload;
		ul_rate = mRateUpload;
	}

	std::cerr << "p3ServerConfig::switchToOperatingMode(" << opMode << ")";
	std::cerr << std::endl;

	switch (opMode)
	{
		default:
		case RS_OPMODE_FULL:
			/* switch on all transfers */
			/* 100% bandwidth */
			/* switch on popups, enable hashing */
                	//setMaxRate(true, mri); // In / Download
                	//setMaxRate(false, mro); // Out / Upload.
			turtle_enabled = true;
		break;
		case RS_OPMODE_NOTURTLE:
			/* switch on all transfers - except turtle, enable hashing */
			/* 100% bandwidth */
			/* switch on popups, enable hashing */
			turtle_enabled = false;

		break;
		case RS_OPMODE_GAMING:
			/* switch on all transfers */
			/* reduce bandwidth to 25% */
			/* switch off popups, enable hashing */
			turtle_enabled = true;

			dl_rate *= 0.25;
			ul_rate *= 0.25;
		break;
		case RS_OPMODE_MINIMAL:
			/* switch off all transfers */
			/* reduce bandwidth to 10%, but make sure there is enough for VoIP */
			/* switch on popups, enable hashing */

			turtle_enabled = false;

			dl_rate *= 0.10;
			ul_rate *= 0.10;
			if (dl_rate < MIN_MINIMAL_RATE)
			{
				dl_rate = MIN_MINIMAL_RATE;
			}
			if (ul_rate < MIN_MINIMAL_RATE)
			{
				ul_rate = MIN_MINIMAL_RATE;
			}

		break;
	}

	if (mPqiHandler)
	{
        	mPqiHandler -> setMaxRate(true, dl_rate);
        	mPqiHandler -> setMaxRate(false, ul_rate);

		std::cerr << "p3ServerConfig::switchToOperatingMode() D/L: " << dl_rate << " U/L: " << ul_rate;
		std::cerr << std::endl;

	}

	std::cerr << "p3ServerConfig::switchToOperatingMode() Turtle Mode: " << turtle_enabled;
	std::cerr << std::endl;

	rsTurtle->setSessionEnabled(turtle_enabled);
	return true;
}

/* handle data rates.
 * Mutex must be handled at the lower levels: TODO */

int p3ServerConfig::SetMaxDataRates( int downKb, int upKb ) /* in kbrates */
{
        char line[512];

	{
		RsStackMutex stack(configMtx); /******* LOCKED MUTEX *****/
		mRateDownload = downKb;
		mRateUpload = upKb;
        	sprintf(line, "%f %f", mRateDownload, mRateUpload);
	}
	mGeneralConfig->setSetting(pqih_ftr, std::string(line));

	load_config(); // load and setup everything.
        return 1;
}


int p3ServerConfig::GetMaxDataRates( int &inKb, int &outKb ) /* in kbrates */
{
	RsStackMutex stack(configMtx); /******* LOCKED MUTEX *****/

	inKb = mRateDownload;
	outKb = mRateUpload;
        return 1;
}

int p3ServerConfig::GetCurrentDataRates( float &inKb, float &outKb )
{
	mPqiHandler->getCurrentRates(inKb, outKb);
	return 1;
}

int p3ServerConfig::GetTrafficSum(uint64_t &inb, uint64_t &outb )
{
	mPqiHandler->GetTraffic(inb, outb);
	return 1;
}
