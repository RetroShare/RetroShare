#ifndef RSGXSMETA_H
#define RSGXSMETA_H

#include <string>

#include "serialiser/rsserial.h"
#include "serialiser/rstlvbase.h"
#include "serialiser/rstlvtypes.h"
#include "serialiser/rstlvkeys.h"

class RsGxsGrpMetaData
{
public:

    RsGxsGrpMetaData();
    bool deserialise(void *data, uint32_t &pktsize);
    bool serialise(void* data, uint32_t &pktsize);
    uint32_t serial_size();
    void clear();


    std::string mGroupId;
    std::string mOrigGrpId;
    std::string mGroupName;
    uint32_t    mGroupFlags;
    uint32_t    mPublishTs;
    std::string mAuthorId;


    RsTlvKeySignature adminSign;
    RsTlvSecurityKeySet keys;
    RsTlvKeySignature idSign;



    // BELOW HERE IS LOCAL DATA, THAT IS NOT FROM MSG.

    uint32_t    mSubscribeFlags;

    uint32_t    mPop; // HOW DO WE DO THIS NOW.
    uint32_t    mMsgCount; // ???
    time_t      mLastPost; // ???

    uint32_t    mGroupStatus;

};




class RsGxsMsgMetaData
{
public:

    RsGxsMsgMetaData();
    bool deserialise(void *data, uint32_t *size);
    bool serialise(void* data, uint32_t *size);
    uint32_t serial_size();
    void clear();

    std::string mGroupId;
    std::string mMsgId;

    std::string mThreadId;
    std::string mParentId;
    std::string mOrigMsgId;
    std::string mAuthorId;

    RsTlvKeySignature pubSign;
    RsTlvKeySignature idSign;

    std::string mMsgName;
    time_t      mPublishTs;
    uint32_t    mMsgFlags; // Whats this for?

    // BELOW HERE IS LOCAL DATA, THAT IS NOT FROM MSG.
    // normally READ / UNREAD flags. LOCAL Data.
    uint32_t    mMsgStatus;
    time_t      mChildTs;

};





#endif // RSGXSMETA_H
