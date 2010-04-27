#ifndef RS_STATUS_INTERFACE_H
#define RS_STATUS_INTERFACE_H

/*
 * libretroshare/src/rsiface: rsstatus.h
 *
 * RetroShare C++ .
 *
 * Copyright 2007-2008 by Vinny Do, Chris Evi-Parker.
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

class RsStatus;

extern RsStatus *rsStatus;

#include <iostream>
#include <string>
#include <inttypes.h>
#include <list>


const uint32_t RS_STATUS_AWAY    = 0x0001;
const uint32_t RS_STATUS_BUSY    = 0x0002;
const uint32_t RS_STATUS_ONLINE  = 0x0003;
const uint32_t RS_STATUS_INACTIVE = 0x0004;

//! data object for peer status information
/*!
 * data object used for peer status information
 */
class StatusInfo
{
	public:
	std::string id;
	uint32_t status;
	time_t time_stamp; /// for owner time set, and for their peers time sent
};


//! Interface to retroshare for Rs status
/*!
 * Provides an interface for retroshare's status functionality
 */
class RsStatus
{
	public:

	/**
	 * This retrieves the status info on the client's peers
	 * @param statusInfo is populated with client's peer's status
	 */
	virtual bool getStatus(std::list<StatusInfo>& statusInfo) = 0;

	/**
	 * send the client's status to his/her peers
	 * @param statusInfo the status of the peers
	 * @return will return false if status info does not belong to client
	 */
	virtual bool sendStatus(StatusInfo& statusInfo)                 = 0;

	/**
	 * checks to see if any status items have been received
	 */
	virtual bool statusAvailable() = 0;

	/**
	 * translates the status field of a peer to a string
	 * @status the status id that needs to be translated
	 * @statusString the string translation is passed here
	 */
	virtual void getStatusString(uint32_t status, std::string& statusString) = 0;

};


#endif
