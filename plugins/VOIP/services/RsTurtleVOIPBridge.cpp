/*******************************************************************************
 * plugins/VOIP/services/RsTurtleVOIPBridge.cpp                                 *
 *                                                                             *
 * Copyright 2026 by retroshare team                                           *
 *******************************************************************************/

#include "RsTurtleVOIPBridge.h"
#include <retroshare/rschats.h>
#include "services/p3VOIP.h"
#include "serialiser/rsserial.h"
#include "util/rsdebug.h"

#include <cstdlib>      // free()
#include <cstring>      // memcpy()

// Unique gxstunnel client service id for VOIP.
// (distant chat uses 0xa0001 - see chat/distantchat.cc - so we pick 0xa0002.)
static const uint32_t VOIP_GXS_TUNNEL_SERVICE_ID = 0xa0002 ;

RsTurtleVOIPBridge::RsTurtleVOIPBridge(p3VOIP *voip_service, RsIdentity* ident, RsChats* chats)
    : mVOIP(voip_service), mGxsTunnels(NULL), mIdentity(ident), mChats(chats), mMutex("RsTurtleVOIPBridge")
{
    RsDbg() << "DISTANT_VOIP: RsTurtleVOIPBridge (GXS tunnel client) created.";
}

RsTurtleVOIPBridge::~RsTurtleVOIPBridge()
{
}

void RsTurtleVOIPBridge::connectToGxsTunnelService(RsGxsTunnelService *tunnel_service)
{
    if (mGxsTunnels != NULL) return; // already connected & registered

    mGxsTunnels = tunnel_service;
    if (mGxsTunnels) {
        mGxsTunnels->registerClientService(VOIP_GXS_TUNNEL_SERVICE_ID, this);
        RsDbg() << "DISTANT_VOIP: Registered as client of the GXS tunnel service.";
    } else {
        RsDbg() << "DISTANT_VOIP: ERROR - connectToGxsTunnelService passed NULL!";
    }
}

RsPeerId RsTurtleVOIPBridge::getOrCreateTunnelForChat(const ChatId& chatId)
{
    RsDbg() << "DISTANT_VOIP: getOrCreateTunnelForChat triggered for chatId=" << chatId.toStdString();

    // Emergency binds, kept from the original (interfaces may arrive late).
    if (mChats == NULL)      mChats = rsChats;
    if (mGxsTunnels == NULL) connectToGxsTunnelService(rsGxsTunnel);

    if (!chatId.isDistantChatId() || mChats == NULL || mGxsTunnels == NULL) {
        RsDbg() << "DISTANT_VOIP: Rejecting request. isDistant=" << (chatId.isDistantChatId()?"yes":"no")
                << ", mChatsReady=" << (mChats!=NULL?"yes":"no")
                << ", mTunnelsReady=" << (mGxsTunnels!=NULL?"yes":"no");
        return RsPeerId();
    }

    DistantChatPeerInfo info;
    if (!mChats->getDistantChatStatus(chatId.toDistantChatId(), info)) {
        RsDbg() << "DISTANT_VOIP: FAILED to resolve distant chat state from mChats.";
        return RsPeerId();
    }
    RsDbg() << "DISTANT_VOIP: Resolved target identity: " << info.to_id.toStdString();

    RsStackMutex lock(mMutex);

    // 1. Existing tunnel to this destination?
    std::map<RsGxsId, RsGxsTunnelId>::iterator it = mTunnelByDest.find(info.to_id);
    if (it != mTunnelByDest.end()) {
        RsGxsTunnelId tid = it->second;
        if (mReadyTunnels.find(tid) != mReadyTunnels.end()) {
            RsPeerId vp = RsPeerId(tid);
            mVirtualToDistantMap[vp] = chatId.toDistantChatId();   // keep GUI mapping fresh
            RsDbg() << "DISTANT_VOIP: Reusing ready tunnel, virtual peer=" << vp.toStdString();
            return vp;
        }
        RsDbg() << "DISTANT_VOIP: Tunnel still being established. Waiting for CAN_TALK...";
        return RsPeerId();
    }

    // 2. Request a new secured tunnel (gxstunnel does DH + AES + auth + turtle plumbing).
    RsGxsTunnelId tunnel_id;
    uint32_t error_code = 0;
    if (mGxsTunnels->requestSecuredTunnel(info.to_id, info.own_id, tunnel_id, VOIP_GXS_TUNNEL_SERVICE_ID, error_code)) {
        mTunnelByDest[info.to_id] = tunnel_id;
        // pre-register the chat mapping so resolveVirtualToDistantChat works as soon as data flows
        mVirtualToDistantMap[RsPeerId(tunnel_id)] = chatId.toDistantChatId();
        RsDbg() << "DISTANT_VOIP: Requested secured tunnel " << tunnel_id.toStdString()
                << " to " << info.to_id.toStdString() << ". Waiting for CAN_TALK...";
    } else {
        RsDbg() << "DISTANT_VOIP: requestSecuredTunnel FAILED, error_code=" << error_code;
    }

    return RsPeerId(); // not ready yet; GUI auto-poller retries
}

