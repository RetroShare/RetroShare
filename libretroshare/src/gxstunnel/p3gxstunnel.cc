/*
 * libretroshare/src/chat: distantchat.cc
 *
 * Services for RetroShare.
 *
 * Copyright 2014 by Cyril Soler
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
 * Please report all bugs and problems to "csoler@users.sourceforge.net".
 *
 */


#include <unistd.h>

#include "openssl/rand.h"
#include "openssl/dh.h"
#include "openssl/err.h"

#include "util/rsaes.h"

#include <retroshare/rsidentity.h>
#include <retroshare/rsiface.h>

#include <rsserver/p3face.h>
#include <services/p3idservice.h>
#include <gxs/gxssecurity.h>
#include <turtle/p3turtle.h>
#include <retroshare/rsids.h>

#include "p3gxstunnel.h"

#define DEBUG_GXS_TUNNEL

static const uint32_t GXS_TUNNEL_KEEP_ALIVE_TIMEOUT = 6 ; // send keep alive packet so as to avoid tunnel breaks.

static const uint32_t RS_GXS_TUNNEL_DH_STATUS_UNINITIALIZED = 0x0000 ;
static const uint32_t RS_GXS_TUNNEL_DH_STATUS_HALF_KEY_DONE = 0x0001 ;
static const uint32_t RS_GXS_TUNNEL_DH_STATUS_KEY_AVAILABLE = 0x0002 ;

static const uint32_t RS_GXS_TUNNEL_STATUS_UNKNOWN    = 0x00 ;
static const uint32_t RS_GXS_TUNNEL_STATUS_CAN_TALK   = 0x01 ;
static const uint32_t RS_GXS_TUNNEL_STATUS_TUNNEL_DN  = 0x02 ;

static const uint32_t GXS_TUNNEL_HMAC_SIZE    = SHA_DIGEST_LENGTH ;
static const uint32_t GXS_TUNNEL_IV_SIZE      = 8 ;

void p3GxsTunnelService::connectToTurtleRouter(p3turtle *tr)
{
	mTurtle = tr ;
	tr->registerTunnelService(this) ;
}

void p3GxsTunnelService::flush()
{
    // Flush items that could not be sent, probably because of a Mutex protected zone.
    //
    while(!pendingGxsTunnelItems.empty())
    {
        sendTurtleData( pendingGxsTunnelItems.front() ) ;
        pendingGxsTunnelItems.pop_front() ;
    }

    // TODO:  also sweep GXS id map and disable any ID with no virtual peer id in the list.

    RS_STACK_MUTEX(mGxsTunnelMtx); /********** STACK LOCKED MTX ******/

    time_t now = time(NULL) ;

    for(std::map<RsGxsId,GxsTunnelPeerInfo>::iterator it(_gxs_tunnel_contacts.begin());it!=_gxs_tunnel_contacts.end();++it)
    {
        if(it->second.last_contact+20+GXS_TUNNEL_KEEP_ALIVE_TIMEOUT < now && it->second.status == RS_GXS_TUNNEL_STATUS_CAN_TALK)
        {
            std::cerr << "(II) GxsTunnelService:: connexion interrupted with peer." << std::endl;
            it->second.status = RS_GXS_TUNNEL_STATUS_TUNNEL_DN ;
            it->second.virtual_peer_id.clear() ;

            // Also reset turtle router monitoring so as to make the tunnel handling more responsive. If we don't do that,
            // the TR will wait 60 secs for the tunnel to die, which causes a significant waiting time in the chat window.

            if(it->second.direction == RsTurtleGenericTunnelItem::DIRECTION_SERVER)
            {
                std::cerr << "(II) GxsTunnelService:: forcing new tunnel campain." << std::endl;

                mTurtle->forceReDiggTunnels( hashFromGxsId(it->first) );
            }
        }
        if(it->second.last_keep_alive_sent + GXS_TUNNEL_KEEP_ALIVE_TIMEOUT < now && it->second.status == RS_GXS_TUNNEL_STATUS_CAN_TALK)
        {
            RsGxsTunnelStatusItem *cs = new RsGxsTunnelStatusItem ;

#warning should we send that unencrypted??
            cs->status_string.clear() ;
            cs->flags = RS_CHAT_FLAG_PRIVATE | RS_CHAT_FLAG_KEEP_ALIVE;
            cs->PeerId(RsPeerId(it->first)) ;

            // we send off-mutex to avoid deadlock.

            pendingGxsTunnelItems.push_back(cs) ;

            it->second.last_keep_alive_sent = now ;
#ifdef DEBUG_GXS_TUNNEL
            std::cerr << "(II) GxsTunnelService:: Sending keep alive packet to gxs id " << it->first << std::endl;
#endif
        }
    }
}

bool p3GxsTunnelService::handleRecvItem(RsGxsTunnelItem *item)
{
	if(item == NULL)
		return false ;

	switch(item->PacketSubType())
	{
	case RS_PKT_SUBTYPE_GXS_TUNNEL_DH_PUBLIC_KEY:	handleRecvDHPublicKey(dynamic_cast<RsGxsTunnelDHPublicKeyItem*>(item)) ; break ;
							return true ;

#warning need to implement tunnel data handling here
	case RS_PKT_SUBTYPE_GXS_TUNNEL_DATA:
        	return true ;
            
#warning need to implement tunnel data ACK handling here
	case RS_PKT_SUBTYPE_GXS_TUNNEL_DATA_ACK:
        	return true ;
            
	case RS_PKT_SUBTYPE_GXS_TUNNEL_STATUS:		handleRecvStatusItem(dynamic_cast<RsGxsTunnelStatusItem*>(item)) ;
	    						return true ;
	}

	default:
		return false ;
	}

	return false ;
}

#warning is this function still used??
bool p3GxsTunnelService::handleOutgoingItem(RsGxsTunnelItem *item)
{
    {
        RS_STACK_MUTEX(mGxsTunnelMtx) ;

        std::map<RsGxsId,GxsTunnelPeerInfo>::const_iterator it=_gxs_tunnel_contacts.find(RsGxsId(item->PeerId()));

        if(it == _gxs_tunnel_contacts.end())
            return false ;
    }

#ifdef CHAT_DEBUG
    std::cerr << "p3GxsTunnelService::handleOutgoingItem(): sending to " << item->PeerId() << ": interpreted as a distant chat virtual peer id." << std::endl;
#endif
    sendTurtleData(item) ;
    return true;
}

