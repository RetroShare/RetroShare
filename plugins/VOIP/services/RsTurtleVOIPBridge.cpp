/*******************************************************************************
 * plugins/VOIP/services/RsTurtleVOIPBridge.cpp                                 *
 *                                                                             *
 * Copyright 2026 by retroshare team                                           *
 *******************************************************************************/

#include "RsTurtleVOIPBridge.h"
#include <retroshare/rschats.h>
#include "services/p3VOIP.h"
#include "turtle/p3turtle.h"
#include "turtle/rsturtleitem.h"
#include "util/rsdebug.h"
#include "util/rsrandom.h"

#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/evp.h>

#define VOIP_HASH_MAGIC_XOR 0xAA

RsTurtleVOIPBridge::RsTurtleVOIPBridge(p3VOIP *voip_service, RsIdentity* ident, RsChats* chats)
    : mVOIP(voip_service), mTurtleRouter(NULL), mIdentity(ident), mChats(chats), mMutex("RsTurtleVOIPBridge")
{
    RsDbg() << "DISTANT_VOIP: RsTurtleVOIPBridge Instance Created.";
}

RsTurtleVOIPBridge::~RsTurtleVOIPBridge()
{
    RsStackMutex lock(mMutex);
    RsDbg() << "DISTANT_VOIP: Destructor cleaning up " << mActiveTunnels.size() << " active tunnels.";
    for(std::map<TurtleVirtualPeerId, VOIPTunnelState>::iterator it = mActiveTunnels.begin(); it != mActiveTunnels.end(); ++it) {
        cleanupTunnelState(it->second);
    }
    mActiveTunnels.clear();
}

uint16_t RsTurtleVOIPBridge::serviceId() const
{
    return RS_SERVICE_TYPE_VOIP_PLUGIN;
}

void RsTurtleVOIPBridge::cleanupTunnelState(VOIPTunnelState& state)
{
    if (state.dhObj != NULL) {
        DH_free(state.dhObj);
        state.dhObj = NULL;
    }
}

bool RsTurtleVOIPBridge::initDHSessionKey(DH *& dh)
{
    static const std::string dh_prime_2048_hex = "B3B86A844550486C7EA459FA468D3A8EFD71139593FE1C658BBEFA9B2FC0AD2628242C2CDC2F91F5B220ED29AAC271192A7374DFA28CDDCA70252F342D0821273940344A7A6A3CB70C7897A39864309F6CAC5C7EA18020EF882693CA2C12BB211B7BA8367D5A7C7252A5B5E840C9E8F081469EBA0B98BCC3F593A4D9C4D5DF539362084F1B9581316C1F80FDAD452FD56DBC6B8ED0775F596F7BB22A3FE2B4753764221528D33DB4140DE58083DB660E3E105123FC963BFF108AC3A268B7380FFA72005A1515C371287C5706FFA6062C9AC73A9B1A6AC842C2764CDACFC85556607E86611FDF486C222E4896CDF6908F239E177ACC641FCBFF72A758D1C10CBB" ;
    if(dh != NULL) { DH_free(dh); dh = NULL; }
    dh = DH_new();
    if(!dh) {
        RsDbg() << "DISTANT_VOIP: Failed to allocate OpenSSL DH object!";
        return false;
    }

#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
    BN_hex2bn(&dh->p,dh_prime_2048_hex.c_str()) ; BN_hex2bn(&dh->g,"5") ;
#else
    BIGNUM *pp=NULL ; BIGNUM *gg=NULL ;
    BN_hex2bn(&pp,dh_prime_2048_hex.c_str()) ; BN_hex2bn(&gg,"5") ;
    DH_set0_pqg(dh,pp,NULL,gg) ;
#endif

    int codes = 0 ;
    if(!DH_check(dh, &codes) || codes != 0) {
        RsDbg() << "DISTANT_VOIP: OpenSSL DH parameter check failed! Code=" << codes;
        return false ;
    }
    if(!DH_generate_key(dh)) {
        RsDbg() << "DISTANT_VOIP: OpenSSL DH key generation failed!";
        return false ;
    }
    
    RsDbg() << "DISTANT_VOIP: DH Local Keypair successfully built.";
    return true ;
}

void RsTurtleVOIPBridge::connectToTurtleRouter(p3turtle *pt)
{
    if (mTurtleRouter != NULL) return; // Already active

    mTurtleRouter = pt;
    if (mTurtleRouter) {
        mTurtleRouter->registerTunnelService(this);
        RsDbg() << "DISTANT_VOIP: Registered successfully to Turtle Router.";
    } else {
        RsDbg() << "DISTANT_VOIP: ERROR - connectToTurtleRouter passed NULL!";
    }
}

