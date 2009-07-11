/*
 * "$Id: p3face-msgs.cc,v 1.7 2007-05-05 16:10:06 rmf24 Exp $"
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

#include "_rsserver/rsserver.h"
#include "_commonfuncs/getcurrentts.h"
#include "util/rsdir.h"

#include <iostream>
#include <sstream>

#include "util/rsdebug.h"


#include <sys/time.h>
#include <time.h>

// CHANGED: DELETED : reason-> never used in any project;
//const int p3facemsgzone = 11453;

void RsServer::lockRsCore()
{
    //	std::cerr << "RsServer::lockRsCore()" << std::endl;
    coreMutex.lock();
}

void RsServer::unlockRsCore()
{
    //	std::cerr << "RsServer::unlockRsCore()" << std::endl;
    coreMutex.unlock();
}

/* Flagging Persons / Channels / Files in or out of a set (CheckLists) */
int RsServer::ClearInChat()
{
    lockRsCore(); /* LOCK */

    mInChatList.clear();

    unlockRsCore();   /* UNLOCK */

    return 1;
}


/* Flagging Persons / Channels / Files in or out of a set (CheckLists) */
int RsServer::SetInChat(std::string id, bool in)             /* friend : chat msgs */
{
    /* so we send this.... */
    lockRsCore();     /* LOCK */

    //std::cerr << "Set InChat(" << id << ") to " << (in ? "True" : "False") << std::endl;
    std::list<std::string>::iterator it;
    it = std::find(mInChatList.begin(), mInChatList.end(), id);
    if (it == mInChatList.end())
    {
        if (in)
        {
            mInChatList.push_back(id);
        }
    }
    else
    {
        if (!in)
        {
            mInChatList.erase(it);
        }
    }

    unlockRsCore();   /* UNLOCK */

    return 1;
}


int RsServer::ClearInMsg()
{
    lockRsCore(); /* LOCK */

    mInMsgList.clear();

    unlockRsCore();   /* UNLOCK */

    return 1;
}


int RsServer::SetInMsg(std::string id, bool in)             /* friend : msgs */
{
    /* so we send this.... */
    lockRsCore();     /* LOCK */

    //std::cerr << "Set InMsg(" << id << ") to " << (in ? "True" : "False") << std::endl;
    std::list<std::string>::iterator it;
    it = std::find(mInMsgList.begin(), mInMsgList.end(), id);
    if (it == mInMsgList.end())
    {
        if (in)
        {
            mInMsgList.push_back(id);
        }
    }
    else
    {
        if (!in)
        {
            mInMsgList.erase(it);
        }
    }

    unlockRsCore();   /* UNLOCK */
    return 1;
}

bool RsServer::IsInChat(std::string id)  /* friend : chat msgs */
{
    /* so we send this.... */
    lockRsCore();     /* LOCK */

    std::list<std::string>::iterator it;
    it = std::find(mInChatList.begin(), mInChatList.end(), id);
    bool inChat = (it != mInChatList.end());

    unlockRsCore();   /* UNLOCK */

    return inChat;
}


bool RsServer::IsInMsg(std::string id)          /* friend : msg recpts*/
{
    /* so we send this.... */
    lockRsCore();     /* LOCK */

    std::list<std::string>::iterator it;
    it = std::find(mInMsgList.begin(), mInMsgList.end(), id);
    bool inMsg = (it != mInMsgList.end());

    unlockRsCore();   /* UNLOCK */

    return inMsg;
}




int RsServer::ClearInBroadcast()
{
    return 1;
}

int RsServer::ClearInSubscribe()
{
    return 1;
}

int RsServer::SetInBroadcast(std::string id, bool in)        /* channel : channel broadcast */
{
    return 1;
}

int RsServer::SetInSubscribe(std::string id, bool in)        /* channel : subscribed channels */
{
    return 1;
}

int     RsServer::ClearInRecommend()
{
    /* find in people ... set chat flag */
    RsIface &iface = getIface();
    iface.lockData(); /* LOCK IFACE */

    std::list<FileInfo> &recs = iface.mRecommendList;
    std::list<FileInfo>::iterator it;

    for (it = recs.begin(); it != recs.end(); it++)
    {
        it -> inRecommend = false;
    }

    iface.unlockData(); /* UNLOCK IFACE */

    return 1;
}


int RsServer::SetInRecommend(std::string id, bool in)        /* file : recommended file */
{
    /* find in people ... set chat flag */
    RsIface &iface = getIface();
    iface.lockData(); /* LOCK IFACE */

    std::list<FileInfo> &recs = iface.mRecommendList;
    std::list<FileInfo>::iterator it;

    for (it = recs.begin(); it != recs.end(); it++)
    {
        if (it -> fname == id)
        {
            /* set flag */
            it -> inRecommend = in;
            //std::cerr << "Set InRecommend (" << id << ") to " << (in ? "True" : "False") << std::endl;
        }
    }

    iface.unlockData(); /* UNLOCK IFACE */

    return 1;
}

