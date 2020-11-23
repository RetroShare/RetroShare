/*******************************************************************************
 * libretroshare/src/unused: p3dsdv.h                                          *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2011 by Robert Fernie <retroshare@lunamutt.com>                   *
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
#ifndef SERVICE_RSDSDV_HEADER
#define SERVICE_RSDSDV_HEADER

#include <list>
#include <string>

#include "serialiser/rsdsdvitems.h"
#include "services/p3service.h"
#include "pqi/p3cfgmgr.h"
#include "pqi/pqimonitor.h"

#include "retroshare/rsdsdv.h"

class p3ServiceControl;


#define RSDSDV_MAX_DISTANCE	3
#define RSDSDV_MAX_SEND_TABLE 100

//!The RS DSDV service.
 /**
  *
  * Finds RS wide paths to Services and Peers.
  */

class p3Dsdv: public RsDsdv, public p3Service /* , public p3Config */, public pqiServiceMonitor
{
	public:
	p3Dsdv(p3ServiceControl *cm);
virtual RsServiceInfo getServiceInfo();

		/*** internal librs interface ****/

int 	addDsdvId(RsDsdvId *id, std::string realHash);
int 	dropDsdvId(RsDsdvId *id);
int 	printDsdvTable(std::ostream &out);

int	addTestService();

	private:

int     sendTables();
void    advanceLocalSequenceNumbers();
void    clearSignificantChangesFlags();


int 	generateRoutingTables(bool incremental);
int 	generateRoutingTable(const RsPeerId &peerId, bool incremental);

int     processIncoming();

int 	handleDSDV(RsDsdvRouteItem *dsdv);

int 	selectStableRoutes();
int 	clearOldRoutes();

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
		virtual void statusChange(const std::list<pqiServicePeer> &plist);

		/************* from p3Config *******************/
		//virtual RsSerialiser *setupSerialiser() ;
		//virtual bool saveList(bool& cleanup, std::list<RsItem*>&) ;
		//virtual void saveDone();
		//virtual bool loadList(std::list<RsItem*>& load) ;

	private:
		RsMutex mDsdvMtx;

		std::map<std::string, RsDsdvTableEntry> mTable;

		rstime_t mSentTablesTime;
		rstime_t mSentIncrementTime;
	
		bool mSignificantChanges;

		p3ServiceControl *mServiceCtrl;

};

#endif // SERVICE_RSDSDV_HEADER

