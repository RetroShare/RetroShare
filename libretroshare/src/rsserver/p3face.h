#ifndef MRK_P3RS_INTERFACE_H
#define MRK_P3RS_INTERFACE_H

/*
 * "$Id: p3face.h,v 1.9 2007-05-05 16:10:06 rmf24 Exp $"
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2004-2006 by Robert Fernie.
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

#include "services/p3msgservice.h"
#include "services/p3statusservice.h"

class p3heartbeat;
class p3discovery2;

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
		/****************************************/
		/* p3face-startup.cc: init... */
		virtual int StartupRetroShare();

		/****************************************/
		/* p3face.cc: main loop / util fns / locking. */

		RsServer() ;
		virtual ~RsServer();

		/* Thread Fn: Run the Core */
        virtual void data_tick();

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
        virtual void	startServiceThread(RsTickingThread *t) ;

		/************* Rs shut down function: in upnp 'port lease time' bug *****************/

		/**
		 * This function is responsible for ensuring Retroshare exits in a legal state:
		 * i.e. releases all held resources and saves current configuration
		 */
		virtual void 	rsGlobalShutDown( ); 

		/****************************************/

	public:
		virtual bool getPeerCryptoDetails(const RsPeerId& ssl_id,RsPeerCryptoParams& params) { return pqih->getCryptoParams(ssl_id,params); }
		virtual void getLibraries(std::list<RsLibraryInfo> &libraries);

	private: 

		// The real Server Parts.

		//filedexserver *server;
		//ftServer *ftserver;

		p3PeerMgrIMPL *mPeerMgr;
		p3LinkMgrIMPL *mLinkMgr;
		p3NetMgrIMPL *mNetMgr;
		p3HistoryMgr *mHistoryMgr;

		pqipersongrp *pqih;

		RsPluginManager *mPluginsManager;

		//sslroot *sslr;

		/* services */
		p3heartbeat *mHeart;
		p3discovery2 *mDisc;
		p3MsgService  *msgSrv;
		p3ChatService *chatSrv;
		p3StatusService *mStatusSrv;

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

    static const double minTimeDelta = 0.1; // 25;
    static const double maxTimeDelta = 0.5;
    static const double kickLimit = 0.15;
};

/* Helper function to convert windows paths
 * into unix (ie switch \ to /) for FLTK's file chooser
 */

std::string make_path_unix(std::string winpath);

#endif
