/*
 * "$Id: pqiproxy.cc,v 1.10 2007-02-18 21:46:49 rmf24 Exp $"
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





#include "pqi/pqiproxy.h"

#include "pqi/pqipacket.h"
#include "pqi/pqitunnelproxy.h"

#include "pqi/p3disc.h"

#include <sstream>
#include "pqi/pqidebug.h"

const int pqiproxyzone = 1728;


static const unsigned long P3PROXY_CONNECT_TIMEOUT = 30;

// PQTunnelService Overloading.
int 	p3proxy::receive(PQTunnel *in)
{
	{
		std::ostringstream out;
		out << "p3proxy::receive()";
		out << " Received PQTunnel Packet!" << std::endl;
		in -> print(out);
		pqioutput(PQL_DEBUG_ALL, pqiproxyzone, out.str());
	}

	PQTunnelProxy *pqtp = dynamic_cast<PQTunnelProxy *>(in);
	if (pqtp == NULL)
	{
		delete in;
		pqioutput(PQL_WARNING, pqiproxyzone, 
			"PQTunnel is Not a PQTunnelProxy Pkt...Deleting");
		return -1;
	}


	cert *from = (cert *) pqtp -> p;

	pqioutput(PQL_DEBUG_ALL,pqiproxyzone, "Finding Src");
	cert *src = findcert(pqtp -> src);
	pqioutput(PQL_DEBUG_ALL,pqiproxyzone, "Finding Dest");
	cert *dest = findcert(pqtp -> dest);

	if (from != NULL)
	{
		std::ostringstream out;
		out << "ProxyTunnel Pkt from: " << from -> Name() << std::endl;
		pqioutput(PQL_DEBUG_BASIC,pqiproxyzone, out.str());
	}

	if (src != NULL)
	{
		std::ostringstream out;
		out << "ProxyTunnel Pkt Src: " << src -> Name() << std::endl;
		pqioutput(PQL_DEBUG_BASIC,pqiproxyzone, out.str());
	}

	if (dest != NULL)
	{
		std::ostringstream out;
		out << "ProxyTunnel Pkt dest: " << dest -> Name() << std::endl;
		pqioutput(PQL_DEBUG_BASIC,pqiproxyzone, out.str());
	}

	// if either is unknown...
	if ((dest == NULL) || (src == NULL))
	{
		// ignore/discard packet.
		pqioutput(PQL_WARNING, pqiproxyzone, 
				"Unknown Source/Destination");
		delete pqtp;
		return -1;
	}

	// if they are in the wrong order.
	// 1) dest == real source (we are proxy)
	// 2) own = src (we are dest)
	cert *own = sroot -> getOwnCert();
	if ((dest == (cert *) pqtp -> p) || (src == own))
	{
		// swap them.
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, 
				"Swapping Dest/Src Order");
		cert *tmp = dest;
		dest = src;
		src = tmp;
	}

	// check if we are a proxy 
	if (dest != own)
	{
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, 
			"Packet Not Meant for us....");
		// forward onto the correct location.
		// all we need to do is correct the cid,
		// and filter....

		if (isConnection(src, dest))
		{
			std::ostringstream out;
			out << "Proxy Packet Connection (" << src -> Name();
			out << " -> " << dest -> Name() << ") is Okay - Packet received" << std::endl;
			pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());

			return receiveAsProxy(pqtp, src, dest);
		}

		// delete packet...
		{
			pqioutput(PQL_WARNING, pqiproxyzone, 
				"No Connection/Packet Filtered -> deleting");
			delete pqtp;
			return -1;
		}
	}

	// we are the destination.

	// search for it.
	std::map<cert *, cert *>::iterator it;	
	std::map<cert *, pqiproxy *>::iterator pit;	
	
	// first ensure that we have a connection.
	if (connectionmap.end() == (it = connectionmap.find(src)))
	{
		pqioutput(PQL_WARNING, pqiproxyzone, 
			"Packet Reject Not Connected");
		// not connected.. reject.
		delete pqtp;
		return -1;
	}
	else
	{
		std::ostringstream out;
		out << "Proxy Packet Over Connection (" << it->first->Name();
		out << " -> " << it -> second -> Name() << std::endl;
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
	}

	if (proxymap.end() == (pit = proxymap.find(src)))
	{
		pqioutput(PQL_WARNING, pqiproxyzone, 
			"Packet Reject Not in ProxyMap");
		// no proxy. reject.
		delete pqtp;
		return -1;
	}
	else
	{
		std::ostringstream out;
		out << "Proxy Found in ProxyMap";
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
	}

	if (it -> second != from)
	{
		std::ostringstream out;
		out << "Packet Reject Mismatch Proxy : ";
		out << it -> second -> Name() << " != ";
		out << from -> Name();
		pqioutput(PQL_WARNING, pqiproxyzone, out.str());

		// mismatch reject.
		delete pqtp;
		return -1;
	}

	// done!
	{
		std::ostringstream out;
		out << "Proxy Packet PASSES OK (" << src->Name();
		out << " -> " << from -> Name();
		out << " -> " << dest -> Name() << std::endl;
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
	}

	return receiveAsDestination(pqtp, src, from, dest, pit -> second);
}


int 	p3proxy::receiveAsProxy(PQTunnelProxy *pqtp, cert *src, cert *dest)
{
	{
		std::ostringstream out;
		out << "p3proxy::receiveAsProxy()";
		out << " Received PQTunnel Packet!" << std::endl;
		pqtp -> print(out);
		pqioutput(PQL_DEBUG_ALL, pqiproxyzone, out.str());
	}

	if (isConnection(src, dest))
	{
		std::ostringstream out;
		out << "Proxy Packet Connection (" << src -> Name();
		out << " -> " << dest -> Name() << ") is Okay" << std::endl;
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());

		if ((filter(pqtp)))
		{
			pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, 
				"Packet Okay to Proxy, Sending Out");
			pqtp -> cid = dest -> cid;
			// stick into the out queue.
			outPkts.push_back(pqtp);
			return 1;
		}
		else
		{
			pqioutput(PQL_WARNING, pqiproxyzone, 
				"Packet Fails Filter Test!");
		}
	}

	// delete packet...
	{
		pqioutput(PQL_WARNING, pqiproxyzone, 
			"No Connection/Packet Filtered -> deleting");
		delete pqtp;
		return -1;
	}
}


int 	p3proxy::receiveAsDestination(PQTunnelProxy *pqtp, cert *src, cert *from, cert *dest, pqiproxy *pqip)
{
	{
		std::ostringstream out;
		out << "p3proxy::receiveAsDestination(" << src->Name();
		out << " -> " << from -> Name();
		out << " -> " << dest -> Name() << std::endl;
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
	}

	// push_back to correct queue.
 	std::map<pqiproxy *, std::list<PQTunnelProxy *> >::iterator qit;
	if (inqueue.end() == (qit = inqueue.find(pqip)))
	{
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, 
			"p3proxy::receiveAsDestination() Packet Pushed Onto New Queue");
		// make a new list.
		std::list<PQTunnelProxy *> nlist;
		nlist.push_back(pqtp);
		inqueue[pqip] = nlist;
		return 1;
	}

	// add to existing queue.
	pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, 
		"p3proxy::receiveAsDestination() Packet Pushed Onto Existing Queue");

	(qit->second).push_back(pqtp);
	return 1;
}

	
int     p3proxy::receiveAsError(PQTunnelProxy* pqtp, cert *src, cert *from, cert *dest)
{
	{
		std::ostringstream out;
		out << "p3proxy::receiveAsError()";
		out << " Received Bad PQTunnel Packet!" << std::endl;
		pqtp -> print(out);
		pqioutput(PQL_ALERT, pqiproxyzone, out.str());
		delete pqtp;
	}
	return 1;
}


PQTunnel *p3proxy::send()
{
	pqioutput(PQL_DEBUG_ALL, pqiproxyzone, 
		"p3proxy::send()");

	if (outPkts.size() < 1)
		return NULL;

	PQTunnel *in = outPkts.front();
	outPkts.pop_front();

	std::ostringstream out;
	out << "Sending (1/" << outPkts.size() + 1 << "): " << std::endl;
	in -> print(out);
	pqioutput(PQL_DEBUG_ALL,pqiproxyzone,out.str());

	return in;
}

// decide if we let data through.
bool p3proxy::filter(PQTunnelProxy *in)
{
	pqioutput(PQL_WARNING,pqiproxyzone,
	 "p3proxy::filter() Not Implemented!");
	return true;
}

int      p3proxy::receive(PQTunnelInit *in)
{
	{
		std::ostringstream out;
		out << "p3proxy::receive()" << std::endl;
		if (in == NULL)
		{
			out << "NULL Pkt!" << std::endl;
		}
		else
		{
			in -> print(out);
		}
		pqioutput(PQL_DEBUG_ALL,pqiproxyzone, out.str());
	}

	if (in == NULL)
		return 1;

	// make sure it is a ProxyInit....
	PQTunnelProxyInit *pinit = NULL;
	if (NULL == (pinit = dynamic_cast<PQTunnelProxyInit *>(in)))
	{
		// wrong type.
		pqioutput(PQL_WARNING,pqiproxyzone,
		  "Not PQTunnelProxyInit Pkt Deleting");
		delete in;
		return -1;
	}

	cert *from = (cert *) pinit -> p;

	pqioutput(PQL_DEBUG_ALL,pqiproxyzone, "Finding Proxy");
	cert *proxy = findcert(pinit -> proxy);
	pqioutput(PQL_DEBUG_ALL,pqiproxyzone, "Finding Src");
	cert *src = findcert(pinit -> src);
	pqioutput(PQL_DEBUG_ALL,pqiproxyzone, "Finding Dest");
	cert *dest = findcert(pinit -> dest);

	if (from != NULL)
	{
		std::ostringstream out;
		out << "ProxyInit Pkt from: " << from -> Name() << std::endl;
		pqioutput(PQL_DEBUG_BASIC,pqiproxyzone, out.str());
	}

	if (src != NULL)
	{
		std::ostringstream out;
		out << "ProxyInit Pkt Src: " << src -> Name() << std::endl;
		pqioutput(PQL_DEBUG_BASIC,pqiproxyzone, out.str());
	}


	if (proxy != NULL)
	{
		std::ostringstream out;
		out << "ProxyInit Pkt proxy: " << proxy -> Name() << std::endl;
		pqioutput(PQL_DEBUG_BASIC,pqiproxyzone, out.str());
	}


	if (dest != NULL)
	{
		std::ostringstream out;
		out << "ProxyInit Pkt dest: " << dest -> Name() << std::endl;
		pqioutput(PQL_DEBUG_BASIC,pqiproxyzone, out.str());
	}

	if ((from == NULL) || (src == NULL) ||
		(proxy == NULL) || (dest == NULL))
	{
		if (!from)
		{
			pqioutput(PQL_WARNING,pqiproxyzone,
		  	"Sorry: from (IntRef) == NULL");
		}

		if (!src)
		{
			pqioutput(PQL_WARNING,pqiproxyzone,
		  	"Sorry: src (SrcRef) == NULL");
		}

		if (!proxy)
		{
			pqioutput(PQL_WARNING,pqiproxyzone,
		  	"Sorry: proxy (ProxyRef) == NULL");
		}

		if (!dest)
		{
			pqioutput(PQL_WARNING,pqiproxyzone,
		  	"Sorry: dest (DestRef) == NULL");
		}

		pqioutput(PQL_WARNING,pqiproxyzone,
		  "deleting as: who are these people!");
		delete in;
		return -1;
	}

	// At this point we need to decide if we
	// are the dest or the proxy.
	cert *own = sroot -> getOwnCert();

	// at this point we need to flip it around.
	// if 1) we are end then:
	if (src == own)
	{
		// we need to rotate dest/src, as we
		// expect to be the destination, not source.
		// so we flip them
		src = dest;
		dest = own;
	}
	// 2) if we are proxy... and it came from the destination.
	else if (dest == from)
	{
		dest = src;
		src = from;
	}

	if (dest != own)
	{
		pqioutput(PQL_DEBUG_BASIC,pqiproxyzone,
		  	"We are not the destination - so we should be proxy!");
		// this is meant for another.....
		// we are the proxy.
		if (proxy != own)
		{
			pqioutput(PQL_WARNING,pqiproxyzone,
		  		"But we are not the specified Proxy! ERROR!");
			// error!
			delete pinit;
			return -1;
		}

		// record connection possibility.
		// pass on the packet if possible.
		// if not return an endConnect.

		// check if we are connected to dest.
		if (!dest -> Connected())
		{
			pqioutput(PQL_WARNING,pqiproxyzone,
		  		"We Are Not Connected to Destination: rtn End");
			// Return an end Connection, as we cannot connect!.
			// just send back the same packet with mode changed.
			pinit -> PQTunnelInit::mode = PQTunnelInit::End;
			addOutInitPkt(pinit);
			return 1;
		}

		// we can pass it on.
		// register as a potential connection.
		PQTunnelProxyInit *pdup = pinit->clone();

		// pass on the packet.
		pinit -> cid = dest -> cid;
		addOutInitPkt(pinit);

		// must pass before registration...
		registerProxyConnection(src, proxy, dest, pdup);

		delete pdup;

		pqioutput(PQL_DEBUG_BASIC,pqiproxyzone,
		  		"Proxy Pass-Through Not Completed");
		return 1;
	}

	pqioutput(PQL_DEBUG_BASIC,pqiproxyzone,
	  		"We are the End destination of the Proxy Pkt");

	if (from != proxy)
	{
		// error this packet is not from 
		// the expected proxy.... kill.
		pqioutput(PQL_WARNING,pqiproxyzone,
		  	"But the Pkt Not from Proxy ERROR");
		delete pinit;
		return -1;
	}

	// if meant for us
	// switch on the type.
	// These Functions consume the packet,
	switch(pinit -> PQTunnelInit::mode)
	{
		case PQTunnelInit::Request:
			respondConnect(src, from, pinit);
			break;

		case PQTunnelInit::Connect:
			completeConnect(src, from, pinit);
			break;
		
		case PQTunnelInit::End:
			endConnect(src, from, pinit);
			break;
		default:

			pqioutput(PQL_WARNING,pqiproxyzone,
	  			"Unknown Mode => defaulting to endConnect");
			endConnect(src, from, pinit);
			break;
	}
	//delete pinit;
	return 1;
}

// Handling the Ones We can proxy.
int	p3proxy::registerProxyConnection(cert *src, cert *proxy, 
					cert *dest, PQTunnelProxyInit *pinit)
{
	{
		std::ostringstream out;
		out << "p3proxy::registerProxyConnection() Checking <S:";
		out << src -> Name() << ",P:" << proxy -> Name() << ">";
		out << ",D:" << dest -> Name() << ">";
		pqioutput(PQL_DEBUG_BASIC,pqiproxyzone, out.str());
	}

	cert *own = sroot -> getOwnCert();
	if (proxy != own)
	{
		// error.....
		return -1;
	}

	// get the reference.
	std::map<std::pair<cert *, cert *>, int>::iterator cit1, cit2;

	std::pair<cert *, cert *> ccdir(dest, src), ccrev(src, dest);

	cit1 = proxyconnect.find(ccdir);
	cit2 = proxyconnect.find(ccrev);

	if (proxyconnect.end() != cit1)
	{
		std::ostringstream out;
		out << "p3proxy::registerProxyConnection() Prev State(d,s):";
		out << cit1 -> second;
		pqioutput(PQL_DEBUG_BASIC,pqiproxyzone, out.str());
	}
	else
	{
		pqioutput(PQL_DEBUG_BASIC,pqiproxyzone,
		  "p3proxy::registerProxyConnection() No Prev (d,s) Pair");
	}

	if (proxyconnect.end() != cit2)
	{
		std::ostringstream out;
		out << "p3proxy::registerProxyConnection() Prev State(s,d):";
		out << cit2 -> second;
		pqioutput(PQL_DEBUG_BASIC,pqiproxyzone, out.str());
	}
	else
	{
		pqioutput(PQL_DEBUG_BASIC,pqiproxyzone,
		  "p3proxy::registerProxyConnection() No Prev (s,d) Pair");
	}

        switch(pinit -> PQTunnelInit::mode)
        {
		case PQTunnelInit::Request:
		  // insert or override... the same.
		  proxyconnect[ccdir] = 1;
		  pqioutput(PQL_DEBUG_BASIC,pqiproxyzone,
		    "p3proxy::registerProxyConnection() Setting State -> 1");
                  break;

                case PQTunnelInit::Connect:
		  if (cit1 != proxyconnect.end())
		  {
			if (cit1 -> second == 1)
			{
		  	  	pqioutput(PQL_DEBUG_BASIC,pqiproxyzone,
		          	 "p3proxy::registerProxyConnection() Switch 1 -> 2");

				// advance
		  		cit1 -> second = 2;
				if (cit2 != proxyconnect.end())
				{
					if (cit2 -> second == 2)
					{
						// complete!
		  	  	pqioutput(PQL_DEBUG_BASIC,pqiproxyzone,
		          	 "p3proxy::registerProxyConnection() Connect Complete");
				/* call the function tp handle other goodies */
				connectionCompletedAsProxy(cit1->first.first, cit2->first.first);

					}
					else
					{
		  	  	pqioutput(PQL_DEBUG_BASIC,pqiproxyzone,
		          	 "p3proxy::registerProxyConnection() Rev Connect todo");
					}
				}
				else
				{
		  	  	  pqioutput(PQL_DEBUG_BASIC,pqiproxyzone,
		          	   "p3proxy::registerProxyConnection() No Rev Conn");
				}
			}
			else
			{
		  	  	pqioutput(PQL_DEBUG_BASIC,pqiproxyzone,
		          	 "p3proxy::registerProxyConnection() Cannot Ad -> 2");
			}
		  }
		  else
		  {
		  	  pqioutput(PQL_DEBUG_BASIC,pqiproxyzone,
		           "p3proxy::registerProxyConnection() Unknown Connect!");

		  }
                  break;

		case PQTunnelInit::End:
		default:
		  // remove all the items.
		  pqioutput(PQL_DEBUG_BASIC,pqiproxyzone,
		    "p3proxy::registerProxyConnection() End: Cleaning up");
		  if (cit1 != proxyconnect.end())
		  {
			pqioutput(PQL_DEBUG_BASIC,pqiproxyzone,
		  	  "p3proxy::registerProxyConnection() End: Removing Cit1");
			  proxyconnect.erase(cit1);
		  }
		  if (proxyconnect.end() != (cit2 = proxyconnect.find(ccrev)))
		  {
			pqioutput(PQL_DEBUG_BASIC,pqiproxyzone,
		  	  "p3proxy::registerProxyConnection() End: Removing Cit2");
			  proxyconnect.erase(cit2);
		  }
                  break;
	}

	return 1;

}

