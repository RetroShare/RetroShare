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

#include <set>
#include <map>
#include <string>

#include "inttypes.h"
#include "rsgnp.h"

typedef std::map<std::string, std::set<std::string> > MsgGrpId;
typedef std::map<std::string, std::set<RsGxsSignedMessage*> > SignedMsgGrp;


/*!
 * The main role of GDS is the preparation and handing out of messages requested from
 * RsGeneralExchangeService and RsGeneralExchangeService
 * It is important to note that no actual messages are passed by this interface as its is expected
 * architecturally to pass messages to the service via a call back
 *
 * It also acts as a layer between RsGeneralStorageService and its parent RsGeneralExchangeService
 * thus allowing for non-blocking requests, etc.
 * It also provides caching ability
 *
 *
 * Caching feature:
 *   - A cache index should be maintained which is faster than normal message request
 *   - This should allow fast retrieval of message based on grp id or msg id
 *
 * Identity Exchange Service:
 *   - As this is the point where data is accessed by both the GNP and GXS the identities
 *     used to decrypt, encrypt and verify is handle here.
 *
 *
 */
class RsGeneralDataService
{
public:

    /*!
     * Retrieves signed message
     * @param msgGrp this contains grp and the message to retrieve
     * @return request code to be redeemed later
     */
    virtual int request(const MsgGrpId& msgGrp, bool decrypted) = 0;


    /*!
     * allows for more complex queries specific to the service
     * Service should implement its own specific filter
     * @param filter generally stores parameters needed for query
     * @param cacheRequest set to true to cache the messages requested
     * @return request code to be redeemed later
     */
    virtual int request(RequestFilter* filter, bool cacheRequest) = 0;


    /*!
     * stores signed message
     * @param msgs signed messages to store
     */
    virtual bool store(SignedMsgGrp& msgs) = 0;

    /*!
     * Gets group and any associated meta data
     * @param grpIds set with group Ids available from storage
     * @return request code to be redeemed later
     */
    virtual int getGroups(const std::set<std::string>& grpIds) = 0;

    /*!
     * Gets list of message ids in storage
     * @param msgIds gets message ids in storage
     * @return request code to be redeemed later
     */
    virtual int getMessageIds(std::set<std::string>& msgIds) = 0;

    /*!
     * caches message for faster retrieval later
     * @return false if caching failed, msg may not exist, false otherwise
     */
    virtual bool cacheMsg(const std::string& grpId, const std::string& msgId) = 0;

    /*!
     * caches all messages of this grpId for faster retrieval later
     * @param grpId all message of this grpId are cached
     * @return false if caching failed, msg may not exist, false otherwise
     */
    bool cacheGrp(const std::string& grpId) = 0;

    /*!
     * checks if msg is cached
     * @param msgId message to check if cached
     * @param grpId
     * @return false if caching failed, msg may not exist, false otherwise
     */
    bool msgCached(const std::string& grpId, const std::string& msgId) = 0;

    /*!
     * check if messages of the grpId are cached
     * @param grpId all message of this grpId are checked
     * @return false if caching failed, msg may not exist, false otherwise
     */
    bool grpCached(const std::string& grpId) = 0;

};


/*!
 * Might be better off simply sending request codes
 *
 */
class RequestFilter {

    /*!
     *
     */
    virtual int code() = 0;
};

/*!
 *
 * This is implemented by the concrete GXS class to represent and define how \n
 * RsGxsSignedMessage is stored and retrieved from disk \n
 * More complicated queries are enabled through the use of \n
 * RequestFilter which through rtti store generic parameters used by the RsGeneralStorageService implementation \n
 *
 *
 * The main reason for layering RsGeneralDataService between this and RsGeneralExchangeService implementer \n
 * is to avoid the overhead of storage access and while allowing a rich variety of implementations not having to \n
 * worry about efficiency \n
 */
class RsGeneralStorageService {

public:


    /*!
     * Retrieves signed messages from storage
     * @param msgGrp this contains grp and the message to retrieve
     */
    virtual void retrieve(const MsgGrpId& msgGrp, SignedMsgGrp& result, bool decrypted) = 0;


    /*!
     * allows for more complex queries specific to the service
     * Service should implement method taking case
     * @param filter
     */
    virtual void retrieve(RequestFilter* filter, SignedMsgGrp& msgs) = 0;

    /*!
     * stores signed message in internal storage
     * @param msgs signed messages to store
     */
    virtual void store(SignedMsgGrp& msgs) = 0;

    /*!
     * retrieves the group ids from storage
     * @param grpIds set with grpIds available from storage
     */
    virtual void getGroups(std::set<std::string>& grpIds) = 0;

    /*!
     * retrieve message in this stored in this storage
     * @param msgIds gets message ids in storage
     */
    virtual bool getMessageIds(std::set<std::string>& msgIds) = 0;

    /*!
     * Use this find out if Rss i/o status
     * @return the io status
     */
    virtual uint8_t getIoStat() = 0;

    static uint8_t IOSTAT_READ_ONLY;
    static uint8_t IOSTAT_READ_AND_WRITE;

};

#endif // RSGDP_H
