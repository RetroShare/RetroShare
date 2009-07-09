#ifndef RSFEEDITEM_H
#define RSFEEDITEM_H

class RsFeedItem
{
public:
    RsFeedItem();
    RsFeedItem(uint32_t type, std::string id1, std::string id2, std::string id3);
    uint32_t mType;
    std::string mId1, mId2, mId3;
};


#endif // RSFEEDITEM_H
