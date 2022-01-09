/*******************************************************************************
 * libretroshare/src/pqi: pqisslproxy.h                                        *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2013 by Robert Fernie <retroshare@lunamutt.com>              *
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
#include "pqi/pqisslproxy.h"
#include "pqi/pqinetwork.h"

#include <errno.h>
#include <openssl/err.h>

#include <sstream>

#include "util/rsdebug.h"
#include "util/rsnet.h"

#include "pqi/p3linkmgr.h"

static struct RsLog::logInfo pqisslproxyzoneInfo = {RsLog::Default, "pqisslproxy"};
#define pqisslproxyzone &pqisslproxyzoneInfo

// #define PROXY_DEBUG	1
// #define PROXY_DEBUG_LOG	1

pqisslproxy::pqisslproxy(pqissllistener *l, PQInterface *parent, p3LinkMgr *lm)
        :pqissl(l, parent, lm)
{
	sockaddr_storage_clear(remote_addr);
	return;
}


pqisslproxy::~pqisslproxy()
{ 
        rslog(RSL_ALERT, pqisslproxyzone,  
            "pqisslproxy::~pqisslproxy -> destroying pqisslproxy");

	stoplistening();
	reset();

	return;
}

int 	pqisslproxy::Initiate_Connection()
{

#ifdef PROXY_DEBUG
	std::cerr << "pqisslproxy::Initiate_Connection()";
	std::cerr << std::endl;
#endif

  	rslog(RSL_DEBUG_BASIC, pqisslproxyzone, 
		 "pqisslproxy::Initiate_Connection() Connection to Proxy");
	/* init proxy state */
    proxy_init();

	/* call standard Init_Conn() */
	return pqissl::Initiate_Connection();
}


/********* VERY DIFFERENT **********/
int 	pqisslproxy::Basic_Connection_Complete()
{
    rslog(RSL_DEBUG_BASIC, pqisslproxyzone,
          "pqisslproxy::Basic_Connection_Complete()...");

#ifdef PROXY_DEBUG
    std::cerr << "pqisslproxy::Basic_Connection_Complete() STATE: " << mProxyState;
    std::cerr << std::endl;
#endif

    if (CheckConnectionTimeout())
    {
        // calls reset.
        return -1;
    }

    int ret;

    if(proxyConnectionState() == PROXY_STATE_INIT && 1!=(ret=pqissl::Basic_Connection_Complete()))
        return ret; // basic connection not complete.

    ret = proxy_negociate_connection(sockfd);

    if(ret < 0)
        reset_locked();

    return ret;
}

bool pqisslproxy::connect_parameter(uint32_t type, const std::string &value)
{
	{
		RsStackMutex stack(mSslMtx); /**** LOCKED MUTEX ****/
	
	        if (type == NET_PARAM_CONNECT_DOMAIN_ADDRESS)
	        {
	                std::string out;
	                rs_sprintf(out, "pqisslproxy::connect_parameter() Peer: %s DOMAIN_ADDRESS: %s", PeerId().toStdString().c_str(), value.c_str());
#ifdef PROXY_DEBUG_LOG
	                rslog(RSL_WARNING, pqisslproxyzone, out);
#endif
                    setRemoteAddress(value);
#ifdef PROXY_DEBUG
	                std::cerr << out << std::endl;
#endif
	                return true;
	        }
	}

        return pqissl::connect_parameter(type, value);
}

bool pqisslproxy::connect_parameter(uint32_t type, uint32_t value)
{
	{
		RsStackMutex stack(mSslMtx); /**** LOCKED MUTEX ****/

	        if (type == NET_PARAM_CONNECT_REMOTE_PORT)
	        {
	                std::string out;
	                rs_sprintf(out, "pqisslproxy::connect_parameter() Peer: %s REMOTE_PORT: %lu", PeerId().toStdString().c_str(), value);
#ifdef PROXY_DEBUG_LOG
	                rslog(RSL_WARNING, pqisslproxyzone, out);
#endif
                    setRemotePort(value);
#ifdef PROXY_DEBUG
	                std::cerr << out << std::endl;
#endif
	                return true;
	        }
	}
        return pqissl::connect_parameter(type, value);
}




