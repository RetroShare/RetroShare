/*******************************************************************************
 * libretroshare/src/pqi: pqisslpersongrp.cc                                   *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2008 by Robert Fernie.                                       *
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
#include "serialiser/rsserializer.h"
#include "services/autoproxy/rsautoproxymonitor.h"
#include "util/rsdebug.h"

#include "pqi/pqisslpersongrp.h"
#include "pqi/authssl.h"

static struct RsLog::logInfo pqipersongrpzoneInfo = {RsLog::Default, "pqipersongrp"};
#define pqipersongrpzone &pqipersongrpzoneInfo

/****
 * #define PQI_DISABLE_UDP 1
 ***/

/********************************** SSL Specific features ***************************/

#include "pqi/pqissl.h"
#include "pqi/pqissllistener.h"
#include "pqi/p3peermgr.h"

//#define PQISSLPERSON_DEBUG

#ifndef PQI_DISABLE_UDP
  #include "pqi/pqissludp.h"
#endif

#include "pqi/pqisslproxy.h"
#include "pqi/pqissli2psam3.h"

pqilistener * pqisslpersongrp::locked_createListener(const struct sockaddr_storage &laddr)
{
	pqilistener *listener = new pqissllistener(laddr, mPeerMgr);
	return listener;
}

pqiperson * pqisslpersongrp::locked_createPerson(const RsPeerId& id, pqilistener *listener)
{
#ifdef PQISSLPERSON_DEBUG
	std::cerr << "pqisslpersongrp::locked_createPerson() PeerId: " << id;
    std::cerr << std::endl;
#endif

	pqioutput(PQL_DEBUG_BASIC, pqipersongrpzone, "pqipersongrp::createPerson() PeerId: " + id.toStdString());

	pqiperson *pqip = new pqiperson(id, this);

	// If using proxy, then only create a proxy item, otherwise can use any.
	// If we are a hidden node - then all connections should be via proxy.
	if (mPeerMgr->isHiddenPeer(id) || mPeerMgr->isHidden())
	{
#ifdef PQISTREAMER_DEBUG
        std::cerr << "pqisslpersongrp::locked_createPerson() Is Hidden Peer!";
        std::cerr << std::endl;
#endif

		// Use pqicI2P for I2P
		pqiconnect *pqicSOCKSProxy, *pqicI2P;
		{
			pqisslproxy  *pqis = new pqisslproxy((pqissllistener *) listener, pqip, mLinkMgr);
			RsSerialiser *rss  = new RsSerialiser();
			rss->addSerialType(new RsRawSerialiser());
			pqicSOCKSProxy = new pqiconnect(pqip, rss, pqis);
		}

		if (rsAutoProxyMonitor::instance()->isEnabled(autoProxyType::I2PSAM3))
		{
			pqissli2psam3 *pqis = new pqissli2psam3((pqissllistener *) listener, pqip, mLinkMgr);
			RsSerialiser *rss  = new RsSerialiser();
			rss->addSerialType(new RsRawSerialiser());

			pqicI2P = new pqiconnect(pqip, rss, pqis);
		} else {
			pqicI2P = pqicSOCKSProxy;
		}

		/* first select type based on peer */
		uint32_t typePeer = mPeerMgr->getHiddenType(id);
		switch (typePeer) {
		case RS_HIDDEN_TYPE_TOR:
			pqip -> addChildInterface(PQI_CONNECT_HIDDEN_TOR_TCP, pqicSOCKSProxy);
			break;
		case RS_HIDDEN_TYPE_I2P:
			pqip -> addChildInterface(PQI_CONNECT_HIDDEN_I2P_TCP, pqicI2P);
			break;
		default:
			/* peer is not a hidden one but we are */
			/* select type based on ourselves */
			uint32_t typeOwn = mPeerMgr->getHiddenType(AuthSSL::getAuthSSL()->OwnId());
			switch (typeOwn) {
			case RS_HIDDEN_TYPE_I2P:
				pqip -> addChildInterface(PQI_CONNECT_HIDDEN_I2P_TCP, pqicI2P);
				break;
			default:
				/* this case shouldn't happen! */
				std::cerr << "pqisslpersongrp::locked_createPerson WARNING INVALID HIDDEN TYPES - THIS SHOULD NOT HAPPEN!" << std::endl;
				std::cerr << " - ID: " << id << std::endl;
				std::cerr << " - mPeerMgr->isHidden(): " << mPeerMgr->isHidden() << std::endl;
				std::cerr << " - mPeerMgr->isHiddenPeer(id): " << mPeerMgr->isHiddenPeer(id) << std::endl;
				std::cerr << " - hidden types: peer=" << typePeer << " own=" << typeOwn << std::endl;
				std::cerr << " --> falling back to Tor" << std::endl;
				/* fallthrough */
			case RS_HIDDEN_TYPE_TOR:
				pqip -> addChildInterface(PQI_CONNECT_HIDDEN_TOR_TCP, pqicSOCKSProxy);
				break;
			}
		}
	}
	else
	{	
#ifdef PQISTREAMER_DEBUG
        std::cerr << "pqisslpersongrp::locked_createPerson() Is Normal Peer!";
        std::cerr << std::endl;
#endif

		pqissl *pqis   = new pqissl((pqissllistener *) listener, pqip, mLinkMgr);
	
		/* construct the serialiser ....
		 * Needs:
		 * * FileItem
		 * * FileData
		 * * ServiceGeneric
		 */
	
		ssl_tunnels[id] = pqis ;	// keeps for getting crypt info per peer.
	
		RsSerialiser *rss = new RsSerialiser();
		rss->addSerialType(new RsRawSerialiser());
	
		pqiconnect *pqisc = new pqiconnect(pqip, rss, pqis);
	
		pqip -> addChildInterface(PQI_CONNECT_TCP, pqisc);
	
#ifndef PQI_DISABLE_UDP
		pqissludp *pqius 	= new pqissludp(pqip, mLinkMgr);
	
		RsSerialiser *rss2 = new RsSerialiser();
		rss2->addSerialType(new RsRawSerialiser());
		
		pqiconnect *pqiusc 	= new pqiconnect(pqip, rss2, pqius);
	
		// add a ssl + proxy interface.
		// Add Proxy First.
		pqip -> addChildInterface(PQI_CONNECT_UDP, pqiusc);
#endif
	}

    return pqip;
}

void pqisslpersongrp::disconnectPeer(const RsPeerId &peer)
{
    std::map<RsPeerId,pqissl*>::iterator it = ssl_tunnels.find(peer) ;

    if(it != ssl_tunnels.end())
        it->second->disconnect() ;
    else
        std::cerr << "pqisslpersongrp::cannot find peer " << peer << ". cannot disconnect!" << std::endl;
}


/********************************** SSL Specific features ***************************/