void p3GxsTunnelService::handleRecvStatusItem(RsGxsTunnelStatusItem *cs)
{
    if(cs->flags & RS_GXS_TUNNEL_FLAG_CLOSING_DISTANT_CONNECTION)
        markGxsTunnelAsClosed(RsGxsId(cs->PeerId())) ;

    // nothing more to do, because the decryption routing will update the last_contact time when decrypting.

    if(cs->flags & RS_GXS_TUNNEL_FLAG_KEEP_ALIVE)
        std::cerr << "GxsTunnelService::handleRecvGxsTunnelStatusItem(): received keep alive packet for inactive tunnel! peerId=" << cs->PeerId() << std::endl;
}

bool p3GxsTunnelService::handleTunnelRequest(const RsFileHash& hash,const RsPeerId& /*peer_id*/)
{
	RsStackMutex stack(mGxsTunnelMtx); /********** STACK LOCKED MTX ******/

	// look into owned GXS ids, and see if the hash corresponds to the expected hash
	//
	std::list<RsGxsId> own_id_list ;
	rsIdentity->getOwnIds(own_id_list) ;

    // re-computing the hash from the GXS id allows to dynamically change the hash. That will allow
    // the use of a contact passphrase, if needed.

	for(std::list<RsGxsId>::const_iterator it(own_id_list.begin());it!=own_id_list.end();++it)
		if(hashFromGxsId(*it) == hash)
		{
#ifdef DEBUG_GXS_TUNNEL
			std::cerr << "GxsTunnelService::handleTunnelRequest: received tunnel request for hash " << hash << std::endl;
			std::cerr << "  answering true!" << std::endl;
#endif
			return true ;
        }

	return false ;
}

void p3GxsTunnelService::addVirtualPeer(const TurtleFileHash& hash,const TurtleVirtualPeerId& virtual_peer_id,RsTurtleGenericTunnelItem::Direction dir)
{
#ifdef DEBUG_GXS_TUNNEL
	std::cerr << "GxsTunnelService:: received new virtual peer " << virtual_peer_id << " for hash " << hash << ", dir=" << dir << std::endl;
#endif
	RsGxsId own_gxs_id ;

	{
		RS_STACK_MUTEX(mGxsTunnelMtx); /********** STACK LOCKED MTX ******/

		GxsTunnelDHInfo& dhinfo( _gxs_tunnel_virtual_peer_ids[virtual_peer_id] ) ;
		dhinfo.gxs_id.clear() ;

		if(dhinfo.dh != NULL)
			DH_free(dhinfo.dh) ;

		dhinfo.dh = NULL ;
		dhinfo.direction = dir ;
		dhinfo.hash = hash ;
		dhinfo.status = RS_GXS_TUNNEL_DH_STATUS_UNINITIALIZED ;

		if(dir == RsTurtleGenericTunnelItem::DIRECTION_CLIENT)
		{
			// check that a tunnel is not already working for this hash. If so, give up.

			own_gxs_id = gxsIdFromHash(hash) ;
		}
		else	// client side
		{
			RsGxsId to_gxs_id = gxsIdFromHash(hash) ;
			std::map<RsGxsId,GxsTunnelPeerInfo>::const_iterator it = _gxs_tunnel_contacts.find(to_gxs_id) ;

			if(it == _gxs_tunnel_contacts.end())
			{
				std::cerr << "(EE) no pre-registered peer for hash " << hash << " on client side. This is a bug." << std::endl;
				return ;
			}

			if(it->second.status == RS_GXS_TUNNEL_STATUS_CAN_TALK)
			{
				std::cerr << "  virtual peer is for a distant chat session that is already openned and alive. Giving it up." << std::endl;
				return ;
			}

			own_gxs_id = it->second.own_gxs_id ;
		}

#ifdef DEBUG_GXS_TUNNEL
		std::cerr << "  Creating new virtual peer ID entry and empty DH session key." << std::endl;
#endif

	}

#ifdef DEBUG_GXS_TUNNEL
	std::cerr << "  Adding virtual peer " << virtual_peer_id << " for chat hash " << hash << std::endl;
#endif

	// Start a new DH session for this tunnel
	RS_STACK_MUTEX(mGxsTunnelMtx); /********** STACK LOCKED MTX ******/

	locked_restartDHSession(virtual_peer_id,own_gxs_id) ;
}

void p3GxsTunnelService::locked_restartDHSession(const RsPeerId& virtual_peer_id,const RsGxsId& own_gxs_id)
{
#ifdef DEBUG_GXS_TUNNEL
    std::cerr << "Starting new DH session." << std::endl;
#endif
    GxsTunnelDHInfo& dhinfo = _gxs_tunnel_virtual_peer_ids[virtual_peer_id] ;

    dhinfo.status = RS_GXS_TUNNEL_DH_STATUS_UNINITIALIZED ;

    if(!locked_initDHSessionKey(dhinfo.dh))
    {
        std::cerr << "  (EE) Cannot start DH session. Something went wrong." << std::endl;
        return ;
    }
    dhinfo.status = RS_GXS_TUNNEL_DH_STATUS_HALF_KEY_DONE ;

    if(!locked_sendDHPublicKey(dhinfo.dh,own_gxs_id,virtual_peer_id))
        std::cerr << "  (EE) Cannot send DH public key. Something went wrong." << std::endl;
}