int	p3proxy::isConnection(cert *src, cert *dest)
{
	if ((src == NULL) || (dest == NULL))
	{
		pqioutput(PQL_DEBUG_BASIC,pqiproxyzone,
		  "p3proxy::isConnection() ERROR: NULL dest/src");
		return -1;
	}

	{
		std::ostringstream out;
		out << "p3proxy::isConnection() Checking <";
		out << dest -> Name() << "," << src -> Name() << ">";
		pqioutput(PQL_DEBUG_BASIC,pqiproxyzone, out.str());
	}

	// get the reference.
	std::map<std::pair<cert *, cert *>, int>::iterator cit1, cit2;

	std::pair<cert *, cert *> ccdir(dest, src), ccrev(src, dest);

	cit1 = proxyconnect.find(ccdir);
	cit2 = proxyconnect.find(ccrev);
	if (proxyconnect.end() != cit1)
	{
		std::ostringstream out;
		out << "p3proxy::isConnection() State(d,s):" << cit1 -> second;
		pqioutput(PQL_DEBUG_BASIC,pqiproxyzone, out.str());
	}
	else
	{
		pqioutput(PQL_DEBUG_BASIC,pqiproxyzone,
		  "p3proxy::isConnection() Unknown (d,s) Pair");
	}

	if (proxyconnect.end() != cit2)
	{
		std::ostringstream out;
		out << "p3proxy::isConnection() State(s,d):" << cit2 -> second;
		pqioutput(PQL_DEBUG_BASIC,pqiproxyzone, out.str());
	}
	else
	{
		pqioutput(PQL_DEBUG_BASIC,pqiproxyzone,
		  "p3proxy::isConnection() Unknown (s,d) Pair");
	}


	if ((proxyconnect.end() == cit1) || (proxyconnect.end() == cit2))
	{
		pqioutput(PQL_DEBUG_BASIC,pqiproxyzone,
		  "p3proxy::isConnection() Unable to find pc pairs: return -1");
		// connection not established!
		return -1;
	}

	if ((cit1 -> second == 2) && (cit2 -> second == 2))
	{
		pqioutput(PQL_DEBUG_BASIC,pqiproxyzone,
		  "p3proxy::isConnection() Connection Okay!");
		// connection complete.
		return 1;
	}

	pqioutput(PQL_DEBUG_BASIC,pqiproxyzone,
	  "p3proxy::isConnection() Incomplete Connection State!");
	return -1;
}


	
int      p3proxy::respondConnect(cert *other, cert *proxy, PQTunnelProxyInit *in)
{
	pqioutput(PQL_DEBUG_BASIC,pqiproxyzone,
		  "p3proxy::respondConnect()");
	// a couple of cases 
	// 1) random init from some other person 
	// 	wanting to create a proxyconnection.
	// 	-check listening queue.
	// 2) return init from someone.
	//      -check init queue.

	std::list<cert *>::iterator lit;
	std::map<cert *, cert *>::iterator it, iit, rit, cit;

	iit = initmap.find(other);
	rit = replymap.find(other);
	cit = connectionmap.find(other);
	lit = std::find(listenqueue.begin(), listenqueue.end(), other);

	if ((cit != connectionmap.end()) || (rit != replymap.end()))
	{
		// error, shouldn't receive this packet 
		// if in these maps.
		//
		// close connection / and return endConnect.
	
		in -> PQTunnelInit::mode = PQTunnelInit::End;
		addOutInitPkt(in);

		// clean up.
		if (iit != initmap.end())
			initmap.erase(iit);
		if (rit != replymap.end())
			replymap.erase(rit);
		if (cit != connectionmap.end())
			connectionmap.erase(cit);

		pqioutput(PQL_DEBUG_BASIC,pqiproxyzone,
			  "p3proxy::respondConnect() Bad State: in connect or reply state");

		return -1;
	}

	// if not in initmap, and we are listening for it.
	// send out own init.
	if ((iit == initmap.end()) && (lit != listenqueue.end()))
	{
		// have found a listen item. (with no current attempt)
		// send reply 
	
		PQTunnelProxyInit *pi = createProxyInit(other, proxy);

		{
			std::ostringstream out;
			out << "Generated Proxy Init:" << std::endl;
			pi -> print(out);
			pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
		}

		addOutInitPkt(pi);

		// addtotheInit list... as we are now in that state!
		// should save the packet with the challenges.
		initmap[other] = proxy;
		passivemap[other] = true;
	}
	else
	{
		pqioutput(PQL_DEBUG_BASIC,pqiproxyzone,
			  "p3proxy::respondConnect() No Need to Send InitPkt!");
	}


	// Find it again.....
	if (initmap.end() == (it = initmap.find(other)))
	{
		// Not here... failed.
		pqioutput(PQL_DEBUG_BASIC,pqiproxyzone,
			"Error Not in InitMap/ListenQueue - Returning End.");

		in -> PQTunnelInit::mode = PQTunnelInit::End;
		addOutInitPkt(in);
		return -1;
	}
	else
	{
		pqioutput(PQL_DEBUG_BASIC,pqiproxyzone,
			"Found It in InitMap!");
	}

	if (it -> second != proxy)
	{
		// Init for the wrong one, what should happen.
		pqioutput(PQL_DEBUG_BASIC,pqiproxyzone,
			"Error Init For the wrong one! - Returning End");
		in -> PQTunnelInit::mode = PQTunnelInit::End;
		addOutInitPkt(in);
		return -1;
	}
	else
	{
		pqioutput(PQL_DEBUG_BASIC,pqiproxyzone,
			"Proxy Matches!");
	}



	pqioutput(PQL_DEBUG_BASIC,pqiproxyzone,
		"Sending PQTunnelProxyInitConnect unfinished!");

	in -> PQTunnelInit::mode = PQTunnelInit::Connect;
	addOutInitPkt(in);

	// remove from initmap.
	initmap.erase(it);
	// add to replymap.
	replymap[other] = proxy;

	return 1;
}


