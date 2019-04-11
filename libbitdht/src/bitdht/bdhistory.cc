/*******************************************************************************
 * bitdht/bdhistory.cc                                                         *
 *                                                                             *
 * BitDHT: An Flexible DHT library.                                            *
 *                                                                             *
 * Copyright 2010 by Robert Fernie <bitdht@lunamutt.com>                       *
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


#include "bitdht/bdhistory.h"
#include "bitdht/bdstddht.h"
#include "bitdht/bdmsgs.h"
#include <set>

#define MIN_RESEND_PERIOD  60

bdMsgHistoryList::bdMsgHistoryList()
:mPeerVersion("Unknown")
{
	return;
}



void	bdMsgHistoryList::addMsg(time_t ts, uint32_t msgType, bool incoming, const bdNodeId *aboutId)
{
//	std::cerr << "bdMsgHistoryList::addMsg()";
//	std::cerr << std::endl;

	bdMsgHistoryItem msg(msgType, incoming, aboutId);
	msgHistory.insert(std::make_pair(ts, msg));
}

void 	bdMsgHistoryList::setPeerType(time_t /* ts */, std::string version)
{
	mPeerVersion = version;
}

int	bdMsgHistoryList::msgCount(time_t start_ts, time_t end_ts)
{
	std::multimap<time_t, bdMsgHistoryItem>::iterator sit, eit, it;
	sit = msgHistory.lower_bound(start_ts);
	eit = msgHistory.upper_bound(end_ts);
	int count = 0;
	for (it = sit; it != eit; it++, count++) ; // empty loop.

	return count;
}

bool    bdMsgHistoryList::msgClear(time_t before)
{
	if (before == 0)
	{
		msgHistory.clear();
		return true;
	}

	// Delete the old stuff in the list.
	while((msgHistory.begin() != msgHistory.end()) && (msgHistory.begin()->first < before))
	{
		msgHistory.erase(msgHistory.begin());
	}

	// return true if empty.
	if (msgHistory.begin() == msgHistory.end())
	{
		return true;
	}
	return false;
}

void	bdMsgHistoryList::msgClear()
{
	msgHistory.clear();
}


void	bdMsgHistoryList::clearHistory()
{
	msgClear();
}

void	bdMsgHistoryList::printHistory(std::ostream &out, int mode, time_t start_ts, time_t end_ts)
{
	//out << "AGE: MSGS  => incoming, <= outgoing"  << std::endl;
	std::multimap<time_t, bdMsgHistoryItem>::iterator sit, eit, it;
	sit = msgHistory.lower_bound(start_ts);
	eit = msgHistory.upper_bound(end_ts);
	time_t curr_ts = 0;
	bool time_changed = false;
	bool first_line = true;
	
	for(it = sit; it != eit; it++)
	{
		time_changed = false;
		if (curr_ts != it->first)
		{
			curr_ts = it->first;
			time_changed = true;
		}
		switch(mode)
		{
			default:
		{
			/* print one line per ts */
			if (time_changed)
			{
				if (!first_line)
				{
					/* finish existing line */
					out << " " << std::endl;
				}
				else
				{
					first_line = false;
				}
				out << "\tTS: " << time(NULL) - curr_ts << " ";
			}
	
			std::string name;
			bitdht_msgtype(it->second.msgType, name);

			if (it->second.incoming)
			{
				out << "( =I> ";
			}
			else
			{
				out << "( <O= ";
			}

			out << name << " ";
			if ((it->second.aboutId.data[0] == 0)
				&& (it->second.aboutId.data[3] == 0)
				&& (it->second.aboutId.data[3] == 0)
				&& (it->second.aboutId.data[3] == 0))
			{
				/* don't print anything */
			}
			else
			{
				bdStdPrintNodeId(out, &(it->second.aboutId));
			}
			out << " )";

		}
			break;
		} // end of switch.
	}

	/* finish up last line */
	if (!first_line)
	{
		out << " " << std::endl;
	}
}


