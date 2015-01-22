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

#include <serialiser/rsmsgitems.h>

#include <retroshare/rsmsgs.h>
#include <retroshare/rsidentity.h>
#include <retroshare/rsiface.h>

#include <rsserver/p3face.h>
#include <services/p3idservice.h>
#include <gxs/gxssecurity.h>
#include <turtle/p3turtle.h>
#include <retroshare/rsids.h>
#include "distantchat.h"

//#define DEBUG_DISTANT_CHAT

static const uint32_t DISTANT_CHAT_KEEP_ALIVE_TIMEOUT = 15 ; // send keep alive packet so as to avoid tunnel breaks.

void DistantChatService::connectToTurtleRouter(p3turtle *tr)
{
	mTurtle = tr ;
	tr->registerTunnelService(this) ;
}

void DistantChatService::flush()
{
    // Flush items that could not be sent, probably because of a Mutex protected zone.
    //
    while(!pendingDistantChatItems.empty())
    {
        sendTurtleData( pendingDistantChatItems.front() ) ;
        pendingDistantChatItems.pop_front() ;
    }

    // TODO:  also sweep GXS id map and disable any ID with no virtual peer id in the list.

    RS_STACK_MUTEX(mDistantChatMtx); /********** STACK LOCKED MTX ******/

    time_t now = time(NULL) ;

    for(std::map<RsGxsId,DistantChatPeerInfo>::iterator it(_distant_chat_contacts.begin());it!=_distant_chat_contacts.end();++it)
    {
        if(it->second.last_contact+10+DISTANT_CHAT_KEEP_ALIVE_TIMEOUT < now && it->second.status == RS_DISTANT_CHAT_STATUS_CAN_TALK)
        {
            std::cerr << "(II) DistantChatService:: connexion interrupted with peer." << std::endl;
            it->second.status = RS_DISTANT_CHAT_STATUS_TUNNEL_DN ;
            it->second.virtual_peer_id.clear() ;
        }
        if(it->second.last_keep_alive_sent + DISTANT_CHAT_KEEP_ALIVE_TIMEOUT < now && it->second.status == RS_DISTANT_CHAT_STATUS_CAN_TALK)
        {
            RsChatStatusItem *cs = new RsChatStatusItem ;

            cs->status_string.clear() ;
            cs->flags = RS_CHAT_FLAG_PRIVATE | RS_CHAT_FLAG_KEEP_ALIVE;
            cs->PeerId(RsPeerId(it->first)) ;

            // we send off-mutex to avoid deadlock.

            pendingDistantChatItems.push_back(cs) ;

            it->second.last_keep_alive_sent = now ;

            std::cerr << "(II) DistantChatService:: Sending keep alive packet to gxs id " << it->first << std::endl;
        }
    }
}

bool DistantChatService::handleRecvItem(RsChatItem *item)
{
    if(item == NULL)
        return false ;

    switch(item->PacketSubType())
    {
    case RS_PKT_SUBTYPE_DISTANT_CHAT_DH_PUBLIC_KEY: handleRecvDHPublicKey(dynamic_cast<RsChatDHPublicKeyItem*>(item)) ; break ;
        return true ;

    case RS_PKT_SUBTYPE_CHAT_STATUS:
    {
        // Keep alive packets should not be forwarded to the GUI. It's just for keeping the tunnel up.

        RsChatStatusItem *csi = dynamic_cast<RsChatStatusItem*>(item) ;
        if(csi != NULL && csi->flags & RS_CHAT_FLAG_KEEP_ALIVE)
            return true ;
    }

    default:
        return false ;
    }

    return false ;
}
bool DistantChatService::handleOutgoingItem(RsChatItem *item)
{
    {
        RS_STACK_MUTEX(mDistantChatMtx) ;

        std::map<RsGxsId,DistantChatPeerInfo>::const_iterator it=_distant_chat_contacts.find(RsGxsId(item->PeerId()));

        if(it == _distant_chat_contacts.end())
            return false ;
    }

#ifdef CHAT_DEBUG
    std::cerr << "p3ChatService::handleOutgoingItem(): sending to " << item->PeerId() << ": interpreted as a distant chat virtual peer id." << std::endl;
#endif
    sendTurtleData(item) ;
    return true;
}

void DistantChatService::handleRecvChatStatusItem(RsChatStatusItem *cs)
{
    if(cs->flags & RS_CHAT_FLAG_CLOSING_DISTANT_CONNECTION)
        markDistantChatAsClosed(RsGxsId(cs->PeerId())) ;

    // nothing more to do, because the decryption routing will update the last_contact time when decrypting.

    if(cs->flags & RS_CHAT_FLAG_KEEP_ALIVE)
        std::cerr << "DistantChatService::handleRecvChatStatusItem(): received keep alive packet for inactive chat! peerId=" << cs->PeerId() << std::endl;
}

