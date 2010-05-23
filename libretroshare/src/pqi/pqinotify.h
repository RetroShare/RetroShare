#ifndef PQI_NOTIFY_INTERFACE_H
#define PQI_NOTIFY_INTERFACE_H

/*
 * libretroshare/src/rsserver: pqinotify.h
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2007-2008 by Robert Fernie.
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

#include "rsiface/rsnotify.h"  /* for ids */

	/* Interface for System Notification: Implemented in rsserver */
	/* Global Access -> so we don't need everyone to have a pointer to this! */


class pqiNotify
{
	public:

	pqiNotify() { return; }
virtual ~pqiNotify() { return; }

	/* Input from libretroshare */
virtual bool AddPopupMessage(uint32_t ptype, std::string name, std::string msg) = 0;
virtual bool AddSysMessage(uint32_t sysid, uint32_t type, std::string title, std::string msg) = 0;
virtual bool AddLogMessage(uint32_t sysid, uint32_t type, std::string title, std::string msg) = 0;
virtual bool AddFeedItem(uint32_t type, std::string id1, std::string id2, std::string id3) = 0;
virtual bool ClearFeedItems(uint32_t type) = 0;
};

extern pqiNotify *getPqiNotify();

#endif