void p3GxsTunnelService::removeVirtualPeer(const TurtleFileHash& hash,const TurtleVirtualPeerId& virtual_peer_id)
{
    bool tunnel_dn = false ;

#ifdef DEBUG_GXS_TUNNEL
    std::cerr << "GxsTunnelService: Removing virtual peer " << virtual_peer_id << " for hash " << hash << std::endl;
#else
    /* remove unused parameter warnings */
    (void) hash;
#endif
    {
        RsStackMutex stack(mGxsTunnelMtx); /********** STACK LOCKED MTX ******/

        RsGxsId gxs_id ;
        std::map<RsPeerId,GxsTunnelDHInfo>::iterator it = _gxs_tunnel_virtual_peer_ids.find(virtual_peer_id) ;

        if(it == _gxs_tunnel_virtual_peer_ids.end())
        {
            std::cerr << "(EE) Cannot remove virtual peer " << virtual_peer_id << ": not found in tunnel list!!" << std::endl;
            return ;
        }

        gxs_id = it->second.gxs_id ;

        if(it->second.dh != NULL)
            DH_free(it->second.dh) ;
        _gxs_tunnel_virtual_peer_ids.erase(it) ;

        std::map<RsGxsId,GxsTunnelPeerInfo>::iterator it2 = _gxs_tunnel_contacts.find(gxs_id) ;

        if(it2 == _gxs_tunnel_contacts.end())
        {
            std::cerr << "(EE) Cannot find GXS id " << gxs_id << " in contact list. Weird." << std::endl;
            return ;
        }
        if(it2->second.virtual_peer_id == virtual_peer_id)
        {
            it2->second.status = RS_GXS_TUNNEL_STATUS_TUNNEL_DN ;
            it2->second.virtual_peer_id.clear() ;
            tunnel_dn = true ;
        }
    }

    if(tunnel_dn)
    {
#warning we should notify the client here
        //RsServer::notify()->notifyChatStatus(ChatId(RsGxsId(virtual_peer_id)),"tunnel is down...") ;
        //RsServer::notify()->notifyPeerStatusChanged(virtual_peer_id.toStdString(),RS_STATUS_OFFLINE) ;
    }
}

void p3GxsTunnelService::receiveTurtleData(RsTurtleGenericTunnelItem *gitem,const RsFileHash& hash, const RsPeerId& virtual_peer_id,RsTurtleGenericTunnelItem::Direction direction)
{
#ifdef DEBUG_GXS_TUNNEL
    std::cerr << "GxsTunnelService::receiveTurtleData(): Received turtle data. " << std::endl;
    std::cerr << "         hash = " << hash << std::endl;
    std::cerr << "         vpid = " << virtual_peer_id << std::endl;
    std::cerr << "    acting as = " << direction << std::endl;
#else
    /* remove unused parameter warnings */
    (void) direction;
#endif

    RsTurtleGenericDataItem *item = dynamic_cast<RsTurtleGenericDataItem*>(gitem) ;

    if(item == NULL)
    {
        std::cerr << "(EE) item is not a data item. That is an error." << std::endl;
        return ;
    }
    // Call the AES crypto module
    // - the IV is the first 8 bytes of item->data_bytes

    if(item->data_size < 8)
    {
        std::cerr << "(EE) item encrypted data stream is too small: size = " << item->data_size << std::endl;
        return ;
    }
    if(*((uint64_t*)item->data_bytes) != 0)	// WTF?? we should use flags
    {
#ifdef DEBUG_GXS_TUNNEL
        std::cerr << "  Item is encrypted." << std::endl;
#endif

        // if cannot decrypt, it means the key is wrong. We need to re-negociate a new key.

        handleEncryptedData((uint8_t*)item->data_bytes,item->data_size,hash,virtual_peer_id) ;
    }
    else
    {
#ifdef DEBUG_GXS_TUNNEL
        std::cerr << "  Item is not encrypted." << std::endl;
#endif

        // Now try deserialise the decrypted data to make an RsItem out of it.
        //
#warning needs proper passing of item to client
        //RsItem *citem = RsChatSerialiser().deserialise(&((uint8_t*)item->data_bytes)[8],&item->data_size) ;

        if(citem == NULL)
        {
            std::cerr << "(EE) item could not be de-serialized. That is an error." << std::endl;
            return ;
        }

        // DH key items are sent even before we know who we speak to, so the virtual peer id is used in this
        // case only.

        if(dynamic_cast<RsGxsTunnelDHPublicKeyItem*>(citem) != NULL)
        {
            citem->PeerId(virtual_peer_id) ;
            handleIncomingItem(citem) ;
        }
        else
            std::cerr << "(EE) Deserialiased item has unexpected type." << std::endl;
    }
}

// This function encrypts the given data and adds a MAC and an IV into a serialised memory chunk that is then sent through the tunnel.

