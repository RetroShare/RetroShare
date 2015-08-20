#include "ntpfinder.h"

#include "pqi/pqinetwork.h"
#include "util/rsstring.h"
#include <retroshare/rspeers.h>

#ifndef WIN32
#include <netdb.h>
#endif

#include <string.h>
#include <string>
#include <iostream>
#include <set>
#include <vector>
#include <algorithm>
#include <stdio.h>
#include <time.h>

const uint32_t MAX_NTP_STORE =	300; /* seconds NTP timeout */

//#define NTPSEARCH_DEBUG

static const std::string NTP_AGENT  = "Mozilla/5.0";

static time_t scan_date(const std::string& sText)
{
	time_t ttTime = 0;

	if(!sText.empty()){
		std::string sToFind = std::string("Date:");
		std::size_t lFound = sText.find(sToFind);
		std::string sDate = "";

		if(lFound>0){
			sDate=sText.substr(lFound+sToFind.length(), sText.find("\n",lFound+1)-lFound-sToFind.length());
#ifdef NTPSEARCH_DEBUG
			std::cout << "NTPFinder:scan_date found " << sDate << std::endl ;
#endif
			struct tm *tmTime;
			char cMonth[4];
			int iDay,iMonth,iYear,iHour,iMinute,iSecond;
			//Date: Wed, 13 Nov 2013 22:36:41 GMT
			//                         %.Wed,  %13%Nov%2013...
			if (sscanf(sDate.c_str(), "%*[^0-9]%d %3s %d %d:%d:%d", &iDay, cMonth, &iYear, &iHour, &iMinute, &iSecond) == 6)
			{
				const char * acMonthEnNames[12]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
				int i=0;
				while( (i<12) && (strcmp(cMonth,acMonthEnNames[i]) != 0)  )
					i++;

				iMonth=i;

				time(&ttTime);// Current time in GMT
				long diff_secs = 0;
#ifndef __USE_MISC
				//http://stackoverflow.com/questions/761791/converting-between-local-times-and-gmt-utc-in-c-c
				//For windows who don't know timegm
				// Remember that localtime/gmtime overwrite same location
				time_t local_secs, gmt_secs;
				tmTime=localtime(&ttTime);
				local_secs=mktime(tmTime);
				tmTime=gmtime(&ttTime);
				gmt_secs=mktime(tmTime);
				diff_secs = long(local_secs - gmt_secs);
#endif
				tmTime=gmtime(&ttTime);
				tmTime->tm_year=iYear-1900;
				tmTime->tm_mon=iMonth;
				tmTime->tm_mday=iDay;
				tmTime->tm_hour=iHour;
				tmTime->tm_min=iMinute;
				tmTime->tm_sec=iSecond;
#ifdef __USE_MISC
				ttTime=timegm(tmTime);
#else
				ttTime=mktime(tmTime);
#endif
				ttTime+=diff_secs;

#ifdef NTPSEARCH_DEBUG
				std::cout << "NTPFinder:scan_date scan Hour=" << iHour << " Min=" << iMinute << " sec=" << iSecond << std::endl ;
#endif
			}
		}
	}
	return ttTime;
}

static void getPage(const std::string& server_name,std::string& page)
{
	page = "" ;
	int sockfd,n=0;                   // socket descriptor
	struct sockaddr_in serveur;       // server's parameters
	memset(&serveur.sin_zero, 0, sizeof(serveur.sin_zero));
	struct hostent *hostinfo=NULL;    // structure for storing the server's ip

	char buf[1024];
	char request[1024];
#ifdef NTPSEARCH_DEBUG
	std::cout << "NTPFinder: connecting to " << server_name << std::endl ;
#endif
	// socket creation

	sockfd = unix_socket(PF_INET,SOCK_STREAM,0);

	serveur.sin_family = AF_INET;

	// get server's ipv4 adress

	hostinfo = gethostbyname(server_name.c_str());

	if (hostinfo == NULL) /* host don't exist */
	{
		std::cerr << "NTPFinder: Unknown host " << server_name << std::endl;
		return ;
	}
	serveur.sin_addr = *(struct in_addr*) hostinfo->h_addr;
	serveur.sin_port = htons(80);

#ifdef NTPSEARCH_DEBUG
	printf("Connection attempt\n");
#endif

	if(unix_connect(sockfd,(struct sockaddr *)&serveur, sizeof(serveur)) == -1)
	{
		std::cerr << "NTPFinder: Connection error to " << server_name << std::endl ;
		return ;
	}
#ifdef NTPSEARCH_DEBUG
	std::cerr << "NTPFinder: Connection established to " << server_name << std::endl ;
#endif

	// send request
	sprintf( request,
	         /*"GET / HTTP/1.0\r\n"*/
	         "HEAD / HTTP/1.0\r\n"
	         "Host: %s:%d\r\n"
	         "Connection: Close\r\n"
	         "\r\n",
	         server_name.c_str(), 80);

	if(send(sockfd,request,strlen(request),0)== -1)
	{
		std::cerr << "NTPFinder: Could not send request to " << server_name << std::endl ;
		return ;
	}
	// receve

	while((n = recv(sockfd, buf, sizeof buf - 1, 0)) > 0)
	{
		buf[n] = '\0';
		page += std::string(buf,n) ;
	}
	// close the socket

	unix_close(sockfd);
#ifdef NTPSEARCH_DEBUG
	std::cerr << "NTPFinder: Got full page from " << server_name << std::endl ;
#endif
}


