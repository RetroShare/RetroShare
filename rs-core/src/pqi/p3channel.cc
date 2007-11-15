/*
 * "$Id: p3channel.cc,v 1.8 2007-04-07 08:40:54 rmf24 Exp $"
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




#include "pqi/p3channel.h"

// for active local cert stuff.
#include "pqi/pqissl.h"

#include <iostream>
#include <errno.h>
#include <cmath>

#include <sstream>
#include <iomanip>
#include "pqi/pqidebug.h"

const int pqichannelzone = 3334;

/*
 * Compile Flag to generate Rnd messages frequently.
 *
 * PQI_CHANNEL_GENERATE_RND  1
 */
#define PQI_CHANNEL_GENERATE_RND  1



				//            | Publisher Bit.
				//            v 
				//             | Subscriber Bit
				//             | (download)
				//             v
				//              | Listen Bit
				//              | (reBroadcast)
				//              v

const int P3Chan_Publisher_Bit = 0x040;   // 0100 0000
const int P3Chan_Subscriber_Bit = 0x020;  // 0010 0000
const int P3Chan_Listener_Bit = 0x010;    // 0001 0000
const int P3Chan_No_Bit = 0x000;          // 0000 0000

const int P3Chan_Publisher = 0x050;       // 0101 0000
const int P3Chan_Subscriber = 0x030;      // 0011 0000
const int P3Chan_Listener = 0x010;        // 0001 0000
const int P3Chan_Other = 0x000;           // 0000 0000

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
 * receiving the message. (and downloading it???)
 *
 * Maximum Download Cache per channel.
 *
 * Flag stuff to keep.
 *
 * Popularity rating on how much each is rebroadcast.
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

#include "pqi.h"

#include "discItem.h"
#include "pqitunnel.h"

/**************************
class channelSign
**************************/
bool    channelSign::operator<(const channelSign &s) const
{
	return (memcmp(sign, s.sign, CHAN_SIGN_SIZE) < 0);
}

bool    channelSign::operator==(const channelSign &s) const
{
	return (memcmp(sign, s.sign, CHAN_SIGN_SIZE) == 0);
}

std::ostream &channelSign::print(std::ostream &out) const
{
	out << "channelSign :";
	for(int i = 0; i < CHAN_SIGN_SIZE; i++)
	{
		unsigned char n = ((unsigned char *) sign)[i];
		out << (unsigned int) n << ":";
	}
	return out;
}


std::ostream &channelSign::printHex(std::ostream &out) const
{
	out << std::hex << "[";
	for(int i = 0; i < CHAN_SIGN_SIZE; i++)
	{
		unsigned char n = ((unsigned char *) sign)[i];
		out << std::setw(2) << (unsigned int) n;
		if (i < CHAN_SIGN_SIZE-1)
		{
			out << ":";
		}
		else
		{
			out << "]";
		}
			
	}
	out << std::dec;
	return out;
}

/**************************
class channelKey
**************************/

bool    channelKey::operator<(const channelKey &s) const
{
	return (memcmp(sign, s.sign, CHAN_SIGN_SIZE) < 0);
}

bool    channelKey::operator==(const channelKey &s) const
{
	return (memcmp(sign, s.sign, CHAN_SIGN_SIZE) == 0);
}


std::ostream &channelKey::print(std::ostream &out) const
{
	out << "channelKey :";
	for(int i = 0; i < CHAN_SIGN_SIZE; i++)
	{
		unsigned char n = ((unsigned char *) sign)[i];
		out << (unsigned int) n << ":";
	}
	return out;
}


channelKey::channelKey()
	:key(NULL)
{
}


channelKey::channelKey(EVP_PKEY *k)
	:key(k)
{
	// work out sign.
	calcSign();
}

int     channelKey::setKey(EVP_PKEY *k)
{

	{
	  std::ostringstream out;
	  out << "channelKey::setKey()";
	  pqioutput(PQL_DEBUG_BASIC, pqichannelzone, out.str());
	}

	key = k;
	calcSign();
	return 1;
}

channelKey::~channelKey()
{
	if (key)
	{
		EVP_PKEY_free(key);
	}
}