bool	bdMsgHistoryList::canSend()
{
	std::cerr << "bdMsgHistoryList::canSend()";

	std::multimap<time_t, bdMsgHistoryItem>::reverse_iterator rit;

	rit = msgHistory.rbegin();
	if (rit != msgHistory.rend())
	{
		time_t now = time(NULL);
		if (now - rit->first > MIN_RESEND_PERIOD)
		{
			std::cerr << " OVER RESEND_PERIOD... true";
			std::cerr << std::endl;

			return true;
		}
	}

	if (msgHistory.size() % 2 == 0)
	{
		std::cerr << " SIZE: " << msgHistory.size() << " % 2 = 0 ... true";
		std::cerr << std::endl;

		return true;
	}

	std::cerr << " false";
	std::cerr << std::endl;

	return false;
}

bool	bdMsgHistoryList::validPeer()
{
	std::cerr << "bdMsgHistoryList::validPeer()";

	std::multimap<time_t, bdMsgHistoryItem>::iterator it;

	for(it = msgHistory.begin(); it != msgHistory.end(); it++)
	{
		if (it->second.incoming)
		{
			std::cerr << " Incoming Msg... so validPeer";
			std::cerr << std::endl;

			return true;
		}
	}

	std::cerr << " false";
	std::cerr << std::endl;

	return false;
}


#define MAX_PING_PER_MINUTE  	2
#define MAX_QUERY_PER_MINUTE 	2

bool bdMsgHistoryList::analysePeer()

{
	/* analyse and print out details of the peers messages */
	bool flagged = false;

	//out << "AGE: MSGS  => incoming, <= outgoing"  << std::endl;
	std::multimap<time_t, bdMsgHistoryItem>::iterator sit, eit, it;
	sit = msgHistory.begin();
	eit = msgHistory.end();
	if (sit == eit)
	{
		// nothing here.
		return false;
	}

	time_t start_ts = sit->first;
	time_t end_ts = msgHistory.rbegin()->first; // must exist.


	// don't divide by zero.
	if (end_ts - start_ts < 60)
	{
		end_ts = start_ts + 60;
	}


	/* what do we want to analyse? */

	/* if we have sent / recved too many queries or pings */

	int in_ping = 0;
	int out_ping = 0;
	int in_query = 0;
	int out_query = 0;
	int in_other = 0;
	int out_other = 0;

	for(it = sit; it != eit; it++)
	{
		if (it->second.incoming)
		{
			switch(it->second.msgType)
			{
				case BITDHT_MSG_TYPE_PING:
					in_ping++;
					break;
				case BITDHT_MSG_TYPE_FIND_NODE:
					in_query++;
					break;
				default:
					in_other++;
					break;
			}
		}
		else
		{
			switch(it->second.msgType)
			{
				case BITDHT_MSG_TYPE_PING:
					out_ping++;
					break;
				case BITDHT_MSG_TYPE_FIND_NODE:
					out_query++;
					break;
				default:
					out_other++;
					break;
			}
		}

	}

	float in_ping_per_min = in_ping * 60.0 / (end_ts - start_ts);
	float out_ping_per_min = out_ping * 60.0 / (end_ts - start_ts);
	float in_query_per_min = in_query * 60.0 / (end_ts - start_ts);
	float out_query_per_min = out_query * 60.0 / (end_ts - start_ts);

	if ((in_ping_per_min > MAX_PING_PER_MINUTE) ||
		(out_ping_per_min > MAX_PING_PER_MINUTE) ||
		(in_query_per_min > MAX_PING_PER_MINUTE) ||
		(out_query_per_min > MAX_PING_PER_MINUTE))
	{
		flagged = true;
	}

	if (flagged)
	{
		/* print header */
		std::ostream &out = std::cerr;
		out << "BdHistoryAnalysis has flagged peer: ";
		bdStdPrintId(out, &mId);
		out << std::endl;

		out << "PeerType: " << mPeerVersion;
		out << std::endl;

		out << "Ping In Per Min  : " << in_ping_per_min << std::endl;
		out << "Ping Out Per Min : " << out_ping_per_min << std::endl;
		out << "Query In Per Min : " << in_query_per_min << std::endl;
		out << "Query Out Per Min: " << out_query_per_min << std::endl;

		out << "Message History: ";
		out << std::endl;

		printHistory(out, 0, 0, time(NULL));
	}
	return true;
}

