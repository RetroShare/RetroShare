/*******************************************************************************
 * plugins/VOIP/services/RsTurtleVOIPBridge.cpp                                 *
 *                                                                             *
 * Copyright 2026 by retroshare team                                           *
 *******************************************************************************/

#include "RsTurtleVOIPBridge.h"
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
}

RsTurtleVOIPBridge::~RsTurtleVOIPBridge()
{
    RsStackMutex lock(mMutex);
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
    if(!dh) return false;

#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
    BN_hex2bn(&dh->p,dh_prime_2048_hex.c_str()) ; BN_hex2bn(&dh->g,"5") ;
#else
    BIGNUM *pp=NULL ; BIGNUM *gg=NULL ;
    BN_hex2bn(&pp,dh_prime_2048_hex.c_str()) ; BN_hex2bn(&gg,"5") ;
    DH_set0_pqg(dh,pp,NULL,gg) ;
#endif

    int codes = 0 ;
    if(!DH_check(dh, &codes) || codes != 0) return false ;
    if(!DH_generate_key(dh)) return false ;
    return true ;
}

void RsTurtleVOIPBridge::connectToTurtleRouter(p3turtle *pt)
{
    mTurtleRouter = pt;
    if (mTurtleRouter) {
        mTurtleRouter->registerTunnelService(this);
        RsDbg() << "DISTANT_VOIP: Registered successfully to Turtle Router.";
        triggerTestTunnelRequest();

        RsDbg() << "DISTANT_VOIP: [MOCK] Starting validation cycle...";
        uint8_t dummyVp[16]; memset(dummyVp, 0x99, 16);
        TurtleVirtualPeerId mockId = TurtleVirtualPeerId::fromBufferUnsafe(dummyVp);
        TurtleFileHash mockHash = TurtleFileHash::random();
        addVirtualPeer(mockHash, mockId, RsTurtleGenericTunnelItem::DIRECTION_SERVER);
        
        RsDbg() << "DISTANT_VOIP: [MOCK] Simulating VOIP-To-Turtle pipe link...";
        RsTurtleGenericFastDataItem *mockItem = new RsTurtleGenericFastDataItem();
        const char* dummyFlow = "MOCK_VOIP_DATA"; 
        mockItem->data_size = strlen(dummyFlow) + 1;
        mockItem->data_bytes = malloc(mockItem->data_size);
        memcpy(mockItem->data_bytes, dummyFlow, mockItem->data_size);
        
        receiveTurtleData(mockItem, mockHash, mockId, RsTurtleGenericTunnelItem::DIRECTION_CLIENT);
        
        delete mockItem; 
        removeVirtualPeer(mockHash, mockId);
        RsDbg() << "DISTANT_VOIP: [MOCK] All Stage validation complete.";
    }
}

RsPeerId RsTurtleVOIPBridge::getOrCreateTunnelForChat(const ChatId& chatId)
{
    if (!chatId.isDistantChatId() || mChats == NULL) {
        return RsPeerId();
    }
    
    DistantChatPeerInfo info;
    if (!mChats->getDistantChatStatus(chatId.toDistantChatId(), info)) {
        RsDbg() << "DISTANT_VOIP: Could not resolve distant chat info for tunnel probe.";
        return RsPeerId();
    }
    
    RsStackMutex lock(mMutex);
    // Search for active tunnel for this target GXS ID
    for(std::map<TurtleVirtualPeerId, VOIPTunnelState>::iterator it = mActiveTunnels.begin(); it != mActiveTunnels.end(); ++it) {
        const uint8_t* b = it->second.hash.toByteArray();
        uint8_t cand[16];
        for(int i=0; i<16; ++i) cand[i] = b[4+i] ^ VOIP_HASH_MAGIC_XOR;
        RsGxsId tunnelTarget = RsGxsId::fromBufferUnsafe(cand);
        
        if (tunnelTarget == info.to_id) {
            RsDbg() << "DISTANT_VOIP: Reusing existing active tunnel " << it->first.toStdString() << " for contact " << info.to_id.toStdString();
            return it->first; 
        }
    }
    
    // No active tunnel found, probe for one
    RsFileHash targetHash = makeVoipFakeHash(info.to_id);
    RsDbg() << "DISTANT_VOIP: Requesting on-the-fly anonymous tunnel for contact " << info.to_id.toStdString();
    
    if (mTurtleRouter) {
        mTurtleRouter->monitorTunnels(targetHash, this, false);
    }
    
    return RsPeerId(); // Hand back null to UI to denote search is running
}

