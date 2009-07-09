#include "rsfeeditem.h"

RsFeedItem::RsFeedItem() : mType(0)
{
}

RsFeedItem::RsFeedItem(uint32_t type, std::string id1, std::string id2, std::string id3):
        mType(type),
        mId1(id1),
        mId2(id2),
        mId3(id3)
{

}

