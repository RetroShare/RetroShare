/*
 * bitdht/bdaccount.cc
 *
 * BitDHT: An Flexible DHT library.
 *
 * Copyright 2011 by Robert Fernie
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 3 as published by the Free Software Foundation.
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
 * Please report all bugs and problems to "bitdht@lunamutt.com".
 *
 */

#include "bitdht/bdaccount.h"

#include <string.h>
#include <stdlib.h>

#include <iostream>
#include <iomanip>


#define LPF_FACTOR  (0.90)

bdAccount::bdAccount()
	:mNoStats(BDACCOUNT_NUM_ENTRIES), 
	mCountersOut(BDACCOUNT_NUM_ENTRIES), mCountersRecv(BDACCOUNT_NUM_ENTRIES), 
	mLpfOut(BDACCOUNT_NUM_ENTRIES), mLpfRecv(BDACCOUNT_NUM_ENTRIES), 
	mLabel(BDACCOUNT_NUM_ENTRIES)
{

	mLabel[BDACCOUNT_MSG_OUTOFDATEPING] 	=	 "OUTOFDATEPING  ";
	mLabel[BDACCOUNT_MSG_PING] 		=	 "PING           ";
	mLabel[BDACCOUNT_MSG_PONG] 		=	 "PONG           ";
	mLabel[BDACCOUNT_MSG_QUERYNODE] 	=	 "QUERYNODE      ";
	mLabel[BDACCOUNT_MSG_QUERYHASH] 	=	 "QUERYHASH      ";
	mLabel[BDACCOUNT_MSG_REPLYFINDNODE] 	=	 "REPLYFINDNODE  ";
	mLabel[BDACCOUNT_MSG_REPLYQUERYHASH] 	=	 "REPLYQUERYHASH ";

	mLabel[BDACCOUNT_MSG_POSTHASH] 		=	 "POSTHASH       ";
	mLabel[BDACCOUNT_MSG_REPLYPOSTHASH] 	=	 "REPLYPOSTHASH  ";

	mLabel[BDACCOUNT_MSG_CONNECTREQUEST] 	=	 "CONNECTREQUEST ";
	mLabel[BDACCOUNT_MSG_CONNECTREPLY] 	=	 "CONNECTREPLY   ";
	mLabel[BDACCOUNT_MSG_CONNECTSTART] 	=	 "CONNECTSTART   ";
	mLabel[BDACCOUNT_MSG_CONNECTACK] 	=	 "CONNECTACK     ";
	
	resetStats();
}


void bdAccount::incCounter(uint32_t idx, bool out)
{
	if ((signed) idx > mNoStats-1)
	{
		std::cerr << "bdAccount::incCounter() Invalid Index";
		std::cerr << std::endl;
	}

	if (out)
	{
		mCountersOut[idx]++;
	}
	else
	{
		mCountersRecv[idx]++;
	}
	return;
}


void bdAccount::doStats()
{
	int i;
	for(i = 0; i < mNoStats; i++)
	{
		mLpfOut[i] *= (LPF_FACTOR) ;
		mLpfOut[i] += (1.0 - LPF_FACTOR) * mCountersOut[i];

		mLpfRecv[i] *= (LPF_FACTOR) ;
		mLpfRecv[i] += (1.0 - LPF_FACTOR) * mCountersRecv[i];
	}
	resetCounters();
}

void bdAccount::printStats(std::ostream &out)
{
	int i;
	out << "  Send                                                 Recv: ";
	out << std::endl;
	for(i = 0; i < mNoStats; i++)
	{

		out << "Send" << mLabel[i] << " : " << std::setw(10) << mLpfOut[i];
		out << "          ";
		out << "Recv" << mLabel[i] << " : " << std::setw(10) << mLpfRecv[i];
		out << std::endl;
	}
}

void bdAccount::resetCounters()
{
	int i;
	for(i = 0; i < mNoStats; i++)
	{
		mCountersOut[i] = 0;
		mCountersRecv[i] = 0;
	}
}

void bdAccount::resetStats()
{
	int i;
	for(i = 0; i < mNoStats; i++)
	{
		mLpfOut[i] = 0;
		mLpfRecv[i] = 0;
	}
	resetCounters();
}