int channelKey::load(const unsigned char *d, int len)
{
	const unsigned char *ptr = d;

	{
	  std::ostringstream out;
	  out << "channelKey::load(" << len << ") ";
	  pqioutput(PQL_DEBUG_BASIC, pqichannelzone, out.str());
	}

        RSA *rsa;

        if (NULL == (rsa = d2i_RSAPublicKey(NULL, &ptr, len)))
	{
	  std::ostringstream out;
	  out << "channelKey::in() Failed to Load RSA Key.";
	  pqioutput(PQL_DEBUG_BASIC, pqichannelzone, out.str());

	  return -1;
	}

	// get the signature
	

	// read in using ssl stylee.
	key = EVP_PKEY_new();

	EVP_PKEY_assign_RSA(key, rsa);

	return calcSign();
}

int     channelKey::calcSign()
{
	// The signature will be extracted from the 
	// public exponential.
	
	RSA *rsa = EVP_PKEY_get1_RSA(key);
	int len = BN_num_bytes(rsa -> n);
	unsigned char tmp[len];
	BN_bn2bin(rsa -> n, tmp);

	// copy first CHAN_SIGN_SIZE bytes...
	if (len > CHAN_SIGN_SIZE)
	{
		len = CHAN_SIGN_SIZE;
	}

	std::ostringstream out;
	out << "channelKey::calcSign()";

	for(int i = 0; i < len; i++)
	{
		sign[i] = tmp[i];
		unsigned char c = ((unsigned char *) sign)[i];
		out << (unsigned int) c;
		if (i != len - 1)
			out << ":";
	}
	pqioutput(PQL_DEBUG_BASIC, pqichannelzone, out.str());

	RSA_free(rsa);
	return 1;
}


int     channelKey::out(unsigned char *data, int len)
{
	unsigned char *ptr = data;

	if (!key)
	{
		std::ostringstream out;
		out << "channelKey::out() Key = NULL";
		pqioutput(PQL_DEBUG_BASIC, pqichannelzone, out.str());
		return -1;
	}

	RSA *trsa = EVP_PKEY_get1_RSA(key);
	int reqspace = i2d_RSAPublicKey(trsa, NULL);

	if (data == NULL)
	{
		std::ostringstream out;
		out << "channelKey::out() Returning Size";
		pqioutput(PQL_DEBUG_BASIC, pqichannelzone, out.str());
		RSA_free(trsa);
		return reqspace;
	}

	if (len < reqspace)
	{
		std::ostringstream out;
		out << "channelKey::out() not enough space";
		pqioutput(PQL_DEBUG_BASIC, pqichannelzone, out.str());
		RSA_free(trsa);
		return -1;
	}

	int val = i2d_RSAPublicKey(trsa, &ptr);

	// delete the rsa key.
	RSA_free(trsa);

	return val;
}



bool channelKey::valid()
{
	return (key != 0);
}

EVP_PKEY *channelKey::getKey() const
{
	return key;
}

channelSign channelKey::getSign() const
{
	std::ostringstream out;
	out << "channelKey::getSign()";

	channelSign s;
	for(int i = 0; i < CHAN_SIGN_SIZE; i++)
	{
		s.sign[i] = sign[i];
		unsigned char c = ((unsigned char *) s.sign)[i];
	  	out << (unsigned int) c;
		if (i != CHAN_SIGN_SIZE-1)
			out << ":";
	}

	pqioutput(PQL_DEBUG_BASIC, pqichannelzone, out.str());
	return s;
}

/**************************
class channelMsg
**************************/

channelMsg::channelMsg(PQChanItem *in, TimeStamp now, TimeStamp reb)
	:msg(in), receivedCount(1), recvTime(now), rebroadcast(false), 
	rbcTime(reb), downloaded(false), save(false)
{
	{
	  std::ostringstream out;
	  out << "channelMsg::channelMsg()";
	  pqioutput(PQL_DEBUG_BASIC, pqichannelzone, out.str());
	}
	return;
}

/**************************
class pqichannel
**************************/

pqichannel::pqichannel(sslroot *s, channelKey *k, std::string title, int m)
	:sroot(s), mode(m), channelTitle(title), ranking(0), key(k) {}

