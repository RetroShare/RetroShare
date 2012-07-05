#ifndef P3PHOTOSERVICEV2_H
#define P3PHOTOSERVICEV2_H

#include "gxs/rsgenexchange.h"
#include "retroshare/rsphotoV2.h"
#include "gxs/rstokenservice.h"

class p3PhotoServiceV2 : public RsPhotoV2, public RsGenExchange
{
public:

    p3PhotoServiceV2();

public:

    bool updated();

public:

    /** Requests **/

    void groupsChanged(std::list<std::string>& grpIds);


    void msgsChanged(std::map<std::string,
                             std::vector<std::string> >& msgs);

    RsTokenService* getTokenService();

    bool getGroupList(const uint32_t &token,
                              std::list<std::string> &groupIds);
    bool getMsgList(const uint32_t &token,
                            std::list<std::string> &msgIds);

    /* Generic Summary */
    bool getGroupSummary(const uint32_t &token,
                                 std::list<RsGroupMetaData> &groupInfo);

    bool getMsgSummary(const uint32_t &token,
                               MsgMetaResult &msgInfo);

    /* Specific Service Data */
    bool getAlbum(const uint32_t &token, RsPhotoAlbum &album);
    bool getPhoto(const uint32_t &token, PhotoResult &photo);



public:

    /** Modifications **/

    bool submitAlbumDetails(RsPhotoAlbum &album, bool isNew);
    bool submitPhoto(RsPhotoPhoto &photo, bool isNew);
    bool subscribeToAlbum(const std::string& grpId, bool subscribe);
};

#endif // P3PHOTOSERVICEV2_H
