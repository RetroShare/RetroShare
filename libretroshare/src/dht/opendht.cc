/*
 * libretroshare/src/dht: opendht.cc
 *
 * Interface with OpenDHT for RetroShare.
 *
 * Copyright 2007-2008 by Robert Fernie.
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

#include "dht/opendht.h"
#include "dht/opendhtstr.h"
#include "dht/b64.h"
#include <fstream>
#include <sstream>

#include <sys/time.h>
#include <time.h>
#include "util/rsnet.h"
#include "util/rsprint.h"

const std::string openDHT_Client = "Retroshare V0.4";
const std::string openDHT_Agent  = "RS-HTTP-V0.4";

#define MAX_DHT_PEER_FAILS 	2  /* then discard */
#define MAX_DHT_TOTAL_FAILS 	10 /* in a row -> think we're not connected! */
#define MAX_DHT_ATTEMPTS	10 /* attempts per search/publish */
#define MIN_DHT_SERVERS		5

/****
 * #define OPENDHT_DEBUG		1
 ****/

bool OpenDHTClient::checkServerFile(std::string filename)
{
#ifdef	OPENDHT_DEBUG
	std::cerr << "OpenDHTClient::checkServerFile(" << filename << ")" << std::endl;
#endif

	/* open the file */
	std::ifstream file(filename.c_str());

	if (file.fail())
	{
#ifdef	OPENDHT_DEBUG
		std::cerr << "OpenDHTClient::checkServerFile() Open Failed" << std::endl;
#endif
		return false;
	}

	/* get the first line */
	std::string line;
	getline(file, line);
	char day[16], month[16];
	int  date;
	char day2[16], month2[16];
	int  date2;

	if (3 != sscanf(line.c_str(), "%15s %15s %d", day, month, &date))
	{
#ifdef	OPENDHT_DEBUG
		std::cerr << "OpenDHTClient::checkServerFile() failed file TS parse";
		std::cerr << std::endl;
#endif
		return false;
	}

#ifdef	OPENDHT_DEBUG
	std::cerr << "OpenDHTClient::checkServerFile() file TS month: ";
	std::cerr << month << " date: " << date << std::endl;
#endif

	/* store current timestamp */
	struct tm result;
	time_t now = time(NULL);
	char   nowstr[1023];

	asctime_r(gmtime_r(&now, &result), nowstr);

	if (3 != sscanf(nowstr, "%15s %15s %d", day2, month2, &date2))
	{
#ifdef	OPENDHT_DEBUG
		std::cerr << "OpenDHTClient::checkServerFile() failed now TS parse";
		std::cerr << std::endl;
#endif
		return false;
	}

#ifdef	OPENDHT_DEBUG
	std::cerr << "OpenDHTClient::checkServerFile() current TS month: ";
	std::cerr << month2 << " date: " << date2 << std::endl;
#endif

	/* if month is different */
	if (0 != strcmp(month, month2))
	{
#ifdef	OPENDHT_DEBUG
		std::cerr << "OpenDHTClient::checkServerFile() different MONTHS fail";
		std::cerr << std::endl;
#endif
		return false;
	}

	/* if month is different */
	int delta = abs(date-date2);
	if (delta > 2)
	{
#ifdef	OPENDHT_DEBUG
		std::cerr << "OpenDHTClient::checkServerFile() fail - large DATE diff: " << delta;
		std::cerr << std::endl;
#endif
		return false;
	}

#ifdef	OPENDHT_DEBUG
	std::cerr << "OpenDHTClient::checkServerFile() file is up-to-date!";
	std::cerr << std::endl;
#endif

	return true;
}

	

bool OpenDHTClient::loadServers(std::string filename)
{
	/* open the file */
	std::ifstream file(filename.c_str());


	return loadServers(file);
}

