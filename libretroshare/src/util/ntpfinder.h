#pragma once

#include <list>
#include <string>
#include "util/rsthreads.h"

#define NTP_Error_NoResult     1 /*No result from NTP servers.*/
#define NTP_Error_TimeBwSrvTL  2 /*The time between two server is too large.*/


struct sockaddr ;

class NTPFinder
{
	public:
		NTPFinder() ;
		~NTPFinder() ;

		bool hasValidNTPTime(time_t *ntpTime) ;
		void setNTPServersList(std::list<std::string>& ntp_servers) ;
		void getNTPServersList(std::list<std::string>& ntp_servers) ;
		void getNTPError(int *ntp_error) { *ntp_error = *_error ; }
		void forceNTPRefresh() {*_doRefresh = true; }
		void start_request() ;

		void reset() ;

	private:
		friend void* doNTPSearch(void *p) ;

		RsMutex _ntpMtx ;
		time_t   *_ntpTime ;
		bool *_found ;
		bool *_searching ;
		time_t   *mFoundTS ;
		std::list<std::string> _ntp_servers ;
		int *_error ;
		bool *_doRefresh ;
};
