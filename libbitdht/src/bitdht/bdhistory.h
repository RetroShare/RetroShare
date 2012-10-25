#ifndef BITDHT_HISTORY_H
#define BITDHT_HISTORY_H

#include "bitdht/bdpeer.h"
#include "bitdht/bdobj.h"
#include <map>

#define MSG_TYPE_DIRECTION_MASK	0x000f0000

#define MSG_DIRECTION_INCOMING  0x00010000
#define MSG_DIRECTION_OUTGOING  0x00020000

/**** DEBUGGING HISTORY ****/

class MsgRegister
{
	public:
	MsgRegister() { return; }
	MsgRegister(const bdId *inId, uint32_t inMsgType, bool inIncoming)
	  :id(*inId), msgType(inMsgType), incoming(inIncoming) { return; }

	bdId id;
	uint32_t msgType;
	bool incoming;
};



class bdMsgHistoryList
{
	public:
void	addMsg(time_t ts, uint32_t msgType, bool incoming);
int	msgCount(time_t start_ts, time_t end_ts);
bool	msgClear(time_t before); // 0 => clear all.
void	printHistory(std::ostream &out, int mode, time_t start_ts, time_t end_ts);

bool    canSend();
bool    validPeer();

	std::multimap<time_t, uint32_t> msgHistory;
};



class bdHistory
{
	public:
        bdHistory(time_t store_period);

void	addMsg(const bdId *id, bdToken *transId, uint32_t msgType, bool incoming);
void	printMsgs();

void 	cleanupOldMsgs();
void    clearHistory();

bool    canSend(const bdId *id);
bool    validPeer(const bdId *id);

	/* recent history */
	//std::list<bdId> lastMsgs;
	std::map<bdId, bdMsgHistoryList> mHistory;
        std::multimap<time_t, MsgRegister> mMsgTimeline;

	int mStorePeriod;
};



#endif