bool OpenDHTClient::loadServers(std::istream &instr)
{

	std::string line;
	char number[1024];
	char ipaddr[1024];
	char dnsname[1024];

	dhtMutex.lock();   /****  LOCK  ****/
	mServers.clear();
	dhtMutex.unlock(); /**** UNLOCK ****/

	/* chew first line */
	instr.ignore(1024, '\n');

	while((!instr.eof()) && (!instr.fail()))
	{
		line = "";
		getline(instr, line);

		if (3 == sscanf(line.c_str(), "%1023s %1023s %1023s", number, ipaddr, dnsname))
		{
			dhtServer srv;
			srv.host = dnsname;
			srv.port = 5851;
			srv.failed = 0;
			srv.ts = 0;
			srv.addr.sin_addr.s_addr = 0;
			srv.addr.sin_port = 0;

#ifdef	OPENDHT_DEBUG
			std::cerr << "Read Server: " << dnsname << std::endl;
#endif

			dhtMutex.lock();   /****  LOCK  ****/
			mServers[dnsname] = srv;
			dhtMutex.unlock(); /**** UNLOCK ****/

		}
		else
		{

#ifdef	OPENDHT_DEBUG
			std::cerr << "Failed to Read Server" << std::endl;
#endif
		}

		dhtMutex.lock();   /****  LOCK  ****/
		mDHTFailCount = 0;
		dhtMutex.unlock(); /**** UNLOCK ****/
	}

	dhtMutex.lock();   /****  LOCK  ****/
	uint32_t count = mServers.size();
	dhtMutex.unlock(); /**** UNLOCK ****/

	return (count >= MIN_DHT_SERVERS);
}

/******* refresh Servers from WebPage ******/

bool OpenDHTClient::loadServersFromWeb(std::string storename)
{

#ifdef	OPENDHT_DEBUG
	std::cerr << "OpenDHTClient::loadServersFromWeb()" << std::endl;
#endif

	std::string response;
	if (!openDHT_getDHTList(response))
	{
#ifdef	OPENDHT_DEBUG
		std::cerr << "OpenDHTClient::loadServersFromWeb() Web GET failed" << std::endl;
#endif
		return false;
	}

	std::string::size_type i;
	if (std::string::npos == (i = response.find("\r\n\r\n")))
	{
#ifdef	OPENDHT_DEBUG
		std::cerr << "OpenDHTClient::loadServersFromWeb() Failed to Find Content" << std::endl;
#endif
		return false;
	}

	/* now step past 4 chars */
	i += 4;

	std::string content(response, i, response.length() - i);

#ifdef	OPENDHT_DEBUG
	std::cerr << "OpenDHTClient::loadServersFromWeb() Content:" << std::endl;
	std::cerr << content << std::endl;
	std::cerr << "<== OpenDHTClient::loadServersFromWeb() Content" << std::endl;
#endif

	std::istringstream iss(content);	

	if (loadServers(iss))
	{
#ifdef	OPENDHT_DEBUG
		std::cerr << "OpenDHTClient::loadServersFromWeb() Saving WebData to: ";
		std::cerr << storename << std::endl;
#endif
		/* save the data to file - replacing old data */
		std::ofstream ofstr(storename.c_str());
		ofstr << content;
		ofstr.close();

		return true;
	}

	return false;

}


