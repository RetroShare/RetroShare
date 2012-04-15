#include "extaddrfinder.h"

#include "pqi/pqinetwork.h"
#include "util/rsstring.h"

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

const uint32_t MAX_IP_STORE =	300; /* seconds ip address timeout */

//#define EXTADDRSEARCH_DEBUG

static const std::string ADDR_AGENT  = "Mozilla/5.0";

static std::string scan_ip(const std::string& text)
{
	std::set<unsigned char> digits ;
	digits.insert('0') ; digits.insert('3') ; digits.insert('6') ;
	digits.insert('1') ; digits.insert('4') ; digits.insert('7') ;
	digits.insert('2') ; digits.insert('5') ; digits.insert('8') ;
	digits.insert('9') ; 

	for(int i=0;i<(int)text.size();++i)
	{
		while(i < (int)text.size() && digits.find(text[i])==digits.end()) ++i ;

		if(i>=(int)text.size())
			return "" ;

		unsigned int a,b,c,d ;

		if(sscanf(text.c_str()+i,"%u.%u.%u.%u",&a,&b,&c,&d) != 4)
			continue ;

		if(a < 256 && b<256 && c<256 && d<256)
		{
			std::string s ;
			rs_sprintf(s, "%u.%u.%u.%u", a, b, c, d) ;
			return s;
		}
	}
	return "" ;
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
#ifdef EXTADDRSEARCH_DEBUG
	std::cout << "ExtAddrFinder: connecting to " << server_name << std::endl ;
#endif
	// socket creation

	sockfd = unix_socket(PF_INET,SOCK_STREAM,0);

	serveur.sin_family = AF_INET;

	// get server's ipv4 adress

	hostinfo = gethostbyname(server_name.c_str()); 

	if (hostinfo == NULL) /* l'hôte n'existe pas */
	{
		std::cerr << "ExtAddrFinder: Unknown host " << server_name << std::endl;
		return ;
	}
	serveur.sin_addr = *(struct in_addr*) hostinfo->h_addr; 
	serveur.sin_port = htons(80);

#ifdef EXTADDRSEARCH_DEBUG
	printf("Connexion attempt\n");
#endif

	if(unix_connect(sockfd,(struct sockaddr *)&serveur, sizeof(serveur)) == -1)
	{
		std::cerr << "ExtAddrFinder: Connexion error to " << server_name << std::endl ;
		return ;
	}
#ifdef EXTADDRSEARCH_DEBUG
	std::cerr << "ExtAddrFinder: Connexion established to " << server_name << std::endl ;
#endif

	// envoi 
	sprintf( request, 
			"GET / HTTP/1.0\r\n"
			"Host: %s:%d\r\n"
			"Connection: Close\r\n"
			"\r\n", 
			server_name.c_str(), 80);

	if(send(sockfd,request,strlen(request),0)== -1)
	{
		std::cerr << "ExtAddrFinder: Could not send request to " << server_name << std::endl ;
		return ;
	}
	// recéption 

	while((n = recv(sockfd, buf, sizeof buf - 1, 0)) > 0)
	{
		buf[n] = '\0';
		page += std::string(buf,n) ;
	}
	// fermeture de la socket

	unix_close(sockfd);
#ifdef EXTADDRSEARCH_DEBUG
	std::cerr << "ExtAddrFinder: Got full page from " << server_name << std::endl ;
#endif
}


void* doExtAddrSearch(void *p)
{
	
	std::vector<std::string> res ;

	ExtAddrFinder *af = (ExtAddrFinder*)p ;

	for(std::list<std::string>::const_iterator it(af->_ip_servers.begin());it!=af->_ip_servers.end();++it)
	{
		std::string page ;

		getPage(*it,page) ;
		std::string ip = scan_ip(page) ;

		if(ip != "")
			res.push_back(ip) ;
#ifdef EXTADDRSEARCH_DEBUG
		std::cout << "ip found through " << *it << ": \"" << ip << "\"" << std::endl ;
#endif
	}

	if(res.empty())
	{
		// thread safe copy results.
		//
		{
			RsStackMutex mtx(af->_addrMtx) ;

			*(af->_found) = false ;
			*(af->mFoundTS) = time(NULL) ;
			*(af->_searching) = false ;
		}
		pthread_exit(NULL);
		return NULL ;
	}

	sort(res.begin(),res.end()) ; // eliminates outliers.


	if(!inet_aton(res[res.size()/2].c_str(),af->_addr))
	{
		std::cerr << "ExtAddrFinder: Could not convert " << res[res.size()/2] << " into an address." << std::endl ;
		{
			RsStackMutex mtx(af->_addrMtx) ;
			*(af->_found) = false ;
			*(af->mFoundTS) = time(NULL) ;
			*(af->_searching) = false ;
		}
		pthread_exit(NULL);
		return NULL ;
	}

	{
		RsStackMutex mtx(af->_addrMtx) ;
		*(af->_found) = true ;
		*(af->mFoundTS) = time(NULL) ;
		*(af->_searching) = false ;
	}

	pthread_exit(NULL);
	return NULL ;
}


