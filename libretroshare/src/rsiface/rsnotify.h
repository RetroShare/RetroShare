#ifndef RS_NOTIFY_GUI_INTERFACE_H
#define RS_NOTIFY_GUI_INTERFACE_H

/*
 * libretroshare/src/rsiface: rsnotify.h
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


#include <map>
#include <list>
#include <iostream>
#include <string>


class RsNotify;
extern RsNotify   *rsNotify;

const uint32_t RS_SYS_ERROR 	= 0x0001;
const uint32_t RS_SYS_WARNING 	= 0x0002;
const uint32_t RS_SYS_INFO 	= 0x0004;

const uint32_t RS_POPUP_MSG	= 0x0001;
const uint32_t RS_POPUP_CHAT	= 0x0002;
const uint32_t RS_POPUP_CALL	= 0x0004;
const uint32_t RS_POPUP_CONNECT = 0x0008;


class RsNotify 
{
	public:

	RsNotify() { return; }
virtual ~RsNotify() { return; }

	/* Output for retroshare-gui */
virtual bool NotifySysMessage(uint32_t &sysid, uint32_t &type, 
					std::string &title, std::string &msg)		= 0;
virtual bool NotifyPopupMessage(uint32_t &ptype, std::string &name, std::string &msg) 	= 0;

	/* Control over Messages */
virtual bool GetSysMessageList(std::map<uint32_t, std::string> &list)  			= 0;
virtual bool GetPopupMessageList(std::map<uint32_t, std::string> &list)			= 0;

virtual bool SetSysMessageMode(uint32_t sysid, uint32_t mode)				= 0;
virtual bool SetPopupMessageMode(uint32_t ptype, uint32_t mode)				= 0;

	/* Input from libretroshare */
virtual bool AddPopupMessage(uint32_t ptype, std::string name, std::string msg) 	= 0;
virtual bool AddSysMessage(uint32_t sysid, uint32_t type, 
					std::string title, std::string msg) 		= 0;

};


#endif
