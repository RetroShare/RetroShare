/*
 * libretroshare/src/services/p3dsdv.h
 *
 * Network-Wide Routing Service.
 *
 * Copyright 2011 by Robert Fernie.
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


#ifndef SERVICE_RSDSDV_HEADER
#define SERVICE_RSDSDV_HEADER

#include <list>
#include <string>

#include "serialiser/rsdsdvitems.h"
#include "services/p3service.h"
#include "pqi/p3cfgmgr.h"
#include "pqi/pqimonitor.h"

#include "retroshare/rsdsdv.h"

class p3LinkMgr;


#define RSDSDV_MAX_DISTANCE	3
#define RSDSDV_MAX_SEND_TABLE 100

//!The RS DSDV service.
 /**
  *
  * Finds RS wide paths to Services and Peers.
  */

class p3Dsdv: public RsDsdv, public p3Service /* , public p3Config */, public pqiMonitor
{
	public:
	p3Dsdv(p3LinkMgr *cm);

		/*** internal librs interface ****/

int 	addDsdvId(RsDsdvId *id, std::string realHash);
int 	dropDsdvId(RsDsdvId *id);
int 	printDsdvTable(std::ostream &out);

int	addTestService();

	private:

int     sendTables();
void    advanceLocalSequenceNumbers();
void    clearSignificantChangesFlags();

int 	selectStableRoutes();
int 	generateRoutingTables(bool incremental);
int 	generateRoutingTable(const std::string &peerId, bool incremental);

int     processIncoming();

int 	handleDSDV(RsDsdvRouteItem *dsdv);


	public:

		/***** overloaded from rsDsdv *****/

virtual uint32_t getLocalServices(std::list<std::string> &hashes);
virtual uint32_t getAllServices(std::list<std::string> &hashes);
virtual int getDsdvEntry(const std::string &hash, RsDsdvTableEntry &entry);

		/***** overloaded from p3Service *****/
		/*!
		 * Process stuff.
		 */

		virtual int   tick();
		virtual int   status();

		/*************** pqiMonitor callback ***********************/
		virtual void statusChange(const std::list<pqipeer> &plist);

		/************* from p3Config *******************/
		//virtual RsSerialiser *setupSerialiser() ;
		//virtual bool saveList(bool& cleanup, std::list<RsItem*>&) ;
		//virtual void saveDone();
		//virtual bool loadList(std::list<RsItem*>& load) ;

	private:
		RsMutex mDsdvMtx;

		std::map<std::string, RsDsdvTableEntry> mTable;

		time_t mSentTablesTime;
		time_t mSentIncrementTime;
	
		bool mSignificantChanges;

		p3LinkMgr *mLinkMgr;

};

#endif // SERVICE_RSDSDV_HEADER

