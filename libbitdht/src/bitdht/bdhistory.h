#ifndef BITDHT_HISTORY_H
#define BITDHT_HISTORY_H

#include "bitdht/bdpeer.h"
#include "bitdht/bdobj.h"
#include <map>

#define MSG_TYPE_DIRECTION_MASK	0x000f0000

#define MSG_DIRECTION_INCOMING  0x00010000
#define MSG_DIRECTION_OUTGOING  0x00020000

/**** DEBUGGING HISTORY ****/

class bdMsgHistoryList
{
	public:

void	addMsg(time_t ts, uint32_t msgType, bool incoming);
int	msgCount(time_t start_ts, time_t end_ts);
void	msgClear();
void	printHistory(std::ostream &out, int mode, time_t start_ts, time_t end_ts);

bool    canSend();
bool    validPeer();

	std::multimap<time_t, uint32_t> msgHistory;
};



class bdHistory
{
	public:
void	addMsg(const bdId *id, bdToken *transId, uint32_t msgType, bool incoming);
void	printMsgs();
void    clearHistory();

bool    canSend(const bdId *id);
bool    validPeer(const bdId *id);

	/* recent history */
	//std::list<bdId> lastMsgs;
	std::map<bdId, bdMsgHistoryList> mHistory;
};



#endif

