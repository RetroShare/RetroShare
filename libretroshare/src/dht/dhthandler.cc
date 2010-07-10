
#include "dht/dhthandler.h"


/* This stuff is actually C */

#ifdef  __cplusplus
extern "C" {
#endif

#include <int128.h>
#include <rbt.h>
#include <KadCalloc.h>
#include <KadClog.h>

#ifdef  __cplusplus
} /* extern C */
#endif
/* This stuff is actually C */


/* HACK TO SWITCH THIS OFF during testing */
/*define  NO_DHT_RUNNING  1*/


#include <iostream>
#include <sstream>

std::ostream &operator<<(std::ostream &out, dhtentry &ent)
{
	out << "DHTENTRY(" << ent.id << "): Status: " << ent.status;
	out << std::endl;
	out << "\taddr: " << rs_inet_ntoa(ent.addr.sin_addr) << ":" << ntohs(ent.addr.sin_port);
	out << std::endl;
	out << "\tlastTS: " << time(NULL) - ent.lastTs << " secs ago";
	out << "\tFlags: " << ent.flags;
	out << std::endl;
	return out;
}
	
#define DHT_UNKNOWN   0
#define DHT_SEARCHING  1 /* for peers */
#define DHT_PUBLISHING 1 /* for self */
#define DHT_FOUND     2

/* time periods */
#define DHT_MIN_PERIOD        10
#define DHT_SEARCH_PERIOD     300
#define DHT_REPUBLISH_PERIOD  1200

void cleardhtentry(dhtentry *ent, std::string id)
{
	ent -> name = "";
	ent -> id = id;
	ent -> addr.sin_addr.s_addr  = 0;
	ent -> addr.sin_port = 0;
	ent -> flags = 0;
	ent -> status = DHT_UNKNOWN;
	ent -> lastTs = 0;
	return;
}


void initdhtentry(dhtentry *ent)
{
	ent -> name = "";
	// leave these ...
	//ent -> addr.sin_addr.in_addr  = 0;
	//ent -> addr.sin_port = 0;
	//ent -> flags = 0;
	ent -> status = DHT_SEARCHING;
	ent -> lastTs = time(NULL);
	return;
}

void founddhtentry(dhtentry *ent, struct sockaddr_in inaddr, unsigned int flags)
{
	ent -> addr = inaddr;
	ent -> flags = flags;
	ent -> status = DHT_FOUND;
	ent -> lastTs = time(NULL);
	return;
}


dhthandler::dhthandler(std::string inifile)
	:mShutdown(false), dhtOk(false)
{
	/* init own to null */
	dataMtx.lock(); /* LOCK MUTEX */
	cleardhtentry(&ownId, "");
	kadcFile = inifile;
	dataMtx.unlock(); /* UNLOCK MUTEX */

	/* start up the threads... */
	init();
}

dhthandler::~dhthandler()
{

}


	/* This is internal - only called when active */
bool    dhthandler::networkUp()
{
	/* no need for mutex? */
	return (20 < KadC_getnknodes(pkcc));
}

	/* this is external */
int     dhthandler::dhtPeers()
{
	int count = 0;
	dataMtx.lock(); /* LOCK MUTEX */
	if (dhtOk)
	{
		count = KadC_getnknodes(pkcc);
	}
	dataMtx.unlock(); /* UNLOCK MUTEX */
	return count;

}


	/* set own tag */
void    dhthandler::setOwnHash(std::string id)
{
	dataMtx.lock(); /* LOCK MUTEX */
	ownId.id = id;
	dataMtx.unlock(); /* UNLOCK MUTEX */
}

void    dhthandler::setOwnPort(short port)
{
	dataMtx.lock(); /* LOCK MUTEX */
	ownId.addr.sin_port = htons(port);
	/* reset own status -> so we republish */
	ownId.status = DHT_UNKNOWN;

	dataMtx.unlock(); /* UNLOCK MUTEX */
}