bool OpenDHTClient::getServer(std::string &host, uint16_t &port, struct sockaddr_in &addr)
{
	/* randomly choose one */
	dhtMutex.lock();   /****  LOCK  ****/

#ifndef WINDOWS_SYS
#else

	/* WINDOWS don't randomise properly so we'll do it ourselves...
         */

        uint32_t randomize = timeGetTime();
        srand(randomize);
#endif

	uint32_t len = mServers.size();
	uint32_t rnd = len * (rand() / (RAND_MAX + 1.0));

	if (len < 1)
	{
#ifdef	OPENDHT_DEBUG
		std::cerr << "OpenDHTClient::getServer() No Servers available!" << std::endl;
#endif
		dhtMutex.unlock(); /**** UNLOCK ****/
		return false;
	}
		
	std::map<std::string, dhtServer>::const_iterator it;
	uint32_t i = 0;
	for(it = mServers.begin(); (it != mServers.end()) && (i < rnd); it++, i++);

	if (it == mServers.end())
	{
#ifdef	OPENDHT_DEBUG
		std::cerr << "OpenDHTClient::getServer() Error getting Server!" << std::endl;
#endif
		dhtMutex.unlock(); /**** UNLOCK ****/
		return false;
	}

	host = (it->second).host;
	port = (it->second).port;

	time_t now = time(NULL);

	if (now - (it->second).ts < 3600)
	{
		addr = (it->second).addr;
	}
	else
	{
		addr.sin_addr.s_addr = 0;
	}

	dhtMutex.unlock(); /**** UNLOCK ****/

	return true;
}


bool OpenDHTClient::setServerIp(std::string host, struct sockaddr_in addr)
{
	dhtMutex.lock();   /****  LOCK  ****/

	std::map<std::string, dhtServer>::iterator it;
	it = mServers.find(host);
	if (it == mServers.end())
	{
		dhtMutex.unlock(); /**** UNLOCK ****/
#ifdef	OPENDHT_DEBUG
		std::cerr << "OpenDHTClient::setServerIp() Error finding Server!" << std::endl;
#endif
		return false;
	}

	(it -> second).addr = addr;
	(it -> second).ts = time(NULL);
	(it -> second).failed = 0;

	mDHTFailCount = 0;

	dhtMutex.unlock(); /**** UNLOCK ****/

	return true;
}


void OpenDHTClient::setServerFailed(std::string host)
{
	dhtMutex.lock();   /****  LOCK  ****/

	std::map<std::string, dhtServer>::iterator it;
	it = mServers.find(host);
	if (it == mServers.end())
	{
#ifdef	OPENDHT_DEBUG
		std::cerr << "OpenDHTClient::setServerFailed() Error finding Server!" << std::endl;
#endif
		dhtMutex.unlock(); /**** UNLOCK ****/
		return;
	}

	mDHTFailCount++;
	if (mDHTFailCount > MAX_DHT_TOTAL_FAILS) /* might be not connected to Internet */
	{
#ifdef	OPENDHT_DEBUG
		std::cerr << "OpenDHTClient::setServerFailed() Probably not connected!" << std::endl;
#endif
		dhtMutex.unlock(); /**** UNLOCK ****/
		return;
	}

	/* up the fail count on this one */
	(it -> second).failed++;

	if ((it -> second).failed > MAX_DHT_PEER_FAILS)
	{
#ifdef	OPENDHT_DEBUG
		std::cerr << "OpenDHTClient::setServerFailed() fail count high -> removing: ";
		std::cerr << host << " from list" << std::endl;
#endif
		/* remove from list */
		mServers.erase(it);
	}

	dhtMutex.unlock(); /**** UNLOCK ****/
	return;
}

bool OpenDHTClient::dhtActive()
{
	dhtMutex.lock();   /****  LOCK  ****/

	bool ok = (mDHTFailCount <= MAX_DHT_TOTAL_FAILS) &&
		  (mServers.size() > MIN_DHT_SERVERS);

	dhtMutex.unlock(); /**** UNLOCK ****/

	return ok;
}

bool OpenDHTClient::publishKey(std::string key, std::string value, uint32_t ttl)
{
	/* create request */
#ifdef	OPENDHT_DEBUG
	std::cerr << "OpenDHTClient::openDHT_publishKey() key: 0x" << RsUtil::BinToHex(key) << " value: 0x" << RsUtil::BinToHex(value);
	std::cerr << std::endl;
#endif
        std::string putmsg = createOpenDHT_put(key, value, ttl, openDHT_Client);
	std::string response;

	for(uint16_t i = 0; (!openDHT_sendMessage(putmsg, response)); i++)
	{
		if (i > MAX_DHT_ATTEMPTS)
		{
#ifdef	OPENDHT_DEBUG
			std::cerr << "OpenDHTClient::openDHT_publishKey() Failed -> Giving Up";
			std::cerr << std::endl;
#endif
			return false;
		}

#ifdef	OPENDHT_DEBUG
		std::cerr << "OpenDHTClient::openDHT_publishKey() Failed -> reattempting";
		std::cerr << std::endl;
#endif
	}

	/* check response */
	return true;
}


