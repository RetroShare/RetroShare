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

#include "pqi/pqisslpersongrp.h"
#include "pqi/pqidebug.h"
#include "pqi/p3authmgr.h"

#include <sstream>

const int pqipersongrpzone = 354;

/********************************** SSL Specific features ***************************/

#include "pqi/pqissl.h"
#include "pqi/pqissllistener.h"

#ifdef PQI_USE_PROXY
  #include "pqi/pqissludp.h"
#endif

pqilistener * pqisslpersongrp::createListener(struct sockaddr_in laddr)
{
	p3AuthMgr *authMgr = getAuthMgr();
	pqilistener *listener = new pqissllistener(laddr, authMgr, mConnMgr);
	return listener;
}

pqiperson * pqisslpersongrp::createPerson(std::string id, pqilistener *listener)
{
	{
		std::ostringstream out;
		out << "pqipersongrp::createPerson() PeerId: " << id;
		pqioutput(PQL_DEBUG_BASIC, pqipersongrpzone, out.str());
	}

	p3AuthMgr *authMgr = getAuthMgr();
	pqiperson *pqip = new pqiperson(id, this);
	pqissl *pqis   = new pqissl((pqissllistener *) listener, pqip, authMgr, mConnMgr);

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

#ifdef PQI_USE_PROXY
	pqissludp *pqius 	= new pqissludp(pqip, authMgr, mConnMgr);

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