int      p3proxy::completeConnect(cert *other, cert *proxy, PQTunnelProxyInit *in)
{
	{
		std::ostringstream out;
		out << "p3proxy::completeConnect()";
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
	}

	std::map<cert *, cert *>::iterator iit, rit, cit;

	// check maps.
	iit = initmap.find(other);
	rit = replymap.find(other);
	cit = connectionmap.find(other);

	// if not in the replymap ... error....
	if (replymap.end() == rit)
	{
		pqioutput(PQL_DEBUG_BASIC,pqiproxyzone,
			"Not expecting Reply Pkt - Sending End");
		// Send Error.
		
		in -> PQTunnelInit::mode = PQTunnelInit::End;
		addOutInitPkt(in);

		return -1;
	}

	if (proxy != rit -> second)
	{
		pqioutput(PQL_DEBUG_BASIC,pqiproxyzone,
			"Expect Reply from different Proxy - Sending End");
		// Send Error.
		
		in -> PQTunnelInit::mode = PQTunnelInit::End;
		addOutInitPkt(in);

		return -1;
	}

	// if in any other maps... error.
	if ((iit != initmap.end()) || (cit != connectionmap.end()))
	{
		std::ostringstream out;

		out << "Found Target in other list! - Sending End";
		out << std::endl;

		if (iit != initmap.end())
		{
			out << "Target in InitMap!" << std::endl;
			initmap.erase(iit);
		}
		if (rit != replymap.end())
		{
			out << "Target in ReplyMap (expected)!" << std::endl;
			replymap.erase(rit);
		}
		if (cit != connectionmap.end()) 
		{
			connectionmap.erase(cit);
			out << "Target in ConnectionMap!" << std::endl;
		}

		pqioutput(PQL_DEBUG_BASIC,pqiproxyzone,out.str());

		in -> PQTunnelInit::mode = PQTunnelInit::End;
		addOutInitPkt(in);

		return -1;
	}

	// remove cos the attempt has either failed or succeeded.
	replymap.erase(rit);

	std::map<cert *, pqiproxy *>::iterator pit;
	if(proxymap.end() == (pit = proxymap.find(other)))
	{
		pqioutput(PQL_DEBUG_BASIC,pqiproxyzone,
			"Couldn't Find PqiProxy");
		// Error.
		return -1;
	}

	// At this point we should check the correctness
	// of the response to the challenge...
	
	// if okay, save -> and we are connected.
	connectionmap[other] = proxy;

	// tell the pqiproxy that it can send.

	/* call the virtual fn */
	connectionCompletedAsPeer(proxy, other);
	/* notify the pqiproxy */
	(pit -> second) -> connected(passivemap[other]);


	{
		std::ostringstream out;
		out << "Proxy Connection Complete!" << std::endl;
		out << "Destination: " << other -> Name() << std::endl;
		out << "Proxy: " << proxy -> Name() << std::endl;

		pqioutput(PQL_DEBUG_BASIC,pqiproxyzone,out.str());
	}

	// at this point we can remove it from the listenqueue.
	std::list<cert *>::iterator lit;
	if (listenqueue.end() != (lit = std::find(listenqueue.begin(),
			listenqueue.end(), other)))
	{
		listenqueue.erase(lit);
		pqioutput(PQL_DEBUG_BASIC,pqiproxyzone,
			"Removing Cert from listenqueue");
	}


	// and from the connectqueue.
	if (connectqueue.end() != (lit = std::find(connectqueue.begin(),
			connectqueue.end(), other)))
	{
		connectqueue.erase(lit);
		pqioutput(PQL_DEBUG_BASIC,pqiproxyzone,
			"Removing Cert from connectqueue");
	}

	// also clear lastproxymap
	std::map<cert *, cert *>::iterator lpit;
	lpit = lastproxymap.find(other);
	if (lpit != lastproxymap.end())
	{
		lastproxymap.erase(lpit);
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone,
	 		"Cleared Last Proxy Map");
	}

	std::map<cert *, unsigned long>::iterator ctit;
	ctit = connecttimemap.find(other);
	if (ctit != connecttimemap.end())
	{
		connecttimemap.erase(ctit);
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone,
	 		"Cleared Connect Time Map");
	}
	// clean up if we make here.
	delete in;
	return 1;
}

