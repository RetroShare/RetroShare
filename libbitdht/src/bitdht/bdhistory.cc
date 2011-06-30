

#include "bitdht/bdhistory.h"
#include "bitdht/bdstddht.h"

#define MIN_RESEND_PERIOD  60

void	bdMsgHistoryList::addMsg(time_t ts, uint32_t msgType, bool incoming)

{
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

void	bdMsgHistoryList::msgClear()
{
	msgHistory.clear();
}


void	bdMsgHistoryList::printHistory(std::ostream &out, int mode, time_t start_ts, time_t end_ts)
{
	//out << "AGE: MSGS  => incoming, <= outgoing"  << std::endl;
	std::multimap<time_t, uint32_t>::iterator sit, eit, it;
	sit = msgHistory.lower_bound(start_ts);
	eit = msgHistory.upper_bound(end_ts);
	time_t curr_ts = 0;
	time_t old_ts = 0;
	bool time_changed = false;
	bool first_line = true;
	
	for(it = sit; it != eit; it++)
	{
		time_changed = false;
		if (curr_ts != it->first)
		{
			old_ts = curr_ts;
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
				out << " =>" << it->second - MSG_DIRECTION_INCOMING;
				out << " ";
			}
			else
			{
				out << " ";
				out << it->second - MSG_DIRECTION_OUTGOING << "=> ";
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

void bdHistory::addMsg(const bdId *id, bdToken * /*transId*/, uint32_t msgType, bool incoming)
{
	std::map<bdId, bdMsgHistoryList>::iterator it;
	bdMsgHistoryList &histRef = mHistory[*id]; /* will instaniate empty */
	histRef.addMsg(time(NULL), msgType, incoming);
}

void bdHistory::printMsgs()
{
	/* print and clear msgs */
	std::ostream &out = std::cerr;

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


