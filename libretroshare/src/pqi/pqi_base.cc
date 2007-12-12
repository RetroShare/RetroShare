/*
 * "$Id: pqi_base.cc,v 1.17 2007-03-31 09:41:32 rmf24 Exp $"
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




#include "pqi/pqi_base.h"

#include <ctype.h>

// local functions.
int pqiroute_setshift(ChanId *item, int chan);
int pqiroute_getshift(ChanId *item);

// these ones are also exported!
int	pqicid_clear(ChanId *cid);
int	pqicid_copy(const ChanId *cid, ChanId *newcid);
int	pqicid_cmp(const ChanId *cid1, ChanId *cid2);

// Helper functions for the PQInterface.

static int next_search_id = 1;

int getPQIsearchId()
{
	return next_search_id++;
}


// CHANID Operations.
int	pqicid_clear(ChanId *cid)
{
	for(int i = 0; i < 10; i++)
	{
		cid -> route[i] = 0;
	}
	return 1;
}

int	pqicid_copy(const ChanId *cid, ChanId *newcid)
{
	for(int i = 0; i < 10; i++)
	{
		(newcid -> route)[i] = (cid -> route)[i];
	}
	return 1;
}

int	pqicid_cmp(const ChanId *cid1, ChanId *cid2)
{
	int ret = 0;
	for(int i = 0; i < 10; i++)
	{
		ret = cid1->route[i] - cid2->route[i];
		if (ret != 0)
		{
			return ret;
		}
	}
	return 0;
}




int pqiroute_getshift(ChanId *id)
{
	int *array = id -> route;
	int next = array[0];

	// shift.
	for(int i = 0; i < 10 - 1; i++)
	{
		array[i] = array[i+1];
	}
	array[10 - 1] = 0;

	return next;
}

int pqiroute_setshift(ChanId *id, int chan)
{
	int *array = id -> route;

	// shift.
	for(int i = 10 - 1; i > 0; i--)
	{
		array[i] = array[i-1];
	}
	array[0] = chan;

	return 1;
}

/****************** PERSON DETAILS ***********************/

Person::Person()
	:dhtFound(false), dhtFlags(0),
	lc_timestamp(0), lr_timestamp(0),
	nc_timestamp(0), nc_timeintvl(5),
	name("Unknown"), status(PERSON_STATUS_MANUAL)


	{
		for(int i = 0; i < (signed) sizeof(lastaddr); i++)
		{
			((unsigned char *) (&lastaddr))[i] = 0;
			((unsigned char *) (&localaddr))[i] = 0;
			((unsigned char *) (&serveraddr))[i] = 0;
			((unsigned char *) (&dhtaddr))[i] = 0;
		}
		pqicid_clear(&cid);


		return; 
	}

Person::~Person()
	{
	}
	

int	Person::cidpop()
{
	return pqiroute_getshift(&cid);
}

void	Person::cidpush(int id)
{
	pqiroute_setshift(&cid, id);
	return;
}

bool	Person::Group(std::string in)
	{
		std::list<std::string>::iterator it;
		for(it = groups.begin(); it != groups.end(); it++)
		{
			if (in == (*it))
			{
				return true;
			}
		}
		return false;
	}

	
int	Person::addGroup(std::string in)
	{
		groups.push_back(in);
		return 1;
	}

int	Person::removeGroup(std::string in)
	{
		std::list<std::string>::iterator it;
		for(it = groups.begin(); it != groups.end(); it++)
		{
			if (in == (*it))
			{
				groups.erase(it);
				return 1;
			}
		}
		return 0;
	}



bool	Person::Valid()
	{
		return (status & PERSON_STATUS_VALID);
	}

void	Person::Valid(bool b)
	{
		if (b)
			status |= PERSON_STATUS_VALID;
		else
			status &= ~PERSON_STATUS_VALID;
	}

bool	Person::Accepted()
	{
		return (status & PERSON_STATUS_ACCEPTED);
	}

void	Person::Accepted(bool b)
	{
		if (b)
			status |= PERSON_STATUS_ACCEPTED;
		else
			status &= ~PERSON_STATUS_ACCEPTED;
	}