// retrieving msgs.
int pqichannel::getMsgSummary(std::list<chanMsgSummary> &summary)
{
	if (msgByHash.size() == 0)
	{
		return 0;
	}
	std::map<MsgHash, channelMsg *>::const_iterator it;
	PQChanItem::FileList::const_iterator flit;

	for(it = msgByHash.begin(); it != msgByHash.end(); it++)
	{
		chanMsgSummary ms;
		PQChanItem *item = it -> second -> msg;
		//ms.msg = item -> title;
		ms.msg = item -> msg;
		ms.mh = it -> first;
		ms.nofiles = item -> files.size();

		ms.totalsize = 0;
		for(flit = item->files.begin(); flit != item->files.end(); flit++)
		{
			ms.totalsize += flit->size;
		}
		ms.recvd = it->second->recvTime;

		summary.push_back(ms);
	}
	return 1;
}


channelMsg *pqichannel::findMsg(MsgHash mh)
{
	std::map<MsgHash, channelMsg *>::const_iterator it;
	it = msgByHash.find(mh);
	if (it == msgByHash.end())
	{
		return NULL;
	}
	return (it -> second);
}
	

int pqichannel::processMsg(PQChanItem *in)
{
	{
	  std::ostringstream out;
	  out << "pqichannel::processMsg()";
	  pqioutput(PQL_DEBUG_BASIC, pqichannelzone, out.str());
	}
	if (!checkPktSignature(in))
	{
		std::ostringstream out;
		out << "pqichannel::processMsg() Failed Signature Check!";
		out << std::endl;
		out << "pqichannel::processMsg() Deleting Incoming Pkt.";
		out << std::endl;

		std::cerr << out.str();
		delete in;
		return -1;
	}

	// get time stamp.
	TimeStamp now = time(NULL);
	MsgHash   hash = getMsgHash(in);
	std::map<MsgHash, channelMsg *>::iterator it;

	if (msgByHash.end() != (it = msgByHash.find(hash)))
	{
		// already there.... (check).
	
		// increment the counter.
		it -> second -> receivedCount++;

		// discard.
		std::ostringstream out;
		out << "pqichannel::processMsg() Msg Already There! Received";
		out << it -> second -> receivedCount << " times" << std::endl;
		out << "pqichannel::processMsg() Deleting Incoming Pkt.";
		out << std::endl;

		std::cerr << out.str();
		
		delete in;
		return -1;
	}
	else
	{
		// new....
		// calc Rebroadcast time.
		TimeStamp reb = now;

#ifdef PQI_CHANNEL_GENERATE_RND
		float secIn10min = 60 * 10;
		int rndsec = (int) (rand() * secIn10min / RAND_MAX);
#else
		float secIn4hrs = 60 * 4 * 60;
		int rndsec = (int) (rand() * secIn4hrs / RAND_MAX);
#endif
		reb += rndsec;

		// create channelMsg.
		channelMsg *msg = new channelMsg(in, now, reb);

		// save it.
		msgByHash[hash] = msg;

		std::ostringstream out;
		out << "pqichannel::processMsg() Saved New Msg";
		out << std::endl;

		std::cerr << out.str();
	}
	return 1;
}


	// fill with outgoing 
int pqichannel::reBroadcast(std::list<PQTunnel *> &outgoing)
{
	{
	  std::ostringstream out;
	  out << "pqichannel::reBroadcast()";
	  pqioutput(PQL_DEBUG_BASIC, pqichannelzone, out.str());
	}
	TimeStamp now = time(NULL);

	// iterate through the messages.
	// if the rebroadcast flag is unset, 
	// and the RBC is in the past....
	// add to the queue.
	
	std::map<MsgHash, channelMsg *>::iterator it;
	for(it = msgByHash.begin(); it != msgByHash.end(); it++)
	{
	  if ((!it -> second -> rebroadcast) &&
	  	(it -> second -> rbcTime < now))
	  {
		PQChanItem *nitem = it -> second -> msg -> clone();
		// reset address to everyone.
		// XXX FIX.
		for(int i = 0; i < 10; i++)
			nitem -> cid.route[i] = 0;

		// add to list.
		outgoing.push_back(nitem);

		std::ostringstream out;
		out << "pqichannel::reBroadcast() Sending Msg";
		out << std::endl;
		nitem->print(out);
		out << std::endl;

		std::cerr << out.str();

		// set flag.
	        it -> second -> rebroadcast = true;
	  }
	  else
	  {
	    std::ostringstream out;
	    if (it -> second -> rebroadcast)
            {
		out << "pqichannel::reBroadcast() Msg Already Rebroadcast";
	    }
	    else
	    {
		out << "pqichannel::reBroadcast() Not ready for rebroadcast";
	    }
	    out << std::endl;
	    std::cerr << out.str();
	  }
	}
	return 1;
}