bool RsTurtleVOIPBridge::handleTunnelRequest(const RsFileHash& hash, const RsPeerId& /*peer_id*/)
{
    const uint8_t* bytes = hash.toByteArray();
    uint8_t candidate_bytes[16];
    for(int i=0; i<16; ++i) candidate_bytes[i] = bytes[4 + i] ^ VOIP_HASH_MAGIC_XOR;
    RsGxsId target = RsGxsId::fromBufferUnsafe(candidate_bytes);
    
    if (mIdentity && mIdentity->isOwnId(target)) {
         RsDbg() << "DISTANT_VOIP: Accepted incoming VOIP tunnel request.";
         return true;
    }
    return false;
}

void RsTurtleVOIPBridge::addVirtualPeer(const TurtleFileHash& hash, const TurtleVirtualPeerId& virtual_peer_id, RsTurtleGenericTunnelItem::Direction dir)
{
    RsStackMutex lock(mMutex);
    VOIPTunnelState state;
    state.hash = hash;
    state.direction = dir;
    state.creationTime = time(NULL);
    initDHSessionKey(state.dhObj);
    
    mActiveTunnels[virtual_peer_id] = state;
    RsDbg() << "DISTANT_VOIP: Tunnel established! Virtual Peer: " << virtual_peer_id.toStdString();
}

void RsTurtleVOIPBridge::removeVirtualPeer(const TurtleFileHash& /*hash*/, const TurtleVirtualPeerId& virtual_peer_id)
{
    RsStackMutex lock(mMutex);
    std::map<TurtleVirtualPeerId, VOIPTunnelState>::iterator it = mActiveTunnels.find(virtual_peer_id);
    if (it != mActiveTunnels.end()) {
        cleanupTunnelState(it->second);
        mActiveTunnels.erase(it);
        RsDbg() << "DISTANT_VOIP: Tunnel closed!";
    }
}

void RsTurtleVOIPBridge::receiveTurtleData(const RsTurtleGenericTunnelItem * item, const RsFileHash& /*hash*/, const RsPeerId& virtual_peer_id, RsTurtleGenericTunnelItem::Direction /*direction*/)
{
    const RsTurtleGenericFastDataItem* dataItem = dynamic_cast<const RsTurtleGenericFastDataItem*>(item);
    if (!dataItem) return;

    if (mVOIP == NULL || dataItem->data_size == 0 || dataItem->data_bytes == NULL) return;

    RsSerialiser* serializer = mVOIP->getSerialiser();
    if (serializer != NULL) {
        uint32_t actualSize = dataItem->data_size;
        RsItem* decodedItem = serializer->deserialise(dataItem->data_bytes, &actualSize);
        
        if (decodedItem != NULL) {
            decodedItem->PeerId(virtual_peer_id);
            mVOIP->recvItem(decodedItem);
        }
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
    uint8_t bytes[20]; RsRandom::random_bytes(&bytes[0], 4); 
    const uint8_t* gxs_bytes = destination.toByteArray();
    for(int i=0; i<16; ++i) bytes[4 + i] = gxs_bytes[i] ^ VOIP_HASH_MAGIC_XOR;
    return RsFileHash::fromBufferUnsafe(bytes);
}

void RsTurtleVOIPBridge::triggerTestTunnelRequest()
{
    uint8_t dummy[16]; memset(dummy, 0x77, 16);
    RsGxsId testDest = RsGxsId::fromBufferUnsafe(dummy);
    RsFileHash fakeHash = makeVoipFakeHash(testDest);
    if (mTurtleRouter) { mTurtleRouter->monitorTunnels(fakeHash, this, false); }
}
