/*******************************************************************************
 * libretroshare/src/rsserver: p3face.h                                        *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2006 by Robert Fernie <retroshare@lunamutt.com>              *
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
#pragma once

#include <functional>

//#include "server/filedexserver.h"
#include "ft/ftserver.h"
//#include "pqi/pqissl.h"

#include "pqi/p3cfgmgr.h"
#include "pqi/p3notify.h"
#include "pqi/pqipersongrp.h"

#include "retroshare/rsiface.h"
#include "retroshare/rstypes.h"
#include "util/rsthreads.h"

#include "chat/p3chatservice.h"
#include "gxstunnel/p3gxstunnel.h"

#include "services/p3msgservice.h"
#include "services/p3statusservice.h"

class p3heartbeat;
class p3discovery2;
class p3I2pBob;

/* GXS Classes - just declare the classes.
   so we don't have to totally recompile to switch */

class p3IdService;
class p3GxsCircles;
class p3GxsForums;
class p3GxsChannels;
class p3Wiki;
class p3Posted;
class p3PhotoService;
class p3Wire;


class p3PeerMgrIMPL;
class p3LinkMgrIMPL;
class p3NetMgrIMPL;
class p3HistoryMgr;
class RsPluginManager;

/* The Main Interface Class - for controlling the server */

/* The init functions are actually Defined in p3face-startup.cc
 */
//RsInit *InitRsConfig();
//void    CleanupRsConfig(RsInit *);
//int InitRetroShare(int argc, char **argv, RsInit *config);
//int LoadCertificates(RsInit *config);

class RsServer: public RsControl, public RsTickingThread
{
public:
	RsServer();
	virtual ~RsServer();

	virtual int StartupRetroShare();

	/// @see RsControl::isReady()
	virtual bool isReady() { return coreReady; }

	/// @see RsControl::setShutdownCallback
	void setShutdownCallback(const std::function<void(int)>& callback)
	{ mShutdownCallback = callback; }

	void threadTick() override; /// @see RsTickingThread

		/* locking stuff */
		void    lockRsCore() 
		{ 
			//	std::cerr << "RsServer::lockRsCore()" << std::endl;
			coreMutex.lock(); 
		}

		void    unlockRsCore() 
		{ 
			//	std::cerr << "RsServer::unlockRsCore()" << std::endl;
			coreMutex.unlock(); 
		}

		static p3Notify *notify() { return dynamic_cast<RsServer*>(instance())->mNotify ; }

	private:

		/* mutex */
		RsMutex coreMutex;

	private:

		/****************************************/
		/****************************************/
		/****************************************/
		/****************************************/
	public:
		/* Config */

		virtual void    ConfigFinalSave( );
		virtual void	startServiceThread(RsTickingThread *t, const std::string &threadName) ;

		/************* Rs shut down function: in upnp 'port lease time' bug *****************/

	/**
	 * This function is responsible for ensuring Retroshare exits in a legal state:
	 * i.e. releases all held resources and saves current configuration
	 */
	virtual void rsGlobalShutDown();

		/****************************************/

	public:
		virtual bool getPeerCryptoDetails(const RsPeerId& ssl_id,RsPeerCryptoParams& params) { return pqih->getCryptoParams(ssl_id,params); }
		virtual void getLibraries(std::list<RsLibraryInfo> &libraries);

	private: 

		std::string getSQLCipherVersion(); // TODO: move to rsversion.h

		// The real Server Parts.

		//filedexserver *server;
		//ftServer *ftserver;

		p3PeerMgrIMPL *mPeerMgr;
		p3LinkMgrIMPL *mLinkMgr;
		p3NetMgrIMPL *mNetMgr;
		p3HistoryMgr *mHistoryMgr;

		pqipersongrp *pqih;

		RsPluginManager *mPluginsManager;

		/* services */
		p3heartbeat *mHeart;
		p3discovery2 *mDisc;
		p3MsgService  *msgSrv;
		p3ChatService *chatSrv;
		p3StatusService *mStatusSrv;
		p3GxsTunnelService *mGxsTunnels;
		p3I2pBob *mI2pBob;

        // This list contains all threaded services. It will be used to shut them down properly.

        std::list<RsTickingThread*> mRegisteredServiceThreads ;

        /* GXS */
//		p3Wiki *mWiki;
//		p3Posted *mPosted;
//		p3PhotoService *mPhoto;
//		p3GxsCircles *mGxsCircles;
//        p3GxsNetService *mGxsNetService;
//        p3IdService *mGxsIdService;
//		p3GxsForums *mGxsForums;
//		p3GxsChannels *mGxsChannels;
//		p3Wire *mWire;
		p3GxsTrans* mGxsTrans;

		/* Config */
		p3ConfigMgr     *mConfigMgr;
		p3GeneralConfig *mGeneralConfig;

		// notify
		p3Notify *mNotify ;

		// Worker Data.....

    int mMin ;
    int mLoop ;
    int mLastts ;
    long mLastSec ;
    double mAvgTickRate ;
    double mTimeDelta ;

    static const double minTimeDelta; // 25;
    static const double maxTimeDelta;
    static const double kickLimit;

	/// @see RsControl::setShutdownCallback
	std::function<void(int)> mShutdownCallback;

	/** Keep track of the core being fully ready, true only after
	 *  StartupRetroShare() finish and before rsGlobalShutDown() begin
	 */
	bool coreReady;
};

/* Helper function to convert windows paths
 * into unix (ie switch \ to /) for FLTK's file chooser
 */

std::string make_path_unix(std::string winpath);