bool OpenDHTClient::searchKey(std::string key, std::list<std::string> &values)
{
#ifdef	OPENDHT_DEBUG
	std::cerr << "OpenDHTClient::openDHT_searchKey() key: 0x" << RsUtil::BinToHex(key);
	std::cerr << std::endl;
#endif

	/* create request */
        std::string getmsg = createOpenDHT_get(key, 1024, openDHT_Client);
	std::string response;

	for(uint16_t i = 0; (!openDHT_sendMessage(getmsg, response)); i++)
	{
		if (i > MAX_DHT_ATTEMPTS)
		{
#ifdef	OPENDHT_DEBUG
			std::cerr << "OpenDHTClient::openDHT_searchKey() Failed -> Giving Up";
			std::cerr << std::endl;
#endif
			return false;
		}
#ifdef	OPENDHT_DEBUG
		std::cerr << "OpenDHTClient::openDHT_searchKey() Failed -> reattempting";
		std::cerr << std::endl;
#endif
	}

#ifdef	OPENDHT_DEBUG
		std::cerr << "Parsing expression :$$$$$$$$$-" << response << "-$$$$$$$$" << std::endl ;
#endif

	/* search through the response for <base64> ... </base64> */

	std::string::size_type start = 0;
	std::string::size_type loc = 0;
	std::string::size_type end = 0;

	while(1)
	{
		loc = response.find("<base64>", start);
		if (loc == std::string::npos)
			return true; /* finished */
#ifdef	OPENDHT_DEBUG
		std::cerr << "found loc=" << loc << std::endl ;
#endif
		loc += 8; /* shift to end of <base64> */

		end = response.find("</base64>", loc);
		if (end == std::string::npos)
			return true; /* finished */
#ifdef	OPENDHT_DEBUG
		std::cerr << "found end=" << end << std::endl ;
#endif

		std::string value = response.substr(loc, end-loc);
#ifdef	OPENDHT_DEBUG
		std::cerr << "found value=" << value << std::endl ;
#endif

		/* clear out whitespace */
		for(std::string::size_type i = 0; i < value.length();)
		{
			if (isspace(value[i]))
			{
				value.erase(i,1);
#ifdef	OPENDHT_DEBUG
				std::cerr << "Cleanup Result:" << value << ":END:" << std::endl;
#endif
			}
			else
			{
				i++;
			}
		}

		if (value.length() > 0)
		{
			std::string result = convertFromBase64(value);
			values.push_back(result);
#ifdef	OPENDHT_DEBUG
			std::cerr << "openDHT_searchKey() Value:" << value << ":END:" << std::endl;
			std::cerr << "openDHT_searchKey() Result: 0x" << RsUtil::BinToHex(result) << ":END:" << std::endl;
#endif
		}

		/* the answer should be between loc and end */
		start = end + 9;
	}

	/* parse response */
	return true;
}


