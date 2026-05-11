/*******************************************************************************
 * plugins/VOIP/services/RsTurtleVOIPBridge.h                                  *
 *                                                                             *
 * Copyright 2026 by retroshare team                                           *
 *******************************************************************************/

#ifndef RSTURTLEVOIPBRIDGE_H
#define RSTURTLEVOIPBRIDGE_H

#include "turtle/turtleclientservice.h"
#include "retroshare/rsturtle.h"
#include <retroshare/rstypes.h>
#include <retroshare/rsidentity.h>
#include "util/rsdebug.h"

#include "util/rsthreads.h"
#include <map>
#include <time.h>

#include <openssl/dh.h>
#include <string>
#include <retroshare/rschats.h>

class p3VOIP;
class p3turtle;

class RsTurtleVOIPBridge : public RsTurtleClientService
{
public:
    // Step 7 GUI API
    RsPeerId getOrCreateTunnelForChat(const ChatId& chatId);
    struct VOIPTunnelState {
        TurtleFileHash hash;
        RsTurtleGenericTunnelItem::Direction direction;
        time_t creationTime;
        
        DH* dhObj;
        unsigned char sharedKey[32]; // Final 256-bit AES key computed later
        bool keyExchangeComplete;
        
        VOIPTunnelState() : dhObj(NULL), keyExchangeComplete(false) {
            memset(sharedKey, 0, 32);
        }
    };

    RsTurtleVOIPBridge(p3VOIP* voip, RsIdentity* ident, class RsChats* chats);
    virtual ~RsTurtleVOIPBridge();

    // Step 5 Encryption helpers
    bool initDHSessionKey(DH *& dh);
    void cleanupTunnelState(VOIPTunnelState& state);

    void connectToTurtleRouter(p3turtle* pt) override;

    // RsTurtleClientService overrides
    uint16_t serviceId() const override;
    bool handleTunnelRequest(const RsFileHash& hash, const RsPeerId& peer_id) override;
    
    void receiveTurtleData(const RsTurtleGenericTunnelItem * item, const RsFileHash& hash, const RsPeerId& virtual_peer_id, RsTurtleGenericTunnelItem::Direction direction) override;
    
    void addVirtualPeer(const TurtleFileHash& hash, const TurtleVirtualPeerId& virtual_peer_id, RsTurtleGenericTunnelItem::Direction dir) override;
    void removeVirtualPeer(const TurtleFileHash& hash, const TurtleVirtualPeerId& virtual_peer_id) override;

    // Step 6 Integration methods
    bool isVirtualPeer(const RsPeerId& id);
    void sendRawDataViaTunnel(const RsPeerId& virtual_peer_id, void* data, uint32_t size);

    // Step 2 helpers
    RsFileHash makeVoipFakeHash(const RsGxsId& destination);
    void triggerTestTunnelRequest();

private:
    p3VOIP* mVOIP;
    p3turtle* mTurtleRouter;
    RsIdentity* mIdentity;
    class RsChats* mChats;

    mutable RsMutex mMutex;
    std::map<TurtleVirtualPeerId, VOIPTunnelState> mActiveTunnels;
};

#endif // RSTURTLEVOIPBRIDGE_H