bool p3GxsTunnelService::handleEncryptedData(const uint8_t *data_bytes,uint32_t data_size,const TurtleFileHash& hash,const RsPeerId& virtual_peer_id)
{
#ifdef DEBUG_GXS_TUNNEL
    std::cerr << "p3GxsTunnelService::handleEncryptedDataItem()" << std::endl;
    std::cerr << "   size = " << data_size << std::endl;
    std::cerr << "   data = " << (void*)data_bytes << std::endl;
    std::cerr << "     IV = " << std::hex << *(uint64_t*)data_bytes << std::dec << std::endl;
    std::cerr << "   data = " << Bin2Hex((uint8_t*)data_bytes,data_size) ;
    std::cerr << std::endl;
#endif

    RsItem *citem = NULL;

    {
        RS_STACK_MUTEX(mGxsTunnelMtx); /********** STACK LOCKED MTX ******/

        uint32_t encrypted_size = data_size - GXS_TUNNEL_IV_SIZE - GXS_TUNNEL_HMAC_SIZE;
        uint32_t decrypted_size = RsAES::get_buffer_size(encrypted_size);
        uint8_t *encrypted_data = (uint8_t*)data_bytes+GXS_TUNNEL_IV_SIZE+GXS_TUNNEL_HMAC_SIZE;
        uint8_t *decrypted_data = new uint8_t[decrypted_size];
        uint8_t aes_key[GXS_TUNNEL_AES_KEY_SIZE] ;

        std::map<RsPeerId,GxsTunnelDHInfo>::iterator it = _gxs_tunnel_virtual_peer_ids.find(virtual_peer_id) ;

        if(it == _gxs_tunnel_virtual_peer_ids.end())
        {
            std::cerr << "(EE) item is not coming out of a registered tunnel. Weird. hash=" << hash << ", peer id = " << virtual_peer_id << std::endl;
            return true ;
        }

        RsGxsId gxs_id = it->second.gxs_id ;
        std::map<RsGxsId,GxsTunnelPeerInfo>::iterator it2 = _gxs_tunnel_contacts.find(gxs_id) ;

        if(it2 == _gxs_tunnel_contacts.end())
        {
            std::cerr << "(EE) no GXS id data for ID=" << gxs_id << ". This is a bug." << std::endl;
            return true ;
        }
        memcpy(aes_key,it2->second.aes_key,GXS_TUNNEL_AES_KEY_SIZE) ;

#ifdef DEBUG_GXS_TUNNEL
        std::cerr << "   Using IV: " << std::hex << *(uint64_t*)data_bytes << std::dec << std::endl;
        std::cerr << "   Decrypted buffer size: " << decrypted_size << std::endl;
        std::cerr << "   key  : " << Bin2Hex(aes_key,GXS_TUNNEL_AES_KEY_SIZE) << ; std::cerr << std::endl;
        std::cerr << "   hmac : " << Bin2Hex((uint8_t*)data_bytes+GXS_TUNNEL_IV_SIZE,GXS_TUNNEL_HMAC_SIZE) << ; std::cerr << std::endl;
        std::cerr << "   data : " << Bin2Hex((uint8_t*)data_bytes,data_size) << ; std::cerr << std::endl;
#endif
        // first, check the HMAC
        
        unsigned char *hm = HMAC(EVP_sha1(),aes_key,GXS_TUNNEL_AES_KEY_SIZE,encrypted_data,encrypted_size,NULL,NULL) ;
        
        if(memcmp(hm,&data_bytes[GXS_TUNNEL_IV_SIZE],GXS_TUNNEL_HMAC_SIZE))
        {
            std::cerr << "(EE) packet HMAC does not match. Computed HMAC=" << Bin2Hex(md,GXS_TUNNEL_HMAC_SIZE) << std::endl;
            std::cerr << "(EE) resetting new DH session." << std::endl;

            delete[] decrypted_data ;

            locked_restartDHSession(virtual_peer_id,it2->second.own_gxs_id) ;

            return false ;
        }

        if(!RsAES::aes_decrypt_8_16(encrypted_data,encrypted_size, aes_key,(uint8_t*)data_bytes,decrypted_data,decrypted_size))
        {
            std::cerr << "(EE) packet decryption failed." << std::endl;
            std::cerr << "(EE) resetting new DH session." << std::endl;

            delete[] decrypted_data ;

            locked_restartDHSession(virtual_peer_id,it2->second.own_gxs_id) ;

            return false ;
        }
        it2->second.status = RS_GXS_TUNNEL_STATUS_CAN_TALK ;
        it2->second.last_contact = time(NULL) ;

#ifdef DEBUG_GXS_TUNNEL
        std::cerr << "(II) Decrypted data: size=" << decrypted_size << std::endl;
#endif

        // Now try deserialise the decrypted data to make an RsItem out of it.
        //
#warning pass on the serialised data directly to client (see what GRouter does)
        //citem = RsChatSerialiser().deserialise(decrypted_data,&decrypted_size) ;
        
        delete[] decrypted_data ;

        if(citem == NULL)
        {
            std::cerr << "(EE) item could not be de-serialized. That is an error." << std::endl;
            return true;
        }

        // DH key items are sent even before we know who we speak to, so the virtual peer id is used in this
        // case only.

        citem->PeerId(RsPeerId(gxs_id)) ;
    }

#ifdef DEBUG_GXS_TUNNEL
    std::cerr << "(II) Setting peer id to " << citem->PeerId() << std::endl;
#endif
    handleIncomingItem(citem) ; // Treats the item, and deletes it

    return true ;
}