void RsTurtleVOIPBridge::cancelSearchForChat(const ChatId& chatId)
{
    if (!chatId.isDistantChatId() || mChats == NULL || mGxsTunnels == NULL) return;

    DistantChatPeerInfo info;
    if (!mChats->getDistantChatStatus(chatId.toDistantChatId(), info)) return;

    RsStackMutex lock(mMutex);

    std::map<RsGxsId, RsGxsTunnelId>::iterator it = mTunnelByDest.find(info.to_id);
    if (it != mTunnelByDest.end()) {
        RsGxsTunnelId tid = it->second;
        RsDbg() << "DISTANT_VOIP: Cancelling/closing tunnel " << tid.toStdString();
        mGxsTunnels->closeExistingTunnel(tid, VOIP_GXS_TUNNEL_SERVICE_ID);
        mReadyTunnels.erase(tid);
        mTunnelByDest.erase(it);
    }
}

bool RsTurtleVOIPBridge::isVirtualPeer(const RsPeerId& id)
{
    RsStackMutex lock(mMutex);
    return mReadyTunnels.find(RsGxsTunnelId(id)) != mReadyTunnels.end();
}

void RsTurtleVOIPBridge::sendRawDataViaTunnel(const RsPeerId& virtual_peer_id, void* data, uint32_t size)
{
    if (mGxsTunnels == NULL || size == 0 || data == NULL) return;

    // gxstunnel copies the buffer (it does NOT take ownership); the caller frees it.
    mGxsTunnels->sendData(RsGxsTunnelId(virtual_peer_id), VOIP_GXS_TUNNEL_SERVICE_ID,
                          static_cast<const uint8_t*>(data), size);
}

void RsTurtleVOIPBridge::notifyTunnelStatus(const RsGxsTunnelId& tunnel_id, uint32_t tunnel_status)
{
    RsStackMutex lock(mMutex);

    switch (tunnel_status) {
    case RsGxsTunnelService::RS_GXS_TUNNEL_STATUS_CAN_TALK:
        mReadyTunnels.insert(tunnel_id);
        RsDbg() << "DISTANT_VOIP: Tunnel CAN_TALK: " << tunnel_id.toStdString();
        break;

    case RsGxsTunnelService::RS_GXS_TUNNEL_STATUS_TUNNEL_DN:
    case RsGxsTunnelService::RS_GXS_TUNNEL_STATUS_REMOTELY_CLOSED:
        mReadyTunnels.erase(tunnel_id);
        for (std::map<RsGxsId, RsGxsTunnelId>::iterator it = mTunnelByDest.begin(); it != mTunnelByDest.end(); )
            if (it->second == tunnel_id) it = mTunnelByDest.erase(it); else ++it;
        RsDbg() << "DISTANT_VOIP: Tunnel DOWN/closed: " << tunnel_id.toStdString();
        break;

    default:
        break;
    }
}

bool RsTurtleVOIPBridge::acceptDataFromPeer(const RsGxsId& gxs_id, const RsGxsTunnelId& /*tunnel_id*/, bool /*am_I_client_side*/)
{
    // Accept incoming anonymous VOIP calls. (Permission filtering could be added
    // here later, mirroring DistantChatService::acceptDataFromPeer.)
    RsDbg() << "DISTANT_VOIP: Accepting VOIP tunnel data from " << gxs_id.toStdString();
    return true;
}

void RsTurtleVOIPBridge::receiveData(const RsGxsTunnelId& tunnel_id, unsigned char* data, uint32_t data_size)
{
    // gxstunnel transfers memory ownership of 'data' to us: we must free() it.
    const RsPeerId virtual_peer_id = RsPeerId(tunnel_id);

    RsDbg() << "DISTANT_VOIP: receiveData from virtual peer " << virtual_peer_id.toStdString()
            << " size=" << data_size;

    if (mVOIP == NULL || data == NULL || data_size == 0) {
        if (data) free(data);
        return;
    }

    const uint32_t headerSize = sizeof(DistantChatPeerId);
    if (data_size <= headerSize) {
        RsDbg() << "DISTANT_VOIP: Packet too small for identity wrapper. Size=" << data_size;
        free(data);
        return;
    }

    // 1. Peel identity header (same envelope p3VOIP::sendItem builds on the sender side).
    DistantChatPeerId incomingId;
    memcpy(&incomingId, data, headerSize);
    registerVirtualToDistantMapping(virtual_peer_id, incomingId);

    // 2. Deserialise the remaining payload and hand it to the VOIP engine.
    uint8_t* payloadStart  = data + headerSize;
    uint32_t remainingSize = data_size - headerSize;

    RsSerialiser* serializer = mVOIP->getSerialiser();
    if (serializer != NULL) {
        RsItem* decodedItem = serializer->deserialise(payloadStart, &remainingSize);
        if (decodedItem != NULL) {
            decodedItem->PeerId(virtual_peer_id);
            mVOIP->recvItem(decodedItem);
        } else {
            RsDbg() << "DISTANT_VOIP: [ERROR] Payload deserialisation FAILED.";
        }
    } else {
        RsDbg() << "DISTANT_VOIP: [ERROR] Could not obtain VOIP serialiser.";
    }

    free(data);
}

void RsTurtleVOIPBridge::registerVirtualToDistantMapping(const RsPeerId& virtual_id, const DistantChatPeerId& chat_id)
{
    RsStackMutex lock(mMutex);
    mVirtualToDistantMap[virtual_id] = chat_id;
}

ChatId RsTurtleVOIPBridge::resolveVirtualToDistantChat(const RsPeerId& virtual_id)
{
    RsStackMutex lock(mMutex);
    std::map<RsPeerId, DistantChatPeerId>::const_iterator it = mVirtualToDistantMap.find(virtual_id);
    if (it != mVirtualToDistantMap.end())
        return ChatId(it->second);
    return ChatId(); // not set
}