std::string make_path_unix(std::string path)
{
    for (unsigned int i = 0; i < path.length(); i++)
    {
        if (path[i] == '\\')
            path[i] = '/';
    }
    return path;
}

/* Thread Fn: Run the Core */
void 	RsServer::run()
{

    double timeDelta = 0.25;
    double minTimeDelta = 0.1; // 25;
    double maxTimeDelta = 0.5;
    double kickLimit = 0.15;

    double avgTickRate = timeDelta;

    double lastts, ts;
    lastts = ts = getCurrentTS();

    long   lastSec = 0; /* for the slower ticked stuff */

    int min = 0;
    int loop = 0;

    while (1)
    {
#ifndef WINDOWS_SYS
        usleep((int) (timeDelta * 1000000));
#else
        Sleep((int) (timeDelta * 1000));
#endif

        ts = getCurrentTS();
        double delta = ts - lastts;

        /* for the fast ticked stuff */
        if (delta > timeDelta)
        {
#ifdef	DEBUG_TICK
            std::cerr << "Delta: " << delta << std::endl;
            std::cerr << "Time Delta: " << timeDelta << std::endl;
            std::cerr << "Avg Tick Rate: " << avgTickRate << std::endl;
#endif

            lastts = ts;

            /******************************** RUN SERVER *****************/
            lockRsCore();

            int moreToTick = ftserver -> tick();

#ifdef	DEBUG_TICK
            std::cerr << "RsServer::run() ftserver->tick(): moreToTick: " << moreToTick << std::endl;
#endif

            unlockRsCore();

            /* tick the connection Manager */
            mConnMgr->tick();
            /******************************** RUN SERVER *****************/

            /* adjust tick rate depending on whether there is more.
             */

            avgTickRate = 0.2 * timeDelta + 0.8 * avgTickRate;

            if (1 == moreToTick)
            {
                timeDelta = 0.9 * avgTickRate;
                if (timeDelta > kickLimit)
                {
                    /* force next tick in one sec
                     * if we are reading data.
                     */
                    timeDelta = kickLimit;
                    avgTickRate = kickLimit;
                }
            }
            else
            {
                timeDelta = 1.1 * avgTickRate;
            }

            /* limiter */
            if (timeDelta < minTimeDelta)
            {
                timeDelta = minTimeDelta;
            }
            else if (timeDelta > maxTimeDelta)
            {
                timeDelta = maxTimeDelta;
            }

            /* Fast Updates */


            /* now we have the slow ticking stuff */
            /* stuff ticked once a second (but can be slowed down) */
            if ((int) ts > lastSec)
            {
                lastSec = (int) ts;

                // Every second! (UDP keepalive).
                tou_tick_stunkeepalive();

                // every five loops (> 5 secs)
                if (loop % 5 == 0)
                {
                    //	update_quick_stats();

                    // Update All Every 5 Seconds.
                    // These Update Functions do the locking themselves.
#ifdef	DEBUG_TICK
                    std::cerr << "RsServer::run() Updates()" << std::endl;
#endif

                    // These two have been completed!
                    //std::cerr << "RsServer::run() UpdateAllCerts()" << std::endl;
                    //UpdateAllCerts();
                    //std::cerr << "RsServer::run() UpdateAllNetwork()" << std::endl;
                    //UpdateAllNetwork();

                    // currently Dummy Functions.
                    //std::cerr << "RsServer::run() UpdateAllTransfers()" << std::endl;

                    //std::cerr << "RsServer::run() ";
                    //std::cerr << "UpdateRemotePeople()"<<std::endl;
                    //UpdateRemotePeople();

                    //std::cerr << "RsServer::run() UpdateAllFiles()" << std::endl;
                    //UpdateAllFiles();

                    //std::cerr << "RsServer::run() UpdateAllConfig()" << std::endl;
                    UpdateAllConfig();



                    //std::cerr << "RsServer::run() CheckDHT()" << std::endl;
                    //CheckNetworking();


                    /* Tick slow services */
                    if (mRanking)
                        mRanking->tick();


                    if (mQblog)
                        mQblog->tick();



#if 0
                    std::string opt;
                    std::string val = "VALUE";
                    {
                        std::ostringstream out;
                        out << "SEC:" << lastSec;
                        opt = out.str();
                    }

                    mGeneralConfig->setSetting(opt, val);
#endif

                    mConfigMgr->tick(); /* saves stuff */

                }

                // every 60 loops (> 1 min)
                if (++loop >= 60)
                {
                    loop = 0;

                    /* force saving FileTransferStatus TODO */
                    //ftserver->saveFileTransferStatus();

                    /* see if we need to resave certs */
                    mAuthMgr->CheckSaveCertificates();

                    /* hour loop */
                    if (++min >= 60)
                    {
                        min = 0;
                    }
                }

                // slow update tick as well.
                // update();
            } // end of slow tick.

        } // end of only once a second.
    }
    return;
}
