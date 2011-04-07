#ifndef RS_P3_CHANNELS_INTERFACE_H
#define RS_P3_CHANNELS_INTERFACE_H

/*
 * libretroshare/src/services: p3channels.h
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

#include "retroshare/rschannels.h"
#include "retroshare/rsfiles.h"
#include "services/p3distrib.h"

#include "serialiser/rstlvtypes.h"
#include "serialiser/rschannelitems.h"

#define RS_CHAN_STATUS_MASK 0x000f
#define RS_CHAN_STATUS_AUTO_DL 0x0002

typedef std::map<std::string, uint32_t> statMap;
typedef std::map<std::string, statMap> chanStatMap;

//! Channels is a distributed 'feed' service
/*!
 * Implementations of rschannels interface
 *  @see RsChannels for definition of implemented interface
 */
class p3Channels: public p3GroupDistrib, public RsChannels 
{
	public:

	p3Channels(uint16_t type, CacheStrapper *cs, CacheTransfer *cft, RsFiles *files,
                std::string srcdir, std::string storedir, std::string channelsdir);
virtual ~p3Channels();

/*!
 * cleans up dowloaded files older than one month,
 * should be called during shutdown of rs
 */
void cleanUpOldFiles();

/****************************************/
/********* rsChannels Interface ***********/

virtual bool channelsChanged(std::list<std::string> &chanIds);

virtual std::string createChannel(std::wstring chanName, std::wstring chanDesc, uint32_t chanFlags,
		unsigned char* pngImageData, uint32_t size);

virtual bool getChannelInfo(std::string cId, ChannelInfo &ci);
virtual bool getChannelList(std::list<ChannelInfo> &chanList);
virtual bool getChannelMsgList(std::string cId, std::list<ChannelMsgSummary> &msgs);
virtual bool getChannelMessage(std::string cId, std::string mId, ChannelMsgInfo &msg);

virtual	bool ChannelMessageSend(ChannelMsgInfo &info);
virtual bool setMessageStatus(const std::string& cId, const std::string& mId, const uint32_t status, const uint32_t statusMask);
virtual bool getMessageStatus(const std::string& cId, const std::string& mId, uint32_t& status);

virtual	bool getMessageCount(const std::string cId, unsigned int &newCount, unsigned int &unreadCount);
virtual bool channelSubscribe(std::string cId, bool subscribe, bool autoDl);
virtual bool channelExtraFileHash(std::string path, std::string chId, FileInfo& fInfo);
virtual bool channelExtraFileRemove(std::string hash, std::string chId);
virtual bool channelRestoreKeys(std::string chId);
virtual bool channelShareKeys(std::string chId, std::list<std::string>& peers);
virtual bool channelEditInfo(std::string chId, ChannelInfo &ci);
virtual void getPubKeysAvailableGrpIds(std::list<std::string>& grpIds);
virtual bool channelSetAutoDl(const std::string& chId, bool autoDl);
virtual bool channelGetAutoDl(const std::string& chId, bool& autoDl);

/***************************************************************************************/
/****************** Event Feedback (Overloaded form p3distrib) *************************/
/***************************************************************************************/

	protected:
virtual void locked_notifyGroupChanged(GroupInfo &info, uint32_t flags, bool historical);
virtual bool locked_eventNewMsg(GroupInfo *, RsDistribMsg *, std::string, bool historical);
virtual bool locked_eventDuplicateMsg(GroupInfo *, RsDistribMsg *, std::string, bool historical);


/****************************************/
/********* Overloaded Functions *********/

virtual RsSerialType *createSerialiser();

virtual bool    locked_checkDistribMsg(RsDistribMsg *msg);

virtual bool childLoadList(std::list<RsItem* >& configSaves);
virtual std::list<RsItem *> childSaveList();


/****************************************/

	private:

void processChanReadStatus(RsChannelReadStatus* drs);
void addChannelReadStatusEntry(const std::string& cId);
void removeChannelReadStatusEntry(const std::string& cId);
	RsFiles *mRsFiles;
	std::string mChannelsDir;
	std::list<RsItem *> saveList;

	std::list<RsChannelReadStatus *> mReadStatus;
	std::map<std::string, RsChannelReadStatus* > mChanReadStatus;

	chanStatMap mMsgReadStatus;
	statMap mChannelStatus;
};


#endif