bool DistantChatService::handleTunnelRequest(const RsFileHash& hash,const RsPeerId& /*peer_id*/)
{
	RsStackMutex stack(mDistantChatMtx); /********** STACK LOCKED MTX ******/

	// look into owned GXS ids, and see if the hash corresponds to the expected hash
	//
	std::list<RsGxsId> own_id_list ;
	rsIdentity->getOwnIds(own_id_list) ;

	for(std::list<RsGxsId>::const_iterator it(own_id_list.begin());it!=own_id_list.end();++it)
		if(hashFromGxsId(*it) == hash)
		{
#ifdef DEBUG_DISTANT_CHAT
			std::cerr << "DistantChatService::handleTunnelRequest: received tunnel request for hash " << hash << std::endl;
			std::cerr << "  answering true!" << std::endl;
#endif
			return true ;
        }

	return false ;
}

void DistantChatService::addVirtualPeer(const TurtleFileHash& hash,const TurtleVirtualPeerId& virtual_peer_id,RsTurtleGenericTunnelItem::Direction dir)
{
#ifdef DEBUG_DISTANT_CHAT
    std::cerr << "DistantChatService:: received new virtual peer " << virtual_peer_id << " for hash " << hash << ", dir=" << dir << std::endl;
#endif
    RsGxsId own_gxs_id ;

    {
        RS_STACK_MUTEX(mDistantChatMtx); /********** STACK LOCKED MTX ******/

        if( _distant_chat_virtual_peer_ids.find(virtual_peer_id) != _distant_chat_virtual_peer_ids.end())
        {
            std::cerr << "  Tunnel already registered for " << hash << " and virtual peer " << virtual_peer_id << ". Doing nothing." << std::endl;
            return ;
        }

        if(dir == RsTurtleGenericTunnelItem::DIRECTION_CLIENT)
        {
            // check that a tunnel is not already working for this hash. If so, give up.

            own_gxs_id = gxsIdFromHash(hash) ;

        }
        else	// client side
        {
            RsGxsId to_gxs_id = gxsIdFromHash(hash) ;
            std::map<RsGxsId,DistantChatPeerInfo>::const_iterator it = _distant_chat_contacts.find(to_gxs_id) ;

            if(it == _distant_chat_contacts.end())
            {
                std::cerr << "(EE) no pre-registered peer for hash " << hash << " on client side. This is a bug." << std::endl;
                return ;
            }

				if(it->second.status == RS_DISTANT_CHAT_STATUS_CAN_TALK)
				{
					std::cerr << "  virtual peer is for a distant chat session that is already openned and alive. Giving it up." << std::endl;
					return ;
				}

            own_gxs_id = it->second.own_gxs_id ;
        }

		  std::cerr << "  Creating new virtual peer ID entry and empty DH session key." << std::endl;

        DistantChatDHInfo& dhinfo( _distant_chat_virtual_peer_ids[virtual_peer_id] ) ;
		  dhinfo.gxs_id.clear() ;
		  dhinfo.dh = NULL ;
		  dhinfo.direction = dir ;
    }

#ifdef DEBUG_DISTANT_CHAT
    std::cerr << "  Adding virtual peer " << virtual_peer_id << " for chat hash " << hash << std::endl;
#endif

    // Start a new DH session for this tunnel
    RS_STACK_MUTEX(mDistantChatMtx); /********** STACK LOCKED MTX ******/

    DH *& dh(_distant_chat_virtual_peer_ids[virtual_peer_id].dh) ;

#ifdef DEBUG_DISTANT_CHAT
    std::cerr << "  Starting new DH session." << std::endl;
#endif

    if(!locked_initDHSessionKey(dh))
    {
        std::cerr << "  (EE) Cannot start DH session. Something went wrong." << std::endl;
        return ;
    }

    if(!locked_sendDHPublicKey(dh,own_gxs_id,virtual_peer_id))
    {
        std::cerr << "  (EE) Cannot send DH public key. Something went wrong." << std::endl;
        return ;
    }
}

