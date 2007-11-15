/*
 * "$Id: p3channel.h,v 1.6 2007-03-05 21:26:03 rmf24 Exp $"
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2004-2006 by Robert Fernie.
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



#ifndef MRK_P3_CHANNEL_H
#define MRK_P3_CHANNEL_H

#include "pqi/pqichannel.h"

/****
 * This is our channel class.
 * It performs various functions.
 * 1) collect channel messages.
 * 2) save/restore messages.
 * 3) manage their downloads....
 * 4) rebroadcast messages.
 *
 * You need to be able to categorise the channels
 * 1) Favourites.
 * 2) Standard Subscription
 * 3) Listen
 * 4) Others.
 *
 * The first three groups will be maintained.
 *
 * Controllable how long lists are maintained. (1 week standard)
 *
 * Messages are rebroadcast, At a random interval (0 - 4 hours) after
 * receiving the message.
 *
 * Popularity rating on how much each is rebroadcast.
 *
 *
 * There should be a couple of rating schemes to decide
 * on how long something stays downloaded.
 *
 * 1) Time, Old stuff first.
 * 2) Volume....
 * 3) Size.
 * 4) Popularity.
 *
 */

#include <string>
#include <list>
#include <time.h>

#include "pqi/pqi.h"

/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)

#include "pqi/xpgpcert.h"

#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

#include "pqi/sslcert.h"

#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

#include "pqi/discItem.h"
#include "pqi/pqitunnel.h"

/* Size of these should match the RsInterface (for ease of use ) - 16 bytes */

//#define CHAN_SIGN_SIZE 16

class channelSign
{
	public:
bool    operator<(const channelSign &s) const;
bool    operator==(const channelSign &s) const;
std::ostream& print(std::ostream&) const;
std::ostream& printHex(std::ostream&) const;
	char sign[CHAN_SIGN_SIZE];

};

typedef channelSign MsgHash;

class channelKey
{
	public:
	channelKey(EVP_PKEY *k);
	channelKey();
virtual ~channelKey();

bool	valid();
int	load(const unsigned char *d, int len);
int	setKey(EVP_PKEY *k);
int	out(unsigned char *d, int len);

bool    operator<(const channelKey &k) const;
bool    operator==(const channelKey &k) const;
std::ostream& print(std::ostream&) const;

channelSign getSign() const;
EVP_PKEY *  getKey() const;

	private:
int	calcSign();

	EVP_PKEY *key;
	char sign[CHAN_SIGN_SIZE];
};

typedef time_t TimeStamp;

//class TimeStamp
//{
//	public:
//
//	int yr, month, day, hour, min, sec;
//};
//

class chanMsgSummary
{
	public:
		std::string 	msg;
		MsgHash		mh;
		int		nofiles;
		int		totalsize;
		TimeStamp	recvd;
};



// This is used For Msg + Info storage.
class channelMsg
{
	public:
channelMsg(PQChanItem *in, TimeStamp now, TimeStamp reb);

	PQChanItem *msg;

	int receivedCount;
	TimeStamp recvTime;

	bool rebroadcast;
	TimeStamp rbcTime;

	bool downloaded;
	TimeStamp dldTime; 

	bool save;
};

// a little helper function.
channelSign 	getChannelSign(PQChanItem *ci);
channelKey *	getChannelKey(PQChanItem *ci);
MsgHash         getMsgHash(PQChanItem *ci);

class pqichannel
{
	public:

	pqichannel(sslroot *, channelKey *, std::string title, int mode);
virtual ~pqichannel() { return; }

	// retrieval.

channelMsg *findMsg(MsgHash mh);
int 	getMsgSummary(std::list<chanMsgSummary> &summary);


int	processMsg(PQChanItem *in);
int		pruneOldMsgs();

	// check packet signature (with channel key)
	// check key matches ours.
	// if we have it, increment the counter.
	// else add in.

	// fill with outgoing 
int	reBroadcast(std::list<PQTunnel *> &outgoing);

channelSign getSign();
channelKey *getKey();

int	getMode() { return mode; }
std::string getName() { return channelTitle; }
float 	getRanking()  { return ranking; }
int  	getMsgCount() { return msgByHash.size(); }

std::ostream& print(std::ostream&);

	protected:
sslroot *sroot;

	private:

	// analysis functions
TimeStamp getCurrentTimeStamp();
bool      checkPktSignature(PQChanItem*);
//MsgHash   getPktHash(PQChanItem*);

	int mode; // Fav, Sub, Listen, Other.
	std::map<MsgHash, channelMsg *> msgByHash;

	std::string channelTitle;
	float ranking;

	protected: // needed by pqichanneler.
	channelKey *key;
};

// specialisation with the private key, and output.
class pqichanneler: public pqichannel
{
	public:
	  pqichanneler(sslroot *s, 
	  		channelKey *priv, channelKey *pub,
			std::string title);

virtual  ~pqichanneler() { return; }

	  // Takes a partially constructed Msg, 
	  // fills in the signature....
	  // then all it has to do is pass it
	  // to the pqichannel which will handle the
	  // distribution.
int	  sendmsg(PQChanItem *); 

	private:
	channelKey *privkey;
};


class p3channel: public PQTunnelService
{
	public:
		p3channel(sslroot *r);
virtual		~p3channel();

		// PQTunnelService functions.
virtual int     receive(PQTunnel *);
virtual PQTunnel *send();

virtual		int tick();

		// doesn't use the init functions.
virtual int     receive(PQTunnelInit *i) { delete i; return 1; }
virtual PQTunnelInit *sendInit() { return NULL; }

		// End of PQTunnelService functions.
	
		// load and save configuration.
		int save_configuration();
		int load_configuration();

		// test functions.
int		generateRandomMsg();
int 		generateRandomKeys(channelKey *priv, channelKey *pub);

		// retrieval fns.
pqichannel	*findChannel(channelSign s);
int		getChannelList(std::list<pqichannel *> &chans);

	private:
	
		// so we need an interface for the data.
	
int 		handleIncoming();

int 		printMsgs();

		// first storage.
		std::map<channelSign, pqichannel *> channels;


		// 
int		reBroadcast();
		// maintainance.
int		pruneOldMsgs();

		// counters.
int 		ts_nextlp;
int		min10Count;

		std::list<cert *> discovered;
		sslroot *sroot;

		// Storage for incoming/outgoing.
		std::list<PQItem *> inpkts;
		std::list<PQItem *> outpkts;
};

#endif // MRK_P3_CHANNEL_H
