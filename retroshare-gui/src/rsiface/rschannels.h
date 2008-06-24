#ifndef RS_CHANNEL_GUI_INTERFACE_H
#define RS_CHANNEL_GUI_INTERFACE_H

/*
 * libretroshare/src/rsiface: rschannels.h
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2008 by Robert Fernie.
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


#include <list>
#include <iostream>
#include <string>

#include "rsiface/rstypes.h"
#include "rsiface/rsdistrib.h" /* For FLAGS */

class ChannelInfo 
{
	public:
	ChannelInfo() {}
	std::string  channelId;
	std::wstring channelName;
	std::wstring channelDesc;

	uint32_t channelFlags;
	uint32_t pop;

	time_t lastPost;
};

class ChannelMsgInfo 
{
	public:
	ChannelMsgInfo() {}
	std::string channelId;
	std::string msgId;

	unsigned int msgflags;

	std::wstring subject;
	std::wstring msg;
	time_t ts;

	std::list<FileInfo> files;
	uint32_t count;
	uint64_t size;
};


class ChannelMsgSummary 
{
	public:
	ChannelMsgSummary() {}
	std::string channelId;
	std::string msgId;

	uint32_t msgflags;

	std::wstring subject;
	std::wstring msg;
	uint32_t count; /* file count     */
	time_t ts;

};

std::ostream &operator<<(std::ostream &out, const ChannelInfo &info);
std::ostream &operator<<(std::ostream &out, const ChannelMsgSummary &info);
std::ostream &operator<<(std::ostream &out, const ChannelMsgInfo &info);

class RsChannels;
extern RsChannels   *rsChannels;

class RsChannels 
{
	public:

	RsChannels() { return; }
virtual ~RsChannels() { return; }

/****************************************/

virtual bool channelsChanged(std::list<std::string> &chanIds) = 0;


virtual std::string createChannel(std::wstring chanName, std::wstring chanDesc, uint32_t chanFlags) = 0;

virtual bool getChannelInfo(std::string cId, ChannelInfo &ci) = 0;
virtual bool getChannelList(std::list<ChannelInfo> &chanList) = 0;
virtual bool getChannelMsgList(std::string cId, std::list<ChannelMsgSummary> &msgs) = 0;
virtual bool getChannelMessage(std::string cId, std::string mId, ChannelMsgInfo &msg) = 0;

virtual	bool ChannelMessageSend(ChannelMsgInfo &info)                 = 0;

virtual bool channelSubscribe(std::string cId, bool subscribe)	= 0;
/****************************************/

};


#endif
