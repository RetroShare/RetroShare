/*******************************************************************************
 * libretroshare/src/file_sharing: fsclient.h                                  *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2021 by retroshare team <retroshare.project@gmail.com>            *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 ******************************************************************************/

#include <string>
#include "fsitem.h"
#include "pqi/pqi_base.h"

// This class runs a client connection to the friend server. It opens a socket at each connection.

class FsClient: public PQInterface
{
public:
    FsClient() :PQInterface(RsPeerId()) {}

    bool requestFriends(const std::string& address,uint16_t port,uint32_t reqs,std::map<std::string,bool>& friend_certificates);

protected:
    // Implements PQInterface

    bool RecvItem(RsItem *item) override;
    int  SendItem(RsItem *) override { RsErr() << "FsClient::SendItem() called although it should not." ; return 0;}
    RsItem *GetItem() override;

private:
    bool sendItem(const std::string &address, uint16_t port, RsItem *item, std::list<RsItem *> &response);
    void handleServerResponse(RsFriendServerServerResponseItem *item);

    std::list<RsItem*> mIncomingItems;
};