bdHistory::bdHistory(time_t store_period)
 :mStorePeriod(store_period) { return; }

void bdHistory::addMsg(const bdId *id, bdToken * /*transId*/, uint32_t msgType, bool incoming, const bdNodeId *aboutId)
{
	//std::cerr << "bdHistory::addMsg() ";
	//bdStdPrintId(std::cerr, id);
	//std::cerr << std::endl;

	time_t now = time(NULL);

	std::map<bdId, bdMsgHistoryList>::iterator it;
	bdMsgHistoryList &histRef = mHistory[*id]; /* will instaniate empty */
	histRef.mId = *id;
	histRef.addMsg(now, msgType, incoming, aboutId);

	/* add to mMsgTimeline */
	mMsgTimeline.insert(std::make_pair(now, MsgRegister(id, msgType, incoming, aboutId)));
}

void bdHistory::setPeerType(const bdId *id, std::string version)
{
	std::map<bdId, bdMsgHistoryList>::iterator it;
	bdMsgHistoryList &histRef = mHistory[*id]; /* will instaniate empty */
	histRef.setPeerType(time(NULL), version);
}

void bdHistory::printMsgs()
{
	/* print and clear msgs */
	std::ostream &out = std::cerr;

	std::cerr << "bdHistory::printMsgs()";
	std::cerr << std::endl;


	std::map<bdId, bdMsgHistoryList> ::iterator it;
	for(it = mHistory.begin(); it != mHistory.end(); it++)
	{
		if (it->second.msgCount(0, time(NULL))) // all msg count.
		{
			/* print header */
			out << "Msgs for ";
			bdStdPrintId(out, &(it->first));
			out << " v:" << it->second.mPeerVersion;
			out << std::endl;

			it->second.printHistory(out, 0, 0, time(NULL));
		}
	}

	out << "Msg Timeline:";
	time_t now = time(NULL);
	std::multimap<time_t, MsgRegister>::iterator hit;
	for(hit = mMsgTimeline.begin(); hit != mMsgTimeline.end(); hit++)
	{
		out << now - hit->first << "   ";
		bdStdPrintId(out, &(hit->second.id));

		if (hit->second.incoming)
		{
			out << " =I> ";
		}
		else
		{
			out << " <O= ";
		}

		std::string name;
		if (bitdht_msgtype(hit->second.msgType, name))
		{
			out << name;
		}
		else
		{
			out << "UNKNOWN MSG";
		}
		out << std::endl;
	}
}


void bdHistory::cleanupOldMsgs()
{
	std::cerr << "bdHistory::cleanupOldMsgs()";
	std::cerr << std::endl;

	if (mStorePeriod == 0)
	{
		return; // no cleanup
	}

	std::set<bdId> to_cleanup;
	std::set<bdId>::iterator cit;

	time_t before = time(NULL) - mStorePeriod;

	// Delete the old stuff in the list.
	while((mMsgTimeline.begin() != mMsgTimeline.end()) && (mMsgTimeline.begin()->first < before))
	{
		std::multimap<time_t, MsgRegister>::iterator it = mMsgTimeline.begin();
		to_cleanup.insert(it->second.id);
		mMsgTimeline.erase(it);
	}

	// remove old msgs, delete entry if its empty.
	std::map<bdId, bdMsgHistoryList>::iterator hit;
	for(cit = to_cleanup.begin(); cit != to_cleanup.end(); cit++)
	{
		hit = mHistory.find(*cit);
		if (hit != mHistory.end())
		{
			if (hit->second.msgClear(before))
			{
				// don't erase actual entry (so we remember peer type).
				//mHistory.erase(hit);
			}
		}
	}
}



void bdHistory::clearHistory()
{
	// Switched to a alternative clear, so we don't drop peers, and remember their type.
	//mHistory.clear();
	

	std::map<bdId, bdMsgHistoryList> ::iterator it;
	for(it = mHistory.begin(); it != mHistory.end(); it++)
	{
		it->second.clearHistory();
	}
}

