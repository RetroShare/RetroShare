#ifndef RSGNP_H
#define RSGNP_H

#include <set>
#include <time.h>
#include <stdlib.h>

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
    void addGdp(RsGdp* gdp) = 0;
    void removeGdp(RsGdp* gdp) = 0;

public:

    /*** IMPLEMENTATION DETAILS ****/

    /* Get/Send Messages */
    void getAvailableMsgs(std::string peerId, time_t from, time_t to); /* request over the network */
    void sendAvailableMsgs(std::string peerId, time_t from, time_t to); /* send to peers */

};

#endif // RSGNP_H