void p3GxsTunnelService::handleRecvDHPublicKey(RsGxsTunnelDHPublicKeyItem *item)
{
    if (!item)
    {
	    std::cerr << "p3GxsTunnelService:  Received null DH public key item. This should not happen." << std::endl;
        return;
    }

#ifdef DEBUG_GXS_TUNNEL
    std::cerr << "GxsTunnelService:  Received DH public key." << std::endl;
    item->print(std::cerr, 0) ;
#endif

    // Look for the current state of the key agreement.

    TurtleVirtualPeerId vpid = item->PeerId() ;

    RS_STACK_MUTEX(mGxsTunnelMtx); /********** STACK LOCKED MTX ******/

    std::map<RsPeerId,GxsTunnelDHInfo>::iterator it = _gxs_tunnel_virtual_peer_ids.find(vpid) ;

    if(it == _gxs_tunnel_virtual_peer_ids.end())
    {
        std::cerr << "  (EE) Cannot find hash in gxs_tunnel peer list!!" << std::endl;
        return ;
    }

    // Now check the signature of the DH public key item.

#ifdef DEBUG_GXS_TUNNEL
    std::cerr << "  Checking signature. " << std::endl;
#endif

    uint32_t pubkey_size = BN_num_bytes(item->public_key) ;
    unsigned char *data = (unsigned char *)malloc(pubkey_size) ;
    BN_bn2bin(item->public_key, data) ;

    RsTlvSecurityKey signature_key ;

    // We need to get the key of the sender, but if the key is not cached, we
    // need to get it first. So we let the system work for 2-3 seconds before
    // giving up. Normally this would only cause a delay for uncached keys,
    // which is rare. To force the system to cache the key, we first call for
    // getIdDetails().
    //
    RsIdentityDetails details  ;
    RsGxsId senders_id( item->signature.keyId ) ;

    for(int i=0;i<6;++i)
        if(!mGixs->getKey(senders_id,signature_key) || signature_key.keyData.bin_data == NULL)
        {
#ifdef DEBUG_GXS_TUNNEL
            std::cerr << "  Cannot get key. Waiting for caching. try " << i << "/6" << std::endl;
#endif
            usleep(500 * 1000) ;	// sleep for 500 msec.
        }
        else
            break ;

    if(signature_key.keyData.bin_data == NULL)
    {
	    std::cerr << "  (EE) Key unknown for checking signature from " << senders_id << ", can't verify signature. Using key provided in DH packet (without adding to the keyring)." << std::endl;

	    // check GXS key for defects.

	    if(!GxsSecurity::checkPublicKey(item->gxs_key))
	    {
		    std::cerr << "(SS) Security error in distant chat DH handshake: supplied key " << item->gxs_key.keyId << " is inconsistent. Refusing chat!" << std::endl;
		    return ;
	    }
	    if(item->gxs_key.keyId != item->signature.keyId)
	    {
		    std::cerr << "(SS) Security error in distant chat DH handshake: supplied key " << item->gxs_key.keyId << " is not the same than the item's signature key " << item->signature.keyId << ". Refusing chat!" << std::endl;
		    return ;
	    }

	    signature_key = item->gxs_key ;
    }

    if(!GxsSecurity::validateSignature((char*)data,pubkey_size,signature_key,item->signature))
    {
        std::cerr << "(SS) Signature was verified and it doesn't check! This is a security issue!" << std::endl;
        return ;
    }
    mGixs->timeStampKey(item->signature.keyId) ;

#ifdef DEBUG_GXS_TUNNEL
    std::cerr << "  Signature checks! Sender's ID = " << senders_id << std::endl;
    std::cerr << "  Computing AES key" << std::endl;
#endif

    if(it->second.dh == NULL)
    {
        std::cerr << "  (EE) no DH information for that peer. This is an error." << std::endl;
        return ;
    }
    if(it->second.status == RS_GXS_TUNNEL_DH_STATUS_KEY_AVAILABLE)
    {
#ifdef DEBUG_GXS_TUNNEL
        std::cerr << "  DH Session already set for this tunnel. Re-initing a new session!" << std::endl;
#endif

        locked_restartDHSession(vpid,_gxs_tunnel_contacts[senders_id].own_gxs_id) ;
    }

    // gets current key params. By default, should contain all null pointers.
    //
    it->second.gxs_id = senders_id ;

    // Looks for the DH params. If not there yet, create them.
    //
    int size = DH_size(it->second.dh) ;
    unsigned char *key_buff = new unsigned char[size] ;

    if(size != DH_compute_key(key_buff,item->public_key,it->second.dh))
    {
        std::cerr << "  (EE) DH computation failed. Probably a bug. Error code=" << ERR_get_error() << std::endl;
        return ;
    }
    it->second.status = RS_GXS_TUNNEL_DH_STATUS_KEY_AVAILABLE ;

#ifdef DEBUG_GXS_TUNNEL
    std::cerr << "  DH key computation successed. New key in place." << std::endl;
#endif
    GxsTunnelPeerInfo& pinfo(_gxs_tunnel_contacts[senders_id]) ;

    // Now hash the key buffer into a 16 bytes key.

    assert(GXS_TUNNEL_AES_KEY_SIZE <= Sha1CheckSum::SIZE_IN_BYTES) ;
    memcpy(pinfo.aes_key, RsDirUtil::sha1sum(key_buff,size).toByteArray(),GXS_TUNNEL_AES_KEY_SIZE) ;
    delete[] key_buff ;

    pinfo.last_contact = time(NULL) ;
    pinfo.last_keep_alive_sent = time(NULL) ;
    pinfo.status = RS_GXS_TUNNEL_STATUS_CAN_TALK ;
    pinfo.virtual_peer_id = vpid ;
    pinfo.direction = it->second.direction ;

    if(pinfo.direction == RsTurtleGenericTunnelItem::DIRECTION_CLIENT)
        pinfo.own_gxs_id = gxsIdFromHash(it->second.hash) ;

#ifdef DEBUG_GXS_TUNNEL
    std::cerr << "  DH key computed. Tunnel is now secured!" << std::endl;
    std::cerr << "  Key computed: " << Bin2Hex(pinfo.aes_key,16) << std::cerr << std::endl;
    std::cerr << "  Sending a ACK packet." << std::endl;
#endif

    // then we send an ACK packet to notify that the tunnel works. That's useful
    // because it makes the peer at the other end of the tunnel know that all
    // intermediate peer in the tunnel are able to transmit the data.
    // However, it is not possible here to call sendTurtleData(), without dead-locking
    // the turtle router, so we store the item is a list of items to be sent.

    RsGxsTunnelStatusItem *cs = new RsGxsTunnelStatusItem ;

    cs->flags = RS_GXS_TUNNEL_FLAG_ACK_DISTANT_CONNECTION;
    cs->PeerId(RsPeerId(senders_id));

    pendingGxsTunnelItems.push_back(cs) ;

#warning should notify client here
    //RsServer::notify()->notifyListChange(NOTIFY_LIST_PRIVATE_INCOMING_CHAT, NOTIFY_TYPE_ADD);
}

bool GxsTunnelService::locked_sendDHPublicKey(const DH *dh,const RsGxsId& own_gxs_id,const RsPeerId& virtual_peer_id)
{
	if(dh == NULL)
	{
		std::cerr << "  (EE) DH struct is not initialised! Error." << std::endl;
		return false ;
	}

	RsGxsTunnelDHPublicKeyItem *dhitem = new RsGxsTunnelDHPublicKeyItem ;
	dhitem->public_key = BN_dup(dh->pub_key) ;

	// we should also sign the data and check the signature on the other end.
	//
	RsTlvKeySignature signature ;
	RsTlvSecurityKey  signature_key ;
	RsTlvSecurityKey  signature_key_public ;

	uint32_t error_status ;

	uint32_t size = BN_num_bytes(dhitem->public_key) ;
	unsigned char *data = (unsigned char *)malloc(size) ;
	BN_bn2bin(dhitem->public_key, data) ;

	if(!mGixs->signData((unsigned char*)data,size,own_gxs_id,signature,error_status))
	{
		switch(error_status)
		{
		case RsGixs::RS_GIXS_ERROR_KEY_NOT_AVAILABLE: std::cerr << "(EE) Key is not available. Cannot sign." << std::endl;
			break ;
		default: std::cerr << "(EE) Unknown error when signing" << std::endl;
			break ;
		}
		free(data) ;
		delete(dhitem);
		return false;
	}
	free(data) ;

	if(!mGixs->getKey(own_gxs_id,signature_key_public))
	{
		std::cerr << "  (EE) Could not retrieve own public key for ID = " << own_gxs_id << ". Giging up sending DH session params." << std::endl;
		return false ;
	}


	assert(!(signature_key_public.keyFlags & RSTLV_KEY_TYPE_FULL)) ;

	dhitem->signature = signature ;
	dhitem->gxs_key = signature_key_public ;
	dhitem->PeerId(RsPeerId(virtual_peer_id)) ;	// special case for DH items

#ifdef DEBUG_GXS_TUNNEL
	std::cerr << "  Pushing DH session key item to pending distant messages..." << std::endl;
	dhitem->print(std::cerr, 2) ;
	std::cerr << std::endl;
#endif
	pendingGxsTunnelItems.push_back(dhitem) ; // sent off-mutex to avoid deadlocking.

	return true ;
}

