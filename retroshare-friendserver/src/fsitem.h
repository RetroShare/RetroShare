#include "serialiser/rsserial.h"
#include "serialiser/rsserializer.h"

#include "rsitems/rsitem.h"
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
    virtual void clear()  override;

    virtual ~RsFriendServerItem() {}
};

class RsFriendServerClientPublishItem: public RsFriendServerItem
{
public:
    RsFriendServerClientPublishItem() : RsFriendServerItem(RS_PKT_SUBTYPE_FS_CLIENT_PUBLISH) {}

    void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx) override
    {
        RS_SERIAL_PROCESS(n_requested_friends);
        RS_SERIAL_PROCESS(long_invite);
    }
    virtual void clear()  override
    {
        long_invite = std::string();
        n_requested_friends=0;
    }

    // specific members for that item

    uint32_t    n_requested_friends;
    std::string long_invite;
};

class RsFriendServerClientRemoveItem: public RsFriendServerItem
{
public:
    RsFriendServerClientRemoveItem() : RsFriendServerItem(RS_PKT_SUBTYPE_FS_CLIENT_REMOVE) {}

    void serial_process(RsGenericSerializer::SerializeJob /* j */,RsGenericSerializer::SerializeContext& /* ctx */)
    {
    }
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