bool OpenDHTClient::openDHT_sendMessage(std::string msg, std::string &response)
{
	struct sockaddr_in addr;
	std::string host; 
	uint16_t    port;

	if (!getServer(host, port, addr))
	{
#ifdef	OPENDHT_DEBUG
		std::cerr << "OpenDHTClient::openDHT_sendMessage() Failed to get Server";
		std::cerr << std::endl;
#endif
		return false;
	}

	if (addr.sin_addr.s_addr == 0)
	{
		/* lookup the address */
		addr.sin_port = htons(port);
		if (LookupDNSAddr(host, addr) &&
			(addr.sin_addr.s_addr != 0))
		{
			/* update the IP addr if necessary */
			setServerIp(host, addr);
		}
		else
		{
			/* no address */
#ifdef	OPENDHT_DEBUG
			std::cerr << "OpenDHTClient::openDHT_sendMessage()";
			std::cerr << " ERROR: No Address";
			std::cerr << std::endl;
#endif
			setServerFailed(host);

			return false;
		}
	}

#ifdef	OPENDHT_DEBUG
	std::cerr << "OpenDHTClient::openDHT_sendMessage()";
	std::cerr << " Connecting to:" << host << ":" << port;
	std::cerr << " (" << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port) << ")";
	std::cerr << std::endl;
#endif

	/* create request */
	std::string putheader = createHttpHeader(host, port, openDHT_Agent, msg.length());

	/* open a socket */
        int sockfd = unix_socket(PF_INET, SOCK_STREAM, 0);

	/* connect */
        int err = unix_connect(sockfd, (struct sockaddr *) &addr, sizeof(addr));
	if (err)
	{
		unix_close(sockfd);
#ifdef	OPENDHT_DEBUG
		std::cerr << "OpenDHTClient::openDHT_sendMessage()";
		std::cerr << " ERROR: Failed to Connect";
		std::cerr << std::endl;
#endif

		setServerFailed(host);

		return false;
	}

#ifdef	OPENDHT_DEBUG
	std::cerr << "HTTP message *******************" << std::endl;
	std::cerr << putheader;
	std::cerr << msg;
	std::cerr << std::endl;
	std::cerr << "HTTP message *******************" << std::endl;
#endif

	/* send data */
	int sendsize = strlen(putheader.c_str());
	int size = send(sockfd, putheader.c_str(), sendsize, 0);
	if (sendsize != size)
	{
		unix_close(sockfd);
#ifdef	OPENDHT_DEBUG
		std::cerr << "OpenDHTClient::openDHT_sendMessage()";
		std::cerr << " ERROR: Failed to Send(1)";
		std::cerr << std::endl;
#endif

		setServerFailed(host);

		return false;
	}
#ifdef	OPENDHT_DEBUG
	std::cerr << "OpenDHTClient::openDHT_sendMessage()";
	std::cerr << " Send(1):" << size;
	std::cerr << std::endl;
#endif

	sendsize = strlen(msg.c_str());
	size = send(sockfd, msg.c_str(), sendsize, 0);
	if (sendsize != size)
	{
		unix_close(sockfd);
#ifdef	OPENDHT_DEBUG
		std::cerr << "OpenDHTClient::openDHT_sendMessage()";
		std::cerr << " ERROR: Failed to Send(2)";
		std::cerr << std::endl;
#endif

		setServerFailed(host);

		return false;
	}

#ifdef	OPENDHT_DEBUG
	std::cerr << "OpenDHTClient::openDHT_sendMessage()";
	std::cerr << " Send(2):" << size;
	std::cerr << std::endl;
#endif

	/* now wait for the response */
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
	sleep(1);
#else
	Sleep(1000);
#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/

	int recvsize = 51200; /* 50kb */
	char *inbuf = (char *) malloc(recvsize);
	uint32_t idx = 0;
	while(0 < (size = recv(sockfd, &(inbuf[idx]), recvsize - idx, 0)))
	{
#ifdef	OPENDHT_DEBUG
		std::cerr << "OpenDHTClient::openDHT_sendMessage()";
		std::cerr << " Recvd Chunk:" << size;
		std::cerr << std::endl;
#endif
		idx += size;
	}

#ifdef	OPENDHT_DEBUG
	std::cerr << "OpenDHTClient::openDHT_sendMessage()";
	std::cerr << " Recvd Msg:" << idx;
#endif

	response = std::string(inbuf, idx);
	free(inbuf);

	/* print it out */
