#ifndef RSDUMMYSERVICES_H
#define RSDUMMYSERVICES_H


// dummy services to make

#include "gxs/rsnxs.h"
#include "gxs/rsgixs.h"
#include "serialiser/rsgxsitems.h"

class RsDummyNetService: public RsNetworkExchangeService
{
public:

     RsDummyNetService(){ return;}
     virtual ~RsDummyNetService() { }

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
    : RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_DUMMY)
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

/*!
 * Dummy implementation of Gixs service for
 * testing
 * Limited to creating two ids upon construction which can be used
 * for signing data
 */
class RsGixsDummy : public RsGixs
{

public:

    /*!
     * constructs keys for both incoming and outgoing id (no private keys for incoming id)
     * @param
     * @param dummyId This is is the only id thats exists in this dummy interface
     */
    RsGixsDummy(const RsGxsId& incomingId, const RsGxsId& outgoingId){}

    virtual ~RsGixsDummy(){}

    /*!
     *
     * @return id used for signing incoming data (should have both public and private components)
     */
    const RsGxsId& getOutgoing(){ return mOutgoingId; }

    /*!
     *
     * @return id used for signing outgoing data(only have public parts)
     */
    const RsGxsId& getIncoming(){ return mIncomingId; }

    // Key related interface - used for validating msgs and groups.
    /*!
     * Use to query a whether given key is available by its key reference
     * @param keyref the keyref of key that is being checked for
     * @return true if available, false otherwise
     */
    bool haveKey(const RsGxsId &id){ return false;}

    /*!
     * Use to query whether private key member of the given key reference is available
     * @param keyref the KeyRef of the key being checked for
     * @return true if private key is held here, false otherwise
     */
    bool havePrivateKey(const RsGxsId &id){ return false; }

    // The fetchKey has an optional peerList.. this is people that had the msg with the signature.
    // These same people should have the identity - so we ask them first.
    /*!
     * Use to request a given key reference
     * @param keyref the KeyRef of the key being requested
     * @return will
     */
    bool requestKey(const RsGxsId &id, const std::list<PeerId> &peers){ return false ;}
    bool requestPrivateKey(const RsGxsId &id){ return false;}


    /*!
     * Retrieves a key identity
     * @param keyref
     * @return a pointer to a valid profile if successful, otherwise NULL
     *
     */
    int  getKey(const RsGxsId &id, RsTlvSecurityKey &key){ return false; }
    int  getPrivateKey(const RsGxsId &id, RsTlvSecurityKey &key){ return false; }	// For signing outgoing messages.

private:

    RsGxsId mIncomingId, mOutgoingId;
};


#endif // RSDUMMYSERVICES_H