channelSign pqichannel::getSign()
{
	return key -> getSign();
}

channelKey *pqichannel::getKey()
{
	return key;
}

/**************************
class pqichanneler
// specialisation with the private key, and output.

 **************************/

pqichanneler::pqichanneler(sslroot *s, 
				channelKey *priv, channelKey *pub, 
				std::string title)
	:pqichannel(s, pub, title, P3Chan_Publisher), privkey(priv) {}


int	  pqichanneler::sendmsg(PQChanItem *msg)
{
	{
	  std::ostringstream out;
	  out << "pqichanneler::sendmsg()";
	  pqioutput(PQL_DEBUG_BASIC, pqichannelzone, out.str());
	}
	// Takes a partially constructed Msg, 
	// fills in the signature....
	// then all it has to do is pass it
	// to the pqichannel which will handle the
	// distribution.
	
	if ((!key -> valid()) || (!privkey -> valid()))
	{
	  std::ostringstream out;
	  out << "pqichanneler::sendmsg() Keys Not Valid!";
	  pqioutput(PQL_DEBUG_BASIC, pqichannelzone, out.str());
	  delete msg;
	  return -1;
	}

	//
	msg -> certLen = key -> out(NULL, 0);
	msg -> certDER = (unsigned char *) malloc(msg -> certLen);
	key -> out(msg -> certDER, msg -> certLen);
	msg -> signLen = 0;

	// some nasty complexity here.
	// we are going to write it to a buffer (as is)
	// and then chop out the bit we need to sign.
	int tmpsize = msg -> PQChanItem::getSize();
	char *tmpspace = (char *) malloc(tmpsize);
	if (tmpsize != msg -> out(tmpspace, tmpsize))
	{
	  std::ostringstream out;
	  out << "pqichanneler::sendmsg() msg -> out() Failure!";
	  pqioutput(PQL_DEBUG_BASIC, pqichannelzone, out.str());
	  delete msg;
	  return -1;
	}

	char *sign = tmpspace + msg -> PQTunnel::getSize();
	tmpsize -= msg -> PQTunnel::getSize();

	msg -> signLen = EVP_PKEY_size(privkey -> getKey());
	msg -> signature = (unsigned char *) malloc(msg->signLen);

	sroot -> signDigest(privkey -> getKey(), sign, tmpsize, 
			msg->signature, msg->signLen);

	// now we can free the tmp space.
	free(tmpspace);

	return pqichannel::processMsg(msg);
}

/**************************
class p3channel
// the Tunnel Service.

 **************************/

p3channel::p3channel(sslroot *r)
	:PQTunnelService(PQI_TUNNEL_CHANNEL_ITEM_TYPE), sroot(r)
{
	{
	  std::ostringstream out;
	  out << "p3channel::p3channel()";
	  pqioutput(PQL_DEBUG_BASIC, pqichannelzone, out.str());
	}
        // configure...
	load_configuration();
	return;
}

p3channel::~p3channel()
{
	{
	  std::ostringstream out;
	  out << "p3channel::~p3channel()";
	  pqioutput(PQL_DEBUG_BASIC, pqichannelzone, out.str());
	}
	return;
}

// The PQTunnelService interface.
int     p3channel::receive(PQTunnel *i)
{
	{
	  std::ostringstream out;
	  out << "p3channel::receive()";
	  pqioutput(PQL_DEBUG_BASIC, pqichannelzone, out.str());
	}
	inpkts.push_back(i);

	std::ostringstream out;
	out << "p3channel::receive() InQueue:" << inpkts.size() << std::endl;
	out << "incoming packet: " << std::endl;
	i -> print(out);
	pqioutput(PQL_DEBUG_BASIC, pqichannelzone, out.str());

	return 1;
}

PQTunnel *p3channel::send()
{
	std::ostringstream out;
	out << "p3channel::send(" << outpkts.size() << ")" << std::endl;

	if (outpkts.size() < 1)
	{
		out << "------------------> No Packet" << std::endl;
		pqioutput(PQL_DEBUG_ALL, pqichannelzone, out.str());
		return NULL;
	}
	PQTunnel *pkt = (PQTunnel *) outpkts.front();
	outpkts.pop_front();

	out << "outgoing packet: " << std::endl;
	pkt -> print(out);
	pqioutput(PQL_DEBUG_BASIC, pqichannelzone, out.str());

	return pkt;
}