bool GxsTunnelService::locked_initDHSessionKey(DH *& dh)
{
    // We use our own DH group prime. This has been generated with command-line openssl and checked.
    
    static const std::string dh_prime_2048_hex = "B3B86A844550486C7EA459FA468D3A8EFD71139593FE1C658BBEFA9B2FC0AD2628242C2CDC2F91F5B220ED29AAC271192A7374DFA28CDDCA70252F342D0821273940344A7A6A3CB70C7897A39864309F6CAC5C7EA18020EF882693CA2C12BB211B7BA8367D5A7C7252A5B5E840C9E8F081469EBA0B98BCC3F593A4D9C4D5DF539362084F1B9581316C1F80FDAD452FD56DBC6B8ED0775F596F7BB22A3FE2B4753764221528D33DB4140DE58083DB660E3E105123FC963BFF108AC3A268B7380FFA72005A1515C371287C5706FFA6062C9AC73A9B1A6AC842C2764CDACFC85556607E86611FDF486C222E4896CDF6908F239E177ACC641FCBFF72A758D1C10CBB" ;

    if(dh != NULL)
    {
        DH_free(dh) ;
        dh = NULL ;
    }

    dh = DH_new() ;

    if(!dh)
    {
        std::cerr << "  (EE) DH_new() failed." << std::endl;
        return false ;
    }

    BN_hex2bn(&dh->p,dh_prime_2048_hex.c_str()) ;
    BN_hex2bn(&dh->g,"5") ;

    int codes = 0 ;

    if(!DH_check(dh, &codes) || codes != 0)
    {
        std::cerr << "  (EE) DH check failed!" << std::endl;
        return false ;
    }

    if(!DH_generate_key(dh))
    {
        std::cerr << "  (EE) DH generate_key() failed! Error code = " << ERR_get_error() << std::endl;
        return false ;
    }
#ifdef DEBUG_GXS_TUNNEL
    std::cerr << "  (II) DH Session key inited." << std::endl;
#endif
    return true ;
}

// Encrypts and sends the item.

void p3GxsTunnelService::sendTurtleData(RsGxsTunnelItem *item)
{
#ifdef DEBUG_GXS_TUNNEL
    std::cerr << "GxsTunnelService::sendTurtleData(): try sending item " << (void*)item << " to peer " << item->PeerId() << std::endl;
#endif

    if(dynamic_cast<RsGxsTunnelDHPublicKeyItem*>(item) != NULL)
    {
        // make a TurtleGenericData item out of it, and send it in clear.
        //
        RsTurtleGenericDataItem *gitem = new RsTurtleGenericDataItem ;

    uint32_t rssize = item->serial_size() ;

        gitem->data_size  = rssize + 8 ;
        gitem->data_bytes = malloc(rssize+8) ;

        // by convention, we use a IV of 0 for unencrypted data.
        memset(gitem->data_bytes,0,8) ;

        if(!item->serialise(&((uint8_t*)gitem->data_bytes)[8],rssize))
        {
            std::cerr << "(EE) Could not serialise item!!!" << std::endl;
            delete gitem ;
            delete item ;
            return ;
        }

#ifdef DEBUG_GXS_TUNNEL
        std::cerr << "   GxsTunnelService::sendTurtleData(): Sending clear data to virtual peer: " << item->PeerId() << std::endl;
        std::cerr << "     gitem->data_size = " << gitem->data_size << std::endl;
        std::cerr << "     data = " << Bin2Hex(gitem->data_bytes,gitem->data_size) ;
        std::cerr << std::endl;
#endif
        mTurtle->sendTurtleData(item->PeerId(),gitem) ;
    }
    else
    {
        uint32_t rssize = item->serial_size();
        uint8_t *buff = (uint8_t*)malloc(rssize) ;

        if(!item->serialise(buff,rssize))
        {
            std::cerr << "(EE) GxsTunnelService::sendTurtleData(): Could not serialise item!" << std::endl;
            free(buff) ;
            delete item ;
            return ;
        }

        sendEncryptedTurtleData(buff,rssize,RsGxsId(item->PeerId())) ;

        free(buff) ;
    }
    delete item ;
}