bool    dhthandler::getExtAddr(struct sockaddr_in &addr, unsigned int &flags)
{
	dataMtx.lock(); /* LOCK MUTEX */

	if (ownId.status == DHT_UNKNOWN)
	{
		dataMtx.unlock(); /* UNLOCK MUTEX */
		return false;
	}

	addr = ownId.addr;
	flags = ownId.flags;

	dataMtx.unlock(); /* UNLOCK MUTEX */
	return true;
}

	/* at startup */
void    dhthandler::addFriend(std::string id)
{
	dataMtx.lock(); /* LOCK MUTEX */
	std::map<std::string, dhtentry>::iterator it;
	it = addrs.find(id);
	if (it == addrs.end())
	{
		/* not found - add */
		dhtentry ent;
		cleardhtentry(&ent, id);
		addrs[id] = ent;
	}
	else
	{
		/* already there */
		std::cerr << "dhthandler::addFriend() Already there!" << std::endl;
	}
	dataMtx.unlock(); /* UNLOCK MUTEX */
}

void    dhthandler::removeFriend(std::string id)
{
	dataMtx.lock(); /* LOCK MUTEX */
	std::map<std::string, dhtentry>::iterator it;
	it = addrs.find(id);
	if (it == addrs.end())
	{
		/* not found - complain*/
		std::cerr << "dhthandler::addFriend() Already there!" << std::endl;
	}
	else
	{
		/* found */
		addrs.erase(it);
	}
	dataMtx.unlock(); /* UNLOCK MUTEX */
}

	/* called prior to connect */
bool    dhthandler::addrFriend(std::string id, struct sockaddr_in &addr, unsigned int &flags)
{

	dataMtx.lock(); /* LOCK MUTEX */

	/* look it up */
	bool ret = false;
	std::map<std::string, dhtentry>::iterator it;
	it = addrs.find(id);
	if (it == addrs.end())
	{
		/* not found - complain*/
		std::cerr << "dhthandler::addrFriend() Non-existant!" << std::endl;
		ret = false;
	}
	else
	{
		if (it->second.status == DHT_FOUND)
		{
			addr = it->second.addr;
			ret = true;
		}
	}
	dataMtx.unlock(); /* UNLOCK MUTEX */
	return ret;
}

int dhthandler::init()
{
	dataMtx.lock(); /* LOCK MUTEX */

	/* HACK TO SWITCH THIS OFF during testing */
#ifdef  NO_DHT_RUNNING
	dataMtx.unlock(); /* UNLOCK MUTEX */
	dhtOk = false;
	return 1;
#endif

	char *filename = (char *) malloc(1024);
	sprintf(filename, "%.1023s", kadcFile.c_str());

	/* start up the dht server. */
	KadC_log("KadC - library version: %d.%d.%d\n",
		KadC_version.major, KadC_version.minor, KadC_version.patchlevel);

	/* file, Leaf, StartNetworking (->false in full version) */
	kcc = KadC_start(filename, true, 1);
	if(kcc.s != KADC_OK) {
		KadC_log("KadC_start(%s, %d) returned error %d:\n",
			kadcFile.c_str(), 1, kcc.s);
		KadC_log("%s %s", kcc.errmsg1, kcc.errmsg2);

		dhtOk = false;
	}
	else
	{
		dhtOk = true;
	}

	pkcc = &kcc;

	dataMtx.unlock(); /* UNLOCK MUTEX */
	return 1;
}

int dhthandler::shutdown()
{
	dataMtx.lock(); /* LOCK MUTEX */
	/* end the dht server. */
	kcs = KadC_stop(&kcc);
	if(kcs != KADC_OK) {
		KadC_log("KadC_stop(&kcc) returned error %d:\n", kcc.s);
		KadC_log("%s %s", kcc.errmsg1, kcc.errmsg2);
	}

	KadC_list_outstanding_mallocs(10);

	dataMtx.unlock(); /* UNLOCK MUTEX */
	return 0;
}


