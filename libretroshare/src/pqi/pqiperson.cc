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

#include <list>

#include "pqi/pqiperson.h"

const int pqipersonzone = 82371;
#include "pqi/pqidebug.h"
#include <sstream>


pqiperson::pqiperson(cert *c, std::string id)
	:PQInterface(id), sslcert(c), active(false), activepqi(NULL), 
	inConnectAttempt(false), waittimes(0)
{
	sroot = getSSLRoot();

	// check certificate
	
/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
	sroot -> validateCertificateXPGP(sslcert);
#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
	sroot -> validateCertificate(sslcert);
#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

	if (!(sslcert -> Valid()))
	{
		std::cerr << "pqiperson::Warning Certificate Not Approved!";
		std::cerr << " pqiperson will not initialise...." << std::endl;
	}

	return;
}

pqiperson::~pqiperson()
{
	// clean up the children.
	std::list<pqiconnect *>::iterator it = kids.begin();
	for(it = kids.begin(); it != kids.end(); )
	{
		pqiconnect *pc = (*it);
		it = kids.erase(it);
		delete pc;
	}
}


	// The PQInterface interface.
int     pqiperson::SendItem(RsItem *i)
{
	std::ostringstream out;
	out << "pqiperson::SendItem()";
	if (active)
	{
		out << " Active: Sending On";
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

cert *	pqiperson::getContact()
{
	return sslcert;
}

	// tick......
int	pqiperson::tick()
{
	int activeTick = 0;

	{
	  std::ostringstream out;
	  out << "pqiperson::tick() Cert: " << sslcert -> Name() << " ";
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
	std::list<pqiconnect *>::iterator it;
	for(it = kids.begin(); it != kids.end(); it++)
	{
		if (0 < (*it) -> tick())
		{
			activeTick = 1;
		}
	  	out << "\tTicking Child: " << (*it) << std::endl;
	}

	  pqioutput(PQL_DEBUG_ALL, pqipersonzone, out.str());
	} // end of pqioutput.

	// use cert settings to control everything.
	if (!active)
	{
		if ((sslcert -> WillConnect()) && (!inConnectAttempt))
		{
			if (sslcert -> nc_timestamp < time(NULL))
			{
				// calc the time til next connect.
				sslcert -> nc_timestamp = time(NULL) + 
						connectWait();
				connectattempt(NULL);
			}
		}
		if (sslcert -> Listening())
		{
			if (!(sslcert -> WillListen()))
			{
				stoplistening();
			}
		}
		else
		{
			if (sslcert -> WillListen())
			{
				listen();
			}
		}
	}

	return activeTick;
}

// callback function for the child - notify of a change.
// This is only used for out-of-band info....
// otherwise could get dangerous loops.
int 	pqiperson::notifyEvent(NetInterface *ni, int newState)
{
	{
	  std::ostringstream out;
	  out << "pqiperson::notifyEvent() Cert: " << sslcert -> Name();
	  out << std::endl;
	  out << "Message: " << newState << " from: " << ni << std::endl;

	  pqioutput(PQL_DEBUG_BASIC, pqipersonzone, out.str());
	}

	/* find the pqi, */
	pqiconnect *pqi = NULL;
	std::list<pqiconnect *>::iterator it;
		
	/* start again */
	int i = 0;
	for(it = kids.begin(); it != kids.end(); it++)
	{
	 	std::ostringstream out;
	  	out << "pqiperson::connectattempt() Kid# ";
	  	out << i << " of " << kids.size();
	  	out << std::endl;
		out << " pqiconn: " << (*it);
		out << " ni: " << (*it)->ni;
		out << " in_ni: " << ni;
	  	pqioutput(PQL_DEBUG_BASIC, pqipersonzone, out.str());
		i++;

		if ((*it)->thisNetInterface(ni))
		{
			pqi = (*it);
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
		if ((active) && (activepqi != pqi)) // already connected - trouble
		{
	  		pqioutput(PQL_WARNING, pqipersonzone, 
				"CONNECT_SUCCESS+active->trouble: shutdown EXISTING->switch to new one!");

			//pqi -> stoplistening();

			// This is the RESET that's killing the connections.....
			activepqi -> reset();
			// this causes a recursive call back into this fn.
			// which cleans up state.
			// we only do this if its not going to mess with new conn.
		}

		/* now install a new one. */
		{

	  		pqioutput(PQL_WARNING, pqipersonzone, 
				"CONNECT_SUCCESS->marking so!");
			// mark as active.
			active = true;
			activepqi = pqi;
			sslcert -> Connected(true);
			sroot -> IndicateCertsChanged();

			connectSuccess();

	  		inConnectAttempt = false;
			// dont stop listening.
			//stoplistening();
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
				sslcert -> Connected(false);
				sroot -> IndicateCertsChanged();
				return 1;
			}
			else
			{
	  			pqioutput(PQL_WARNING, pqipersonzone, 
					"CONNECT_FAIL+not activepqi->strange!");
				// something strange!
				return -1;
			}
		}
		else
		{
	  		pqioutput(PQL_WARNING, pqipersonzone, 
			  "CONNECT_FAILED+NOT active -> try connect again");

			connectattempt(pqi);
			return 1;
		}
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
	  out << "pqiperson::reset() Cert: " << sslcert -> Name();
	  out << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqipersonzone, out.str());
	}

	std::list<pqiconnect *>::iterator it;
	for(it = kids.begin(); it != kids.end(); it++)
	{
		(*it) -> reset();
	}		

	activepqi = NULL;
	active = false;
	sslcert -> Listening(false);
	sslcert -> Connected(false);
	sroot -> IndicateCertsChanged();

	// check auto setting at reset.
	if (!sslcert -> Manual())
		autoconnect(true);

	return 1;
}

int	pqiperson::autoconnect(bool b)
{
	{
	  std::ostringstream out;
	  out << "pqiperson::autoconnect() Cert: " << sslcert -> Name();
	  out << " - " << (int) b << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqipersonzone, out.str());
	}

	sslcert -> Manual(!b);

	sslcert -> WillConnect(b);
	sslcert -> WillListen(b);
	sroot -> IndicateCertsChanged();

	return 1;
}