RsPeerId RsTurtleVOIPBridge::getOrCreateTunnelForChat(const ChatId& chatId)
{
    RsDbg() << "DISTANT_VOIP: getOrCreateTunnelForChat triggered for chatId=" << chatId.toStdString();

    // ULTIMATE FALLBACK: If local pointer failed, capture the central global handle!
    if (mChats == NULL) {
        RsDbg() << "DISTANT_VOIP: Wire pointer is NULL. Attempting emergency bind to GLOBAL rsChats...";
        mChats = rsChats; 
    }

    if (!chatId.isDistantChatId() || mChats == NULL) {
        RsDbg() << "DISTANT_VOIP: Rejecting request. isDistant=" << (chatId.isDistantChatId()?"yes":"no") << ", mChatsReady=" << (mChats!=NULL?"yes":"no");
        return RsPeerId();
    }
    
    DistantChatPeerInfo info;
    if (!mChats->getDistantChatStatus(chatId.toDistantChatId(), info)) {
        RsDbg() << "DISTANT_VOIP: FAILED to resolve distant chat state from mChats singleton.";
        return RsPeerId();
    }
    
    RsDbg() << "DISTANT_VOIP: Resolved target identity: " << info.to_id.toStdString();

    RsStackMutex lock(mMutex);
    // 1. Check existing active tunnels
    for(std::map<TurtleVirtualPeerId, VOIPTunnelState>::iterator it = mActiveTunnels.begin(); it != mActiveTunnels.end(); ++it) {
        const uint8_t* b = it->second.hash.toByteArray();
        uint8_t cand[16];
        for(int i=0; i<16; ++i) cand[i] = b[i] ^ VOIP_HASH_MAGIC_XOR;
        RsGxsId tunnelTarget = RsGxsId::fromBufferUnsafe(cand);
        
        if (tunnelTarget == info.to_id) {
            RsDbg() << "DISTANT_VOIP: MATCH FOUND! Reusing active virtual peer: " << it->first.toStdString();
            
            // DIRECT ASSIGNMENT to bypass deadlock: mutex is ALREADY LOCKED by line 118!
            mVirtualToDistantMap[it->first] = chatId.toDistantChatId();
            
            return it->first; 
        }
    }
    
    // 2. Check if search probe is ALREADY registered
    std::map<RsGxsId, RsFileHash>::iterator fit = mPendingProbes.find(info.to_id);
    if (fit != mPendingProbes.end()) {
        RsDbg() << "DISTANT_VOIP: Turtle monitor ALREADY ACTIVE. Still searching for: " << fit->second.toStdString();
        return RsPeerId(); // Still busy searching
    }

    // 3. Trigger NEW monitor request
    RsFileHash targetHash = makeVoipFakeHash(info.to_id);
    RsDbg() << "DISTANT_VOIP: No active probe found. ISSUING NEW MONITOR REQUEST to Turtle router.";
    RsDbg() << "DISTANT_VOIP: New Computed Fake Hash = " << targetHash.toStdString();
    
    mPendingProbes[info.to_id] = targetHash;
    
    if (mTurtleRouter) {
        mTurtleRouter->monitorTunnels(targetHash, this, false);
        RsDbg() << "DISTANT_VOIP: monitorTunnels() invocation DONE. Now waiting for addVirtualPeer callback from Router...";
    } else {
        RsDbg() << "DISTANT_VOIP: CRITICAL ERROR - mTurtleRouter is NULL. Cannot probe!";
    }
    
    return RsPeerId(); 
}

bool RsTurtleVOIPBridge::handleTunnelRequest(const RsFileHash& hash, const RsPeerId& /*peer_id*/)
{
    const uint8_t* bytes = hash.toByteArray();
    uint8_t candidate_bytes[16];
    for(int i=0; i<16; ++i) candidate_bytes[i] = bytes[i] ^ VOIP_HASH_MAGIC_XOR;
    RsGxsId target = RsGxsId::fromBufferUnsafe(candidate_bytes);
    
    if (mIdentity && mIdentity->isOwnId(target)) {
         RsDbg() << "DISTANT_VOIP: [INCOMING MATCH!] Accepting incoming anonymous call tunnel request.";
         return true;
    }
    return false;
}