void DistantChatService::removeVirtualPeer(const TurtleFileHash& hash,const TurtleVirtualPeerId& virtual_peer_id)
{
    bool tunnel_dn = false ;

#ifdef DEBUG_DISTANT_CHAT
    std::cerr << "Distant chat: Removing virtual peer " << virtual_peer_id << " for hash " << hash << std::endl;
#endif
    {
        RsStackMutex stack(mDistantChatMtx); /********** STACK LOCKED MTX ******/

        RsGxsId gxs_id ;
        std::map<RsPeerId,DistantChatDHInfo>::iterator it = _distant_chat_virtual_peer_ids.find(virtual_peer_id) ;

        if(it == _distant_chat_virtual_peer_ids.end())
        {
            std::cerr << "(EE) Cannot remove virtual peer " << virtual_peer_id << ": not found in chat list!!" << std::endl;
            return ;
        }

        gxs_id = it->second.gxs_id ;

        if(it->second.dh != NULL)
            DH_free(it->second.dh) ;
        _distant_chat_virtual_peer_ids.erase(it) ;

        std::map<RsGxsId,DistantChatPeerInfo>::iterator it2 = _distant_chat_contacts.find(gxs_id) ;

        if(it2 == _distant_chat_contacts.end())
        {
            std::cerr << "(EE) Cannot find GXS id " << gxs_id << " in contact list. Weird." << std::endl;
            return ;
        }
        if(it2->second.virtual_peer_id == virtual_peer_id)
        {
            it2->second.status = RS_DISTANT_CHAT_STATUS_TUNNEL_DN ;
            it2->second.virtual_peer_id.clear() ;
            tunnel_dn = true ;
        }
    }

    if(tunnel_dn)
    {
        RsServer::notify()->notifyChatStatus(ChatId(RsGxsId(virtual_peer_id)),"tunnel is down...") ;
        RsServer::notify()->notifyPeerStatusChanged(virtual_peer_id.toStdString(),RS_STATUS_OFFLINE) ;
    }
}

#ifdef DEBUG_DISTANT_CHAT
static void printBinaryData(void *data,uint32_t size)
{
	static const char outl[16] = { '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f' } ;

    uint32_t mx = std::min(50u,size) ;

    for(uint32_t j = 0; j < mx; j++)
	{ 
		std::cerr << outl[ ( ((uint8_t*)data)[j]>>4) ] ; 
		std::cerr << outl[ ((uint8_t*)data)[j] & 0xf ] ; 
    }
    if(size > 50)
        std::cerr << "..." ;
}
#endif

void DistantChatService::receiveTurtleData(RsTurtleGenericTunnelItem *gitem,const RsFileHash& hash,
                       const RsPeerId& virtual_peer_id,RsTurtleGenericTunnelItem::Direction direction)
{
#ifdef DEBUG_DISTANT_CHAT
	std::cerr << "DistantChatService::receiveTurtleData(): Received turtle data. " << std::endl;
    std::cerr << "         hash = " << hash << std::endl;
    std::cerr << "         vpid = " << virtual_peer_id << std::endl;
    std::cerr << "    acting as = " << direction << std::endl;
#endif

	RsTurtleGenericDataItem *item = dynamic_cast<RsTurtleGenericDataItem*>(gitem) ;

	if(item == NULL)
	{
		std::cerr << "(EE) item is not a data item. That is an error." << std::endl;
		return ;
	}
#ifdef DEBUG_DISTANT_CHAT
	std::cerr << "   size = " << item->data_size << std::endl;
	std::cerr << "   data = " << (void*)item->data_bytes << std::endl;
	std::cerr << "     IV = " << std::hex << *(uint64_t*)item->data_bytes << std::dec << std::endl;
	std::cerr << "   data = " ;

	printBinaryData(item->data_bytes,item->data_size) ;
	std::cerr << std::endl;
#endif


	// Call the AES crypto module
	// - the IV is the first 8 bytes of item->data_bytes

	if(item->data_size < 8)
	{
		std::cerr << "(EE) item encrypted data stream is too small: size = " << item->data_size << std::endl;
		return ;
	}
    uint32_t decrypted_size = RsAES::get_buffer_size(item->data_size-8);
    uint8_t *decrypted_data = new uint8_t[decrypted_size];
    bool decrypted = false ;

    unsigned char no_key[DISTANT_CHAT_AES_KEY_SIZE] ;
    memset(no_key,0,DISTANT_CHAT_AES_KEY_SIZE) ;

    if(RsAES::aes_decrypt_8_16((uint8_t*)item->data_bytes+8,item->data_size-8,no_key,(uint8_t*)item->data_bytes,decrypted_data,decrypted_size))
    {
        decrypted = true ;

#ifdef DEBUG_DISTANT_CHAT
        std::cerr << "   Data is not encrypted. Probably a DH session key." << std::endl;
#endif
    }
    RsGxsId gxs_id ;

    if(!decrypted)
    {
        uint8_t aes_key[DISTANT_CHAT_AES_KEY_SIZE] ;
    {
        RS_STACK_MUTEX(mDistantChatMtx); /********** STACK LOCKED MTX ******/
        std::map<RsPeerId,DistantChatDHInfo>::const_iterator it = _distant_chat_virtual_peer_ids.find(virtual_peer_id) ;

        if(it == _distant_chat_virtual_peer_ids.end())
        {
            std::cerr << "(EE) item is not coming out of a registered tunnel. Weird. hash=" << hash << ", peer id = " << virtual_peer_id << std::endl;
            return ;
        }

        gxs_id = it->second.gxs_id ;
        std::map<RsGxsId,DistantChatPeerInfo>::iterator it2 = _distant_chat_contacts.find(gxs_id) ;

        if(it2 == _distant_chat_contacts.end())
        {
            std::cerr << "(EE) no GXS id data for ID=" << gxs_id << ". This is a bug." << std::endl;
            return ;
        }
        memcpy(aes_key,it2->second.aes_key,DISTANT_CHAT_AES_KEY_SIZE) ;

#ifdef DEBUG_DISTANT_CHAT
        std::cerr << "   Using IV: " << std::hex << *(uint64_t*)item->data_bytes << std::dec << std::endl;
        std::cerr << "   Decrypted buffer size: " << decrypted_size << std::endl;
        std::cerr << "   key  : " ; printBinaryData(aes_key,16) ; std::cerr << std::endl;
        std::cerr << "   data : " ; printBinaryData(item->data_bytes,item->data_size) ; std::cerr << std::endl;
#endif

        if(!RsAES::aes_decrypt_8_16((uint8_t*)item->data_bytes+8,item->data_size-8,aes_key,(uint8_t*)item->data_bytes,decrypted_data,decrypted_size))
        {
            std::cerr << "(EE) packet decryption failed." << std::endl;
            delete[] decrypted_data ;
            return ;
        }
        it2->second.status = RS_DISTANT_CHAT_STATUS_CAN_TALK ;
        it2->second.last_contact = time(NULL) ;
    }
    }

#ifdef DEBUG_DISTANT_CHAT
	std::cerr << "(II) Decrypted data: size=" << decrypted_size << std::endl;
#endif

	// Now try deserialise the decrypted data to make an RsItem out of it.
	//
	RsItem *citem = RsChatSerialiser().deserialise(decrypted_data,&decrypted_size) ;
	delete[] decrypted_data ;

	if(citem == NULL)
	{
		std::cerr << "(EE) item could not be de-serialized. That is an error." << std::endl;
		return ;
	}

        // DH key items are sent even before we know who we speak to, so the virtual peer id is used in this
        // case only.

        if(dynamic_cast<RsChatDHPublicKeyItem*>(citem) != NULL)
            citem->PeerId(virtual_peer_id) ;
        else
            citem->PeerId(RsPeerId(gxs_id)) ;

#ifdef DEBUG_DISTANT_CHAT
    std::cerr << "(II) Setting peer id to " << citem->PeerId() << std::endl;
#endif
	handleIncomingItem(citem) ; // Treats the item, and deletes it 
}