int dhthandler::write_inifile()
{
	/* if we're up and we have enough valid ones */

#define MIN_KONTACTS 50

	if (KadC_getncontacts(pkcc) > MIN_KONTACTS)
	{
		std::cerr << "DhtHandler::Write_IniFile() Writing File" << std::endl;
		if (KADC_OK != KadC_write_inifile(pkcc, NULL))
		{
		KadC_log("KadC_write_inifile(%s, %d) returned error %d:\n",
			kadcFile.c_str(), 1, kcc.s);
		KadC_log("%s %s", kcc.errmsg1, kcc.errmsg2);
		}
	}
	else
	{
		std::cerr << "DhtHandler::Write_IniFile() Not enough contacts" << std::endl;
	}
	return 1;
}


void dhthandler::run()
{

	/* infinite loop */
	int totalsleep = 0;
	while(1)
	{
	//	std::cerr << "DhtHandler::Run()" << std::endl;

		if (!dhtOk)
		{
			std::cerr << "DhtHandler::Run() Failed to Start" << std::endl;

/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
			sleep(1);
#else

			Sleep(1000);
#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/

			continue;
		}

		/* lock it up */
		dataMtx.lock(); /* LOCK MUTEX */

		bool toShutdown = mShutdown;

		/* shutdown */
		dataMtx.unlock(); /* UNLOCK MUTEX */


		print();

		if (toShutdown)
		{
			shutdown();
			dhtOk = false;
		}


		/* check ids */

		int allowedSleep = checkOwnStatus();
		int nextPeerCheck = checkPeerIds();
		if (nextPeerCheck < allowedSleep)
		{
			allowedSleep = nextPeerCheck;
		}
		if (allowedSleep > 10)
		{
			allowedSleep = 10;
		}
		else if (allowedSleep < 10)
		{
			allowedSleep = 10;
		}
	//	std::cerr << "DhtHandler::Run() sleeping for:" << allowedSleep << std::endl;
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
		sleep(allowedSleep); 
#else
		Sleep(1000 * allowedSleep); 
#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/

#define DHT_INIT_STORE_PERIOD	 300

		totalsleep += allowedSleep;
		if (totalsleep > DHT_INIT_STORE_PERIOD)
		{
			write_inifile();
			totalsleep = 0;
		}


	}
	return;
}

int dhthandler::print()
{
	dataMtx.lock(); /* LOCK MUTEX */

	std::cerr << "DHT Status:" << std::endl;
	std::cerr << "KNodes: " << KadC_getnknodes(pkcc);
	std::cerr << std::endl;
	std::cerr << "Kontacts: " << KadC_getncontacts(pkcc);
	std::cerr << std::endl;
	std::cerr << "KBuckets: ";
	std::cerr << std::endl;
	KadC_listkbuckets(pkcc);
	std::cerr << std::endl;
	std::cerr << "Own DHT:" << std::endl;
	std::cerr << ownId << std::endl;

	std::cerr << addrs.size() << " Peers:" << std::endl;

	std::map<std::string, dhtentry>::iterator it;
	for(it = addrs.begin(); it != addrs.end(); it++)
	{
		std::cerr << "Peer DHT" << std::endl;
		std::cerr << it -> second << std::endl;
	}

	dataMtx.unlock(); /* UNLOCK MUTEX */
	return 1;
}