void RsTurtleVOIPBridge::addVirtualPeer(const TurtleFileHash& hash, const TurtleVirtualPeerId& virtual_peer_id, RsTurtleGenericTunnelItem::Direction dir)
{
    RsStackMutex lock(mMutex);
    RsDbg() << "DISTANT_VOIP: [!!! SUCCESS !!!] addVirtualPeer triggered by Router! VirtualPeer=" << virtual_peer_id.toStdString();
    RsDbg() << "DISTANT_VOIP: Hash used by Router: " << hash.toStdString() << ", Direction=" << (int)dir;

    VOIPTunnelState state;
    state.hash = hash;
    state.direction = dir;
    state.creationTime = time(NULL);
    bool cryptoReady = initDHSessionKey(state.dhObj);
    
    RsDbg() << "DISTANT_VOIP: Tunnel context creation: " << (cryptoReady ? "CRYPTO OK" : "CRYPTO FAILED");

    mActiveTunnels[virtual_peer_id] = state;

    // Cleanup pending probe for this identity
    const uint8_t* b = hash.toByteArray();
    uint8_t cand[16];
    for(int i=0; i<16; ++i) cand[i] = b[i] ^ VOIP_HASH_MAGIC_XOR;
    RsGxsId resolvedId = RsGxsId::fromBufferUnsafe(cand);
    
    std::map<RsGxsId, RsFileHash>::iterator it = mPendingProbes.find(resolvedId);
    if (it != mPendingProbes.end()) {
        RsDbg() << "DISTANT_VOIP: Successfully cleared pending probe registry for contact " << resolvedId.toStdString();
        mPendingProbes.erase(it);
    }
    
    RsDbg() << "DISTANT_VOIP: TUNNEL FULLY OPERATIONAL & REGISTERED.";
}

void RsTurtleVOIPBridge::removeVirtualPeer(const TurtleFileHash& hash, const TurtleVirtualPeerId& virtual_peer_id)
{
    RsStackMutex lock(mMutex);
    RsDbg() << "DISTANT_VOIP: removeVirtualPeer triggered for VP=" << virtual_peer_id.toStdString() << ", Hash=" << hash.toStdString();
    
    // ALWAYS release persistent monitor lock on Router whenever a VOIP tunnel is dropped,
    // to prevent the router from performing infinite automatic re-probes in the background!
    if (mTurtleRouter) {
        mTurtleRouter->stopMonitoringTunnels(hash);
        RsDbg() << "DISTANT_VOIP: Explicitly terminated background persistent search for Hash " << hash.toStdString();
    }

    std::map<TurtleVirtualPeerId, VOIPTunnelState>::iterator it = mActiveTunnels.find(virtual_peer_id);
    if (it != mActiveTunnels.end()) {
        cleanupTunnelState(it->second);
        mActiveTunnels.erase(it);
        RsDbg() << "DISTANT_VOIP: Tunnel context destroyed. Registry cleared.";
    } else {
        RsDbg() << "DISTANT_VOIP: WARNING - removeVirtualPeer called but VP not in registry!";
    }
}

void RsTurtleVOIPBridge::receiveTurtleData(const RsTurtleGenericTunnelItem * item, const RsFileHash& /*hash*/, const RsPeerId& virtual_peer_id, RsTurtleGenericTunnelItem::Direction /*direction*/)
{
    RsDbg() << "DISTANT_VOIP: TRACE - receiveTurtleData triggered! Raw inbound packet detected from virtual peer " << virtual_peer_id.toStdString();
    const RsTurtleGenericFastDataItem* dataItem = dynamic_cast<const RsTurtleGenericFastDataItem*>(item);
    if (!dataItem) {
        RsDbg() << "DISTANT_VOIP: Dropping incoming turtle item. Not a FastDataItem instance.";
        return;
    }

    if (mVOIP == NULL || dataItem->data_size == 0 || dataItem->data_bytes == NULL) {
        RsDbg() << "DISTANT_VOIP: Warning - Packet body is empty or VOIP service is detached. DataSize=" << dataItem->data_size;
        return;
    }

    uint32_t headerSize = sizeof(DistantChatPeerId);
    if (dataItem->data_size <= headerSize) {
        RsDbg() << "DISTANT_VOIP: Packet is too small to contain identity wrapper. Size=" << dataItem->data_size;
        return;
    }

    // 1. Peel off identity header
    DistantChatPeerId incomingId;
    memcpy(&incomingId, dataItem->data_bytes, headerSize);
    
    // 2. Register mapping dynamically for incoming channel
    RsDbg() << "DISTANT_VOIP: Peeling envelope. Found incoming mapped chat ID: " << incomingId.toStdString();
    registerVirtualToDistantMapping(virtual_peer_id, incomingId);

    // 3. Prepare remainder of payload for standard deserialization
    uint8_t* payloadStart = ((uint8_t*)dataItem->data_bytes) + headerSize;
    uint32_t remainingSize = dataItem->data_size - headerSize;

    RsSerialiser* serializer = mVOIP->getSerialiser();
    if (serializer != NULL) {
        RsItem* decodedItem = serializer->deserialise(payloadStart, &remainingSize);
        
        if (decodedItem != NULL) {
            RsDbg() << "DISTANT_VOIP: [INBOUND SUCCESS] Envelope opened! Payload deserialized! Size=" << remainingSize << ". Forwarding to Engine...";
            decodedItem->PeerId(virtual_peer_id);
            mVOIP->recvItem(decodedItem);
        } else {
            RsDbg() << "DISTANT_VOIP: [CRITICAL ERROR] Payload deserialisation FAILED! Corrupted stream.";
        }
    } else {
        RsDbg() << "DISTANT_VOIP: Major failure - Bridge could not obtain valid Serialiser handle!";
    }
}