void DistantChatService::handleRecvDHPublicKey(RsChatDHPublicKeyItem *item)
{
#ifdef DEBUG_DISTANT_CHAT
	std::cerr << "DistantChatService:  Received DH public key." << std::endl;
	item->print(std::cerr, 0) ;
#endif

	// Look for the current state of the key agreement.

	TurtleVirtualPeerId vpid = item->PeerId() ;
	TurtleFileHash hash ;

    RS_STACK_MUTEX(mDistantChatMtx); /********** STACK LOCKED MTX ******/

    std::map<RsPeerId,DistantChatDHInfo>::iterator it = _distant_chat_virtual_peer_ids.find(vpid) ;

    if(it == _distant_chat_virtual_peer_ids.end())
	{
		std::cerr << "  (EE) Cannot find hash in distant chat peer list!!" << std::endl;
		return ;
	}

	// Now check the signature of the DH public key item. 
	
#ifdef DEBUG_DISTANT_CHAT
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

	mIdService->getIdDetails(senders_id,details);

	for(int i=0;i<6;++i)
		if(!mIdService->getKey(senders_id,signature_key) || signature_key.keyData.bin_data == NULL)
		{
#ifdef DEBUG_DISTANT_CHAT
			std::cerr << "  Cannot get key. Waiting for caching. try " << i << "/6" << std::endl;
#endif
			usleep(500 * 1000) ;	// sleep for 500 msec.
		}
		else
			break ;

	if(signature_key.keyData.bin_data == NULL)
	{
		std::cerr << "  (EE) Key unknown for checking signature from " << senders_id << ", can't verify signature." << std::endl;
		std::cerr << "       Using key provided in DH packet." << std::endl;

		signature_key = item->gxs_key ;

#warning At this point, we should check that the key Ids  match!!
    }
    else if(signature_key.keyId != item->gxs_key.keyId)
    {
        std::cerr << "(EE) DH session key is signed by an ID that is not the ID of the key provided inthe packet. Refusing distant chat with this peer." << std::endl;
        return;
    }

	if(!GxsSecurity::validateSignature((char*)data,pubkey_size,signature_key,item->signature))
	{
		std::cerr << "  (EE) Signature was verified and it doesn't check! This is a security issue!" << std::endl;
		return ;
	}
#ifdef DEBUG_DISTANT_CHAT
    std::cerr << "  Signature checks! Sender's ID = " << senders_id << std::endl;
	std::cerr << "  Computing AES key" << std::endl;
#endif

    if(it->second.dh == NULL)
    {
        std::cerr << "  (EE) no DH information for that peer. This is an error." << std::endl;
        return ;
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
#ifdef DEBUG_DISTANT_CHAT
    std::cerr << "  DH key computation successed. New key in place." << std::endl;
#endif

    DistantChatPeerInfo& pinfo(_distant_chat_contacts[senders_id]) ;

	// Now hash the key buffer into a 16 bytes key.

    assert(DISTANT_CHAT_AES_KEY_SIZE <= Sha1CheckSum::SIZE_IN_BYTES) ;
    memcpy(pinfo.aes_key, RsDirUtil::sha1sum(key_buff,size).toByteArray(),DISTANT_CHAT_AES_KEY_SIZE) ;
    delete[] key_buff ;

    pinfo.last_contact = time(NULL) ;
    pinfo.last_keep_alive_sent = time(NULL) ;
    pinfo.status = RS_DISTANT_CHAT_STATUS_CAN_TALK ;
    pinfo.virtual_peer_id = vpid ;
    pinfo.direction = it->second.direction ;

#ifdef DEBUG_DISTANT_CHAT
    std::cerr << "  DH key computed. Tunnel is now secured!" << std::endl;
    std::cerr << "  Key computed: " ; printBinaryData(pinfo.aes_key,16) ; std::cerr << std::endl;
    std::cerr << "  Sending a ACK packet." << std::endl;
#endif

	// then we send an ACK packet to notify that the tunnel works. That's useful
	// because it makes the peer at the other end of the tunnel know that all
	// intermediate peer in the tunnel are able to transmit the data.
    // However, it is not possible here to call sendTurtleData(), without dead-locking
	// the turtle router, so we store the item is a list of items to be sent.

	RsChatStatusItem *cs = new RsChatStatusItem ;

    cs->status_string = "Tunnel is secured with PFS session. ACK received. You can talk!" ;
	cs->flags = RS_CHAT_FLAG_PRIVATE | RS_CHAT_FLAG_ACK_DISTANT_CONNECTION;
    cs->PeerId(RsPeerId(senders_id));

	pendingDistantChatItems.push_back(cs) ;

	RsServer::notify()->notifyListChange(NOTIFY_LIST_PRIVATE_INCOMING_CHAT, NOTIFY_TYPE_ADD);
}

bool DistantChatService::locked_sendDHPublicKey(const DH *dh,const RsGxsId& own_gxs_id,const RsPeerId& virtual_peer_id)
{
	RsChatDHPublicKeyItem *dhitem = new RsChatDHPublicKeyItem ;

    if(dh == NULL)
	{
        std::cerr << "  (EE) DH struct is not initialised! Error." << std::endl;
        delete dhitem ;
		return false ;
	}

    dhitem->public_key = BN_dup(dh->pub_key) ;

	// we should also sign the data and check the signature on the other end.
	//
	RsTlvKeySignature signature ;
	RsTlvSecurityKey  signature_key ;
	RsTlvSecurityKey  signature_key_public ;

#ifdef DEBUG_DISTANT_CHAT
    std::cerr << "  Getting key material for signature with GXS id " << own_gxs_id << std::endl;
#endif
	// The following code is only here to force caching the keys. 
	//
	RsIdentityDetails details  ;
    mIdService->getIdDetails(own_gxs_id,details);

    int i ;
    for(i=0;i<6;++i)
        if(!mIdService->getPrivateKey(own_gxs_id,signature_key) || signature_key.keyData.bin_data == NULL)
        {
#ifdef DEBUG_DISTANT_CHAT
            std::cerr << "  Cannot get key. Waiting for caching. try " << i << "/6" << std::endl;
#endif
            usleep(500 * 1000) ;	// sleep for 500 msec.
        }
        else
            break ;

    if(i == 6)
    {
        std::cerr << "  (EE) Could not retrieve own private key for ID = " << own_gxs_id << ". Giging up sending DH session params." << std::endl;
        return false ;
    }

	GxsSecurity::extractPublicKey(signature_key,signature_key_public) ;

	assert(!(signature_key_public.keyFlags & RSTLV_KEY_TYPE_FULL)) ;

#ifdef DEBUG_DISTANT_CHAT
	std::cerr << "  Signing..." << std::endl;
#endif
	uint32_t size = BN_num_bytes(dhitem->public_key) ;
	unsigned char *data = (unsigned char *)malloc(size) ;
	BN_bn2bin(dhitem->public_key, data) ;

	if(!GxsSecurity::getSignature((char *)data,size,signature_key,signature))
	{
        std::cerr << "  (EE) Cannot sign for id " << own_gxs_id << ". Signature call failed." << std::endl;
		return false ;
	}

	free(data) ;

	dhitem->signature = signature ;
	dhitem->gxs_key = signature_key_public ;
    dhitem->PeerId(RsPeerId(virtual_peer_id)) ;	// special case for DH items

#ifdef DEBUG_DISTANT_CHAT
    std::cerr << "  Pushing DH session key item to pending distant messages..." << std::endl;
    dhitem->print(std::cerr, 2) ;
    std::cerr << std::endl;
#endif
    pendingDistantChatItems.push_back(dhitem) ; // sent off-mutex to avoid deadlocking.

	return true ;
}

bool DistantChatService::locked_initDHSessionKey(DH *& dh)
{
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
#ifdef DEBUG_DISTANT_CHAT
    std::cerr << "  (II) DH Session key inited." << std::endl;
#endif
    return true ;
}

void DistantChatService::sendTurtleData(RsChatItem *item)
{
#ifdef DEBUG_DISTANT_CHAT
    std::cerr << "DistantChatService::sendTurtleData(): try sending item " << (void*)item << " to peer " << item->PeerId() << std::endl;
#endif

	uint32_t rssize = item->serial_size();
	uint8_t *buff = new uint8_t[rssize] ;

	if(!item->serialise(buff,rssize))
	{
		std::cerr << "(EE) DistantChatService::sendTurtleData(): Could not serialise item!" << std::endl;
		delete[] buff ;
		return ;
	}
#ifdef DEBUG_DISTANT_CHAT
	std::cerr << "     Serialized item has size " << rssize << std::endl;
#endif
	
    uint8_t aes_key[DISTANT_CHAT_AES_KEY_SIZE] ;
    uint64_t IV ;
    TurtleVirtualPeerId virtual_peer_id ;

    if(dynamic_cast<RsChatDHPublicKeyItem*>(item))
    {
#ifdef DEBUG_DISTANT_CHAT
        std::cerr << "     Packet is a DH session key. Using Peer Id " << item->PeerId() << " as virtual peer id" << std::endl;
#endif
        virtual_peer_id = item->PeerId() ;

#ifdef DEBUG_DISTANT_CHAT
        std::cerr << "  Special item DH session key --> will be sent unencrypted." << std::endl ;
#endif
        memset(aes_key,0,DISTANT_CHAT_AES_KEY_SIZE) ;
        IV = 0 ;
    }
    else
    {
#ifdef DEBUG_DISTANT_CHAT
        std::cerr << "     Packet is a normal chat message. Sending to GXS id " << item->PeerId() << std::endl;
#endif
        RsStackMutex stack(mDistantChatMtx); /********** STACK LOCKED MTX ******/

        std::map<RsGxsId,DistantChatPeerInfo>::iterator it = _distant_chat_contacts.find(RsGxsId(item->PeerId())) ;

        if(it == _distant_chat_contacts.end())
        {
            std::cerr << "(EE) Cannot find contact key info for ID " << item->PeerId() << ". Cannot send message!" << std::endl;
            delete[] buff ;
            return ;
        }
        if(it->second.status != RS_DISTANT_CHAT_STATUS_CAN_TALK)
        {
            std::cerr << "(EE) Cannot talk to " << item->PeerId() << ". Tunnel status is: " << it->second.status << std::endl;
            delete[] buff ;
            return ;
        }

        memcpy(aes_key,it->second.aes_key,DISTANT_CHAT_AES_KEY_SIZE) ;
        virtual_peer_id = it->second.virtual_peer_id ;
        IV = RSRandom::random_u64() ; // make a random 8 bytes IV
        }
#ifdef DEBUG_DISTANT_CHAT
    std::cerr << "DistantChatService::sendTurtleData(): tunnel found. Encrypting data." << std::endl;
#endif

    // Now encrypt this data using AES.
    //
    uint8_t *encrypted_data = new uint8_t[RsAES::get_buffer_size(rssize)];
    uint32_t encrypted_size = RsAES::get_buffer_size(rssize);

#ifdef DEBUG_DISTANT_CHAT
    std::cerr << "   Using IV: " << std::hex << IV << std::dec << std::endl;
    std::cerr << "   Using Key: " ; printBinaryData(aes_key,16) ; std::cerr << std::endl;
#endif
    if(!RsAES::aes_crypt_8_16(buff,rssize,aes_key,(uint8_t*)&IV,encrypted_data,encrypted_size))
    {
        std::cerr << "(EE) packet encryption failed." << std::endl;
        delete[] encrypted_data ;
        delete[] buff ;
        return ;
    }
    delete[] buff ;

	// make a TurtleGenericData item out of it:
	//
	RsTurtleGenericDataItem *gitem = new RsTurtleGenericDataItem ;

	gitem->data_size  = encrypted_size + 8 ;
	gitem->data_bytes = malloc(gitem->data_size) ;

	memcpy(gitem->data_bytes  ,&IV,8) ;
	memcpy(& (((uint8_t*)gitem->data_bytes)[8]),encrypted_data,encrypted_size) ;

	delete[] encrypted_data ;
	delete item ;

#ifdef DEBUG_DISTANT_CHAT
	std::cerr << "DistantChatService::sendTurtleData(): Sending through virtual peer: " << virtual_peer_id << std::endl;
	std::cerr << "   gitem->data_size = " << gitem->data_size << std::endl;
	std::cerr << "   data = " ;

	printBinaryData(gitem->data_bytes,gitem->data_size) ;
	std::cerr << std::endl;
#endif

	mTurtle->sendTurtleData(virtual_peer_id,gitem) ;
}

bool DistantChatService::initiateDistantChatConnexion(const RsGxsId& to_gxs_id,const RsGxsId& from_gxs_id,uint32_t& error_code)
{
    // should be a parameter.
	
    std::list<RsGxsId> lst ;
    mIdService->getOwnIds(lst) ;

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

    startClientDistantChatConnection(to_gxs_id,own_gxs_id) ;

    error_code = RS_DISTANT_CHAT_ERROR_NO_ERROR ;

    return true ;
}

void DistantChatService::startClientDistantChatConnection(const RsGxsId& to_gxs_id,const RsGxsId& from_gxs_id)
{
    {
        RsStackMutex stack(mDistantChatMtx); /********** STACK LOCKED MTX ******/

        if(_distant_chat_contacts.find(to_gxs_id) != _distant_chat_contacts.end())
        {
            std::cerr << "DistantChatService:: asking distant chat connexion to a peer who's already in a chat. Ignoring." << std::endl;
            return ;
        }
    }
    DistantChatPeerInfo info ;

    time_t now = time(NULL) ;

    info.last_contact = now ;
    info.last_keep_alive_sent = now ;
    info.status = RS_DISTANT_CHAT_STATUS_TUNNEL_DN ;
    info.own_gxs_id = from_gxs_id ;
    info.direction = RsTurtleGenericTunnelItem::DIRECTION_SERVER ;
    info.virtual_peer_id.clear();

    memset(info.aes_key,0,DISTANT_CHAT_AES_KEY_SIZE) ;

    {
        RsStackMutex stack(mDistantChatMtx); /********** STACK LOCKED MTX ******/
        _distant_chat_contacts[to_gxs_id] = info ;
    }

    // Now ask the turtle router to manage a tunnel for that hash.

    RsFileHash hash = hashFromGxsId(to_gxs_id) ;
#ifdef DEBUG_DISTANT_CHAT
    std::cerr << "Starting distant chat to " << to_gxs_id << ", hash = " << hash << ", from " << from_gxs_id << std::endl;
    std::cerr << "Asking turtle router to monitor tunnels for hash " << hash << std::endl;
#endif

    mTurtle->monitorTunnels(hash,this,false) ;

    // spawn a status item so as to open the chat window.
    RsChatMsgItem *item = new RsChatMsgItem;
    item->message = "[Starting distant chat. Please wait for secure tunnel to be established]" ;
    item->chatFlags = RS_CHAT_FLAG_PRIVATE ;
    item->PeerId(RsPeerId(to_gxs_id)) ;
    handleRecvChatMsgItem(item) ;

}

TurtleFileHash DistantChatService::hashFromGxsId(const RsGxsId& gid)
{
    if(RsGxsId::SIZE_IN_BYTES > Sha1CheckSum::SIZE_IN_BYTES)
        std::cerr << __PRETTY_FUNCTION__ << ": Serious inconsistency error." << std::endl;

    assert(Sha1CheckSum::SIZE_IN_BYTES >= RsGxsId::SIZE_IN_BYTES) ;

    unsigned char tmp[Sha1CheckSum::SIZE_IN_BYTES] ;
    memset(tmp,0,Sha1CheckSum::SIZE_IN_BYTES) ;
    memcpy(tmp,gid.toByteArray(),RsGxsId::SIZE_IN_BYTES) ;

    return Sha1CheckSum(tmp);
}
RsGxsId DistantChatService::gxsIdFromHash(const TurtleFileHash& hash)
{
    if(RsGxsId::SIZE_IN_BYTES > Sha1CheckSum::SIZE_IN_BYTES)
        std::cerr << __PRETTY_FUNCTION__ << ": Serious inconsistency error." << std::endl;

    assert(Sha1CheckSum::SIZE_IN_BYTES >= RsGxsId::SIZE_IN_BYTES) ;

    return RsGxsId(hash.toByteArray());
}
bool DistantChatService::getDistantChatStatus(const RsGxsId& gxs_id,uint32_t& status)
{
    RsStackMutex stack(mDistantChatMtx); /********** STACK LOCKED MTX ******/

    std::map<RsGxsId,DistantChatPeerInfo>::const_iterator it = _distant_chat_contacts.find(gxs_id) ;

    if(it != _distant_chat_contacts.end())
    {
        status = it->second.status ;
        return true ;
    }

    status = RS_DISTANT_CHAT_STATUS_UNKNOWN ;
    return false ;
}

bool DistantChatService::closeDistantChatConnexion(const RsGxsId& gxs_id)
{
	// two cases: 
	// 	- client needs to stop asking for tunnels => remove the hash from the list of tunnelled files
	// 	- server needs to only close the window and let the tunnel die. But the window should only open 
	// 	  if a message arrives.

    {
        RsStackMutex stack(mDistantChatMtx); /********** STACK LOCKED MTX ******/
        std::map<RsGxsId,DistantChatPeerInfo>::const_iterator it = _distant_chat_contacts.find(gxs_id) ;

        if(it == _distant_chat_contacts.end())
        {
            std::cerr << "(EE) Cannot close distant chat connection. No connection openned for gxs id " << gxs_id << std::endl;

        // We don't know if we are server or client side, but mTurtle will not complain if the hash doesn't exist.

        mTurtle->stopMonitoringTunnels( hashFromGxsId(gxs_id) );

            return false ;
        }
        if(it->second.direction == RsTurtleGenericTunnelItem::DIRECTION_CLIENT) 	// nothing more to do for server side.
            return true ;
    }

    // send a status item saying that we're closing the connection
#ifdef DEBUG_DISTANT_CHAT
    std::cerr << "  Sending a ACK to close the tunnel since we're managing it. Peer id=." << gxs_id << std::endl;
#endif

    RsChatStatusItem *cs = new RsChatStatusItem ;

    cs->status_string = "" ;
    cs->flags = RS_CHAT_FLAG_PRIVATE | RS_CHAT_FLAG_CLOSING_DISTANT_CONNECTION;
    cs->PeerId(RsPeerId(gxs_id));

    sendTurtleData(cs) ;	// that needs to be done off-mutex and before we close the tunnel.

#ifdef DEBUG_DISTANT_CHAT
    std::cerr << "  This is client side. Stopping tunnel manageement for gxs_id " << gxs_id << std::endl;
#endif
    mTurtle->stopMonitoringTunnels( hashFromGxsId(gxs_id) );
    {
        RsStackMutex stack(mDistantChatMtx); /********** STACK LOCKED MTX ******/
        std::map<RsGxsId,DistantChatPeerInfo>::iterator it = _distant_chat_contacts.find(gxs_id) ;

        if(it == _distant_chat_contacts.end())		// server side. Nothing to do.
        {
            std::cerr << "(EE) Cannot close chat associated to GXS id " << gxs_id << ": not found." << std::endl;
            return false ;
        }

        // remove virtual peer from list
        std::map<RsPeerId,DistantChatDHInfo>::iterator it2 = _distant_chat_virtual_peer_ids.find(it->second.virtual_peer_id) ;

        if(it2 == _distant_chat_virtual_peer_ids.end())
    {
        std::cerr << "(WW) Cannot remove virtual peer id " << it->second.virtual_peer_id << ": unknown! Weird situation." << std::endl;
        return true ;
    }

        DH_free(it2->second.dh) ;

        _distant_chat_virtual_peer_ids.erase(it2) ;
    _distant_chat_contacts.erase(it) ;
    }
    return true ;
}

void DistantChatService::markDistantChatAsClosed(const RsGxsId& gxs_id)
{
    RS_STACK_MUTEX(mDistantChatMtx); /********** STACK LOCKED MTX ******/

        std::map<RsGxsId,DistantChatPeerInfo>::iterator it = _distant_chat_contacts.find(gxs_id) ;

        if(it == _distant_chat_contacts.end())
        {
            std::cerr << "(EE) Cannot mark distant chat connection as closed. No connection openned for gxs id " << gxs_id << ". Unexpected situation." << std::endl;
            return ;
        }

    if(it->second.direction == RsTurtleGenericDataItem::DIRECTION_CLIENT)
        it->second.status = RS_DISTANT_CHAT_STATUS_REMOTELY_CLOSED ;
}