void* doNTPSearch(void *p)
{

	std::vector<time_t> res ;

	NTPFinder *ntpF = (NTPFinder*)p ;

	for(std::list<std::string>::const_iterator it(ntpF->_ntp_servers.begin());it!=ntpF->_ntp_servers.end();++it)
	{
		std::string page ;

		getPage(*it,page) ;
		time_t ntpTime = scan_date(page) ;

		if(ntpTime != 0)
			res.push_back(ntpTime) ;

#ifdef NTPSEARCH_DEBUG
		std::cout << "ntpTime found through " << *it << ": \"" << ntpTime << "\"" << std::endl ;
#endif
	}

	if(res.empty())
	{
		// thread safe copy results.
		//
		{
			RsStackMutex mtx(ntpF->_ntpMtx) ;
			*(ntpF->_ntpTime) = 0 ;
			*(ntpF->_found) = false ;
			*(ntpF->mFoundTS) = time(NULL) ;
			*(ntpF->_searching) = false ;
			*(ntpF->_error) = NTP_Error_NoResult;
		}
		pthread_exit(NULL);
		return NULL ;
	}

	sort(res.begin(),res.end()) ; // eliminates outliers.

	double diff=difftime(res.back(),res.front());
	if(diff>60)
	{
		std::cerr << "NTPFinder: The time between two server is too large." << std::endl ;
		// thread safe copy results.
		//
		{
			RsStackMutex mtx(ntpF->_ntpMtx) ;
			*(ntpF->_ntpTime) = 0;
			*(ntpF->_found) = false ;
			*(ntpF->mFoundTS) = time(NULL) ;
			*(ntpF->_searching) = false ;
			*(ntpF->_error) = NTP_Error_TimeBwSrvTL;
		}
		pthread_exit(NULL);
		return NULL ;
	}


	{
		RsStackMutex mtx(ntpF->_ntpMtx) ;
		*(ntpF->_ntpTime) = res.front();
		*(ntpF->_found) = true ;
		*(ntpF->mFoundTS) = time(NULL) ;
		*(ntpF->_searching) = false ;
		*(ntpF->_error) = 0;
	}

	pthread_exit(NULL);
	return NULL ;
}


void NTPFinder::start_request()
{
	void *data = (void *)this;
	pthread_t tid ;
	pthread_create(&tid, 0, &doNTPSearch, data);
	pthread_detach(tid); /* so memory is reclaimed in linux */
}

bool NTPFinder::hasValidNTPTime(time_t *ntpTime)
{
#ifdef NTPSEARCH_DEBUG
	std::cerr << "NTPFinder: Getting NTP Time." << std::endl ;
#endif
	time_t delta = 0;
	bool doRefresh=false;
	{
		RsStackMutex mut(_ntpMtx) ;
		//timeout the current ntpTime
		delta = time(NULL) - *mFoundTS;
		doRefresh=*_doRefresh;
		*_doRefresh = false ;
	}

	{
		RsStackMutex mut(_ntpMtx) ;
		if(*_found)
		{
#ifdef NTPSEARCH_DEBUG
			std::cerr << "NTPFinder: Has stored NTP Time: responding with this time." << std::endl ;
#endif
			*ntpTime = *_ntpTime + delta;
		}
	}
	if( ((uint32_t)delta > MAX_NTP_STORE) || doRefresh ){//launch a research
		if( _ntpMtx.trylock())
		{
			if(!*_searching)
			{
#ifdef NTPSEARCH_DEBUG
				std::cerr << "NTPFinder: No stored NTP Time: Initiating new search." << std::endl ;
#endif
				*_searching = true ;
				start_request() ;
			}
#ifdef NTPSEARCH_DEBUG
			else
				std::cerr << "NTPFinder: Already searching." << std::endl ;
#endif
			_ntpMtx.unlock();
		}
#ifdef NTPSEARCH_DEBUG
		else
			std::cerr << "NTPFinder: (Note) Could not acquire lock. Busy." << std::endl ;
#endif
	}

	RsStackMutex mut(_ntpMtx) ;
	return *_found ;
}

void NTPFinder::setNTPServersList(std::list<std::string>& ntp_servers)
{
	RsStackMutex mut(_ntpMtx) ;

	_ntp_servers = ntp_servers ;
}

void NTPFinder::getNTPServersList(std::list<std::string>& ntp_servers)
{
	RsStackMutex mut(_ntpMtx) ;

	ntp_servers = _ntp_servers ;
}

void NTPFinder::reset()
{
	RsStackMutex mut(_ntpMtx) ;

	*_found = false ;
	*_searching = false ;
	*mFoundTS = time(NULL) - MAX_NTP_STORE;
}

NTPFinder::~NTPFinder()
{
#ifdef NTPSEARCH_DEBUG
	std::cerr << "NTPFinder: Deleting NTPFinder." << std::endl ;
#endif
	RsStackMutex mut(_ntpMtx) ;

	delete _found ;
	delete _searching ;
	delete mFoundTS ;
	delete _doRefresh ;
	free (_ntpTime) ;
	delete _ntpTime ;
	delete _error ;
	delete _doRefresh ;
}

NTPFinder::NTPFinder() : _ntpMtx("NTPFinder")
{
#ifdef NTPSEARCH_DEBUG
	std::cerr << "NTPFinder: Creating new NTPFinder." << std::endl ;
#endif
	RsStackMutex mut(_ntpMtx) ;

	_found = new bool ;
	*_found = false ;

	_searching = new bool ;
	*_searching = false ;

	mFoundTS = new time_t;
	*mFoundTS = time(NULL) - MAX_NTP_STORE;

	_ntpTime = new time_t;
	*_ntpTime = 0;

	_error = new int;
	*_error = 0;

	_doRefresh = new bool;
	*_doRefresh = false;

}
