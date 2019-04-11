/*******************************************************************************
 * bitdht/bdquerymgr.h                                                         *
 *                                                                             *
 * BitDHT: An Flexible DHT library.                                            *
 *                                                                             *
 * Copyright 2011 by Robert Fernie <bitdht@lunamutt.com>                       *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#ifndef BITDHT_QUERY_MANAGER_H
#define BITDHT_QUERY_MANAGER_H

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