bool RsTurtleVOIPBridge::isVirtualPeer(const RsPeerId& id)
{
    RsStackMutex lock(mMutex);
    return mActiveTunnels.find(id) != mActiveTunnels.end();
}

void RsTurtleVOIPBridge::sendRawDataViaTunnel(const RsPeerId& virtual_peer_id, void* data, uint32_t size)
{
    if (!mTurtleRouter || size == 0) return;

    RsTurtleGenericFastDataItem* turtleWrapper = new RsTurtleGenericFastDataItem();
    turtleWrapper->data_size = size;
    turtleWrapper->data_bytes = malloc(size);
    if (turtleWrapper->data_bytes) memcpy(turtleWrapper->data_bytes, data, size);
    
    mTurtleRouter->sendTurtleData(virtual_peer_id, turtleWrapper);
}

RsFileHash RsTurtleVOIPBridge::makeVoipFakeHash(const RsGxsId& destination)
{
    uint8_t bytes[20]; 
    const uint8_t* gxs_bytes = destination.toByteArray();
    
    // DISTANT_VOIP FIX: Shift the GxsId to the beginning of the hash (0-15)
    // to avoid overlapping with p3gxstunnel's expected layout (4-19).
    // This prevents "Cannot find tunnel id" error spam in logs.
    for(int i=0; i<16; ++i) bytes[i] = gxs_bytes[i] ^ VOIP_HASH_MAGIC_XOR;
    
    // Random bytes at the end (16-19)
    RsRandom::random_bytes(&bytes[16], 4); 

    return RsFileHash::fromBufferUnsafe(bytes);
}

void RsTurtleVOIPBridge::triggerTestTunnelRequest()
{
    // User instructed to trace live flows, so suppressing initial mock test.
}

void RsTurtleVOIPBridge::registerVirtualToDistantMapping(const RsPeerId& virtual_id, const DistantChatPeerId& chat_id)
{
    RsStackMutex lock(mMutex);
    mVirtualToDistantMap[virtual_id] = chat_id;
    RsDbg() << "DISTANT_VOIP: BINDING REGISTERED: Virtual " << virtual_id.toStdString() << " maps to DistantChat " << chat_id.toStdString();
}

ChatId RsTurtleVOIPBridge::resolveVirtualToDistantChat(const RsPeerId& virtual_id)
{
    RsStackMutex lock(mMutex);
    std::map<RsPeerId, DistantChatPeerId>::const_iterator it = mVirtualToDistantMap.find(virtual_id);
    if (it != mVirtualToDistantMap.end()) {
        return ChatId(it->second);
    }
    return ChatId(); // Not Set
}

void RsTurtleVOIPBridge::cancelSearchForChat(const ChatId& chatId)
{
    if (!chatId.isDistantChatId() || mChats == NULL) return;

    DistantChatPeerInfo info;
    if (!mChats->getDistantChatStatus(chatId.toDistantChatId(), info)) return;

    RsStackMutex lock(mMutex);
    
    // Look up if there is an active probe pending for this contact
    std::map<RsGxsId, RsFileHash>::iterator it = mPendingProbes.find(info.to_id);
    if (it != mPendingProbes.end()) {
        RsFileHash activeHash = it->second;
        RsDbg() << "DISTANT_VOIP: MANUAL ABORT detected. Cancelling active search for " << activeHash.toStdString();
        
        if (mTurtleRouter) {
            mTurtleRouter->stopMonitoringTunnels(activeHash);
            RsDbg() << "DISTANT_VOIP: Successfully released manual abort monitor lock on Router.";
        }
        
        mPendingProbes.erase(it);
    }
}

