#include "serialiser/rsserial.h"
#include "serialiser/rsserializer.h"

#include "rsitems/rsitem.h"
#include "rsitems/rsserviceids.h"
#include "rsitems/itempriorities.h"

class FsItem: public RsItem
{
public:
    FsItem(uint8_t item_subtype) : RsItem(RS_PKT_VERSION_SERVICE,RS_SERVICE_TYPE_FRIEND_SERVER,item_subtype)
	{
		setPriorityLevel(QOS_PRIORITY_DEFAULT) ;
	}

	virtual ~FsItem() {}
	virtual void clear() {}
};

struct FsSerializer : RsServiceSerializer
{
    FsSerializer(RsSerializationFlags flags = RsSerializationFlags::NONE): RsServiceSerializer(RS_SERVICE_TYPE_FRIEND_SERVER, flags) {}

    virtual RsItem *create_item(uint16_t service_id,uint8_t item_sub_id) const {};
};