int      p3proxy::endConnect(cert *other, cert *proxy, PQTunnelProxyInit *in)
{
	{
		std::ostringstream out;
		out << "p3proxy::endConnect()";
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
	}

	// if we get this..... delete all references
	// but only ones that exactly match both the other and the proxy.


	// This can be sent at any point,
	// we need to check all the queues.
	std::map<cert *, cert *>::iterator it;
	if ((connectionmap.end() != (it = connectionmap.find(other))) 
		&& (it -> second == proxy))
	{
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, 
			"End Matches an Active Connection: Ending");
		connectionmap.erase(it);
		// if here - we must tell it to disconnect.
		std::map<cert *, pqiproxy *>::iterator it2;
		if (proxymap.end() != (it2 = proxymap.find(other)))
		{
			(it2 -> second) -> disconnected();
		}
	}

	// clean up initmap.
	if ((initmap.end() != (it = initmap.find(other)))
		&& (it -> second == proxy))
	{
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, 
			"End Matches an InitMap item: Erasing");
		  initmap.erase(it);
	}
	
	// clean up replymap.
	if ((replymap.end() != (it = replymap.find(other)))
		&& (it -> second == proxy))
	{
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, 
			"End Matches an ReplyMap item: Erasing");
		  replymap.erase(it);
	}

	// clean up the packet.
	delete in;
	return 1;
}


PQTunnelInit *p3proxy::sendInit()
{
	{
		std::ostringstream out;
		out << "p3proxy::sendInit(" << outInitPkts.size() << ")";
		pqioutput(PQL_DEBUG_ALL, pqiproxyzone, out.str());
	}
	if (outInitPkts.size() < 1)
		return NULL;

	PQTunnelInit *in = outInitPkts.front();
	outInitPkts.pop_front();
	{
		std::ostringstream out;
		out << "p3proxy::sendInit Sending:";
		in -> print(out);
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
	}
	return in;
}

