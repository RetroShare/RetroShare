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

    /*!
     * @return
     */
    bool updated();

public:

    /** Requests **/

    void groupsChanged(std::list<std::string>& grpIds);


    void msgsChanged(std::map<std::string,
                             std::vector<std::string> >& msgs);

    RsTokenServiceV2* getTokenService();

    bool getGroupList(const uint32_t &token,
                              std::list<std::string> &groupIds);
    bool getMsgList(const uint32_t &token,
                            GxsMsgIdResult& msgIds);

    /* Generic Summary */
    bool getGroupSummary(const uint32_t &token,
                                 std::list<RsGroupMetaData> &groupInfo);

    bool getMsgSummary(const uint32_t &token,
                               MsgMetaResult &msgInfo);

    /* Specific Service Data */
    bool getAlbum(const uint32_t &token, std::vector<RsPhotoAlbum> &albums);
    bool getPhoto(const uint32_t &token, PhotoResult &photos);

public:

    /** Modifications **/

    bool submitAlbumDetails(RsPhotoAlbum &album);
    bool submitPhoto(RsPhotoPhoto &photo);
};

#endif // P3PHOTOSERVICEV2_H
