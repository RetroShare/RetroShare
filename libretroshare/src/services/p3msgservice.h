/*
 * libretroshare/src/services msgservice.h
 *
 * Services for RetroShare.
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


#ifndef MESSAGE_SERVICE_HEADER
#define MESSAGE_SERVICE_HEADER

#include <list>
#include <map>
#include <iostream>

#include "pqi/pqi.h"
#include "pqi/pqiindic.h"
#include "services/p3service.h"
#include "serialiser/rsmsgitems.h"

#include "rsiface/rsiface.h"
class pqimonitor;
class sslroot;

class p3MsgService: public p3Service
{
	public:
	p3MsgService();

void    loadWelcomeMsg(); /* startup message */

int     sendMessage(RsMsgItem *item);
int     checkOutgoingMessages();

std::list<RsMsgItem *> &getMsgList();
std::list<RsMsgItem *> &getMsgOutList();

	// cleaning up....
int     removeMsgId(uint32_t   mid); /* id stored in sid */
int     markMsgIdRead(uint32_t mid);

int	load_config();
int	save_config();

int	tick();
int	status();

	private:

int 	incomingMsgs();


std::list<RsMsgItem *> imsg;
std::list<RsMsgItem *> msgOutgoing; /* ones that haven't made it out yet! */

// bool state flags.
	public:
	Indicator msgChanged;
	Indicator msgMajorChanged;

sslroot *sslr;
std::string config_dir;

};

#endif // MESSAGE_SERVICE_HEADER
