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

#include "rsiface/rsmsgs.h"

#include "pqi/pqi.h"
#include "pqi/pqiindic.h"
#include "services/p3service.h"
#include "serialiser/rsmsgitems.h"
#include "util/rsthreads.h"

class p3ConnectMgr;

class p3MsgService: public p3Service
{
	public:
	p3MsgService(p3ConnectMgr *cm);

	/* External Interface */
bool    ModifiedMsgs();
bool 	getMessageSummaries(std::list<MsgInfoSummary> &msgList);
bool 	getMessage(std::string mid, MessageInfo &msg);

bool    removeMsgId(std::string mid); 
bool    markMsgIdRead(std::string mid);

bool    MessageSend(MessageInfo &info);



void    loadWelcomeMsg(); /* startup message */

int     checkOutgoingMessages();

std::list<RsMsgItem *> &getMsgList();
std::list<RsMsgItem *> &getMsgOutList();


int	load_config();
int	save_config();

int	tick();
int	status();

	private:

int     sendMessage(RsMsgItem *item);
int 	incomingMsgs();

void 	initRsMI(RsMsgItem *msg, MessageInfo &mi);
void 	initRsMIS(RsMsgItem *msg, MsgInfoSummary &mis);
RsMsgItem *initMIRsMsg(MessageInfo &info, std::string to);


	/* Mutex Required for stuff below */

	RsMutex msgMtx;

std::map<uint32_t, RsMsgItem *> imsg;
std::map<uint32_t, RsMsgItem *> msgOutgoing; /* ones that haven't made it out yet! */

	p3ConnectMgr *mConnMgr;

	Indicator msgChanged;
	Indicator msgMajorChanged;

	std::string config_dir;
};

#endif // MESSAGE_SERVICE_HEADER
