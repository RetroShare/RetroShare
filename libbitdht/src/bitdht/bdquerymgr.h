#ifndef BITDHT_QUERY_MANAGER_H
#define BITDHT_QUERY_MANAGER_H

/*
 * bitdht/bdquerymgr.h
 *
 * BitDHT: An Flexible DHT library.
 *
 * Copyright 2011 by Robert Fernie
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 3 as published by the Free Software Foundation.
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
 * Please report all bugs and problems to "bitdht@lunamutt.com".
 *
 */


#include "bitdht/bdquery.h"
class bdNodePublisher;


class bdQueryManager
{
	public:

	bdQueryManager(bdSpace *space, bdDhtFunctions *fns, bdNodePublisher *pub);	

	void shutdownQueries();
	void printQueries();

	int iterateQueries(int maxqueries);

	bool checkPotentialPeer(bdId *id, bdId *src);
	void addPeer(const bdId *id, uint32_t peerflags);

	void addQuery(const bdNodeId *id, uint32_t qflags);
	void clearQuery(const bdNodeId *id);
	void QueryStatus(std::map<bdNodeId, bdQueryStatus> &statusMap);
	int  QuerySummary(const bdNodeId *id, bdQuerySummary &query);

	int  result(bdNodeId *target, std::list<bdId> &answer);
	int  proxies(bdNodeId *target, std::list<bdId> &answer);
	int  potentialProxies(bdNodeId *target, std::list<bdId> &answer);

	// extra "Worthy Peers" we will want to ping.
	void addWorthyPeerSource(bdId *src);
	bool checkWorthyPeerSources(bdId *src);

	private:

	int getResults(bdNodeId *target, std::list<bdId> &answer, int querytype);

	/* NB: No Mutex Protection... Single threaded, Mutex at higher level!
	 */

	bdSpace *mNodeSpace;
	bdDhtFunctions *mFns;
	bdNodePublisher *mPub;

	std::list<bdQuery *> mLocalQueries;
	std::list<bdPeer> mWorthyPeerSources;
};



#endif // BITDHT_QUERY_MANAGER_H
