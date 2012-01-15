#ifndef RSGNP_H
#define RSGNP_H

#include <set>

#include "services/p3service.h"
#include "gxs/rsgdp.h"


/*!
 * Retroshare general network protocol
 *
 * This simply deals with the receiving and sending
 * RsGxs data
 *
 */
class RsGnp : p3ThreadedService
{
public:
    RsGnp();


    /*** gdp::iterface *****/
    int requestMsgs(std::set<std::string> msgId, double &delay);
    void addGdp(RsGdp* gdp);
    void removeGdp(RsGdp* gdp);

private:

    /*** IMPLEMENTATION DETAILS ****/

    /* Get/Send Messages */
    void getAvailableMsgs(std::string peerId, time_t from, time_t to); /* request over the network */
    void sendAvailableMsgs(std::string peerId, gdp::id grpId, time_t from, time_t to); /* send to peers */

    requestMessages(std::string peerId, gdp::id grpId, std::list<gdp::id> msgIds);
    sendMessages(std::string peerId, gdp::id grpId, std::list<gdp::id> msgIds);	  /* send to peer, obviously permissions have been checked first */

    /* Search the network */
};

#endif // RSGNP_H
