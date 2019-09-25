/*******************************************************************************
 * libretroshare/src/gxs: rsgixs.h                                             *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2011-2011 by Robert Fernie, Evi-Parker Christopher                *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#ifndef RSGIXS_H
#define RSGIXS_H

#include "gxs/rsgxs.h"
#include "gxs/rsgenexchange.h"

#include "retroshare/rsgxscircles.h"

#include "serialiser/rstlvkeys.h"
#include "retroshare/rsids.h"
/*!
 * GIXP: General Identity Exchange Service.
 *
 * As we're always running into troubles with GPG signatures... we are going to
 * create a layer of RSA Keys for the following properties:
 *
 * 1) RSA Keys can be Anonymous, Self-Signed with Pseudonym, Signed by GPG Key.
 * To clarify:
 *    a. This forms a layer of keys stay between GPG and pub/priv publish key ?
 *    b. Difference between anonymous and pseudonym keys?
 *            - Anonymous cannot be signed?
 *            -
 *    c. To some extent this determines security model of RsGeneralExchangeService

 *	- Anonymous & Pseudonym Keys will be shared network-wide (Hop by Hop).
        - GPG signed Keys will only be shared if we can validate the signature
                (providing similar behaviour to existing GPG Keys).
        - GPG signed Keys can optionally be marked for Network-wide sharing.
 * 2) These keys can be used anywhere, specifically in the protocols described below.
 * 3) These keys can be used to sign, encrypt, verify & decrypt
 * 4) Keys will never need to be directly accessed - stored in this class.
 *     a. I guess can work solely through Id
 *     b. Use Case: Receivve a message, has a key id, request
 * 5) They will be cached locally and exchanged p2p, by pull request.
 * 6) This class will use the generalised packet storage for efficient caching & loading.
 * 7) Data will be stored encrypted.
 */


/******
 * More notes. The ideas above have been crystalised somewhat.
 *
 * The Identity service will now serve two roles:
 * 1) validating msgs.
 * 2) reputation of identity.
 *
 * The identity will be equivalent to a Group Definition.
 * and the reputation contained in the group's messages.
 *
 * 
 * Group
 *   MetaData:
 *     Public Key
 *     Signatures. (Admin & optional GPG).
 *   RealData:
 *     Nickname.
 *     GPGHash
 *
 * The GPGHash will allow people to identify the real gpg user behind the identity.
 * We must make sure that the Hash input has enough entropy that it cannot be brute-forced (e.g. like password hashes).
 *
 * The Identity service only has to provide hooks to access the Keys for each group.
 * All the key definitions are exactly the same as for GroupMetaData.
 *
 * The Interface is split into three parts.
 * 1) Internal interface used by GXS to verify signatures.
 * 2) Internal interface used by GXS to help decide if we get a message or not.
 * 3) External interface to access nicknames, generate new identities etc.
 *
 * The actual implementation will cache frequently used keys and nicknames, 
 * as these will be used very frequently.
 *****/

/* Identity Interface for GXS Message Verification.
 */
class RsGixs
{
public:
	// TODO: cleanup this should be an enum!
    static const uint32_t RS_GIXS_ERROR_NO_ERROR           = 0x0000 ;
    static const uint32_t RS_GIXS_ERROR_UNKNOWN            = 0x0001 ;
    static const uint32_t RS_GIXS_ERROR_KEY_NOT_AVAILABLE  = 0x0002 ;
    static const uint32_t RS_GIXS_ERROR_SIGNATURE_MISMATCH = 0x0003 ;

     /* Performs/validate a signature with the given key ID. The key must be available, otherwise the signature error
     * will report it. Each time a key is used to validate a signature, its usage timestamp is updated.
     *
     * If force_load is true, the key will be forced loaded from the cache. If not, uncached keys will return
     * with error_status=RS_GIXS_SIGNATURE_ERROR_KEY_NOT_AVAILABLE, but will likely be cached on the next call.
     */

    virtual bool signData(const uint8_t *data,uint32_t data_size,const RsGxsId& signer_id,RsTlvKeySignature& signature,uint32_t& signing_error) = 0 ;
    virtual bool validateData(const uint8_t *data,uint32_t data_size,const RsTlvKeySignature& signature,bool force_load,const RsIdentityUsage& info,uint32_t& signing_error) = 0 ;

	virtual bool encryptData( const uint8_t *clear_data,
	                          uint32_t clear_data_size,
	                          uint8_t *& encrypted_data,
	                          uint32_t& encrypted_data_size,
	                          const RsGxsId& encryption_key_id,
	                          uint32_t& encryption_error, bool force_load) = 0 ;
	virtual bool decryptData( const uint8_t *encrypted_data,
	                          uint32_t encrypted_data_size,
	                          uint8_t *& clear_data, uint32_t& clear_data_size,
	                          const RsGxsId& encryption_key_id,
	                          uint32_t& encryption_error, bool force_load) = 0 ;