bool bdHistory::canSend(const bdId *id)
{

	std::map<bdId, bdMsgHistoryList> ::iterator it;
	it = mHistory.find(*id);
	if (it != mHistory.end())
	{
		return (it->second.canSend());
	}

	/* if not found - then can send */
	return true;
}


bool bdHistory::validPeer(const bdId *id)
{
	std::map<bdId, bdMsgHistoryList> ::iterator it;
	it = mHistory.find(*id);
	if (it != mHistory.end())
	{
		return (it->second.validPeer());
	}

	/* if not found - then can send */
	return false;
}

bool bdHistory::analysePeers()
{
	std::map<bdId, bdMsgHistoryList> ::iterator it;
	for(it = mHistory.begin(); it != mHistory.end(); it++)
	{
		it->second.analysePeer();
	}
	return true;
}



/* Temp data class. */
class TypeStats
{
	public:

	TypeStats() :nodes(0) { return; }

	std::map<uint32_t, uint32_t> incoming, outgoing;
	int nodes;


void	printStats(std::ostream &out, const TypeStats *refStats)
{
	std::map<uint32_t, uint32_t>::iterator it;
	std::map<uint32_t, uint32_t>::const_iterator rit;

	out << "  Nodes: " << nodes;
	if (refStats)
	{
		out << " (" << 100.0 * nodes / (float) refStats->nodes << " %)";
	}
	out << std::endl;

	out << "  Incoming Msgs";
	out << std::endl;
	for(it = incoming.begin(); it != incoming.end(); it++)
	{
		uint32_t count = 0;
		if (refStats)
		{			
			rit = refStats->incoming.find(it->first);
			if (rit != refStats->incoming.end())
			{
				count = rit->second;
			}
		}
		printStatsLine(out, it->first, it->second, count);
	}

	out << "  Outgoing Msgs";
	out << std::endl;
	for(it = outgoing.begin(); it != outgoing.end(); it++)
	{
		uint32_t count = 0;
		if (refStats)
		{			
			rit = refStats->outgoing.find(it->first);
			if (rit != refStats->outgoing.end())
			{
				count = rit->second;
			}
		}
		printStatsLine(out, it->first, it->second, count);
	}
}



void   printStatsLine(std::ostream &out, uint32_t msgType, uint32_t count, uint32_t global)
{
	std::string name;
	bitdht_msgtype(msgType, name);
	out << "\t" << name << " " << count;
	if (global != 0)
	{
		out << " (" << 100.0 * count / (float) global << " %)";
	}
	out << std::endl;
}

}; /* end of TypeStats */

bool bdHistory::peerTypeAnalysis()
{

	std::map<std::string, TypeStats> mTypeStats;
	TypeStats globalStats;

	std::map<bdId, bdMsgHistoryList> ::iterator it;
	for(it = mHistory.begin(); it != mHistory.end(); it++)
	{
		if (it->second.msgHistory.empty())
		{
			continue;
		}

		std::string version = it->second.mPeerVersion;
		// group be first two bytes.
		version = it->second.mPeerVersion.substr(0,2);
		TypeStats &stats = mTypeStats[version];

		stats.nodes++;
		globalStats.nodes++;

		std::multimap<time_t, bdMsgHistoryItem>::iterator lit;
		for (lit = it->second.msgHistory.begin(); lit != it->second.msgHistory.end(); lit++)
		{
			if (lit->second.incoming)
			{
				stats.incoming[lit->second.msgType]++;
				globalStats.incoming[lit->second.msgType]++;
			}
			else
			{
				stats.outgoing[lit->second.msgType]++;
				globalStats.outgoing[lit->second.msgType]++;
			}
		}
	}


	std::map<std::string, TypeStats>::iterator tit;
	for(tit = mTypeStats.begin(); tit != mTypeStats.end(); tit++)
	{
		std::cerr << "Stats for Peer Type: " << tit->first;
		std::cerr << std::endl;
		tit->second.printStats(std::cerr, &globalStats);	
	}

	std::cerr << "Global Stats: ";
	std::cerr << std::endl;

	globalStats.printStats(std::cerr, NULL);
	return true;

}



