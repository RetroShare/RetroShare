#ifndef BITDHT_ACCOUNT_H
#define BITDHT_ACCOUNT_H

/*******************************************************************************
 * bitdht/bdaccount.h                                                          *
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

#include <vector>
#include <string>
#include <inttypes.h>

#define 	BDACCOUNT_MSG_OUTOFDATEPING		0
#define 	BDACCOUNT_MSG_PING			1
#define 	BDACCOUNT_MSG_PONG			2
#define 	BDACCOUNT_MSG_QUERYNODE			3
#define 	BDACCOUNT_MSG_QUERYHASH			4
#define 	BDACCOUNT_MSG_REPLYFINDNODE		5
#define 	BDACCOUNT_MSG_REPLYQUERYHASH		6

#define 	BDACCOUNT_MSG_POSTHASH			7
#define 	BDACCOUNT_MSG_REPLYPOSTHASH		8

#define 	BDACCOUNT_MSG_CONNECTREQUEST		9
#define 	BDACCOUNT_MSG_CONNECTREPLY		10
#define 	BDACCOUNT_MSG_CONNECTSTART		11
#define 	BDACCOUNT_MSG_CONNECTACK		12

#define 	BDACCOUNT_NUM_ENTRIES 			13

class bdAccount
{
	public:

	bdAccount();

	void incCounter(uint32_t idx, bool out);
	void doStats();
	void printStats(std::ostream &out);	
	void resetCounters();
	void resetStats();

	private:

	int mNoStats;

	std::vector<double> mCountersOut;
	std::vector<double> mCountersRecv;

	std::vector<double> mLpfOut;
	std::vector<double> mLpfRecv;

	std::vector<std::string> mLabel;
	// Statistics.
};



#endif // BITDHT_ACCOUNT_H
