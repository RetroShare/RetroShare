/*
 * libretroshare/src/pqi pqiperson.cc
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

#include "pqi/pqi.h"
#include "pqi/pqiperson.h"
#include "pqi/pqipersongrp.h"

const int pqipersonzone = 82371;
#include "util/rsdebug.h"
#include <sstream>

/****
 * #define PERSON_DEBUG
 ****/

pqiperson::pqiperson(std::string id, pqipersongrp *pg)
	:PQInterface(id), active(false), activepqi(NULL), 
	inConnectAttempt(false), waittimes(0), 
	pqipg(pg)
{

	/* must check id! */

	return;
}

pqiperson::~pqiperson()
{
	// clean up the children.
	std::map<uint32_t, pqiconnect *>::iterator it;
	for(it = kids.begin(); it != kids.end(); it++)
	{
		pqiconnect *pc = (it->second);
		delete pc;
	}
	kids.clear();
}


	// The PQInterface interface.
int     pqiperson::SendItem(RsItem *i)
{
	std::ostringstream out;
	out << "pqiperson::SendItem()";
	if (active)
	{
                out << " Active: Sending On" << std::endl;
                i->print(out, 5);
#ifdef PERSON_DEBUG
                std::cerr << out.str() << std::endl;
#endif
		return activepqi -> SendItem(i);
	}
	else
	{
		out << " Not Active: Used to put in ToGo Store";
		out << std::endl;
		out << " Now deleting...";
		delete i;
	}

	pqioutput(PQL_DEBUG_BASIC, pqipersonzone, out.str());
	return 0; // queued.	
}

RsItem *pqiperson::GetItem()
{
	if (active)
		return activepqi -> GetItem();
	// else not possible.
	return NULL;
}

int 	pqiperson::status()
{
	if (active)
		return activepqi -> status();
	return -1;
}

	// tick......
int	pqiperson::tick()
{
	int activeTick = 0;

	{
	  std::ostringstream out;
	  out << "pqiperson::tick() Id: " << PeerId() << " ";
	  if (active)
	  	out << "***Active***";
	  else
	  	out << ">>InActive<<";

	  out << std::endl;
	  out << "Activepqi: " << activepqi << " inConnectAttempt: ";

	  if (inConnectAttempt)
	  	out << "In Connection Attempt";
	  else
	  	out << "   Not Connecting    ";
	  out << std::endl;


	// tick the children.
	std::map<uint32_t, pqiconnect *>::iterator it;
	for(it = kids.begin(); it != kids.end(); it++)
	{
		if (0 < (it->second) -> tick())
		{
			activeTick = 1;
		}
	  	out << "\tTicking Child: " << (it->first) << std::endl;
	}

	  pqioutput(PQL_DEBUG_ALL, pqipersonzone, out.str());
	} // end of pqioutput.

	return activeTick;
}

// callback function for the child - notify of a change.
// This is only used for out-of-band info....
// otherwise could get dangerous loops.
int 	pqiperson::notifyEvent(NetInterface *ni, int newState)
{
	{
	  std::ostringstream out;
	  out << "pqiperson::notifyEvent() Id: " << PeerId();
	  out << std::endl;
	  out << "Message: " << newState << " from: " << ni << std::endl;
          out << "Active pqi : " << activepqi;

	  pqioutput(PQL_DEBUG_BASIC, pqipersonzone, out.str());
	}

	/* find the pqi, */
	pqiconnect *pqi = NULL;
	uint32_t    type = 0;
	std::map<uint32_t, pqiconnect *>::iterator it;
		
	/* start again */
	int i = 0;
	for(it = kids.begin(); it != kids.end(); it++)
	{
	 	std::ostringstream out;
	  	out << "pqiperson::connectattempt() Kid# ";
	  	out << i << " of " << kids.size();
	  	out << std::endl;
		out << " type: " << (it->first);
		out << " ni: " << (it->second)->ni;
		out << " in_ni: " << ni;
	  	pqioutput(PQL_DEBUG_BASIC, pqipersonzone, out.str());
                i++;

		if ((it->second)->thisNetInterface(ni))
		{
                        pqi = (it->second);
			type = (it->first);
		}
	}

	if (!pqi)
	{
	  pqioutput(PQL_WARNING, pqipersonzone, "Unknown notfyEvent Source!");
	  return -1;
	}

	switch(newState)
	{
	case CONNECT_RECEIVED:
	case CONNECT_SUCCESS:

		/* notify */
		if (pqipg)
			pqipg->notifyConnect(PeerId(), type, true);

		if ((active) && (activepqi != pqi)) // already connected - trouble
		{
	  		pqioutput(PQL_WARNING, pqipersonzone, 
                                "CONNECT_SUCCESS+active-> activing new connection, shutting others");

			// This is the RESET that's killing the connections.....
                        //activepqi -> reset();
			// this causes a recursive call back into this fn.
			// which cleans up state.
			// we only do this if its not going to mess with new conn.
		}

		/* now install a new one. */
		{

	  		pqioutput(PQL_WARNING, pqipersonzone, 
				"CONNECT_SUCCESS->marking so! (resetting others)");
                        // mark as active.
			active = true;
			activepqi = pqi;
                        inConnectAttempt = false;

			/* reset all other children? (clear up long UDP attempt) */
			for(it = kids.begin(); it != kids.end(); it++)
			{
				if (it->second != activepqi)
				{
                                        std::cerr << "Resetting pqi" << std::endl;
                                        it->second->reset();
                                } else {
                                        std::cerr << "Active pqi : not resetting." << std::endl;
                                }
			}
			return 1;
		}
		break;
	case CONNECT_UNREACHABLE:
	case CONNECT_FIREWALLED:
	case CONNECT_FAILED:


		if (active)
		{
                        if (activepqi == pqi)
			{
	  			pqioutput(PQL_WARNING, pqipersonzone, 
					"CONNECT_FAILED->marking so!");
				active = false;
				activepqi = NULL;
                        } else {
                                pqioutput(PQL_WARNING, pqipersonzone,
                                        "CONNECT_FAILED-> from an unactive connection, don't flag the peer as not connected, just try next attempt !");
                        }
                }
		else
		{
	  		pqioutput(PQL_WARNING, pqipersonzone, 
			  "CONNECT_FAILED+NOT active -> try connect again");
		}

		/* notify up (But not if we are actually active: rtn -1 case above) */
                if (!active) {
                    if (pqipg)
                            pqipg->notifyConnect(PeerId(), type, false);
                } else {
                     if (pqipg)
                           pqipg->notifyConnect(PeerId(), PQI_CONNECT_DO_NEXT_ATTEMPT, false);
                    return -1;
                }

		return 1;

		break;
	default:
		break;
	}
	return -1;
}

