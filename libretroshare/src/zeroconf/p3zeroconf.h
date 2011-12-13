/*
 * libretroshare/src/zeroconf: p3zeroconf.h
 *
 * ZeroConf interface for RetroShare.
 *
 * Copyright 2011-2011 by Robert Fernie.
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


#ifndef MRK_P3_ZEROCONF_H
#define MRK_P3_ZEROCONF_H

#include "pqi/pqiassist.h"
#include "retroshare/rsdht.h"

#include <string>
#include <map>
#include "pqi/pqinetwork.h"
#include "pqi/pqimonitor.h"
#include "util/rsthreads.h"

#include <dns_sd.h>


class zcBrowseResult
{
	public:
	DNSServiceFlags flags;
	uint32_t interfaceIndex;
	std::string serviceName;
	std::string regtype;
	std::string replyDomain;
};


class zcResolveResult
{
	public:
	zcResolveResult() { return; } // :txtRecord(NULL) { return; }
	~zcResolveResult() { return; } //{ if (txtRecord) { free(txtRecord); txtRecord = NULL; } }

	DNSServiceFlags flags;
	uint32_t interfaceIndex;
	std::string fullname;
	std::string hosttarget;
	uint16_t port;
	uint16_t txtLen;
	//unsigned char *txtRecord;
};


class zcQueryResult
{
	public:
	zcQueryResult() { return; } //:rdata(NULL) { return; }
	~zcQueryResult() {return; } //{ if (rdata) { free(rdata); rdata = NULL; } }

	DNSServiceFlags flags;
	uint32_t interfaceIndex;
	std::string fullname;
	uint16_t rrtype;
	uint16_t rrclass;
	uint16_t rdlen;
	//void *rdata;
	uint32_t ttl;
};



class zcPeerDetails
{
	public:

};

class p3NetMgr;

class p3ZeroConf: public pqiNetAssistConnect, public pqiNetListener
{
	public:
	p3ZeroConf(std::string gpgid, std::string sslid, pqiConnectCb *cb, p3NetMgr *nm);
virtual	~p3ZeroConf();

	/*** OVERLOADED from pqiNetListener ***/

virtual bool resetListener(struct sockaddr_in &local);

void	start(); /* starts up the bitdht thread */

	/* pqiNetAssist - external interface functions */
virtual int     tick();
virtual void    enable(bool on);  
virtual void    shutdown(); /* blocking call */
virtual void	restart();

virtual bool    getEnabled();
virtual bool    getActive();
virtual bool    getNetworkStats(uint32_t &netsize, uint32_t &localnetsize);

	/* pqiNetAssistConnect - external interface functions */

	/* add / remove peers */
virtual bool 	findPeer(std::string id);
virtual bool 	dropPeer(std::string id);

virtual int addBadPeer(const struct sockaddr_in &addr, uint32_t reason, uint32_t flags, uint32_t age);
virtual int addKnownPeer(const std::string &pid, const struct sockaddr_in &addr, uint32_t flags);

	/* feedback on success failure of Connections */
virtual void 	ConnectionFeedback(std::string pid, int state);

	/* extract current peer status */
virtual bool 	getPeerStatus(std::string id, 
			struct sockaddr_in &laddr, struct sockaddr_in &raddr, 
					uint32_t &type, uint32_t &mode);

virtual bool    setAttachMode(bool on);

	/* pqiNetAssistConnect - external interface functions */

	
	public:

	// Callbacks must be public -> so they can be accessed.
	void callbackRegister(DNSServiceRef sdRef, DNSServiceFlags flags, 
						  DNSServiceErrorType errorCode,
						  const char *name, const char *regtype, const char *domain);

	void callbackBrowse(DNSServiceRef sdRef, DNSServiceFlags flags,
						uint32_t interfaceIndex, DNSServiceErrorType errorCode,
						const char *serviceName, const char *regtype, const char *replyDomain);
	
	void callbackResolve( DNSServiceRef sdRef, DNSServiceFlags flags,
						 uint32_t interfaceIndex, DNSServiceErrorType errorCode,
						 const char *fullname, const char *hosttarget, uint16_t port,
						 uint16_t txtLen, const unsigned char *txtRecord);

	void callbackQueryIp( DNSServiceRef sdRef, DNSServiceFlags flags,
						 uint32_t interfaceIndex, DNSServiceErrorType errorCode,
						 const char *fullname, uint16_t rrtype, uint16_t rrclass,
						 uint16_t rdlen, const void *rdata, uint32_t ttl);
	
	private:

	void createTxtRecord();

	/* monitoring fns */
	void checkServiceFDs();
	void locked_checkFD(DNSServiceRef ref);
	int checkResolveAction();
	int checkQueryAction();
	int checkQueryResults();


	/**** THESE ARE MAINLY SEMI LOCKED.... not quite sure when the callback will happen! ***/

	int locked_startRegister();
	void locked_stopRegister();

	int locked_startBrowse();
	int locked_stopBrowse();


	void locked_startResolve(uint32_t idx, std::string name, 
			std::string regtype, std::string domain);
	int locked_stopResolve();

	void locked_startQueryIp(uint32_t idx, std::string fullname);
	int locked_stopQueryIp();

	std::string displayDNSServiceError(DNSServiceErrorType errcode);


	bool mRegistered;
	bool mTextOkay;
	bool mPortOkay;
	
	uint16_t mLocalPort;
	std::string mTextRecord;

	DNSServiceRef mRegisterRef;
	DNSServiceRef mBrowseRef;
	DNSServiceRef mResolveRef;
	DNSServiceRef mQueryRef;

	uint32_t mRegisterStatus; 
	uint32_t mBrowseStatus; 
	uint32_t mResolveStatus;
	uint32_t mQueryStatus;


std::list<zcBrowseResult> mBrowseResults;
std::list<zcResolveResult> mResolveResults;
std::list<zcQueryResult> mQueryResults;





	p3NetMgr *mNetMgr;

	RsMutex mZcMtx;

	std::string mOwnGpgId;
	std::string mOwnSslId;

	time_t mMinuteTS;

	/* translation maps */

};

#endif /* MRK_P3_ZEROCONF_H */