	virtual bool getOwnIds(std::list<RsGxsId> &ownIds, bool signed_only = false)=0;
    virtual bool isOwnId(const RsGxsId& key_id) = 0 ;

    virtual void timeStampKey(const RsGxsId& key_id,const RsIdentityUsage& reason) = 0 ;

    // Key related interface - used for validating msgs and groups.
    /*!
     * Use to query a whether given key is available by its key reference
     * @param keyref the keyref of key that is being checked for
     * @return true if available, false otherwise
     */
    virtual bool haveKey(const RsGxsId &id) = 0;

    /*!
     * Use to query whether private key member of the given key reference is available
     * @param keyref the KeyRef of the key being checked for
     * @return true if private key is held here, false otherwise
     */
    virtual bool havePrivateKey(const RsGxsId &id) = 0;

    // The fetchKey has an optional peerList.. this is people that had the msg with the signature.
    // These same people should have the identity - so we ask them first.
    /*!
     * Use to request a given key reference
     * @param keyref the KeyRef of the key being requested
     * @return will
     */
    virtual bool requestKey(const RsGxsId &id, const std::list<RsPeerId> &peers,const RsIdentityUsage& info) = 0;
    virtual bool requestPrivateKey(const RsGxsId &id) = 0;


    /*!
     * Retrieves a key identity
     * @param keyref
     * @return a pointer to a valid profile if successful, otherwise NULL
     *
     */
    virtual bool  getKey(const RsGxsId &id, RsTlvPublicRSAKey& key) = 0;
    virtual bool  getPrivateKey(const RsGxsId &id, RsTlvPrivateRSAKey& key) = 0;	// For signing outgoing messages.
    virtual bool  getIdDetails(const RsGxsId& id, RsIdentityDetails& details) = 0 ;  // Proxy function so that we get p3Identity info from Gxs
};

class GixsReputation
{
public:
	GixsReputation() :reputation_level(0) {}

	RsGxsId id;
	uint32_t reputation_level ;
};

struct RsGixsReputation
{
	virtual RsReputationLevel overallReputationLevel(
	        const RsGxsId& id, uint32_t* identity_flags = nullptr ) = 0;
	virtual ~RsGixsReputation(){}
};

/*** This Class pulls all the GXS Interfaces together ****/

struct RsGxsIdExchange : RsGenExchange, RsGixs
{
	RsGxsIdExchange(
	        RsGeneralDataService* gds, RsNetworkExchangeService* ns,
	        RsSerialType* serviceSerialiser, uint16_t mServType,
	        uint32_t authenPolicy )
	    : RsGenExchange(
	          gds, ns, serviceSerialiser, mServType, this, authenPolicy ) {}
};




/* For Circles Too */

class RsGcxs
{
	public:
	virtual ~RsGcxs(){}

        /* GXS Interface - for working out who can receive */
        virtual bool isLoaded(const RsGxsCircleId &circleId) = 0;
        virtual bool loadCircle(const RsGxsCircleId &circleId) = 0;

        virtual int  canSend(const RsGxsCircleId &circleId, const RsPgpId &id,bool& should_encrypt) = 0;
        virtual int  canReceive(const RsGxsCircleId &circleId, const RsPgpId &id) = 0;
    
        virtual bool recipients(const RsGxsCircleId &circleId, std::list<RsPgpId>& friendlist) = 0;
        virtual bool recipients(const RsGxsCircleId &circleId, const RsGxsGroupId& destination_group, std::list<RsGxsId>& idlist) = 0;
    
        virtual bool isRecipient(const RsGxsCircleId &circleId, const RsGxsGroupId& destination_group, const RsGxsId& id) = 0;
        
	virtual bool getLocalCircleServerUpdateTS(const RsGxsCircleId& gid,rstime_t& grp_server_update_TS,rstime_t& msg_server_update_TS) =0;
};



class RsGxsCircleExchange: public RsGenExchange, public RsGcxs
{
public:
	RsGxsCircleExchange(RsGeneralDataService* gds, RsNetworkExchangeService* ns, RsSerialType* serviceSerialiser, 
			uint16_t mServType, RsGixs* gixs, uint32_t authenPolicy)
	:RsGenExchange(gds,ns,serviceSerialiser,mServType, gixs, authenPolicy)  { return; }
	virtual ~RsGxsCircleExchange() { return; }
    
	virtual bool getLocalCircleServerUpdateTS(const RsGxsCircleId& gid,rstime_t& grp_server_update_TS,rstime_t& msg_server_update_TS) 
	{
		return RsGenExchange::getGroupServerUpdateTS(RsGxsGroupId(gid),grp_server_update_TS,msg_server_update_TS) ;
	}
};


#endif // RSGIXS_H
