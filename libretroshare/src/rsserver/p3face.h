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
#include "pqi/p3connmgr.h"
#include "pqi/pqipersongrp.h"

#include "rsiface/rsiface.h"
#include "rsiface/rstypes.h"
#include "util/rsthreads.h"

#include "services/p3disc.h"
#include "services/p3msgservice.h"
#include "services/p3chatservice.h"
#include "services/p3ranking.h"
#include "services/p3Qblog.h"

/* The Main Interface Class - for controlling the server */

/* The init functions are actually Defined in p3face-startup.cc
 */
//RsInit *InitRsConfig();
//void    CleanupRsConfig(RsInit *);
//int InitRetroShare(int argc, char **argv, RsInit *config);
//int LoadCertificates(RsInit *config);

RsControl *createRsControl(RsIface &iface, NotifyBase &notify);


class RsServer: public RsControl, public RsThread
{
	public:
		/****************************************/
		/* p3face-startup.cc: init... */
		virtual	int StartupRetroShare();

	public:
		/****************************************/
		/* p3face.cc: main loop / util fns / locking. */

		RsServer(RsIface &i, NotifyBase &callback);
		virtual ~RsServer();

		/* Thread Fn: Run the Core */
		virtual void run();

	public: // no longer private:!!!
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

	private:

		/* mutex */
		RsMutex coreMutex;

		/* General Internal Helper Functions
			(Must be Locked)
		 */
#if 0
		cert   *intFindCert(RsCertId id);
		RsCertId intGetCertId(cert *c);
#endif

		/****************************************/
		/****************************************/
		/* p3face-msg Operations */

	public:
		virtual const std::string& certificateFileName() ;

		/* Flagging Persons / Channels / Files in or out of a set (CheckLists) */
		virtual int     SetInChat(std::string id, bool in);         /* friend : chat msgs */
		virtual int     SetInMsg(std::string id, bool in);          /* friend : msg receipients */ 
		virtual int     SetInBroadcast(std::string id, bool in);    /* channel : channel broadcast */
		virtual int     SetInSubscribe(std::string id, bool in);    /* channel : subscribed channels */
		virtual int     SetInRecommend(std::string id, bool in);    /* file : recommended file */
		virtual int     ClearInChat();
		virtual int     ClearInMsg();
		virtual int     ClearInBroadcast();
		virtual int     ClearInSubscribe();
		virtual int     ClearInRecommend();

		virtual bool    IsInChat(std::string id);           /* friend : chat msgs */
		virtual bool    IsInMsg(std::string id);            /* friend : msg recpts*/


	private:

		std::list<std::string> mInChatList, mInMsgList;

		void initRsMI(RsMsgItem *msg, MessageInfo &mi);

		/****************************************/
		/****************************************/
		/****************************************/
		/****************************************/
	public:
		/* Config */

		virtual int 	ConfigGetDataRates(float &inKb, float &outKb);
		virtual int     ConfigSetDataRates( int totalDownload, int totalUpload );
		virtual int     ConfigSetBootPrompt( bool on );

		virtual void    ConfigFinalSave( );

		/************* Rs shut down function: in upnp 'port lease time' bug *****************/

		/**
		 * This function is responsible for ensuring Retroshare exits in a legal state:
		 * i.e. releases all held resources and saves current configuration
		 */
		virtual void 	rsGlobalShutDown( ); 
	private:
		int UpdateAllConfig();

		/****************************************/


	private: 

		// The real Server Parts.

		//filedexserver *server;
		ftServer *ftserver;

		p3ConnectMgr *mConnMgr;
		p3AuthMgr    *mAuthMgr;

		pqipersongrp *pqih;

		//sslroot *sslr;

		/* services */
		p3disc *ad;
		p3MsgService  *msgSrv;
		p3ChatService *chatSrv;

		/* caches (that need ticking) */
		p3Ranking *mRanking;
		p3Qblog *mQblog;

		/* Config */
		p3ConfigMgr     *mConfigMgr;
		p3GeneralConfig *mGeneralConfig;

		// Worker Data.....

};

/* Helper function to convert windows paths
 * into unix (ie switch \ to /) for FLTK's file chooser
 */

std::string make_path_unix(std::string winpath);

#endif
