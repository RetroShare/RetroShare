#ifndef PQI_ADDR_STORE_H 
#define PQI_ADDR_STORE_H 

#include "pqi/pqinetwork.h"

#include <string>

class pqiAddrStore
{
	public:
	virtual ~pqiAddrStore() { return; }
        /* pqiAddrStore ... called prior to connect */
	virtual bool    addrFriend(std::string id, struct sockaddr_in &addr, unsigned int &flags) = 0;
};


/* implemented elsewhere, to provide pqi with AddrStore */

extern pqiAddrStore *getAddrStore();


#endif /* PQI_ADDR_STORE_H */
