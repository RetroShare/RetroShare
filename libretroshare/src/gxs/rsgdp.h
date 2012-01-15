#ifndef RSGDP_H
#define RSGDP_H

/*
 * libretroshare/src/gxp: gxp.h
 *
 * General Data service, interface for RetroShare.
 *
 * Copyright 2011-2011 by Robert Fernie, Evi-Parker Christopher
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

#include <string>

#include "inttypes.h"
#include "util/rssqlite.h"


/*!
 * GDP: Represents the center of gxs. In as far as enabling synchronisation between peers
 *
 * Responsibilty:
 *               GDP has access to concrete RsGxs services internal messages by way of a passed
 *               RsGss (storage module in read only mode)
 *               It also receives messages from GNP which it stores in its external db which
 *               it passed as a handle to the concrete RsGxs service (read only for service)
 *
 *               An important responsibility is the preparation of data for GNP as such
 *               permissions of messages are enforced here
 *
 *****/


/*!
 * The main role of GDP is to receive and start sync requests
 * and prepare exchange data for RsGnp.
 * It is also the curator (write authority) of externally received messages
 */
class RsGdp
{
public:
    RsGdp();
};

class RsGxsSignedMessage {

};

/*!
 * General Storage service
 * This presents an abstract interface used by both concrete Gxs services and the Gdp to store
 * internally and externally generated messages respectively.
 * This abstraction allows GDP to retrieve entries from an SQL lite db without knowledge of the
 * service db schema (columns basically).
 * Concrete services should implement this service
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
    virtual bool store(const RsGxsSignedMessage* message) = 0;

    /*!
     * retrieve message from associate RsSqlite db \n
     * @param msgId
     * @param message
     * @return true if successfully retrieved, false otherwise
     */
    virtual bool retrieve(const std::string msgId, RsGxsSignedMessage* message) = 0;

    /*!
     * Use this find out if Rss i/o status
     * @return the io status
     */
    uint8_t getIoStat();


    static uint8_t READ_ONLY;
    static uint8_t READ_AND_WRITE;
};

#endif // RSGDP_H