int p3channel::tick()
{
	// regularly run reBroadcast.
	pqioutput(PQL_DEBUG_ALL, pqichannelzone,
		"p3channel::tick()");

	handleIncoming();

	if (ts_nextlp == 0)
	{
		pqioutput(PQL_DEBUG_BASIC, pqichannelzone,
		  "p3channel::tick() ReBroadcast Cycle!");
		reBroadcast();
		min10Count++;
		// hourly.
		if (min10Count > 5)
		{
			min10Count = 0;
			// prune
			pruneOldMsgs();
		}
	}

#ifdef PQI_CHANNEL_GENERATE_RND
	if (ts_nextlp % 300 == 0)
	{
		generateRandomMsg();
		printMsgs();
		reBroadcast();
	}
#endif


	// ten minute counter.
	if (--ts_nextlp < 0)
	{
		ts_nextlp = 600;
	}

	return 1;
}

// load and save configuration.
int p3channel::save_configuration()
{
	{
	  std::ostringstream out;
	  out << "p3channel::save_configuration()";
	  pqioutput(PQL_DEBUG_BASIC, pqichannelzone, out.str());
	}
	// save to an independent file.
	return 1;
}

int p3channel::load_configuration()
{
	{
	  std::ostringstream out;
	  out << "p3channel::load_configuration()";
	  pqioutput(PQL_DEBUG_BASIC, pqichannelzone, out.str());
	}
	// load from independent file.
	return 1;
}


// retrieving channels.
pqichannel      *p3channel::findChannel(channelSign s)
{
	std::map<channelSign, pqichannel *>::const_iterator it;
	it = channels.find(s);
	if (it == channels.end())
	{
		return NULL;
	}
	return (it -> second);
}
	

int 	p3channel::getChannelList(std::list<pqichannel *> &chans)
{
	if (channels.size() == 0)
	{
		return 0;
	}

	std::map<channelSign, pqichannel *>::const_iterator it;
	for(it = channels.begin(); it != channels.end(); it++)
	{
		chans.push_back(it->second);
	}
	return 1;
}

		// maintainance.
int p3channel::pruneOldMsgs()
{
	std::ostringstream out;
	out << "p3channel::pruneOldMsgs()";

	std::map<channelSign,pqichannel *>::iterator it;
	int pcount = 0;
	for(it = channels.begin(); it != channels.end(); it++, pcount++)
	{
		it -> second -> pruneOldMsgs();
	}

	pqioutput(PQL_DEBUG_BASIC, pqichannelzone, out.str());
	return 1;
}


		// maintainance.
int pqichannel::pruneOldMsgs()
{
	std::ostringstream out;
	out << "p3channel::pruneOldMsgs()";


	std::map<MsgHash, channelMsg *>::iterator it, it2;
	int i = 0;
	int t = time(NULL);
	for(it = msgByHash.begin(); it != msgByHash.end(); i++)
	{
	  // still in Hours, 
	  // but should change to days....
	  float tpHr = (t - it->second->recvTime) / 3600.0;
	  float pop = (it->second->receivedCount) / (float) tpHr - tpHr / 2.0;
	  if ((pop > 0) || (tpHr < 7))
	  {
		  // leave it..
		  it++;
		  continue;
	  }
	  out << "Deleting->->-> Msg(" << i << ")" << std::endl;
	  out << "Msg(" << i << ") : " << it->second->msg->msg;
	  out << std::endl;
	  out << "Received Count     -> " << it->second->receivedCount;
	  out <<  std::endl;

	  out << "Time Since Arrival -> " << t - it->second->recvTime;
	  out <<  std::endl;
	  out << "Popularity: " << pop;
	  out <<  std::endl;
	  out << "ReB(" << it->second->rebroadcast;
	  out << ") TS: " << it->second->rbcTime;
	  out << " or RTS: " << it->second->rbcTime - time(NULL) << std::endl;
	  out << "Down(" << it->second->downloaded;
	  out << ") TS: " << it->second->dldTime;
	  out << " or RTS: " << it->second->dldTime - time(NULL) << std::endl;

	  channelSign s = getChannelSign(it->second->msg);
	  MsgHash h = getMsgHash(it->second->msg);

	  s.print(out);
	  out << std::endl;
	  h.print(out);
	  out << std::endl;
	  out << "<-<-<--------";

	  // very dubious.... will it work.??
	  it2 = it;
	  it++;
	  msgByHash.erase(it2);
	}
	pqioutput(PQL_DEBUG_BASIC, pqichannelzone, out.str());
	return 1;
}