/* to end a proxy connection
 * this sends the packet, and can be called at any time
 * as it doesn't check any queues....
 *
 * This is used in a ::reset() of an active connection, 
 * but could be used at any point the connection needs to be ended.
 */

int	p3proxy::sendEndProxyConnectionPkt(cert *other, cert *proxy)
{
	PQTunnelProxyInit *pi = createProxyInit(other, proxy);
	if (!pi)
	{
		std::ostringstream out;
		out << "p3proxy::endProxyConnection()";
		out << " Cannot create Init(END) pkt";
		pqioutput(PQL_ALERT, pqiproxyzone, out.str());
		return -1;
	}
		
	pi -> PQTunnelInit::mode = PQTunnelInit::End;

	{
		std::ostringstream out;
		out << "Ending Proxy Connection(us -> ";
		out << proxy->Name() << " -> ";
		out << other->Name() << std::endl;

		out << "Generated ProxyInit(END):" << std::endl;
		pi -> print(out);
		pqioutput(PQL_ALERT, pqiproxyzone, out.str());
	}

	addOutInitPkt(pi);
	return 1;
}

	// interface to pqiproxy.
int	p3proxy::attach(pqiproxy *p, cert *c)
{
	{
		std::ostringstream out;
		out << "p3proxy::attach()";
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
	}
	proxymap[c] = p;
	return 1;
}

int	p3proxy::detach(pqiproxy *p, cert *c)
{
	{
		std::ostringstream out;
		out << "p3proxy::detach()";
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
	}
	// remove this proxy from all references.
	// and send a disconnect to any current connections.
	std::map<cert *, pqiproxy *>::iterator it;
	if (proxymap.end() == (it = proxymap.find(c)))
	{
		return -1;
	}

	// remove from init/reply/connect queues.
	reset(p,c);

	// stoplistening.
	stoplistening(p,c);

	// remove from connect queue.
	stopconnecting(p,c);

	proxymap.erase(it);
	return 1;
}

int	p3proxy::connectattempt(pqiproxy *p, cert *c)
{
	{
		std::ostringstream out;
		out << "p3proxy::connectattempt(pqiproxy *p,c)";
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
	}
	std::list<cert *>::iterator it;
	if (connectqueue.end() == (it = 
		std::find(connectqueue.begin(), connectqueue.end(), c)))
	{
		std::ostringstream out;
		out << "p3proxy::connectattempt() added " << c -> Name();
		out << " to the connection queue";
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());

		connectqueue.push_back(c);
		connecttimemap[c] = time(NULL);
	}

	// also clear lastproxymap
	std::map<cert *, cert *>::iterator lpit;
	lpit = lastproxymap.find(c);
	if (lpit != lastproxymap.end())
	{
		lastproxymap.erase(lpit);
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone,
	 		"p3proxy::connectattempt(p,c) Cleared Last Proxy Map");
	}

	return 1;
}

int	p3proxy::listen(pqiproxy *p, cert *c)
{
	{
		std::ostringstream out;
		out << "p3proxy::listen()";
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
	}
	std::map<cert *, pqiproxy *>::iterator it;
	if (proxymap.end() == (it = proxymap.find(c)))
	{
		std::ostringstream out;
		out << "p3proxy::listen() Error cert: " << c->Name() << " not in ProxyMap";
		pqioutput(PQL_ALERT, pqiproxyzone, out.str());
		return -1;
	}
	if (it -> second != p)
	{
		std::ostringstream out;
		out << "p3proxy::listen() Error for cert: " << c->Name() << " PqiProxy don't match";
		pqioutput(PQL_ALERT, pqiproxyzone, out.str());
		return -1;
	}

	/* check it's not there already. */
	std::list<cert *>::iterator lit;
	lit = std::find(listenqueue.begin(), listenqueue.end(), c);
	if (lit == listenqueue.end())
	{
		listenqueue.push_back(c);
	}
	else
	{
		std::ostringstream out;
		out << "p3proxy::listen() Error cert: " << c->Name() << " already in listenqueue";
		pqioutput(PQL_ALERT, pqiproxyzone, out.str());
	}

	return 1;
}

int	p3proxy::stoplistening(pqiproxy *p, cert *c)
{
	std::list<cert *>::iterator it;
	it = std::find(listenqueue.begin(), listenqueue.end(), c);
	if (it == listenqueue.end())
	{
		std::ostringstream out;
		out << "p3proxy::stoplistening() pqiproxy not found";
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
		return -1;
	}

	// remove from the listen queue,
	listenqueue.erase(it);
	{
		std::ostringstream out;
		out << "p3proxy::stoplistening() pqiproxy removed from queue";
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
	}
	return 1;
}


int	p3proxy::stopconnecting(pqiproxy *p, cert *c)
{
	std::list<cert *>::iterator it;
	it = std::find(connectqueue.begin(), connectqueue.end(), c);
	if (it == connectqueue.end())
	{
		std::ostringstream out;
		out << "p3proxy::stopconnecting() pqiproxy not found";
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
		return -1;
	}

	// remove from the listen queue,
	connectqueue.erase(it);

	{
		std::ostringstream out;
		out << "p3proxy::stopconnecting() pqiproxy removed from queue";
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
	}

	// also clear lastproxymap
	std::map<cert *, cert *>::iterator lpit;
	lpit = lastproxymap.find(c);
	if (lpit != lastproxymap.end())
	{
		lastproxymap.erase(lpit);
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone,
	 		"p3proxy::connectattempt(p,c) Cleared Last Proxy Map");
	}

	// and the connecttime map.
	std::map<cert *, unsigned long>::iterator tit;
	tit = connecttimemap.find(c);
	if (tit != connecttimemap.end())
	{
		connecttimemap.erase(tit);
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone,
	 		"p3proxy::connectattempt(p,c) Cleared Connect Time Map");
	}
	return 1;
}


int	p3proxy::reset(pqiproxy *p, cert *c)
{
	{
		std::ostringstream out;
		out << "p3proxy::reset()";
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
	}


	std::map<cert *, bool>::iterator pit;
	std::map<cert *, cert *>::iterator iit, rit, cit;

	// remove from all except the connect/listen queues.
	// clean init/passive map
	iit = initmap.find(c);
	if (iit != initmap.end())
	{
		initmap.erase(iit);
	}

	pit = passivemap.find(c);
	if (pit != passivemap.end())
	{
		passivemap.erase(pit);
	}

	// replymap?
	rit = replymap.find(c);
	if (rit != replymap.end())
	{
		replymap.erase(rit);
	}

	// connectionmap?
	cit = connectionmap.find(c);
	if (cit != connectionmap.end())
	{
		// if it found in here... send a disconnect packet!.
		sendEndProxyConnectionPkt(cit->first, cit->second);

		// remove the connection.
		connectionmap.erase(cit);
	}

	/* also clear the lastproxymap */
	iit = lastproxymap.find(c);
	if (iit != lastproxymap.end())
	{
		lastproxymap.erase(iit);
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone,
	 		"p3proxy::reset() Cleared Last Proxy Map");
	}

	// finally, remove any packets from the incoming queue.
 	std::map<pqiproxy *, std::list<PQTunnelProxy *> >::iterator qit;
	if (inqueue.end() == (qit = inqueue.find(p)))
	{
		std::ostringstream out;
		out << "p3proxy::reset() no inQueue";
		pqioutput(PQL_DEBUG_ALL, pqiproxyzone, out.str());
	}
	else
	{
		while((qit -> second).size() > 0)
		{
			// add to existing queue.
			PQTunnelProxy *pqtp = (qit -> second).front();
			(qit -> second).pop_front();
			delete pqtp;
		}
	}

	/* if the certificate is active and listening, put back into the listening queue */
	if ((c->Accepted()) && (c->WillListen()) && (c->Listening()))
	{
		std::ostringstream out;
		out << "p3proxy::reset() Putting " << c -> Name() << " back onto Listening Queue";
		pqioutput(PQL_WARNING, pqiproxyzone, out.str());

		listen(p,c);
	}
	else
	{
		std::ostringstream out;
		out << "p3proxy::reset() " << c -> Name() << " Left off Listening Queue";
		pqioutput(PQL_WARNING, pqiproxyzone, out.str());
	}

	return 1;
}


