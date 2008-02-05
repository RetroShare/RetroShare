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

#include "server/filedexserver.h"
//#include "pqi/pqissl.h"

#include "pqi/p3connmgr.h"
#include "pqi/pqipersongrp.h"

#include "rsiface/rsiface.h"
#include "rsiface/rstypes.h"
#include "util/rsthreads.h"

#include "services/p3disc.h"
#include "services/p3msgservice.h"
#include "services/p3chatservice.h"
#include "services/p3ranking.h"

/* The Main Interface Class - for controlling the server */

/* The init functions are actually Defined in p3face-startup.cc
 */
RsInit *InitRsConfig();
void    CleanupRsConfig(RsInit *);
int InitRetroShare(int argc, char **argv, RsInit *config);
int LoadCertificates(RsInit *config);

RsControl *createRsControl(RsIface &iface, NotifyBase &notify);


#if 0

class PendingDirectory
{
        public:
        PendingDirectory(RsCertId in_id, const DirInfo *req_dir, int depth);
void	addEntry(PQFileItem *item);	

        RsCertId id;
        int reqDepth;
        int reqTime;
        DirInfo data;
};

#endif



class RsServer: public RsControl, public RsThread
{
	public:
/****************************************/
	/* p3face-startup.cc: init... */
virtual	int StartupRetroShare(RsInit *config);

	public:
/****************************************/
	/* p3face.cc: main loop / util fns / locking. */

	RsServer(RsIface &i, NotifyBase &callback);
	virtual ~RsServer();

        /* Thread Fn: Run the Core */
virtual void run();

	private:
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
	/* p3face-people Operations */

/****************************************/
/****************************************/
	/* p3face-file Operations */

	public:

	/* Directory Actions */
virtual int RequestDirDetails(std::string uid, std::string path, 
					DirDetails &details);
virtual int RequestDirDetails(void *ref, DirDetails &details, uint32_t flags);
virtual int SearchKeywords(std::list<std::string> keywords, std::list<FileDetail> &results);
virtual int SearchBoolExp(Expression *exp, std::list<FileDetail> &results);


	/* Actions For Upload/Download */

// REDO these three TODO XXX .
//virtual int FileBroadcast(std::string uId, std::string src, int size);
//virtual int FileDelete(std::string, std::string);

virtual int FileRecommend(std::string fname, std::string hash, int size);
virtual int FileRequest(std::string fname, std::string hash, uint32_t size, std::string dest);

virtual int FileCancel(std::string fname, std::string hash, uint32_t size);
virtual int FileClearCompleted();


virtual int FileSetBandwidthTotals(float outkB, float inkB);

	private:

int 	UpdateAllTransfers();

	/* send requests to people */
int     UpdateRemotePeople();

/****************************************/
/****************************************/
	/* p3face-msg Operations */

	public:
	/* Channel Items */
virtual int ChannelCreateNew(ChannelInfo &info);
virtual int ChannelSendMsg(ChannelInfo &info);

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
	         
	/* Internal Update Iface Fns */
int 	UpdateAllChannels();


#ifdef PQI_USE_CHANNELS
	/* Internal Helper Fns */
RsChanId signToChanId(const channelSign &cs) const;


int intAddChannel(ChannelInfo &info);
int intAddChannelMsg(RsChanId id, MessageInfo &msg);


void initRsCI(pqichannel *in, ChannelInfo &out);
void initRsCMI(pqichannel *chan, channelMsg *cm, MessageInfo &msg);
void initRsCMFI(pqichannel *chan, chanMsgSummary *msg,
      const PQChanItem::FileItem *cfdi, FileInfo &file);

#endif

void intCheckFileStatus(FileInfo &file);

void initRsMI(RsMsgItem *msg, MessageInfo &mi);

/****************************************/
/****************************************/

	public:
/****************************************/
	/* RsIface Networking */
//virtual int	NetworkDHTActive(bool active);
//virtual int	NetworkUPnPActive(bool active);
//virtual int	NetworkDHTStatus();
//virtual int	NetworkUPnPStatus();

	private:
/* internal */
//int	InitNetworking(std::string);
//int	CheckNetworking();

//int	InitDHT(std::string);
//int	CheckDHT();

//int	InitUPnP();
//int	CheckUPnP();

//int     UpdateNetworkConfig(RsConfig &config);
//int     SetExternalPorts();


	public:
/****************************************/
	/* RsIface Config */
        /* Config */
virtual int     ConfigAddSharedDir( std::string dir );
virtual int     ConfigRemoveSharedDir( std::string dir );
virtual int     ConfigSetIncomingDir( std::string dir );

//virtual int     ConfigSetLocalAddr( std::string ipAddr, int port );
//virtual int     ConfigSetExtAddr( std::string ipAddr, int port );
//virtual int     ConfigSetExtName( std::string addr );
//virtual int     ConfigSetLanConfig( bool fire, bool forw );

virtual int     ConfigSetDataRates( int total, int indiv );
virtual int     ConfigSetBootPrompt( bool on );
//virtual int     ConfigSave( );

	private:
int UpdateAllConfig();

/****************************************/


	private: 

	// The real Server Parts.

	filedexserver *server;

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

	// Worker Data.....

};

/* Helper function to convert windows paths
 * into unix (ie switch \ to /) for FLTK's file chooser
 */

std::string make_path_unix(std::string winpath);



/* Initialisation Class (not publicly disclosed to RsIFace) */

class RsInit
{
        public:
        /* Commandline/Directory options */

        /* Key Parameters that must be set before
         * RetroShare will start up:
         */
        std::string load_cert;
        std::string load_key;
        std::string passwd;

        bool havePasswd; /* for Commandline password */
        bool autoLogin;  /* autoLogin allowed */

        /* Win/Unix Differences */
        char dirSeperator;

        /* Directories */
        std::string basedir;
        std::string homePath;

        /* Listening Port */
        bool forceLocalAddr;
        unsigned short port;
        char inet[256];

        /* Logging */
        bool haveLogFile;
        bool outStderr;
        bool haveDebugLevel;
        int  debugLevel;
        char logfname[1024];

        bool firsttime_run;
        bool load_trustedpeer;
        std::string load_trustedpeer_file;

        bool udpListenerOnly;
};



#endif