#ifdef	OPENDHT_DEBUG
	std::cerr << "HTTP What We Sent ***************" << std::endl;
	std::cerr << putheader;
	std::cerr << msg;
	std::cerr << std::endl;
	std::cerr << "HTTP response *******************" << std::endl;
	std::cerr << response;
	std::cerr << std::endl;
	std::cerr << "HTTP response *******************" << std::endl;
#endif

	unix_close(sockfd);

	return true;
}

bool OpenDHTClient::openDHT_getDHTList(std::string &response)
{
	struct sockaddr_in addr;
	std::string host = "www.opendht.org"; 
	uint16_t    port = 80;

	sockaddr_clear(&addr);

		/* lookup the address */
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	if (!LookupDNSAddr(host, addr))
	{
		/* no address */
#ifdef	OPENDHT_DEBUG
		std::cerr << "OpenDHTClient::openDHT_getDHTList()";
		std::cerr << " ERROR: No Address";
		std::cerr << std::endl;
#endif

		return false;
	}

#ifdef	OPENDHT_DEBUG
	std::cerr << "OpenDHTClient::openDHT_getDHTList()";
	std::cerr << " Connecting to:" << host << ":" << port;
	std::cerr << " (" << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port) << ")";
	std::cerr << std::endl;
#endif

	/* create request */
	std::string putheader = createHttpHeaderGET(host, port, "servers.txt", openDHT_Agent, 0);

	/* open a socket */
        int sockfd = unix_socket(PF_INET, SOCK_STREAM, 0);

	/* connect */
        int err = unix_connect(sockfd, (struct sockaddr *) &addr, sizeof(addr));
	if (err)
	{
		unix_close(sockfd);
#ifdef	OPENDHT_DEBUG
		std::cerr << "OpenDHTClient::openDHT_getDHTList()";
		std::cerr << " ERROR: Failed to Connect";
		std::cerr << std::endl;
#endif

		return false;
	}

#ifdef	OPENDHT_DEBUG
	std::cerr << "HTTP message *******************" << std::endl;
	std::cerr << putheader;
	std::cerr << std::endl;
	std::cerr << "HTTP message *******************" << std::endl;
#endif

	/* send data */
	int sendsize = strlen(putheader.c_str());
	int size = send(sockfd, putheader.c_str(), sendsize, 0);
	if (sendsize != size)
	{
		unix_close(sockfd);
#ifdef	OPENDHT_DEBUG
		std::cerr << "OpenDHTClient::openDHT_getDHTList()";
		std::cerr << " ERROR: Failed to Send(1)";
		std::cerr << std::endl;
#endif

		return false;
	}
#ifdef	OPENDHT_DEBUG
	std::cerr << "OpenDHTClient::openDHT_getDHTList()";
	std::cerr << " Send(1):" << size;
	std::cerr << std::endl;
#endif

	/* now wait for the response */
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
	sleep(1);
#else
	Sleep(1000);
#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/

	int recvsize = 51200; /* 50kb */
	char *inbuf = (char *) malloc(recvsize);
	uint32_t idx = 0;
	while(0 < (size = recv(sockfd, &(inbuf[idx]), recvsize - idx, 0)))
	{
#ifdef	OPENDHT_DEBUG
		std::cerr << "OpenDHTClient::openDHT_getDHTList()";
		std::cerr << " Recvd Chunk:" << size;
		std::cerr << std::endl;
#endif
		idx += size;
	}

#ifdef	OPENDHT_DEBUG
	std::cerr << "OpenDHTClient::openDHT_getDHTList()";
	std::cerr << " Recvd Msg:" << idx;
#endif

	response = std::string(inbuf, idx);
	free(inbuf);

	/* print it out */
#ifdef	OPENDHT_DEBUG
	std::cerr << "HTTP response *******************" << std::endl;
	std::cerr << response;
	std::cerr << std::endl;
	std::cerr << "HTTP response *******************" << std::endl;
#endif

	unix_close(sockfd);

	return true;
}


