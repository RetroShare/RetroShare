#ifndef RSDUMMYSERVICES_H
#define RSDUMMYSERVICES_H


// dummy services to make

#include "gxs/rsnxs.h"
#include "serialiser/rsgxsitems.h"

class RsDummyNetService: public RsNetworkExchangeService
{
public:

     RsDummyNetService(){ return;}

     void setSyncAge(uint32_t age){}

     void requestGroupsOfPeer(const std::string& peerId){}

     void requestMessagesOfPeer(const std::string& peerId, const std::string& grpId){}

     void pauseSynchronisation(bool enabled) {}

     int requestMsg(const std::string& msgId, uint8_t hops){ return 0;}

     int requestGrp(const std::list<std::string>& grpId, uint8_t hops) { return 0;}
};



const uint16_t RS_SERVICE_TYPE_DUMMY = 0x01;
const uint8_t RS_PKT_SUBTYPE_DUMMY_MSG = 0x02;
const uint8_t RS_PKT_SUBTYPE_DUMMY_GRP = 0x03;


class RsDummyMsg : public RsGxsMsgItem
{
public:
    RsDummyMsg() : RsGxsMsgItem(RS_SERVICE_TYPE_DUMMY, RS_PKT_SUBTYPE_DUMMY_MSG) { return; }
    virtual ~RsDummyMsg() { return; }

    std::string msgData;

    std::ostream &print(std::ostream &out, uint16_t indent = 0){ return out; }
    void clear() { msgData.clear(); }

};

class RsDummyGrp : public RsGxsGrpItem
{
public:

    RsDummyGrp() : RsGxsGrpItem(RS_SERVICE_TYPE_DUMMY, RS_PKT_SUBTYPE_DUMMY_GRP) { return; }
    virtual ~RsDummyGrp() { return; }



    std::string grpData;
    void clear() { grpData.clear(); }
    std::ostream &print(std::ostream &out, uint16_t indent = 0){ return out; }
};



class RsDummySerialiser : public RsSerialType
{

public:


    RsDummySerialiser()
    :RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_DUMMY)
    { return; }
    virtual     ~RsDummySerialiser() { return; }

    uint32_t    size(RsItem *item);
    bool        serialise  (RsItem *item, void *data, uint32_t *size);
    RsItem *    deserialise(void *data, uint32_t *size);

    private:

    uint32_t    sizeDummyMsgItem(RsDummyMsg *item);
    bool        serialiseDummyMsgItem  (RsDummyMsg *item, void *data, uint32_t *size);
    RsDummyMsg *    deserialiseDummyMsgItem(void *data, uint32_t *size);

    uint32_t    sizeDummyGrpItem(RsDummyGrp *item);
    bool        serialiseDummyGrpItem  (RsDummyGrp *item, void *data, uint32_t *size);
    RsDummyGrp *    deserialiseDummyGrpItem(void *data, uint32_t *size);


};




#endif // RSDUMMYSERVICES_H
