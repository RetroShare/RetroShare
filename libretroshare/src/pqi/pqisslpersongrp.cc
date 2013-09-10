/*
 * libretroshare/src/pqi: pqisslpersongrp.cc
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2004-2008 by Robert Fernie.
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

#include "util/rsdebug.h"

#include "pqi/pqisslpersongrp.h"
#include "pqi/authssl.h"


const int pqipersongrpzone = 354;

/****
 * #define PQI_DISABLE_UDP 1
 ***/

/********************************** SSL Specific features ***************************/

#include "pqi/pqissl.h"
#include "pqi/pqissllistener.h"

#ifndef PQI_DISABLE_TUNNEL
#include "pqi/pqissltunnel.h"
#endif

#ifndef PQI_DISABLE_UDP
  #include "pqi/pqissludp.h"
#endif


pqilistener * pqisslpersongrp::locked_createListener(struct sockaddr_in laddr)
{
	pqilistener *listener = new pqissllistener(laddr, mPeerMgr);
	return listener;
}

pqiperson * pqisslpersongrp::locked_createPerson(std::string id, pqilistener *listener)
{
	pqioutput(PQL_DEBUG_BASIC, pqipersongrpzone, "pqipersongrp::createPerson() PeerId: " + id);

	pqiperson *pqip = new pqiperson(id, this);
	pqissl *pqis   = new pqissl((pqissllistener *) listener, pqip, mLinkMgr);

	/* construct the serialiser ....
	 * Needs:
	 * * FileItem
	 * * FileData
	 * * ServiceGeneric
	 */

	RsSerialiser *rss = new RsSerialiser();
	rss->addSerialType(new RsFileItemSerialiser());
	rss->addSerialType(new RsCacheItemSerialiser());
	rss->addSerialType(new RsServiceSerialiser());

	pqiconnect *pqisc = new pqiconnect(rss, pqis);

	pqip -> addChildInterface(PQI_CONNECT_TCP, pqisc);

#ifndef PQI_DISABLE_TUNNEL
	pqissltunnel *pqitun 	= new pqissltunnel(pqip, mLinkMgr);

	RsSerialiser *rss3 = new RsSerialiser();
	rss3->addSerialType(new RsFileItemSerialiser());
	rss3->addSerialType(new RsCacheItemSerialiser());
	rss3->addSerialType(new RsServiceSerialiser());
	pqiconnect *pqicontun 	= new pqiconnect(rss3, pqitun);
	pqip -> addChildInterface(PQI_CONNECT_TUNNEL, pqicontun);
#endif

#ifndef PQI_DISABLE_UDP
	pqissludp *pqius 	= new pqissludp(pqip, mLinkMgr);

	RsSerialiser *rss2 = new RsSerialiser();
	rss2->addSerialType(new RsFileItemSerialiser());
	rss2->addSerialType(new RsCacheItemSerialiser());
	rss2->addSerialType(new RsServiceSerialiser());
	
	pqiconnect *pqiusc 	= new pqiconnect(rss2, pqius);

	// add a ssl + proxy interface.
	// Add Proxy First.
	pqip -> addChildInterface(PQI_CONNECT_UDP, pqiusc);
#endif

	return pqip;
}


/********************************** SSL Specific features ***************************/


