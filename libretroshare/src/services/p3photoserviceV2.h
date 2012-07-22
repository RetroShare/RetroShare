#ifndef P3PHOTOSERVICEV2_H
#define P3PHOTOSERVICEV2_H

/*
 * libretroshare/src/retroshare: rsphoto.h
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2008-2012 by Robert Fernie, Christopher Evi-Parker
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#include "gxs/rsgenexchange.h"
#include "retroshare/rsphotoV2.h"


class p3PhotoServiceV2 : public RsPhotoV2, public RsGenExchange
{
public:

    p3PhotoServiceV2(RsGeneralDataService* gds, RsNetworkExchangeService* nes);

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
