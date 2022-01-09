/*******************************************************************************
 * libretroshare/src/pqi: pqiproxy.h                                           *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2013 by Robert Fernie.                                       *
 * Copyright 2004-2021 by retroshare team <retroshare.project@gmail.com>       *
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
 *******************************************************************************/

#pragma once

#include <string>

class pqiproxyconnection
{
public:
    enum ProxyState: uint8_t {
        PROXY_STATE_FAILED			        = 0x00,
        PROXY_STATE_INIT			        = 0x01,
        PROXY_STATE_WAITING_METHOD_RESPONSE	= 0x02,
        PROXY_STATE_WAITING_SOCKS_RESPONSE	= 0x03,
        PROXY_STATE_CONNECTION_COMPLETE		= 0x04
    };

    pqiproxyconnection() : mProxyState(PROXY_STATE_INIT) {}

    /*!
     * \brief proxy_negotiate_connection
     * 			Negotiate the connection with the proxy that is connected with openned socket sockfd. The caller needs to
     * 			connect the socket *before* trying to call proxy_negotiate_connection(). The function must be called as many times as
     * 			necessary until it returns 1 (success) or -1 (error) in which case the socket needs to be closed.
     * \return
     * 		-1 : error. The socket must be closed as soon as possible.
     * 		 0 : in progress. The function needs to be called again asap.
     * 		 1 : proxy connection is fully negociated. Client can send data to the socket.
     */
    int proxy_negociate_connection(int sockfd);

    void setRemotePort(uint16_t v) { mRemotePort = v; }
    void setRemoteAddress(const std::string& s) { mDomainAddress = s; }

    ProxyState proxyConnectionState() const { return mProxyState ; }

    void proxy_init() { mProxyState = PROXY_STATE_INIT; }
private:

    ProxyState mProxyState;

    std::string mDomainAddress;
    uint16_t mRemotePort;

    // These are the internal steps in setting up the Proxy Connection.
    int Proxy_Send_Method(int sockfd);
    int Proxy_Method_Response(int sockfd);
    int Proxy_Send_Address(int sockfd);
    int Proxy_Connection_Complete(int sockfd);
};