int	p3proxy::outgoingpkt(pqiproxy *p, cert *c, void *d, int size)
{
	{
		std::ostringstream out;
		out << "p3proxy::outgoingpkt() For: " << c -> Name();
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
	}
	// find the correct address.
	// -> check the connectionmap.
	std::map<cert *, cert *>::iterator it;
	if (connectionmap.end() == (it = connectionmap.find(c)))
	{
		std::ostringstream out;
		out << "p3proxy::outgoingpkt() Failed to Find Connection";
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
		return -1;
	}

	// create pqitunnel packet
	PQTunnelProxy *pqtp = createProxyPkt(c, it -> second, 000101010, (const char *) d, size);
	if (pqtp == NULL)
	{
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, 
		 "Failed to Create PQTunnelProxy");
	}

	{
		std::ostringstream out;
		out << "p3proxy::outgoingpkt() Sending off Via: ";
		out << it -> second -> Name();
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
	}

	// send it off.
	outPkts.push_back(pqtp);

	// how much that we've packaged up!
	return size;
}


int	p3proxy::incomingpkt(pqiproxy *p, void *d, int maxsize)
{
	{
		std::ostringstream out;
		out << "p3proxy::incomingpkt()";
		pqioutput(PQL_DEBUG_ALL, pqiproxyzone, out.str());
	}
	// check the incoming queue.
 	std::map<pqiproxy *, std::list<PQTunnelProxy *> >::iterator qit;
	if (inqueue.end() == (qit = inqueue.find(p)))
	{
		std::ostringstream out;
		out << "p3proxy::incomingpkt() Missing Queue";
		pqioutput(PQL_DEBUG_ALL, pqiproxyzone, out.str());
		return -1;
	}

	if ((qit -> second).size() < 1)
	{
		std::ostringstream out;
		out << "p3proxy::incomingpkt() Empty Queue";
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
		return -1;
	}


	// add to existing queue.
	PQTunnelProxy *pqtp = (qit -> second).front();
	if (maxsize < pqtp -> size)
	{
		// not enough space.
		
		std::ostringstream out;
		out << "p3proxy::incomingpkt() Not Enough Space";
		out << "to Decode to pqiproxy";
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
		return -1;
	}

	(qit -> second).pop_front();

	int size = pqtp -> size;
	// copy out the tunneled stuff.
	memcpy(d, pqtp -> data, size);

	std::ostringstream out;
	out << "p3proxy::incomingpkt() Decoding to pqiproxy. (";
	out << size << " bytes) From Packet:" << std::endl;
	out << "Ptr: " << (void *) pqtp << " DataPtr: ";
	out << (void *) pqtp -> data << std::endl;
	pqtp -> print(out);

	pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());

	// decode into pqiproxy space.
	// Need to Remove Tunnel Headers.
	
	// XXX Should Check Signature Here.
	
	delete pqtp;
	return size;
}

int 	p3proxy::connectattempt()
{
	{
		std::ostringstream out;
		out << "p3proxy::connectattempt()";
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
	}
	// get the list of potential connections.
	std::list<cert *>::iterator it;

	std::list<cert *>::iterator itc;
	std::list<cert *> plist;
	std::map<cert *, cert *>::iterator lpit;

	unsigned long ct = time(NULL);

	// find the certificates, that are active.
	for(it = connectqueue.begin(); it != connectqueue.end();)
	{
	  // If not currently querying,
	  if ((initmap.end() == initmap.find(*it)) &&
	      (replymap.end() == replymap.find(*it)))
	  {
	  	/* find last proxy - if there is one */
		lpit = lastproxymap.find(*it);
		cert *lastproxy = NULL;
		if (lpit != lastproxymap.end())
		{
			lastproxy = lpit->second;
		}

		if (0 < nextconnectattempt(*it, lastproxy))
		{
			pqioutput(PQL_DEBUG_BASIC, pqiproxyzone,
			  "Successful Proxy Connect Attempt");
			it++;
		}
		else
		{
		  /* clear the lastproxymap - either way (so it restarts) */
		  lpit = lastproxymap.find(*it);
		  if (lpit != lastproxymap.end())
		  {
			lastproxymap.erase(lpit);
			pqioutput(PQL_DEBUG_BASIC, pqiproxyzone,
		  		"Cleared Last Proxy Map");
		  }
		  else
		  {
			pqioutput(PQL_DEBUG_BASIC, pqiproxyzone,
		  		"Nothing in Last Proxy Map");
		  }

		  // check the connecttime map
		  std::map<cert *, unsigned long>::iterator tit;
		  tit = connecttimemap.find(*it);
		  if ((tit == connecttimemap.end()) || 
			(tit->second + P3PROXY_CONNECT_TIMEOUT < ct))
		  {
			// remove from lists...

			pqioutput(PQL_DEBUG_BASIC, pqiproxyzone,
			  "Failed Proxy Connect Attempt");
			// notify the pqiproxy that it didn't connect.
			std::map<cert *, pqiproxy *>::iterator pit;
			if (proxymap.end() != (pit = proxymap.find(*it)))
			{
				// notify.
				pqioutput(PQL_DEBUG_BASIC, pqiproxyzone,
			  		"Notify pqiproxy of failure");
				pit -> second -> disconnected();
			}
			else
			{
				pqioutput(PQL_DEBUG_BASIC, pqiproxyzone,
				  "ERROR - Unknown pqiproxy");
			}
			// delete connect attempt.
			it = connectqueue.erase(it);

			// delete the connecttimemap.
			connecttimemap.erase(tit);
		  }
		  else
		  {
		  	it++;
		  }

		}

	  }
	  else
	  {
		it++;
	  }
	}
	return 1;
}

int 	p3proxy::nextconnectattempt(cert *other, cert *lastproxy)
{
	{
		std::ostringstream out;
		out << "p3proxy::nextconnectattempt() for " << other -> Name();
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
	}
	// get the list of potential connections.

	// check that it is in the connect queue.
	if (connectqueue.end() ==  
		std::find(connectqueue.begin(), connectqueue.end(), other))
	{
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, 
			"Missing from ConnectQueue, Cannot Connect");
		return -1;
	}

	std::list<cert *>::iterator it;
	std::list<cert *> plist = p3d -> potentialproxy(other);

	if (plist.size() == 0)
	{
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, 
			"No Potential Proxies - Failed Connection Attempt");
		return -1;
	}
	else
	{
		for(it = plist.begin(); it != plist.end(); it++)
		{
			std::ostringstream out;
			out << "Potential Proxy: " <<  (*it) -> Name();
			pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
		}
	}

	bool next = false;
	it = plist.begin();

	/* start at start */
	if (lastproxy == NULL)
	{
		next = true;
	}

	for(; (it != plist.end()) && (!next); it++)
	{
		if ((*it) == lastproxy)
		{
			next = true;
		}
	}

	if (it == plist.end())
	{
		std::ostringstream out;
		out << "Run out of Potential Proxies! :" <<  plist.size() << " in total!";
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
		return -1;
	}


	{
		std::ostringstream out;
		out << "Connection Attempt to: " <<  (*it) -> Name();
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
	}

	pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, 
	  "p3proxy::nextconnectattempt() Don't Build Correct Packets Yet!");

	// it points to the next proxy.
	// send a proxy init.
	PQTunnelProxyInit *pi = createProxyInit(other, (*it));

	{
		std::ostringstream out;
		out << "Generated Proxy Init:" << std::endl;
		pi -> print(out);
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
	}

	addOutInitPkt(pi);

	// should save the packet with the challenges.
	initmap[other] = *it;
	/* save the last proxy, so we can continue next time */
	lastproxymap[other] = *it; 
	passivemap[other] = false;
	return 1;
}

