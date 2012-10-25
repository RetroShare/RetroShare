

#include "bitdht/bdhistory.h"
#include "bitdht/bdstddht.h"
#include "bitdht/bdmsgs.h"

#define MIN_RESEND_PERIOD  60

void	bdMsgHistoryList::addMsg(time_t ts, uint32_t msgType, bool incoming)
{
//	std::cerr << "bdMsgHistoryList::addMsg()";
//	std::cerr << std::endl;

	uint32_t msg = msgType | (incoming ? MSG_DIRECTION_INCOMING : MSG_DIRECTION_OUTGOING);
	msgHistory.insert(std::make_pair(ts, msg));
}

int	bdMsgHistoryList::msgCount(time_t start_ts, time_t end_ts)
{
	std::multimap<time_t, uint32_t>::iterator sit, eit, it;
	sit = msgHistory.lower_bound(start_ts);
	eit = msgHistory.upper_bound(end_ts);
	int count = 0;
	for (it = sit; it != eit; it++, count++) ; // empty loop.

	return count;
}

bool	bdMsgHistoryList::msgClear(time_t before)
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


void	bdMsgHistoryList::printHistory(std::ostream &out, int mode, time_t start_ts, time_t end_ts)
{
	//out << "AGE: MSGS  => incoming, <= outgoing"  << std::endl;
	std::multimap<time_t, uint32_t>::iterator sit, eit, it;
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
	
			if (MSG_DIRECTION_INCOMING & it->second)
			{
				uint32_t type = it->second-MSG_DIRECTION_INCOMING;
				out << "( => ";
				std::string name;
				if (bitdht_msgtype(type, name))
				{
					out << name;
				}
				out << " )";
			}
			else
			{
				out << "( ";
				uint32_t type = it->second - MSG_DIRECTION_OUTGOING;
				std::string name;
				if (bitdht_msgtype(type, name))
				{
					out << name;
				}
				out << " <= )";
			}

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

	std::multimap<time_t, uint32_t>::reverse_iterator rit;

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

	std::multimap<time_t, uint32_t>::iterator it;

	for(it = msgHistory.begin(); it != msgHistory.end(); it++)
	{
		if (MSG_DIRECTION_INCOMING & it->second)
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


bdHistory::bdHistory(time_t store_period)
 :mStorePeriod(store_period) { return; }


void bdHistory::addMsg(const bdId *id, bdToken * /*transId*/, uint32_t msgType, bool incoming)
{
	//std::cerr << "bdHistory::addMsg() ";
	//bdStdPrintId(std::cerr, id);
	//std::cerr << std::endl;

	time_t now = time(NULL);

	std::map<bdId, bdMsgHistoryList>::iterator it;
	bdMsgHistoryList &histRef = mHistory[*id]; /* will instaniate empty */
	histRef.addMsg(now, msgType, incoming);

	/* add to mMsgTimeline */
	mMsgTimeline.insert(std::make_pair(now, MsgRegister(id, msgType, incoming)));
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
			out << " => ";
		}
		else
		{
			out << " <= ";
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

	std::list<bdId> to_cleanup;
	std::list<bdId>::iterator cit;

	time_t before = time(NULL) - mStorePeriod;

	// Delete the old stuff in the list.
	while((mMsgTimeline.begin() != mMsgTimeline.end()) && (mMsgTimeline.begin()->first < before))
	{
		std::multimap<time_t, MsgRegister>::iterator it = mMsgTimeline.begin();
		to_cleanup.push_back(it->second.id);
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
				mHistory.erase(hit);
			}
		}
	}
}


void bdHistory::clearHistory()
{
	mHistory.clear();
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


