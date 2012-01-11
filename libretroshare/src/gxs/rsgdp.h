#ifndef RSGDP_H
#define RSGDP_H

#include <string>

#include "inttypes.h"
#include "util/rssqlite.h"

/*!
 *
 * The main role of GDP is to receive and start sync requests
 * and prepare exchange data for RsGnp.
 * It is also the curator (write authority) of externally received messages
 */
class RsGdp
{
public:
    RsGdp();
};

class RsGxsMessage {

};

/*!
 *
 *
 */
class RsGss {

public:
    RsGss(RsSqlite*, uint8_t io_stat);

    /*!
     * stores message in associate RsSqlite db
     * if RsGss is in read only mode this function will always \n
     * return false
     *
     * @param message the message to store
     * @return true if message successfully stored
     */
    virtual bool store(const RsGxsMessage& message) = 0;

    /*!
     * retrieve message from associate RsSqlite db \n
     *
     * @return true if successfully retrieved, false otherwise
     */
    virtual bool retrieve(const std::string msgId, RsGxsMessage&) = 0;

    uint8_t getIoStat();


    static uint8_t READ_ONLY;
    static uint8_t READ_AND_WRITE;
};

#endif // RSGDP_H