int 	p3proxy::addOutInitPkt(PQTunnelInit *pkt)
{
	{
		std::ostringstream out;
		out << "AddOutInitPkt(" << (void *) pkt << ")" << std::endl;
		pkt -> print(out);
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
	}
	outInitPkts.push_back(pkt);
	return 1;
}



int 	p3proxy::tick()
{
	{
		std::ostringstream out;
		out << "p3proxy::tick()";
		pqioutput(PQL_DEBUG_ALL, pqiproxyzone, out.str());
	}
	// check for incoming.
	processincoming();

	// handle any new connections.
 	connectattempt();

	status();
	return 1;
}

int 	p3proxy::status()
{
	std::ostringstream out;
	out << "p3proxy::status()";
	out << std::endl;

	/* show all the details
	 * in all of these container classes
	 */

	if (proxymap.size() > 0)
	{
		out << std::endl;
		out << "p3proxy::status() proxymap entries:";
		out << std::endl;
	}
        std::map<cert *, pqiproxy *>::iterator cpit;
	for(cpit = proxymap.begin(); cpit != proxymap.end(); ++cpit)
	{
		out << "\t" << cpit->first->Name();	
		out << std::endl;
	}

	if (listenqueue.size() > 0)
	{
		out << std::endl;
		out << "p3proxy::status() listenqueue entries:";
		out << std::endl;
	}
	std::list<cert *>::iterator it;
	for(it = listenqueue.begin(); it != listenqueue.end(); ++it)
	{
		out << "\t" << (*it)->Name();	
		out << std::endl;
	}

	if (connectqueue.size() > 0)
	{
		out << std::endl;
		out << "p3proxy::status() connectqueue entries:";
		out << std::endl;
	}
	for(it = connectqueue.begin(); it != connectqueue.end(); ++it)
	{
		out << "\t" << (*it)->Name();	
		out << std::endl;
	}
	if (connecttimemap.size() > 0)
	{
		out << std::endl;
		out << "p3proxy::status() connecttimemap entries:";
		out << std::endl;
	}
	std::map<cert *, unsigned long>::iterator ctit;
	int ct = time(NULL);
	for(ctit = connecttimemap.begin(); ctit != connecttimemap.end(); ++ctit)
	{
		out << "\t" << ctit->first->Name() << "  : " << ct - ctit->second << " sec ago";	
		out << std::endl;
	}

	std::map<cert *, cert *>::iterator ccit;
	if (lastproxymap.size() > 0)
	{
		out << std::endl;
		out << "p3proxy::status() lastproxymap entries:";
		out << std::endl;
	}
	for(ccit = lastproxymap.begin(); 
				ccit != lastproxymap.end(); ++ccit)
	{
		out << "us -> " << ccit->second->Name();	
		out << " -> " << ccit->first->Name();	
		out << std::endl;
	}

	if (initmap.size() > 0)
	{
		out << std::endl;
		out << "p3proxy::status() initmap entries:";
		out << std::endl;
	}
	for(ccit = initmap.begin(); ccit != initmap.end(); ++ccit)
	{
		out << "us -> " << ccit->second->Name();	
		out << " -> " << ccit->first->Name();	
		out << std::endl;
	}

	if (replymap.size() > 0)
	{
		out << std::endl;
		out << "p3proxy::status() replymap entries:";
		out << std::endl;
	}
	for(ccit = replymap.begin(); ccit != replymap.end(); ++ccit)
	{
		out << "us -> " << ccit->second->Name();	
		out << " -> " << ccit->first->Name();	
		out << std::endl;
	}

	if (connectionmap.size() > 0)
	{
		out << std::endl;
		out << "p3proxy::status() connectionmap entries:";
		out << std::endl;
	}
	for(ccit = connectionmap.begin(); 
				ccit != connectionmap.end(); ++ccit)
	{
		out << "us -> " << ccit->second->Name();	
		out << " -> " << ccit->first->Name();	
		out << std::endl;
	}

	if (passivemap.size() > 0)
	{
		out << std::endl;
		out << "p3proxy::status() passivemap entries:";
		out << std::endl;
	}
	std::map<cert *, bool>::iterator pmit;
	for(pmit = passivemap.begin(); pmit != passivemap.end(); ++pmit)
	{
		out << pmit->first->Name();	
		out << " -> " << (void *) ccit->second;
		out << std::endl;
	}

	if (proxyconnect.size() > 0)
	{
		out << std::endl;
		out << "p3proxy::status() proxyconnect entries:";
		out << std::endl;
	}
	std::map<std::pair<cert *, cert *>, int>::iterator pit;
	for(pit = proxyconnect.begin(); pit != proxyconnect.end(); ++pit)
	{
		out << "pair< " << pit->first.first->Name();	
		out << ", " << pit->first.second->Name();	
		out << "> -> " << pit->second;
		out << std::endl;
	}

	pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());

	return 1;
}


int	p3proxy::processincoming()
{
	{
		std::ostringstream out;
		out << "p3proxy::processincoming()";
		pqioutput(PQL_DEBUG_ALL, pqiproxyzone, out.str());
	}
	// read packets.
	// run the through the filters.
	//
	// catch any control packets.
	// send notification.
	//
	// pack packets in correct queue.
	return 1;
}

int p3proxy::sendProxyInit(cert *c)
{
	{
		std::ostringstream out;
		out << "p3proxy::sendProxyInit()";
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
	}
	// create a proxyInit packet.
	// send it off.
	return 1;
}

/************************ PQI PROXY *************************/

pqipeerproxy::pqipeerproxy(cert *c, p3proxy *l, PQInterface *parent)
	:pqiproxy(c, l), NetBinInterface(parent, c), active(false)
{
	{
		std::ostringstream out;
		out << "pqipeerproxy::pqipeerproxy()";
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
	}

	// register a potential proxy connection.
	p3p -> attach(this, c);

	// create a temp pkt.
	maxpktlen = pqipkt_maxsize();
	pkt = (void *) malloc(maxpktlen);
	pktlen = pktn = 0;

	return;
}

pqipeerproxy::~pqipeerproxy()
{
	{
		std::ostringstream out;
		out << "pqipeerproxy::~pqipeerproxy()";
		pqioutput(PQL_ALERT, pqiproxyzone, out.str());
	}
	// remove references to us.
	p3p -> detach(this, sslcert);
	if (pkt)
		free(pkt);
	return;
}

	// pqiconnect Interface.
int 	pqipeerproxy::connectattempt()
{
	{
		std::ostringstream out;
		out << "pqipeerproxy::connectattempt()";
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
	}

	p3p -> connectattempt(this, sslcert);

	return 1;
}

int	pqipeerproxy::connected(bool cMode)
{
	{
		std::ostringstream out;
		out << "pqipeerproxy::connected()";
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
	}
	active = true;
	connect_mode = cMode;
	if (parent())
	{
		parent() -> notifyEvent(this, CONNECT_SUCCESS);
	}
	return 1; 
}


int	pqipeerproxy::listen()
{
	{
		std::ostringstream out;
		out << "pqipeerproxy::listen()";
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
	}
	return p3p -> listen(this, sslcert);
}

int	pqipeerproxy::stoplistening()
{
	{
		std::ostringstream out;
		out << "pqipeerproxy::stoplistening()";
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
	}
	return p3p -> stoplistening(this, sslcert);
}

int 	pqipeerproxy::reset()
{
	{
		std::ostringstream out;
		out << "pqipeerproxy::reset()";
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
	}
	p3p -> reset(this, sslcert);
	/* also flag as inactive */
	active = false;
	return 1;
}