int dhthandler::checkOwnStatus()
{
	dataMtx.lock(); /* LOCK MUTEX */

	int nextcall = DHT_REPUBLISH_PERIOD;
	bool toPublish = false;

	/* if we are publishing, and time up ... republish */
	if (ownId.status == DHT_UNKNOWN)
	{
		/* if valid Hash and Port */
		if ((ownId.id != "") && (ownId.addr.sin_port != 0) &&
		   (networkUp())) /* network is up */
		{
			unsigned long int extip = KadC_getextIP(pkcc);
			ownId.flags = KadC_getfwstatus(pkcc);
			if (extip != 0)
			{
				ownId.addr.sin_addr.s_addr = htonl(extip);
				toPublish = true;
			}
		}
		nextcall = DHT_MIN_PERIOD;
	}
	else /* ownId.status == DHT_PUBLISHING */
	{
		/* check time.
		 */
		if (ownId.lastTs + DHT_REPUBLISH_PERIOD < time(NULL))
		{
			toPublish = true;
		}
	}

	dataMtx.unlock(); /* UNLOCK MUTEX */

	if (toPublish)
	{
		publishOwnId();
	}

	return nextcall;
}

int dhthandler::checkPeerIds()
{
	dataMtx.lock(); /* LOCK MUTEX */
	/* if we are unknown .... wait */
	int nextcall = DHT_REPUBLISH_PERIOD;
	std::map<std::string, dhtentry>::iterator it;

	/* local list */
	std::list<std::string> idsToUpdate;
	std::list<std::string>::iterator it2;

	for(it = addrs.begin(); it != addrs.end(); it++)
	{
		/* if we are publishing, and time up ... republish */
		if (it -> second.status == DHT_UNKNOWN)
		{
			/* startup */
			idsToUpdate.push_back(it->first);
		}
		else if (it -> second.status == DHT_SEARCHING)
		{
			/* check if time */
			if (it -> second.lastTs + DHT_SEARCH_PERIOD < time(NULL))
			{
				idsToUpdate.push_back(it->first);
			}
			nextcall = DHT_SEARCH_PERIOD;
		}
		else if (it -> second.status == DHT_FOUND)
		{
			/* check if time */
			if (it -> second.lastTs + DHT_REPUBLISH_PERIOD < time(NULL))
			{
				idsToUpdate.push_back(it->first);
			}
		}
	}

	dataMtx.unlock(); /* UNLOCK MUTEX */

	for(it2 = idsToUpdate.begin(); it2 != idsToUpdate.end(); it2++)
	{
		searchId(*it2);
	}

	return nextcall;
}

/* already locked */
int dhthandler::publishOwnId()
{
	dataMtx.lock(); /* LOCK MUTEX */
	/* publish command */
	/* publish {#[khash]|key} {#[vhash]|value} [meta-list [nthreads [nsecs]]] */
	char index[1024];
	sprintf(index, "#%.1023s", ownId.id.c_str());
	char value[1024];
	sprintf(value, "#%.1023s", ownId.id.c_str());

	/* to store the ip address and flags */
	char metalist[1024];
	std::string addr = rs_inet_ntoa(ownId.addr.sin_addr),
	sprintf(metalist, "rsid=%s:%d;flags=%04X;", 
		addr.c_str(),
		ntohs(ownId.addr.sin_port), 
		ownId.flags);

	dataMtx.unlock(); /* UNLOCK MUTEX */


	int nthreads = 10;
	int duration = 15;
	int status;

	/* might as well hash back to us? */
	status = KadC_republish(pkcc, index, value, metalist, nthreads, duration);
	if(status == 1) 
	{
		KadC_log("Syntax error preparing search. Try: p key #hash [tagname=tagvalue[;...]]\n");
	}

	dataMtx.lock(); /* LOCK MUTEX */

	/* update entry */
	initdhtentry(&ownId);

	dataMtx.unlock(); /* UNLOCK MUTEX */

	return 1;
}


/* must be protected by mutex externally */
dhtentry *dhthandler::finddht(std::string id)
{
	std::map<std::string, dhtentry>::iterator it;
	it = addrs.find(id);
	if (it == addrs.end())
	{
		return NULL;
	}
	return &(it->second);
}