bool	Person::InUse()
	{
		return (status & PERSON_STATUS_INUSE);
	}

void	Person::InUse(bool b)
	{
		if (b)
			status |= PERSON_STATUS_INUSE;
		else
			status &= ~(PERSON_STATUS_INUSE);
	}


bool	Person::Listening()
	{
		return (status & PERSON_STATUS_LISTENING);
	}

void	Person::Listening(bool b)
	{
		if (b)
			status |= PERSON_STATUS_LISTENING;
		else
			status &= ~PERSON_STATUS_LISTENING;
	}

bool	Person::Connected()
	{
		return (status & PERSON_STATUS_CONNECTED);
	}

void	Person::Connected(bool b)
	{
		if (b)
			status |= PERSON_STATUS_CONNECTED;
		else
			status &= ~PERSON_STATUS_CONNECTED;
	}

bool	Person::WillListen()
	{
		return (status & PERSON_STATUS_WILL_LISTEN);
	}

void	Person::WillListen(bool b)
	{
		if (b)
			status |= PERSON_STATUS_WILL_LISTEN;
		else
			status &= ~PERSON_STATUS_WILL_LISTEN;
	}

bool	Person::WillConnect()
	{
		return (status & PERSON_STATUS_WILL_CONNECT);
	}

void	Person::WillConnect(bool b)
	{
		if (b)
			status |= PERSON_STATUS_WILL_CONNECT;
		else
			status &= ~PERSON_STATUS_WILL_CONNECT;
	}

bool	Person::Manual()
	{
		return (status & PERSON_STATUS_MANUAL);
	}

void	Person::Manual(bool b)
	{
		if (b)
			status |= PERSON_STATUS_MANUAL;
		else
			status &= ~PERSON_STATUS_MANUAL;
	}

bool	Person::Firewalled()
	{
		return (status & PERSON_STATUS_FIREWALLED);
	}

void	Person::Firewalled(bool b)
	{
		if (b)
			status |= PERSON_STATUS_FIREWALLED;
		else
			status &= ~PERSON_STATUS_FIREWALLED;
	}

bool	Person::Forwarded()
	{
		return (status & PERSON_STATUS_FORWARDED);
	}

void	Person::Forwarded(bool b)
	{
		if (b)
			status |= PERSON_STATUS_FORWARDED;
		else
			status &= ~PERSON_STATUS_FORWARDED;
	}

bool	Person::Local()
	{
		return (status & PERSON_STATUS_LOCAL);
	}

void	Person::Local(bool b)
	{
		if (b)
			status |= PERSON_STATUS_LOCAL;
		else
			status &= ~PERSON_STATUS_LOCAL;
	}


bool	Person::Trusted()
	{
		return (status & PERSON_STATUS_TRUSTED);
	}

void	Person::Trusted(bool b)
	{
		if (b)
			status |= PERSON_STATUS_TRUSTED;
		else
			status &= ~PERSON_STATUS_TRUSTED;
	}


unsigned int	Person::Status()
	{
		return status;
	}


void	Person::Status(unsigned int s)
	{
		status = s;
	}

std::string Person::Name()
	{
		return name;
	}


void	Person::Name(std::string n)
	{
		name = n;
	}

        /* Dynamic Address Foundation */
bool    Person::hasDHT()
{
	return dhtFound;
}

void    Person::setDHT(struct sockaddr_in addr, unsigned int flags)
{
	dhtFound = true;
	dhtFlags = flags;
	dhtaddr = addr;
}

/* GUI Flags */
bool	Person::InChat()
	{
		return (status & PERSON_STATUS_INCHAT);
	}

void	Person::InChat(bool b)
	{
		if (b)
			status |= PERSON_STATUS_INCHAT;
		else
			status &= ~PERSON_STATUS_INCHAT;
	}

bool	Person::InMessage()
	{
		return (status & PERSON_STATUS_INMSG);
	}

void	Person::InMessage(bool b)
	{
		if (b)
			status |= PERSON_STATUS_INMSG;
		else
			status &= ~PERSON_STATUS_INMSG;
	}


