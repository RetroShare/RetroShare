#ifndef RSGIXS_H
#define RSGIXS_H

/*
 * libretroshare/src/gxs: gxs.h
 *
 * General Identity Exchange Service interface for RetroShare.
 *
 * Copyright 2011-2011 by Robert Fernie, Christopher Evi-Prker
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

typedef RsPeerId  PeerId; // SHOULD BE REMOVED => RsPeerId (SSLID)
typedef PGPIdType RsPgpId;

//
//// External Interface - 
//class RsIdentityService
//{
//    enum IdentityType { Pseudonym, Signed, Anonymous };
//
//    virtual bool loadId(const GxsId &id) = 0;	
//
//    virtual bool getNickname(const GxsId &id, std::string &nickname) = 0;	
//
//    virtual bool createKey(RsGixsProfile& profile, uint32_t type) = 0; /* fills in mKeyId, and signature */
//
//    virtual RsGixsProfile* getProfile(const KeyRef& keyref) = 0;
//
//	// modify reputation.
//
//};


/* Identity Interface for GXS Message Verification.
 */
class RsGixs
{
public:
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
    virtual bool requestKey(const RsGxsId &id, const std::list<PeerId> &peers) = 0;
    virtual bool requestPrivateKey(const RsGxsId &id) = 0;


    /*!
     * Retrieves a key identity
     * @param keyref
     * @return a pointer to a valid profile if successful, otherwise NULL
     *
     */
    virtual bool  getKey(const RsGxsId &id, RsTlvSecurityKey &key) = 0;
    virtual bool  getPrivateKey(const RsGxsId &id, RsTlvSecurityKey &key) = 0;	// For signing outgoing messages.


};

class GixsReputation
{
	public:
	GixsReputation() : score(0) {}
		RsGxsId id;
		int score;
};


class RsGixsReputation 
{
public:
	// get Reputation.
    virtual bool haveReputation(const RsGxsId &id) = 0;
    virtual bool loadReputation(const RsGxsId &id, const std::list<RsPeerId>& peers) = 0;
    virtual bool getReputation(const RsGxsId &id, GixsReputation &rep) = 0;
};


/*** This Class pulls all the GXS Interfaces together ****/

class RsGxsIdExchange: 
	public RsGenExchange, 
	public RsGixsReputation, 
	public RsGixs
{
public:
	RsGxsIdExchange(RsGeneralDataService* gds, RsNetworkExchangeService* ns, RsSerialType* serviceSerialiser, uint16_t mServType, uint32_t authenPolicy)
	:RsGenExchange(gds,ns,serviceSerialiser,mServType, this, authenPolicy) { return; }
virtual ~RsGxsIdExchange() { return; }

};




/* For Circles Too */

class RsGcxs
{
	public:

        /* GXS Interface - for working out who can receive */
        virtual bool isLoaded(const RsGxsCircleId &circleId) = 0;
        virtual bool loadCircle(const RsGxsCircleId &circleId) = 0;

        virtual int canSend(const RsGxsCircleId &circleId, const RsPgpId &id) = 0;
        virtual int canReceive(const RsGxsCircleId &circleId, const RsPgpId &id) = 0;
        virtual bool recipients(const RsGxsCircleId &circleId, std::list<RsPgpId> &friendlist) = 0;
};



class RsGxsCircleExchange: public RsGenExchange, public RsGcxs
{
public:
	RsGxsCircleExchange(RsGeneralDataService* gds, RsNetworkExchangeService* ns, RsSerialType* serviceSerialiser, 
			uint16_t mServType, RsGixs* gixs, uint32_t authenPolicy)
	:RsGenExchange(gds,ns,serviceSerialiser,mServType, gixs, authenPolicy)  { return; }
virtual ~RsGxsCircleExchange() { return; }

};


#endif // RSGIXS_H
