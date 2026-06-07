/*******************************************************************************
 * plugins/VOIP/services/RsTurtleVOIPBridge.h                                  *
 *                                                                             *
 * Copyright 2026 by retroshare team                                           *
 *******************************************************************************/

#ifndef RSTURTLEVOIPBRIDGE_H
#define RSTURTLEVOIPBRIDGE_H

// Distant (gxsid<->gxsid) VOIP transport.
//
// NOTE on the name: this class is still called "RsTurtleVOIPBridge" for
// historical reasons, but it no longer talks to the turtle router directly and
// no longer rolls its own crypto. It is now a *client* of p3GxsTunnelService
// (libretroshare), which provides an end-to-end AES-encrypted, GXS-id
// authenticated tunnel over turtle. The hand-rolled (and never completed) DH
// handshake has been removed: the previous code generated a DH keypair, logged
// "CRYPTO OK", but never computed a shared secret nor encrypted anything, so the
// media was actually travelling in clear. Everything crypto/tunnel related is
// now delegated to p3GxsTunnelService. Renaming the class to e.g.
// RsGxsTunnelVOIPBridge is a cosmetic follow-up (touches 6 files).

#include <retroshare/rsgxstunnel.h>     // RsGxsTunnelService + nested RsGxsTunnelClientService + RsGxsTunnelId
#include <retroshare/rstypes.h>         // RsPeerId
#include <retroshare/rsidentity.h>      // RsIdentity
#include <retroshare/rschats.h>         // RsChats, ChatId, DistantChatPeerId, DistantChatPeerInfo
#include "util/rsthreads.h"             // RsMutex
#include "util/rsdebug.h"

#include <map>
#include <set>

class p3VOIP;

class RsTurtleVOIPBridge : public RsGxsTunnelService::RsGxsTunnelClientService
{
public:
    RsTurtleVOIPBridge(p3VOIP* voip, RsIdentity* ident, class RsChats* chats);
    virtual ~RsTurtleVOIPBridge();

    // ------------------------------------------------------------------ //
    //  GUI / engine facing API (unchanged - callers untouched)           //
    // ------------------------------------------------------------------ //

    // Returns the virtual peer id (== RsPeerId(tunnel_id)) once a secured
    // tunnel to the chat's distant identity is up (CAN_TALK). Returns a null
    // RsPeerId while the tunnel is still being established; the caller (GUI
    // auto-poller) retries.
    RsPeerId getOrCreateTunnelForChat(const ChatId& chatId);

    // Aborts/closes the tunnel associated to this chat.
    void cancelSearchForChat(const ChatId& chatId);

    // Maps a virtual peer id back to the distant chat it belongs to (used by the
    // GUI to route incoming calls to the right chat window).
    ChatId resolveVirtualToDistantChat(const RsPeerId& virtual_id);
    void registerVirtualToDistantMapping(const RsPeerId& virtual_id, const DistantChatPeerId& chat_id);

    // True if this RsPeerId is one of our active (CAN_TALK) gxs tunnels.
    bool isVirtualPeer(const RsPeerId& id);

    // Pushes an already-serialised VOIP envelope into the secured tunnel.
    // Memory ownership of 'data' stays with the caller (p3VOIP frees it).
    void sendRawDataViaTunnel(const RsPeerId& virtual_peer_id, void* data, uint32_t size);

    void setChatService(class RsChats* chats) { mChats = chats; }
    void setIdentity(class RsIdentity* ident) { mIdentity = ident; }

    // ------------------------------------------------------------------ //
    //  RsGxsTunnelClientService callbacks                                 //
    // ------------------------------------------------------------------ //

    // Supplies (and registers with) the GXS tunnel service. Replaces the old
    // connectToTurtleRouter().
    virtual void connectToGxsTunnelService(RsGxsTunnelService* tunnel_service) override;

    virtual void notifyTunnelStatus(const RsGxsTunnelId& tunnel_id, uint32_t tunnel_status) override;
    virtual void receiveData(const RsGxsTunnelId& tunnel_id, unsigned char* data, uint32_t data_size) override;
    virtual bool acceptDataFromPeer(const RsGxsId& gxs_id, const RsGxsTunnelId& tunnel_id, bool am_I_client_side) override;

private:
    p3VOIP*             mVOIP;
    RsGxsTunnelService* mGxsTunnels;
    RsIdentity*         mIdentity;
    class RsChats*      mChats;

    mutable RsMutex mMutex;

    // destination GXS id -> tunnel id (dedup outgoing requests)
    std::map<RsGxsId, RsGxsTunnelId>          mTunnelByDest;
    // tunnels currently usable (CAN_TALK)
    std::set<RsGxsTunnelId>                    mReadyTunnels;
    // virtual peer id (== RsPeerId(tunnel_id)) -> distant chat id (GUI routing)
    std::map<RsPeerId, DistantChatPeerId>     mVirtualToDistantMap;
};

#endif // RSTURTLEVOIPBRIDGE_H