int dhthandler::searchId(std::string id)
{
	if (!networkUp())
		return 0;

	/* ack search */
	bool updated = false;

	/* search */
	void *iter;
	KadCdictionary *pkd;
	char *filter = "";
	int nthreads = 10;
	int duration = 15;
	int maxhits = 100;
	time_t starttime = time(NULL);
	void *resdictrbt;
	int nhits;

	char index[1024];
	sprintf(index, "#%.1023s", id.c_str());

	/* cannot be holding mutex here... (up to 15 secs) */
	resdictrbt = KadC_find(pkcc, index, filter, nthreads, maxhits, duration);

	nhits = rbt_size(resdictrbt);

	/* list each KadCdictionary returned in the rbt */
	for(iter = rbt_begin(resdictrbt); iter != NULL; iter = rbt_next(resdictrbt, iter)) {
		pkd = rbt_value(iter);

		KadC_log("Found: ");
		KadC_int128flog(stdout, KadCdictionary_gethash(pkd));
		KadC_log("\n");
		KadCdictionary_dump(pkd);
		KadC_log("\n");

        	KadCtag_iter iter;
	        unsigned int i;

		bool found = false;
		std::string addrline;
		std::string flagsline;
		for(i = 0, KadCtag_begin(pkd, &iter); (i < iter.tagsleft); i++, KadCtag_next(&iter)) {
		        if(i > 0)
				KadC_log(";");
			if ((strncmp("rsid", iter.tagname, 4) == 0) 
				&& (iter.tagtype == KADCTAG_STRING))
			{
				KadC_log("DECODING:%s", (char *)iter.tagvalue);
				addrline = (char *) iter.tagvalue;
				found = true;
			}
			if ((strncmp("flags", iter.tagname, 5) == 0) 
				&& (iter.tagtype == KADCTAG_STRING))
			{
				KadC_log("DECODING:%s", (char *)iter.tagvalue);
				flagsline = (char *) iter.tagvalue;
			}
		}
	
		/* must parse:rsid=ddd.ddd.ddd.ddd:dddd;flags=xxxx */
		struct sockaddr_in addr;
		unsigned int flags = 0;
		unsigned int a, b, c, d, e;
		if ((found) && 
		   (5 == sscanf(addrline.c_str(), "%d.%d.%d.%d:%d", &a, &b, &c, &d, &e)))
		{
			std::ostringstream out;
			out << a << "." << b << "." << c << "." << d;
			inet_aton(out.str().c_str(), &(addr.sin_addr));
			addr.sin_port = htons(e);

			if (flagsline != "") 
				sscanf(flagsline.c_str(), "%x", &flags);

			std::cerr << "Decoded entry: " << out.str() << " : " << e << std::endl;


			dataMtx.lock(); /* LOCK MUTEX */

			dhtentry *ent = finddht(id);
			if (ent)
			{
				founddhtentry(ent, addr, flags);
				updated = true;
			}
	
			dataMtx.unlock(); /* UNLOCK MUTEX */

		}
		else
		{
			std::cerr << "Failed to Scan:" << addrline << " <-----" << std::endl;
		}
			
	}
	KadC_log("Search completed in %d seconds - %d hit%s returned\n",
		time(NULL)-starttime, nhits, (nhits == 1 ? "" : "s"));

	for(iter = rbt_begin(resdictrbt); iter != NULL; iter = rbt_begin(resdictrbt)) {
		pkd = rbt_value(iter);
		rbt_erase(resdictrbt, iter);
		KadCdictionary_destroy(pkd);
	}
	rbt_destroy(resdictrbt);

	dataMtx.lock(); /* LOCK MUTEX */
	if (!updated)
	{
		dhtentry *ent = finddht(id);
		if (ent)
		{
			initdhtentry(ent);
		}
	}
	dataMtx.unlock(); /* UNLOCK MUTEX */

	return 1;
}

#if (0)

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include <int128.h>
#include <rbt.h>
#include <KadCalloc.h>
#include <KadClog.h>
#include <config.h>
#include <queue.h>	/* required by net.h, sigh... */
#include <net.h> /* only for domain2hip() */

#include <KadCapi.h>

#endif