void ExtAddrFinder::start_request()
{
	void *data = (void *)this;
	pthread_t tid ;
	pthread_create(&tid, 0, &doExtAddrSearch, data);
	pthread_detach(tid); /* so memory is reclaimed in linux */
}

bool ExtAddrFinder::hasValidIP(struct in_addr *addr)
{
#ifdef EXTADDRSEARCH_DEBUG
	std::cerr << "ExtAddrFinder: Getting ip." << std::endl ;
#endif

	if(*_found)
	{
#ifdef EXTADDRSEARCH_DEBUG
		std::cerr << "ExtAddrFinder: Has stored ip: responding with this ip." << std::endl ;
#endif
		*addr = *_addr;
	}

	//timeout the current ip
	time_t delta = time(NULL) - *mFoundTS;
	if((uint32_t)delta > MAX_IP_STORE) {//launch a research
	    if( _addrMtx.trylock())
		    {
			    if(!*_searching)
			    {
	    #ifdef EXTADDRSEARCH_DEBUG
				    std::cerr << "ExtAddrFinder: No stored ip: Initiating new search." << std::endl ;
	    #endif
				    *_searching = true ;
				    start_request() ;
			    }
	    #ifdef EXTADDRSEARCH_DEBUG
			    else
				    std::cerr << "ExtAddrFinder: Already searching." << std::endl ;
	    #endif
			    _addrMtx.unlock();
		    }
	    #ifdef EXTADDRSEARCH_DEBUG
		    else
			    std::cerr << "ExtAddrFinder: (Note) Could not acquire lock. Busy." << std::endl ;
	    #endif
	}

	return *_found ;
}

void ExtAddrFinder::reset()
{
//	while(*_searching) 
//#ifdef WIN32
//		Sleep(1000) ;
//#else
//		sleep(1) ;
//#endif

	RsStackMutex mut(_addrMtx) ;

	*_found = false ;
	*_searching = false ;
	*mFoundTS = time(NULL) - MAX_IP_STORE;
}

ExtAddrFinder::~ExtAddrFinder()
{
#ifdef EXTADDRSEARCH_DEBUG
	std::cerr << "ExtAddrFinder: Deleting ExtAddrFinder." << std::endl ;
#endif
//	while(*_searching) 
//#ifdef WIN32
//		Sleep(1000) ;
//#else
//		sleep(1) ;
//#endif

	RsStackMutex mut(_addrMtx) ;

	delete _found ;
	delete _searching ;
	free (_addr) ;
}

ExtAddrFinder::ExtAddrFinder() : _addrMtx("ExtAddrFinder")
{
#ifdef EXTADDRSEARCH_DEBUG
	std::cerr << "ExtAddrFinder: Creating new ExtAddrFinder." << std::endl ;
#endif
	RsStackMutex mut(_addrMtx) ;

	_found = new bool ;
	*_found = false ;

	_searching = new bool ;
	*_searching = false ;

	mFoundTS = new time_t;
	*mFoundTS = time(NULL) - MAX_IP_STORE;

	_addr = (in_addr*)malloc(sizeof(in_addr)) ;

	_ip_servers.push_back(std::string( "checkip.dyndns.org" )) ;
	_ip_servers.push_back(std::string( "www.myip.dk"   )) ;
	_ip_servers.push_back(std::string( "showip.net"         )) ;
	_ip_servers.push_back(std::string( "www.displaymyip.com")) ;
}