int 	pqipeerproxy::disconnected()
{
	{
		std::ostringstream out;
		out << "pqipeerproxy::disconnected()";
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
	}
	active = false;
	if (parent())
	{
		parent() -> notifyEvent(this, CONNECT_FAILED);
	}
	return 1;
}

// called when??
int 	pqipeerproxy::disconnect()
{
	{
		std::ostringstream out;
		out << "pqipeerproxy::disconnect()";
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
	}
	disconnected();
	return 1;
}



	// Overloaded from PQInterface
int 	pqipeerproxy::status()
{
	{
		std::ostringstream out;
		out << "pqipeerproxy::status()";
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
	}
	return 1;
}

int	pqipeerproxy::tick()
{
	{
		std::ostringstream out;
		out << "pqipeerproxy::tick()";
		pqioutput(PQL_DEBUG_ALL, pqiproxyzone, out.str());
	}
	//pqistreamer::tick();
	//more to do???

	return 1;
}

cert *	pqipeerproxy::getContact()
{
	{
		std::ostringstream out;
		out << "pqipeerproxy::getContact()";
		pqioutput(PQL_DEBUG_ALL, pqiproxyzone, out.str());
	}
	return sslcert;
}


// overloaded from pqistreamer.....
int pqipeerproxy::senddata(void *d, int n)
{
	{
		std::ostringstream out;
		out << "pqipeerproxy::senddata(" << d << "," << n << ")";
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
	}
	return p3p -> outgoingpkt(this, sslcert, d, n);
}

int pqipeerproxy::readdata(void* d, int n)
{
	{
		std::ostringstream out;
		out << "pqipeerproxy::readdata(" << d << "," << n << ")";
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
	}

	if (pktlen == 0)
	{
		if (!moretoread())
		{
			std::ostringstream out;
			out << "pqipeerproxy::readdata -> Nothing There";
			pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());

			return 0;
		}
	}

	std::ostringstream out;
	out << "pqipeerproxy::readdata -> Data Used (" << pktn << "/";
	out << pktlen << ")" << std::endl;

	int size = pktlen - pktn;
	if (size > n)
		size = n;

	char *start = (char *) pkt;
	start += pktn;

	memcpy(d, start, size);

	out << "pqipeerproxy::readdata -> Allowing ";
	out << size << " bytes out" << std::endl;

	pktn += size;
	if (pktn == pktlen)
	{
		// flag empty packet.
		pktn = pktlen = 0;
		out << "pqipeerproxy::readdata -> Finished the Packet!";
	}

	pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());

	return size;
}

int pqipeerproxy::netstatus()
{
	{
		std::ostringstream out;
		out << "pqipeerproxy::netstatus() => Always True";
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
	}
	return 1;
}

int pqipeerproxy::isactive()
{
	{
		std::ostringstream out;
		out << "pqipeerproxy::isactive() => ";

		if (active) out << "True";
		else out << "False";

		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
	}
	return active;
}

bool pqipeerproxy::moretoread()
{
	{
		std::ostringstream out;
		out << "pqipeerproxy::moretoread()";
		pqioutput(PQL_DEBUG_ALL, pqiproxyzone, out.str());
	}
	// if haven't finished the current packet, 
	// return true.
	if ((pkt != NULL) && (pktlen != 0) && (pktn < pktlen))
	{
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, 
			"More to Read");
		return true;
	}

	// else packet is empty.
	int size = 0;
	if (0 < (size = p3p -> incomingpkt(this, pkt, maxpktlen)))
	{
		// we've got some more.
		std::stringstream out;
		out << "pqipeerproxy::moretoread()";
		out << "Got Another Packet (" << size << " bytes)";
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());

		pktlen = size;
		pktn = 0;
		return true;
	}

	pqioutput(PQL_DEBUG_ALL, pqiproxyzone, 
		"Nothing to Read");
	return false;
}

bool pqipeerproxy::cansend()
{
	// cansend.... hard to catch
	// if the data's being cached on the way...
	// most likely to be locally.
	
	{
		std::ostringstream out;
		out << "pqipeerproxy::cansend Always returns 1";
		pqioutput(PQL_DEBUG_ALL, pqiproxyzone, out.str());
	}

	return 1;
}


	// The interface to p3proxy.
int pqipeerproxy::notifyEvent(int type)
{
	{
		std::ostringstream out;
		out << "pqipeerproxy::notifyEvent(" << type << ")";
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
	}

	switch(type)
	{
	case CONNECT_RECEIVED:
	case CONNECT_SUCCESS:
		active = true;
		break;

	case CONNECT_FAILED:
	default:
		active = false;
		break;
	}

	// notify the parent.
	if (parent())
	{
		parent() -> notifyEvent(this, type);
	}
	return 1;
}


/********************** CERT SIGNS *************************/

cert *p3proxy::findcert(certsign &cs)
{
	pqioutput(PQL_DEBUG_BASIC,pqiproxyzone,
	 "p3proxy::findcert()");

	cert *c = sroot -> findcertsign(cs);
	return c;
}

PQTunnelProxyInit *p3proxy::createProxyInit(cert *dest, cert *proxy)
{
	PQTunnelProxyInit *pi = new PQTunnelProxyInit();

	{
		std::ostringstream out;
		out << "p3proxy::createProxyInit()" << std::endl;
		out << "Dest: " << dest -> Name() << std::endl;
		out << "Proxy: " << proxy -> Name() << std::endl;
		out << "Src: " << (sroot -> getOwnCert()) -> Name();
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
	}


	// Fill In certificates 
	if ((!sroot -> getcertsign(dest, pi -> dest)) ||
		(!sroot -> getcertsign(proxy, pi -> proxy)) ||
		(!sroot -> getcertsign(sroot -> getOwnCert(), pi -> src)))
	{
		delete pi;
		return NULL;
	}

	// fill in the challenge.
	// with randoms;
	int i;
	for(i = 0; i < PQI_PROXY_CHALLENGE_SIZE; i++)
	{
		pi -> challenge[i] = (unsigned char)  
				(256.0*rand()/(RAND_MAX+1.0));
	}

	// ProxyInit Details.
	cert *o = sroot -> getOwnCert();
	pi -> cid = proxy -> cid; // destination.
	pi -> p = o; // from us.

	// Sign it..... XXX to do.


	return pi;
}


PQTunnelProxy *p3proxy::createProxyPkt(cert *dest, cert *proxy, long nextseq,
				const char *data, int size)
{
	PQTunnelProxy *pi = new PQTunnelProxy();
	cert *o = sroot -> getOwnCert();

	{
		std::ostringstream out;
		out << "p3proxy::createProxyPkt()" << std::endl;
		out << "Dest: " << dest -> Name() << std::endl;
		out << "Proxy: " << proxy -> Name() << std::endl;
		out << "Src: " << o -> Name() << std::endl;
		out << "Seq: " << nextseq << std::endl;
		out << "Data: " << (void *) data << std::endl;
		out << "Size: " << size << std::endl;
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());
	}

	// Fill in Details....
	// PQItem 
	if ((!sroot -> getcertsign(dest, pi -> dest)) ||
		(!sroot -> getcertsign(o, pi -> src)))
	{
		std::ostringstream out;
		out << "p3proxy::createProxyPkt()";
		out << " Error: Failed to Get Signatures...";
		pqioutput(PQL_DEBUG_BASIC, pqiproxyzone, out.str());

		delete pi;
		return NULL;
	}

	// initialise packets.
	pi -> sid = 0;   // doesn't matter.
	pi -> cid = proxy -> cid; // destination.
	pi -> p = o; // from us;
	pi -> seq = nextseq;

	// ProxyInit Details.
	pi -> data = malloc(size);
	memcpy(pi -> data, data, size);
	pi -> size = size;

	// Sign Packet.

	return pi;
}

