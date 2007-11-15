/*
 * "$Id: pqimon.cc,v 1.2 2007-02-18 21:46:49 rmf24 Exp $"
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2004-2006 by Robert Fernie.
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

/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)

#include "pqi/xpgpcert.h"

#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

#include "pqi/sslcert.h"

#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

#include "pqi/pqimon.h"


pqimonitor::pqimonitor() { return; }
pqimonitor::~pqimonitor() { return; }


int     pqimonitor::tick()
{

	/* grab the sslroot - and check the state of the peers */
	sslroot *sslr = getSSLRoot();
	std::list<cert *> &certlist = sslr->getCertList();
	std::list<cert *>::iterator it;
	std::list<pqipeer>::iterator pit;
	std::list<pqimonclient *>::iterator cit;

	//std::cerr << "pqimonitor::tick() plist:" << plist.size() << " clist:" << certlist.size() << std::endl;

	bool ok = true;
	if (plist.size() != certlist.size())
	{
		ok = false;
	}

	pit = plist.begin();
	for(it = certlist.begin(); (ok) && (it != certlist.end()); it++, pit++)
	{
		certsign sign;
		if (!sslr->getcertsign(*it, sign))
		{
			ok = false;
			break;
		}

		std::string id = convert_to_str(sign);

		if (id != pit->id)
		{
			ok = false;
			break;
		}
	}

	if (!ok)
	{
		//std::cerr << "pqimonitor::tick() Updating plist" << std::endl;
		/* copy the list */
		plist.clear();
		for(it = certlist.begin(); it != certlist.end(); it++)
		{
			/* list */
			certsign sign;
			if (!sslr->getcertsign(*it, sign))
			{
				std::cerr << "Major Error in pqimonitor!";
				std::cerr << std::endl;
				exit(1);
			}

			pqipeer peer;
			peer.id = convert_to_str(sign);
			peer.name = (*it)->Name();
			if ((*it)->Connected())
			{
				peer.state = PQI_PEER_STATE_ONLINE;
			}
			else
			{
				peer.state = PQI_PEER_STATE_OFFLINE;
			}
			plist.push_back(peer);
		}

		/* now notify clients */
		for(cit = clients.begin(); cit != clients.end(); cit++)
		{
			//std::cerr << "pqimonitor::tick() Calling Client" << std::endl;
			(*cit)->monUpdate(plist);
		}
	}
	return 0; /* don't make more work for anyone */
}