int	pqiperson::addChildInterface(pqiconnect *pqi)
{
	{
	  std::ostringstream out;
	  out << "pqiperson::addChildInterface() : " << pqi;
	  out << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqipersonzone, out.str());
	}

	kids.push_back(pqi);
	return 1;
}

/***************** PRIVATE FUNCTIONS ***********************/
// functions to iterate over the connects and change state.


int 	pqiperson::listen()
{
	{
	  std::ostringstream out;
	  out << "pqiperson::listen() Cert: " << sslcert -> Name();
	  out << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqipersonzone, out.str());
	}

	if (!active)
	{
		std::list<pqiconnect *>::iterator it;
		for(it = kids.begin(); it != kids.end(); it++)
		{
			// set them all listening.
			(*it) -> listen();
		}
		sslcert -> Listening(true);
		sroot -> IndicateCertsChanged();
	}
	return 1;
}


int 	pqiperson::stoplistening()
{
	{
	  std::ostringstream out;
	  out << "pqiperson::stoplistening() Cert: " << sslcert -> Name();
	  out << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqipersonzone, out.str());
	}

	std::list<pqiconnect *>::iterator it;
	for(it = kids.begin(); it != kids.end(); it++)
	{
		// set them all listening.
		(*it) -> stoplistening();
	}
	sslcert -> Listening(false);
	sroot -> IndicateCertsChanged();

	return 1;
}

int	pqiperson::connectattempt(pqiconnect *last)
{
	{
	  std::ostringstream out;
	  out << "pqiperson::connectattempt() Cert: " << sslcert -> Name();
	  out << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqipersonzone, out.str());
	}

	std::list<pqiconnect *>::iterator it = kids.begin();
	int i = 0; 
	if (last != NULL)
	{
		// find the current connection.
		for(; (it != kids.end()) && ((*it) != last); it++, i++);

		if (it != kids.end())
		{
	  		std::ostringstream out;
	  		out << "pqiperson::connectattempt() Last Cert#: ";
	  		out << i << " of " << kids.size();
	  		out << std::endl;
	  		pqioutput(PQL_DEBUG_BASIC, pqipersonzone, out.str());


			it++;
			i++;
		}
	}
	// now at the first one to try.
	{
	  std::ostringstream out;
	  out << "pqiperson::connectattempt() Cert: " << sslcert -> Name();
	  out << " Starting Attempts at Interface " << i << " of " << kids.size();
	  out << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqipersonzone, out.str());
	}

	// try to connect on the next one.
	for(; (it != kids.end()) && (0 > (*it)->connectattempt()); it++, i++); 

	if (it == kids.end())
	{
	  std::ostringstream out;
	  out << "pqiperson::connectattempt() Cert: " << sslcert -> Name();
	  out << " Failed on All Interfaces";
	  out << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqipersonzone, out.str());
	}
	else
	{
	  std::ostringstream out;
	  out << "pqiperson::connectattempt() Cert: " << sslcert -> Name();
	  out << " attempt on pqiconnect #" << i << " suceeded";
	  out << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqipersonzone, out.str());
	}
		
	// flag if we started a new connectionAttempt.
	inConnectAttempt = (it != kids.end());

	return 1;
}


// returns (in secs) the time til next connect attempt
// this is based on the 
//
#define SEC_PER_LOG_UNIT   600 /* ten minutes? */
#define MAX_WAITTIMES      10  /* ten minutes? */

int	pqiperson::connectWait()
{
	int log_weight = (1 << waittimes);
	// max wait of 2^10 = 1024  * X min (over a month)
	// 			10240 min = 1 day
	waittimes++;
	if (waittimes > MAX_WAITTIMES)
		waittimes = MAX_WAITTIMES;
	// make it linear instead!.
	return waittimes * SEC_PER_LOG_UNIT;
	//return log_weight * SEC_PER_LOG_UNIT;
}

// called to reduce the waittimes, next time.
int	pqiperson::connectSuccess()
{
	return waittimes = 0;
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
	std::list<pqiconnect *>::iterator it = kids.begin();
	for(it = kids.begin(); it != kids.end(); it++)
	{
		(*it) -> setMaxRate(in, val);
	}
}