int p3channel::handleIncoming()
{
	{
	  std::ostringstream out;
	  out << "p3channel::handleIncoming()";
	  pqioutput(PQL_DEBUG_ALL, pqichannelzone, out.str());
	}
	// loop through all incoming pkts.
	// send off to correct pqichannel.

	while(inpkts.size() > 0)
	{
		PQItem *in = inpkts.front();
		inpkts.pop_front();

		{
	  		std::ostringstream out;
	  		out << "p3channel::handleIncoming() Handling Pkt";
	  		pqioutput(PQL_DEBUG_BASIC, pqichannelzone, out.str());
		}


		PQChanItem *msg;
		if (NULL==(msg=dynamic_cast<PQChanItem *>(in)))
		{
			// error with the packet!.

			delete in;
			continue;
		}
		// At this point we need to validate the signature.
		bool validSign = true;

		// save the sign size, so we can blank it for a moment.
		int signlen = msg -> signLen;
		msg -> signLen = 0;

		// some nasty complexity here.
		// we are going to write it to a buffer (as is)
		// and then chop out the bit we need to sign.
		int tmpsize = msg -> PQChanItem::getSize();
		char *tmpspace = (char *) malloc(tmpsize);
		if (0 > msg -> out(tmpspace, tmpsize))
		{
	  		std::ostringstream out;
	  		out << "p3channel::handleIncoming() msg Failed out()";
	  		pqioutput(PQL_DEBUG_BASIC, pqichannelzone, out.str());
			validSign = false;
		}
		// this is okay even if out() fails.
		char *signdata = tmpspace + msg -> PQTunnel::getSize();
		tmpsize -= msg -> PQTunnel::getSize();

		// restore signLen before verifying signature.
		msg -> signLen = signlen;

		// generate the key.
		channelKey tmpkey;
		if (validSign)
		{
	  		std::ostringstream out;
	  		out << "p3channel::handleIncoming() loading key";
	  		pqioutput(PQL_DEBUG_BASIC, pqichannelzone, out.str());

			validSign = (1 == tmpkey.load(
					msg -> certDER, msg -> certLen));
		}

		if (validSign)
		{
	  		std::ostringstream out;
	  		out << "p3channel::handleIncoming() verifying Digest";
	  		pqioutput(PQL_DEBUG_BASIC, pqichannelzone, out.str());

			validSign = (1 == sroot -> verifyDigest(
					tmpkey.getKey(), signdata, tmpsize, 
					msg->signature, msg->signLen));

		}

		// now we can free the tmp space.
		// tmpkey will free itself.
		free(tmpspace);

		if (!validSign)
		{
	  		std::ostringstream out;
	  		out << "p3channel::handleIncoming() Msg Failed Check!";
	  		pqioutput(PQL_DEBUG_BASIC, pqichannelzone, out.str());

			// error somewhere along the way.
			delete in;
			continue;
		}


		// get the channel signature from the pkt.
		channelSign sign = getChannelSign(msg);
		std::map<channelSign,pqichannel *>::iterator it;
		if (channels.end() != (it=channels.find(sign)))
		{
			// add to that channel.
			it -> second -> processMsg(msg);
		}
		else
		{
			// create a new channel.
			channelKey *k = getChannelKey(msg);

			// Temporarily Make all Listeners...
			pqichannel *npc = new pqichannel(sroot, k, 
					msg->title,  P3Chan_Listener);
			channels[sign] = npc;
			npc -> processMsg(msg);
		}
	}
	return 1;
}

