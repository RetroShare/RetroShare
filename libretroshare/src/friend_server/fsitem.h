/*******************************************************************************
 * libretroshare/src/file_sharing: fsitem.h                                    *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2021 by retroshare team <retroshare.project@gmail.com>            *
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
 ******************************************************************************/

#pragma once

#include "serialiser/rsserial.h"
#include "serialiser/rsserializer.h"

#include "rsitems/rsitem.h"
#include "serialiser/rstlvbinary.h"
#include "rsitems/rsserviceids.h"
#include "rsitems/itempriorities.h"

const uint8_t RS_PKT_SUBTYPE_FS_CLIENT_PUBLISH             = 0x01 ;
const uint8_t RS_PKT_SUBTYPE_FS_CLIENT_REMOVE              = 0x02 ;
const uint8_t RS_PKT_SUBTYPE_FS_SERVER_RESPONSE            = 0x03 ;
const uint8_t RS_PKT_SUBTYPE_FS_SERVER_ENCRYPTED_RESPONSE  = 0x04 ;

class RsFriendServerItem: public RsItem
{
public:
    RsFriendServerItem(uint8_t item_subtype) : RsItem(RS_PKT_VERSION_SERVICE,RS_SERVICE_TYPE_FRIEND_SERVER,item_subtype)
	{
		setPriorityLevel(QOS_PRIORITY_DEFAULT) ;
	}
    virtual ~RsFriendServerItem() {}
    virtual void clear()  override {}
};

class RsFriendServerClientPublishItem: public RsFriendServerItem
{
public:
    RsFriendServerClientPublishItem() : RsFriendServerItem(RS_PKT_SUBTYPE_FS_CLIENT_PUBLISH) {}

    void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx) override
    {
        RS_SERIAL_PROCESS(n_requested_friends);
        RS_SERIAL_PROCESS(short_invite);
        RS_SERIAL_PROCESS(pgp_public_key_b64);
    }
    virtual void clear()  override
    {
        pgp_public_key_b64.clear();
        short_invite.clear();
        n_requested_friends=0;
    }

    // specific members for that item

    uint32_t    n_requested_friends;
    std::string short_invite;
    std::string pgp_public_key_b64;
};

class RsFriendServerClientRemoveItem: public RsFriendServerItem
{
public:
    RsFriendServerClientRemoveItem() : RsFriendServerItem(RS_PKT_SUBTYPE_FS_CLIENT_REMOVE) {}

    void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
    {
        RS_SERIAL_PROCESS(peer_id);
        RS_SERIAL_PROCESS(nonce);
    }

    // Peer ID for the peer to remove.

    RsPeerId peer_id;

    // Nonce that was returned by the server after the last client request. Should match in order to proceed. This prevents
    // a malicious actor from removing peers from the server. Since the nonce is sent through Tor tunnels, it cannot be known by
    // anyone else than the client.

    uint64_t nonce;
};

class RsFriendServerEncryptedServerResponseItem: public RsFriendServerItem
{
public:
    RsFriendServerEncryptedServerResponseItem() : RsFriendServerItem(RS_PKT_SUBTYPE_FS_SERVER_ENCRYPTED_RESPONSE) {}

    void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx) override
    {
        RsTypeSerializer::RawMemoryWrapper prox(bin_data, bin_len);
        RsTypeSerializer::serial_process(j, ctx, prox, "data");
    }

    virtual void clear() override
    {
        free(bin_data);
        bin_len = 0;
        bin_data = nullptr;
    }
    //

    void *bin_data;
    uint32_t bin_len;
};

class RsFriendServerServerResponseItem: public RsFriendServerItem
{
public:
    RsFriendServerServerResponseItem() : RsFriendServerItem(RS_PKT_SUBTYPE_FS_SERVER_RESPONSE) {}

    void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx) override
    {
        RS_SERIAL_PROCESS(friend_invites);
    }

    virtual void clear() override
    {
        friend_invites.clear();
    }
    // specific members for that item

    std::map<std::string,bool> friend_invites;
};

struct FsSerializer : RsServiceSerializer
{
    FsSerializer(RsSerializationFlags flags = RsSerializationFlags::NONE): RsServiceSerializer(RS_SERVICE_TYPE_FRIEND_SERVER, flags) {}

    virtual RsItem *create_item(uint16_t service_id,uint8_t item_sub_id) const
    {
        if(service_id != static_cast<uint16_t>(RsServiceType::FRIEND_SERVER))
            return nullptr;

        switch(item_sub_id)
        {
        case RS_PKT_SUBTYPE_FS_CLIENT_REMOVE:             return new RsFriendServerClientRemoveItem();
        case RS_PKT_SUBTYPE_FS_CLIENT_PUBLISH:            return new RsFriendServerClientPublishItem();
        case RS_PKT_SUBTYPE_FS_SERVER_RESPONSE:           return new RsFriendServerServerResponseItem();
        case RS_PKT_SUBTYPE_FS_SERVER_ENCRYPTED_RESPONSE: return new RsFriendServerEncryptedServerResponseItem();
        default:
            RsErr() << "Unknown subitem type " << item_sub_id << " in FsSerialiser" ;
            return nullptr;
        }

    }
};
