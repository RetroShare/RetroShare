/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008 Robert Fernie
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#ifndef _FEED_HOLDER_H
#define _FEED_HOLDER_H

#include <string>
#include <stdint.h>

const uint32_t FEEDHOLDER_MSG_MESSAGE	= 0x0001;
const uint32_t FEEDHOLDER_MSG_FORUM		= 0x0002;
const uint32_t FEEDHOLDER_MSG_CHANNEL	= 0x0003;
const uint32_t FEEDHOLDER_MSG_BLOG		= 0x0004;

class FeedHolder
{
public:
	virtual void deleteFeedItem(QWidget *item, uint32_t type) = 0;
	virtual	void openChat(std::string peerId) = 0;
};

#endif