/***************** Not PQInterface Fns ***********************/

int 	pqiperson::reset()
{
	{
	  std::ostringstream out;
	  out << "pqiperson::reset() Id: " << PeerId();
	  out << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqipersonzone, out.str());
	}

	std::map<uint32_t, pqiconnect *>::iterator it;
	for(it = kids.begin(); it != kids.end(); it++)
	{
		(it->second) -> reset();
	}		

	activepqi = NULL;
	active = false;

	return 1;
}

int	pqiperson::addChildInterface(uint32_t type, pqiconnect *pqi)
{
	{
	  std::ostringstream out;
	  out << "pqiperson::addChildInterface() : Id " << PeerId() << " " << type;
	  out << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqipersonzone, out.str());
	}

	kids[type] = pqi;
	return 1;
}

/***************** PRIVATE FUNCTIONS ***********************/
// functions to iterate over the connects and change state.


int 	pqiperson::listen()
{
	{
	  std::ostringstream out;
	  out << "pqiperson::listen() Id: " << PeerId();
	  out << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqipersonzone, out.str());
	}

	if (!active)
	{
		std::map<uint32_t, pqiconnect *>::iterator it;
		for(it = kids.begin(); it != kids.end(); it++)
		{
			// set them all listening.
			(it->second) -> listen();
		}
	}
	return 1;
}


int 	pqiperson::stoplistening()
{
	{
	  std::ostringstream out;
	  out << "pqiperson::stoplistening() Id: " << PeerId();
	  out << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqipersonzone, out.str());
	}

	std::map<uint32_t, pqiconnect *>::iterator it;
	for(it = kids.begin(); it != kids.end(); it++)
	{
		// set them all listening.
		(it->second) -> stoplistening();
	}
	return 1;
}

int	pqiperson::connect(uint32_t type, struct sockaddr_in raddr, uint32_t delay, uint32_t period, uint32_t timeout)
{
#ifdef PERSON_DEBUG
	{
	  std::ostringstream out;
	  out << "pqiperson::connect() Id: " << PeerId();
	  out << " type: " << type;
	  out << " addr: " << inet_ntoa(raddr.sin_addr);
	  out << ":" << ntohs(raddr.sin_port);
	  out << " delay: " << delay;
	  out << " period: " << period;
	  out << " timeout: " << timeout;
	  out << std::endl;
	  std::cerr << out.str();
	  //pqioutput(PQL_DEBUG_BASIC, pqipersonzone, out.str());
	}
#endif

	std::map<uint32_t, pqiconnect *>::iterator it;
	
	it = kids.find(type);
	if (it == kids.end())
	{
#ifdef PERSON_DEBUG
	  	std::ostringstream out;
	  	out << "pqiperson::connect()";
	  	out << " missing pqiconnect";
	  	out << std::endl;
	  	std::cerr << out.str();
	  	//pqioutput(PQL_DEBUG_BASIC, pqipersonzone, out.str());
#endif
		return 0;
	}

	/* set the parameters */
	(it->second)->reset();

#ifdef PERSON_DEBUG
	std::cerr << "pqiperson::connect() setting connect_parameters" << std::endl;
#endif
	(it->second)->connect_parameter(NET_PARAM_CONNECT_DELAY, delay);
	(it->second)->connect_parameter(NET_PARAM_CONNECT_PERIOD, period);
	(it->second)->connect_parameter(NET_PARAM_CONNECT_TIMEOUT, timeout);

	(it->second)->connect(raddr);	
		
	// flag if we started a new connectionAttempt.
	inConnectAttempt = true;

	return 1;
}


pqiconnect	*pqiperson::getKid(uint32_t type)
{
	std::map<uint32_t, pqiconnect *>::iterator it;

	it = kids.find(type);
	if (it == kids.end())
	{
	    return NULL;
	} else {
	    return it->second;
	}
}

float   pqiperson::getRate(bool in)
{
	// get the rate from the active one.
	if ((!active) || (activepqi == NULL))
		return 0;
	return activepqi -> getRate(in);
}

void    pqiperson::setMaxRate(bool in, float val)
{
	// set to all of them. (and us)
	PQInterface::setMaxRate(in, val);
	// clean up the children.
	std::map<uint32_t, pqiconnect *>::iterator it;
	for(it = kids.begin(); it != kids.end(); it++)
	{
		(it->second) -> setMaxRate(in, val);
	}
}

