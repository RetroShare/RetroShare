/*
 * libretroshare/src/rsserver: p3discovery.cc
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2008-2008 by Robert Fernie.
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

#include "rsserver/p3discovery.h"
#include <iostream>

RsDisc *rsDisc = NULL;

bool p3Discovery::getDiscGPGFriends(std::string id, std::list<std::string>& gpg_friends)
{
	if (mDisc)
	{
		return mDisc->potentialGPGproxies(id, gpg_friends);
	}
	return false;
}

bool p3Discovery::getDiscFriends(std::string id, std::list<std::string> &friends)
{
	if (mDisc)
	{
		return mDisc->potentialproxies(id, friends);
	}
	return false;
}

bool p3Discovery::getDiscVersions(std::map<std::string, std::string> &versions)
{
	if (mDisc)
	{
		mDisc->getversions(versions);
		return true;
	}
	return false;
}

bool p3Discovery::getWaitingDiscCount(unsigned int *sendCount, unsigned int *recvCount)
{
	if (mDisc)
	{
		mDisc->getWaitingDiscCount(sendCount, recvCount);
		return true;
	}
	return false;
}