int p3channel::reBroadcast()
{
	{
	  std::ostringstream out;
	  out << "p3channel::reBroadcast()";
	  pqioutput(PQL_DEBUG_BASIC, pqichannelzone, out.str());
	}
	// 
	std::list<PQTunnel *> outgoing;
	std::map<channelSign,pqichannel *>::iterator it;
	for(it = channels.begin(); it != channels.end(); it++)
	{
		if (it -> second -> getMode() & P3Chan_Listener_Bit)
		{
			it -> second -> reBroadcast(outgoing);
		}
	}

	std::list<PQTunnel *>::iterator tit;
	for(tit = outgoing.begin(); tit!=outgoing.end();tit++)
	{
		outpkts.push_back(*tit);
	}
	return 1;
}


channelSign getChannelSign(PQChanItem *ci)
{
	std::ostringstream out;
	channelSign s;

	out << "getChannelSign() ";

	channelKey tmpkey;
	if (1 != tmpkey.load(ci -> certDER, ci -> certLen))
	{
		out << "FAILED!";
	}
	else
	{
		s = tmpkey.getSign();
	}

	pqioutput(PQL_DEBUG_BASIC, pqichannelzone, out.str());
	return s;
}


channelKey *getChannelKey(PQChanItem *ci)
{
	  std::ostringstream out;
	  out << "p3channel::getChannelKey()";

	channelKey *k = new channelKey();
	if (0 > k -> load(ci -> certDER, ci -> certLen))
	{
		out << "FAILED!" << std::endl;
		delete k;
		k = NULL;
	}
	else
	{
		channelSign s = getChannelSign(ci);
		s.print(out);
	}

	pqioutput(PQL_DEBUG_BASIC, pqichannelzone, out.str());
	return k;
}


/* Helper function */
MsgHash getMsgHash(PQChanItem *in)
{
	std::ostringstream out;

	MsgHash h;
	int len = in -> signLen;

	out << "getMsgHash(" << len << ") ";
	if (len > CHAN_SIGN_SIZE)
		len = CHAN_SIGN_SIZE;

	for (int i = 0; i < len; i++)
	{
		h.sign[i] = in -> signature[i];
		unsigned char c = ((unsigned char *) h.sign)[i];
	  	out << (unsigned int) c;
		if (i != len - 1)
			out << ":";
	}

	pqioutput(PQL_DEBUG_BASIC, pqichannelzone, out.str());

	return h;
}


bool pqichannel::checkPktSignature(PQChanItem *in)
{
	{
	  std::ostringstream out;
	  out << "pqichannel::checkPktSignature";
	  pqioutput(PQL_DEBUG_BASIC, pqichannelzone, out.str());
	}
	return true;
}



int p3channel::generateRandomMsg()
{
	{
	  std::ostringstream out;
	  out << "p3channel::generateRandomMsg()";
	  pqioutput(PQL_DEBUG_BASIC, pqichannelzone, out.str());
	}
	// count how many publisher channels we have.
	// generate Rnd with range (0 -> pchan)
	// if 0 -> create a new pqichanneler.
	// else select that one.
	//
	// using that pqichanneler, generate a message.

	std::map<channelSign,pqichannel *>::iterator it;
	int pcount = 0;
	int i;

	for(it = channels.begin(); it != channels.end(); it++)
	{
		if (it -> second -> getMode() & P3Chan_Publisher_Bit)
		{
			pcount++;
		}
	}

	int rnd = (int) (rand() * (pcount + 1.0) / RAND_MAX);
	pqichanneler *pc = NULL;
	channelKey *pub = NULL;
	channelKey *priv = NULL;
	if (rnd == 0)
	{
		// create a new pqichanneler!.
		pub = new channelKey();
		priv = new channelKey();
		generateRandomKeys(priv, pub);

		pc = new pqichanneler(sroot, priv, pub, "pqiChanneler");
		channelSign sign = pub -> getSign();
		channels[sign] = pc;
	}
	else
	{
		// select the chosen one.
		i = 0;
		for(it = channels.begin(); (i < rnd) && 
				(it != channels.end()); it++)
		{
			if (it -> second -> getMode() & 
				P3Chan_Publisher_Bit)
			{
				i++;
			}
		}
		if (it != channels.end())
		{
			pqichannel *c = it -> second;
			pc = dynamic_cast<pqichanneler *>(c);
		}
	}

	if (pc == NULL)
	{
		// failed this time!
		return -1;
	}

	pub = pc -> getKey();

	// create a new message for the world.
	PQChanItem *msg = new PQChanItem();

static  int count = 0;
	std::ostringstream out;

	cert *own = sroot -> getOwnCert();

	out << "U:" << own -> Name() << " Says ";
	out << "Hello World #" << count++;
	msg -> msg = out.str();

	/* now add in some files */
	int nfiles = (int) ((rand() * 5.0) / RAND_MAX);
	for(i = 0; i < nfiles; i++)
	{
		PQChanItem::FileItem fi;
		std::ostringstream out;
		out << "file-" << count << "-" << nfiles;
		out << "-" << "i" << ".dta";
		fi.name = out.str();
		fi.size = count * nfiles + i + 12354;

		msg -> files.push_back(fi);
	}

	// keys and signing are handled in sendmsg().
	pc -> sendmsg(msg);
	return 1;
}