void p3GxsTunnelService::sendEncryptedTurtleData(const uint8_t *buff,uint32_t rssize,const RsGxsId& gxs_id)
{
    uint8_t aes_key[GXS_TUNNEL_AES_KEY_SIZE] ;
    uint64_t IV = 0;

#ifdef DEBUG_GXS_TUNNEL
    std::cerr << "Sending encrypted data to virtual gxs id " << gxs_id << std::endl;
#endif
    RsStackMutex stack(mGxsTunnelMtx); /********** STACK LOCKED MTX ******/

    std::map<RsGxsId,GxsTunnelPeerInfo>::iterator it = _gxs_tunnel_contacts.find(gxs_id) ;

    if(it == _gxs_tunnel_contacts.end())
    {
        std::cerr << "(EE) Cannot find contact key info for ID " << gxs_id << ". Cannot send message!" << std::endl;
        return ;
    }
    if(it->second.status != RS_GXS_TUNNEL_STATUS_CAN_TALK)
    {
        std::cerr << "(EE) Cannot talk to " << gxs_id << ". Tunnel status is: " << it->second.status << std::endl;
        return ;
    }

    memcpy(aes_key,it->second.aes_key,GXS_TUNNEL_AES_KEY_SIZE) ;
    RsPeerId virtual_peer_id = it->second.virtual_peer_id ;

    while(IV == 0) IV = RSRandom::random_u64() ; // make a random 8 bytes IV, that is not 0

#ifdef DEBUG_GXS_TUNNEL
    std::cerr << "GxsTunnelService::sendTurtleData(): tunnel found. Encrypting data." << std::endl;
#endif

    // Now encrypt this data using AES.
    //
    uint8_t *encrypted_data = new uint8_t[RsAES::get_buffer_size(rssize)];
    uint32_t encrypted_size = RsAES::get_buffer_size(rssize);

    if(!RsAES::aes_crypt_8_16(buff,rssize,aes_key,(uint8_t*)&IV,encrypted_data,encrypted_size))
    {
        std::cerr << "(EE) packet encryption failed." << std::endl;
        delete[] encrypted_data ;
        return ;
    }

    // make a TurtleGenericData item out of it:
    //
    RsTurtleGenericDataItem *gitem = new RsTurtleGenericDataItem ;

    gitem->data_size  = encrypted_size + GXS_TUNNEL_ENCRYPTION_IV_SIZE + GXS_TUNNEL_HMAC_SIZE ;
    gitem->data_bytes = malloc(gitem->data_size) ;

    memcpy(& (((uint8_t*)gitem->data_bytes)[0]                                       ,&IV,8) ;

    unsigned int md_len = GXS_TUNNEL_HMAC_SIZE ;
    HMAC(EVP_sha1(),aes_key,GXS_TUNNEL_AES_KEY_SIZE,encrypted_data,encrypted_size,&(((uint8_t*)gitem->data_bytes)[GXS_TUNNEL_IV_SIZE]),&md_len) ;
    
    memcpy(& (((uint8_t*)gitem->data_bytes)[GXS_TUNNEL_HMAC_SIZE+GXS_TUNNEL_IV_SIZE]),encrypted_data,encrypted_size) ;
    
    delete[] encrypted_data ;

#ifdef DEBUG_GXS_TUNNEL
    std::cerr << "   Using  IV: " << std::hex << IV << std::dec << std::endl;
    std::cerr << "   Using Key: " << Bin2Hex(aes_key,GXS_TUNNEL_AES_KEY_SIZE) ; std::cerr << std::endl;
    std::cerr << "        hmac: " << Bin2Hex(gitem->data_bytes,GXS_TUNNEL_HMAC_SIZE) ;
#endif
#ifdef DEBUG_GXS_TUNNEL
    std::cerr << "GxsTunnelService::sendTurtleData(): Sending encrypted data to virtual peer: " << virtual_peer_id << std::endl;
    std::cerr << "   gitem->data_size = " << gitem->data_size << std::endl;
    std::cerr << "    serialised data = " << Bin2Hex(gitem->data_bytes,gitem->data_size) ;
    std::cerr << std::endl;
#endif

    mTurtle->sendTurtleData(virtual_peer_id,gitem) ;
}

bool p3GxsTunnelService::initiateGxsTunnelConnexion(const RsGxsId& to_gxs_id,const RsGxsId& from_gxs_id,uint32_t& error_code)
{
    // should be a parameter.
	
    std::list<RsGxsId> lst ;
    mGixs->getOwnIds(lst) ;

    bool found = false ;
    for(std::list<RsGxsId>::const_iterator it = lst.begin();it!=lst.end();++it)
        if(*it == from_gxs_id)
        {
            found=true;
            break ;
        }

    if(!found)
    {
        std::cerr << "  (EE) Cannot start distant chat, since GXS id " << from_gxs_id << " is not available." << std::endl;
        return false ;
    }
    RsGxsId own_gxs_id = from_gxs_id ;

    startClientGxsTunnelConnection(to_gxs_id,own_gxs_id) ;

    error_code = RS_GXS_TUNNEL_ERROR_NO_ERROR ;

    return true ;
}

void p3GxsTunnelService::startClientGxsTunnelConnection(const RsGxsId& to_gxs_id,const RsGxsId& from_gxs_id)
{
    {
        RsStackMutex stack(mGxsTunnelMtx); /********** STACK LOCKED MTX ******/

        if(_gxs_tunnel_contacts.find(to_gxs_id) != _gxs_tunnel_contacts.end())
        {
            std::cerr << "GxsTunnelService:: asking distant chat connexion to a peer who's already in a chat. Ignoring." << std::endl;
            return ;
        }
    }
    GxsTunnelPeerInfo info ;

    time_t now = time(NULL) ;

    info.last_contact = now ;
    info.last_keep_alive_sent = now ;
    info.status = RS_GXS_TUNNEL_STATUS_TUNNEL_DN ;
    info.own_gxs_id = from_gxs_id ;
    info.direction = RsTurtleGenericTunnelItem::DIRECTION_SERVER ;
    info.virtual_peer_id.clear();

    memset(info.aes_key,0,GXS_TUNNEL_AES_KEY_SIZE) ;

    {
        RsStackMutex stack(mGxsTunnelMtx); /********** STACK LOCKED MTX ******/
        _gxs_tunnel_contacts[to_gxs_id] = info ;
    }

    // Now ask the turtle router to manage a tunnel for that hash.

    RsFileHash hash = hashFromGxsId(to_gxs_id) ;
#ifdef DEBUG_GXS_TUNNEL
    std::cerr << "Starting distant chat to " << to_gxs_id << ", hash = " << hash << ", from " << from_gxs_id << std::endl;
    std::cerr << "Asking turtle router to monitor tunnels for hash " << hash << std::endl;
#endif

    mTurtle->monitorTunnels(hash,this,false) ;

#warning check that this code should go.
#ifdef TO_BE_REMOVED
    // spawn a status item so as to open the chat window.
    RsGxsTunnelMsgItem *item = new RsGxsTunnelMsgItem;
    item->message = "[Starting distant chat. Please wait for secure tunnel to be established]" ;
    item->chatFlags = RS_CHAT_FLAG_PRIVATE ;
    item->PeerId(RsPeerId(to_gxs_id)) ;
    handleRecvGxsTunnelMsgItem(item) ;
#endif
}

TurtleFileHash p3GxsTunnelService::hashFromGxsId(const RsGxsId& destination)
{
    // This is in prevision for the "secured GXS tunnel" service, which will need a service ID to register,
    // just like GRouter does.

    static const uint32_t client = RS_SERVICE_TYPE_GXS_TUNNEL ;

    assert(  destination.SIZE_IN_BYTES == 16) ;
    assert(Sha1CheckSum::SIZE_IN_BYTES == 20) ;

    uint8_t bytes[20] ;
    memcpy(bytes,destination.toByteArray(),16) ;
    bytes[16] = 0 ;
    bytes[17] = 0 ;
    bytes[18] = (client >> 8) & 0xff ;
    bytes[19] =  client       & 0xff ;

    // We could rehash this, with a secret key to get a HMAC. That would allow to publish secret distant chat
    // passphrases. I'll do this later if needed.

    return Sha1CheckSum(bytes) ;
}

RsGxsId p3GxsTunnelService::gxsIdFromHash(const TurtleFileHash& sum)
{
    assert(     RsGxsId::SIZE_IN_BYTES == 16) ;
    assert(Sha1CheckSum::SIZE_IN_BYTES == 20) ;

    uint32_t client_id = sum.toByteArray()[19] + (sum.toByteArray()[18] << 8) ;

    if(client_id != RS_SERVICE_TYPE_GXS_TUNNEL)
        std::cerr << "WARNING: GxsTunnelService::gxsIdFromHash(). Hash is not a distant file hash. This should not happen." << std::endl;

    return RsGxsId(sum.toByteArray());// takes the first 16 bytes
}

bool p3GxsTunnelService::getGxsTunnelStatus(const RsGxsId& gxs_id,uint32_t& status, RsGxsId *from_gxs_id)
{
    RsStackMutex stack(mGxsTunnelMtx); /********** STACK LOCKED MTX ******/

    std::map<RsGxsId,GxsTunnelPeerInfo>::const_iterator it = _gxs_tunnel_contacts.find(gxs_id) ;

    if(it != _gxs_tunnel_contacts.end())
    {
        status = it->second.status ;

        if(from_gxs_id != NULL)
            *from_gxs_id = it->second.own_gxs_id ;

        return true ;
    }

    status = RS_GXS_TUNNEL_STATUS_UNKNOWN ;

    return false ;
}

bool p3GxsTunnelService::closeGxsTunnelConnexion(const RsGxsId& gxs_id)
{
	// two cases: 
	// 	- client needs to stop asking for tunnels => remove the hash from the list of tunnelled files
	// 	- server needs to only close the window and let the tunnel die. But the window should only open 
	// 	  if a message arrives.

    {
        RsStackMutex stack(mGxsTunnelMtx); /********** STACK LOCKED MTX ******/
        std::map<RsGxsId,GxsTunnelPeerInfo>::const_iterator it = _gxs_tunnel_contacts.find(gxs_id) ;

        if(it == _gxs_tunnel_contacts.end())
        {
            std::cerr << "(EE) Cannot close distant tunnel connection. No connection openned for gxs id " << gxs_id << std::endl;

        // We don't know if we are server or client side, but mTurtle will not complain if the hash doesn't exist.

        mTurtle->stopMonitoringTunnels( hashFromGxsId(gxs_id) );

            return false ;
        }
        if(it->second.direction == RsTurtleGenericTunnelItem::DIRECTION_CLIENT) 	// nothing more to do for server side.
            return true ;
    }

    // send a status item saying that we're closing the connection
#ifdef DEBUG_GXS_TUNNEL
    std::cerr << "  Sending a ACK to close the tunnel since we're managing it. Peer id=." << gxs_id << std::endl;
#endif

    RsGxsTunnelStatusItem *cs = new RsGxsTunnelStatusItem ;

    cs->flags = RS_CHAT_FLAG_CLOSING_DISTANT_CONNECTION;
    cs->PeerId(RsPeerId(gxs_id));

    sendTurtleData(cs) ;	// that needs to be done off-mutex and before we close the tunnel.

#ifdef DEBUG_GXS_TUNNEL
    std::cerr << "  This is client side. Stopping tunnel manageement for gxs_id " << gxs_id << std::endl;
#endif
    mTurtle->stopMonitoringTunnels( hashFromGxsId(gxs_id) );
    {
	    RsStackMutex stack(mGxsTunnelMtx); /********** STACK LOCKED MTX ******/
	    std::map<RsGxsId,GxsTunnelPeerInfo>::iterator it = _gxs_tunnel_contacts.find(gxs_id) ;

	    if(it == _gxs_tunnel_contacts.end())		// server side. Nothing to do.
	    {
		    std::cerr << "(EE) Cannot close chat associated to GXS id " << gxs_id << ": not found." << std::endl;
		    return false ;
	    }

	    _gxs_tunnel_contacts.erase(it) ;

	    // GxsTunnelService::removeVirtualPeerId() will be called by the turtle service.
    }
    return true ;
}

void p3GxsTunnelService::markGxsTunnelAsClosed(const RsGxsId& gxs_id)
{
    RS_STACK_MUTEX(mGxsTunnelMtx); /********** STACK LOCKED MTX ******/

        std::map<RsGxsId,GxsTunnelPeerInfo>::iterator it = _gxs_tunnel_contacts.find(gxs_id) ;

        if(it == _gxs_tunnel_contacts.end())
        {
            std::cerr << "(EE) Cannot mark distant chat connection as closed. No connection openned for gxs id " << gxs_id << ". Unexpected situation." << std::endl;
            return ;
        }

    if(it->second.direction == RsTurtleGenericDataItem::DIRECTION_CLIENT)
    {
#ifdef DEBUG_GXS_TUNNEL
        std::cerr << "  This is server side. Marking distant chat as remotely closed for GXS id " << gxs_id << std::endl;
#endif
        it->second.status = RS_GXS_TUNNEL_STATUS_REMOTELY_CLOSED ;
    }
}