int p3channel::generateRandomKeys(channelKey *priv, channelKey *pub)
{
	// sslroot will generate the pair...
	// we need to split it into an pub/private.

	EVP_PKEY *keypair = EVP_PKEY_new();
	EVP_PKEY *pubkey = EVP_PKEY_new();
	sroot -> generateKeyPair(keypair, 0);

	RSA *rsa1 = EVP_PKEY_get1_RSA(keypair);
	RSA *rsa2 = RSAPublicKey_dup(rsa1);

	{
	  std::ostringstream out;
	  out << "p3channel::generateRandomKeys()" << std::endl;
	  out << "Rsa1: " << (void *) rsa1 << " & Rsa2: ";
	  out << (void *)  rsa2 << std::endl;

	  pqioutput(PQL_DEBUG_BASIC, pqichannelzone, out.str());
	}

	EVP_PKEY_assign_RSA(pubkey, rsa1);
	RSA_free(rsa1); // decrement ref count!

	priv -> setKey(keypair);
	pub -> setKey(pubkey);

	{
	  std::ostringstream out;
	  out << "p3channel::generateRandomKey(): ";
	  priv -> print(out);
	  pqioutput(PQL_DEBUG_BASIC, pqichannelzone, out.str());
	}
	return 1;
}



int p3channel::printMsgs()
{
	std::ostringstream out;
	out << "p3channel::printMsgs()";

	std::map<channelSign,pqichannel *>::iterator it;
	int pcount = 0;
	for(it = channels.begin(); it != channels.end(); it++, pcount++)
	{
		out << std::endl << "Channel[" << pcount << "] : ";
		it->first.print(out);
		out << std::endl;
		it -> second -> print(out);
	}

	pqioutput(PQL_DEBUG_BASIC, pqichannelzone, out.str());
	return 1;
}



std::ostream &pqichannel::print(std::ostream &out)
{
	out << "pqichannel::print()------------------";
	std::map<MsgHash, channelMsg *>::iterator it;
	int i = 0;
	int t = time(NULL);
	int oldmsgs = 0;
	for(it = msgByHash.begin(); it != msgByHash.end(); it++, i++)
	{
	  float tpHr = (t - it->second->recvTime) / 3600.0;
	  float pop = (it->second->receivedCount) / (float) tpHr - tpHr / 2.0;
	  if (pop < 0)
	  {
		  oldmsgs++;
	  	  out << "->->-> Skipped Old Msg(" << i << ")" << std::endl;
		  continue;
	  }

	  out << "-------->->->" << std::endl;
	  out << "Msg(" << i << ") : " << it->second->msg->msg;
	  out << std::endl;
	  out << "Received Count     -> " << it->second->receivedCount;
	  out <<  std::endl;

	  out << "Time Since Arrival -> " << t - it->second->recvTime;
	  out <<  std::endl;
	  out << "Popularity: " << pop;
	  out <<  std::endl;
	  out << "ReB(" << it->second->rebroadcast;
	  out << ") TS: " << it->second->rbcTime;
	  out << " or RTS: " << it->second->rbcTime - time(NULL) << std::endl;
	  out << "Down(" << it->second->downloaded;
	  out << ") TS: " << it->second->dldTime;
	  out << " or RTS: " << it->second->dldTime - time(NULL) << std::endl;

	  channelSign s = getChannelSign(it->second->msg);
	  MsgHash h = getMsgHash(it->second->msg);

	  s.print(out);
	  out << std::endl;
	  h.print(out);
	  out << std::endl;
	  out << "<-<-<--------";
	}
	out << ">>>> Total Skipped Msgs: " << oldmsgs << std::endl;
	out << "-------------------------------------";
	return out;
}


