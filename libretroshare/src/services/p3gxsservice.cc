/*
 * libretroshare/src/services p3gxsservice.cc
 *
 * Generic Service Support Class for RetroShare.
 *
 * Copyright 2012 by Robert Fernie.
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

#include "services/p3gxsservice.h"

p3GxsService::p3GxsService(uint16_t type) 
	:p3Service(type),  mReqMtx("p3GxsService")
{
	mNextToken = 0;
	return; 
}

bool	p3GxsService::generateToken(uint32_t &token)
{
	RsStackMutex stack(mReqMtx); /****** LOCKED *****/

	token = mNextToken++;

	return true;
}

bool    p3GxsService::storeRequest(const uint32_t &token, const uint32_t &ansType, const RsTokReqOptions &opts, const uint32_t &type, const std::list<std::string> &ids)
{
	RsStackMutex stack(mReqMtx); /****** LOCKED *****/

	gxsRequest req;
	req.token = token;
	req.reqTime = time(NULL);
	req.reqType = type;
	req.ansType = ansType;
	req.Options = opts;
	req.status = GXS_REQUEST_STATUS_PENDING;
	req.inList = ids;
	
	mRequests[token] = req;

	return true;
}


bool	p3GxsService::clearRequest(const uint32_t &token)
{
	RsStackMutex stack(mReqMtx); /****** LOCKED *****/

        std::map<uint32_t, gxsRequest>::iterator it;

	it = mRequests.find(token);
	if (it == mRequests.end())
	{
		return false;
	}

	mRequests.erase(it);

	return true;
}

bool	p3GxsService::updateRequestStatus(const uint32_t &token, const uint32_t &status)
{
	RsStackMutex stack(mReqMtx); /****** LOCKED *****/

        std::map<uint32_t, gxsRequest>::iterator it;

	it = mRequests.find(token);
	if (it == mRequests.end())
	{
		return false;
	}

	it->second.status = status;

	return true;
}

bool	p3GxsService::updateRequestInList(const uint32_t &token, std::list<std::string> ids)
{
	RsStackMutex stack(mReqMtx); /****** LOCKED *****/

        std::map<uint32_t, gxsRequest>::iterator it;

	it = mRequests.find(token);
	if (it == mRequests.end())
	{
		return false;
	}

	std::list<std::string>::iterator iit;
	for(iit = ids.begin(); iit != ids.end(); iit++)
	{
		it->second.inList.push_back(*iit);
	}

	return true;
}


bool	p3GxsService::updateRequestOutList(const uint32_t &token, std::list<std::string> ids)
{
	RsStackMutex stack(mReqMtx); /****** LOCKED *****/

        std::map<uint32_t, gxsRequest>::iterator it;

	it = mRequests.find(token);
	if (it == mRequests.end())
	{
		return false;
	}

	std::list<std::string>::iterator iit;
	for(iit = ids.begin(); iit != ids.end(); iit++)
	{
		it->second.outList.push_back(*iit);
	}

	return true;
}

#if 0
bool	p3GxsService::updateRequestData(const uint32_t &token, std::map<std::string, void *> data)
{
	RsStackMutex stack(mReqMtx); /****** LOCKED *****/

        std::map<uint32_t, gxsRequest>::iterator it;

	it = mRequests.find(token);
	if (it == mRequests.end())
	{
		return false;
	}

	std::map<std::string, void *>::iterator dit;
	for(dit = data.begin(); dit != data.end(); dit++)
	{
		it->second.readyData[dit->first] = dit->second;
	}

	return true;
}
#endif

bool    p3GxsService::checkRequestStatus(const uint32_t &token, uint32_t &status, uint32_t &reqtype, uint32_t &anstype, time_t &ts)
{
	RsStackMutex stack(mReqMtx); /****** LOCKED *****/

        std::map<uint32_t, gxsRequest>::iterator it;

	it = mRequests.find(token);
	if (it == mRequests.end())
	{
		return false;
	}
	
	status = it->second.status;
	reqtype = it->second.reqType;
	anstype = it->second.ansType;
	ts = it->second.reqTime;

	return true;
}


	// special ones for testing (not in final design)
bool    p3GxsService::tokenList(std::list<uint32_t> &tokens)
{
	RsStackMutex stack(mReqMtx); /****** LOCKED *****/

        std::map<uint32_t, gxsRequest>::iterator it;

	for(it = mRequests.begin(); it != mRequests.end(); it++)
	{
		tokens.push_back(it->first);
	}

	return true;
}

bool    p3GxsService::popRequestInList(const uint32_t &token, std::string &id)
{
	RsStackMutex stack(mReqMtx); /****** LOCKED *****/

        std::map<uint32_t, gxsRequest>::iterator it;

	it = mRequests.find(token);
	if (it == mRequests.end())
	{
		return false;
	}

	if (it->second.inList.size() == 0)
	{
		return false;
	}

	id = it->second.inList.front();
	it->second.inList.pop_front();

	return true;
}


bool    p3GxsService::popRequestOutList(const uint32_t &token, std::string &id)
{
	RsStackMutex stack(mReqMtx); /****** LOCKED *****/

        std::map<uint32_t, gxsRequest>::iterator it;

	it = mRequests.find(token);
	if (it == mRequests.end())
	{
		return false;
	}

	if (it->second.outList.size() == 0)
	{
		return false;
	}

	id = it->second.outList.front();
	it->second.outList.pop_front();

	return true;
}


bool    p3GxsService::loadRequestOutList(const uint32_t &token, std::list<std::string> &ids)
{
	RsStackMutex stack(mReqMtx); /****** LOCKED *****/

        std::map<uint32_t, gxsRequest>::iterator it;

	it = mRequests.find(token);
	if (it == mRequests.end())
	{
		return false;
	}

	if (it->second.outList.size() == 0)
	{
		return false;
	}

	ids = it->second.outList;
	it->second.outList.clear();

	return true;
}


#define MAX_REQUEST_AGE 10

bool 	p3GxsService::fakeprocessrequests()
        {
        std::list<uint32_t>::iterator it;
        std::list<uint32_t> tokens;

        tokenList(tokens);

        time_t now = time(NULL);
        for(it = tokens.begin(); it != tokens.end(); it++)
        {
                uint32_t status;
                uint32_t reqtype;
                uint32_t anstype;
                uint32_t token = *it;
                time_t   ts;
                checkRequestStatus(token, status, reqtype, anstype, ts);

                std::cerr << "p3GxsService::fakeprocessrequests() Token: " << token << " Status: " << status << " ReqType: " << reqtype << "Age: " << now - ts << std::endl;

                if (status == GXS_REQUEST_STATUS_PENDING)
                {
                        updateRequestStatus(token, GXS_REQUEST_STATUS_PARTIAL);
                }
                else if (status == GXS_REQUEST_STATUS_PARTIAL)
                {
                        updateRequestStatus(token, GXS_REQUEST_STATUS_COMPLETE);
                }
                else if (status == GXS_REQUEST_STATUS_DONE)
                {
                        std::cerr << "p3GxsService::fakeprocessrequests() Clearing Done Request Token: " << token;
                        std::cerr << std::endl;
                        clearRequest(token);
                }
                else if (now - ts > MAX_REQUEST_AGE)
                {
                        std::cerr << "p3GxsService::fakeprocessrequests() Clearing Old Request Token: " << token;
                        std::cerr << std::endl;
                        clearRequest(token);
                }
        }

        return true;
}



/*********************************************************************************************************/
/*********************************************************************************************************/
/*********************************************************************************************************/
/*********************************************************************************************************/
/*********************************************************************************************************/


/****** GXS Data Dummy .... Handles generic data storage and access. designed for testing GUIs
 * without being dependent on the backend.
 *
 *
 * 
 * Important to specify the exact behaviour with flags!
 *
 *
 *
 ****/

GxsDataProxy::GxsDataProxy()
	:mDataMtx("GxsDataProxyMtx")
{
	return;
}



bool GxsDataProxy::getGroupList(     uint32_t &token, const RsTokReqOptions &opts, const std::list<std::string> &groupIds, std::list<std::string> &outGroupIds)
{
	/* CASEs that this handles ... 
	 * 1) if groupIds is Empty... return all groupIds.
	 * 2) else copy list.
	 *
	 */
	std::cerr << "GxsDataProxy::getGroupList()";
	std::cerr << std::endl;

	if (groupIds.size() == 0)
	{
		/* grab all the ids */
		RsStackMutex stack(mDataMtx); /***** LOCKED *****/

        	std::map<std::string, RsGroupMetaData>::iterator mit;
		for(mit = mGroupMetaData.begin(); mit != mGroupMetaData.end(); mit++)
		{
			outGroupIds.push_back(mit->first);
		}
	}
	else
	{
		outGroupIds = groupIds;
	}

	return true;
}


bool GxsDataProxy::getMsgList(       uint32_t &token, const RsTokReqOptions &opts, const std::list<std::string> &groupIds, std::list<std::string> &outMsgIds)
{
	/* CASEs this handles.
	 * Input is groupList + Flags.
	 * 1) No Flags => All Messages in those Groups.
	 *
	 */

	bool onlyOrigMsgs = false;

	if (opts.mOptions & RS_TOKREQOPT_MSG_ORIGMSG)
	{
		onlyOrigMsgs = true;
	}

	std::cerr << "GxsDataProxy::getMsgList()";
	std::cerr << std::endl;

	std::list<std::string>::const_iterator it;
	std::map<std::string, RsMsgMetaData>::iterator mit;

	for(it = groupIds.begin(); it != groupIds.end(); it++)
	{
		for(mit = mMsgMetaData.begin(); mit != mMsgMetaData.end(); mit++)
		{
			if (mit->second.mGroupId == *it)
			{
				bool add = false;

				if (onlyOrigMsgs)
				{
					if (mit->second.mMsgId == mit->second.mOrigMsgId)
					{
						add = true;
					}
				}
				else
				{	
					add = true;
				}

				if (add)
				{
					outMsgIds.push_back(mit->first);
				}
			}
		}
	}
	return true;
}


bool GxsDataProxy::getMsgRelatedList(uint32_t &token, const RsTokReqOptions &opts, const std::list<std::string> &msgIds, std::list<std::string> &outMsgIds)
{
	/* CASEs this handles.
	 * Input is msgList + Flags.
	 * 1) No Flags => just copy msgIds (probably requesting data).
	 *
	 */

	std::cerr << "GxsDataProxy::getMsgRelatedList()";
	std::cerr << std::endl;

	bool onlyLatestMsgs = false;
	bool onlyAllVersions = false;
	if (opts.mOptions & RS_TOKREQOPT_MSG_LATEST)
	{
		onlyLatestMsgs = true;
	}
	else if (opts.mOptions & RS_TOKREQOPT_MSG_VERSIONS)
	{
		onlyAllVersions = true;
	}

	/* FALL BACK OPTION */
	if ((!onlyLatestMsgs) && (!onlyAllVersions))
	{
		/* just copy */
		outMsgIds = msgIds;

		return true;
	}

	std::list<std::string>::const_iterator it;
	std::map<std::string, RsMsgMetaData>::iterator mit;

	for(it = msgIds.begin(); it != msgIds.end(); it++)
	{
		/* getOriginal Message */
		mit = mMsgMetaData.find(*it);
		if (mit == mMsgMetaData.end())
		{
			continue;
		}

		std::string origMsgId = mit->second.mOrigMsgId;

		if (onlyLatestMsgs)
		{
			/* first guess is potentially better than Orig (can't be worse!) */
			time_t latestTs = mit->second.mPublishTs;
			std::string latestMsgId = mit->second.mMsgId;
			
			for(mit = mMsgMetaData.begin(); mit != mMsgMetaData.end(); mit++)
			{
				if (mit->second.mOrigMsgId == origMsgId)
				{
					if (mit->second.mPublishTs > latestTs)
					{
						latestTs = mit->second.mPublishTs;
						latestMsgId = mit->first;
					}
				}
			}
			outMsgIds.push_back(latestMsgId);
		}
		else if (onlyAllVersions)
		{
			for(mit = mMsgMetaData.begin(); mit != mMsgMetaData.end(); mit++)
			{
				if (mit->second.mOrigMsgId == origMsgId)
				{
					outMsgIds.push_back(mit->first);
				}
			}
		}
	}

	return true;
}


bool GxsDataProxy::createGroup(void *groupData)
{
	RsGroupMetaData meta;
	if (convertGroupToMetaData(groupData, meta))
	{
		if (!isUniqueGroup(meta.mGroupId))
		{
			std::cerr << "GxsDataProxy::createGroup() ERROR GroupId Clashes, discarding";
			std::cerr << std::endl;
			return false;
		}

		RsStackMutex stack(mDataMtx); /***** LOCKED *****/

		/* push into maps */
        	mGroupData[meta.mGroupId] = groupData;
        	mGroupMetaData[meta.mGroupId] = meta;

		return true;
	}

	std::cerr << "GxsDataProxy::createGroup() ERROR Failed to convert Data";
	std::cerr << std::endl;
	return false;
}


bool GxsDataProxy::createMsg(void *msgData)
{
	RsMsgMetaData meta;
	if (convertMsgToMetaData(msgData, meta))
	{
		if (!isUniqueMsg(meta.mMsgId))
		{
			std::cerr << "GxsDataProxy::createMsg() ERROR MsgId Clashes, discarding";
			std::cerr << std::endl;
			return false;
		}

		RsStackMutex stack(mDataMtx); /***** LOCKED *****/

		/* push into maps */
        	mMsgData[meta.mMsgId] = msgData;
        	mMsgMetaData[meta.mMsgId] = meta;

		return true;
	}

	std::cerr << "GxsDataProxy::createMsg() ERROR Failed to convert Data";
	std::cerr << std::endl;
	return false;
}



        /* These Functions must be overloaded to complete the service */
bool GxsDataProxy::convertGroupToMetaData(void *groupData, RsGroupMetaData &meta)
{
	std::cerr << "GxsDataProxy::convert fn ... please implement!";
	std::cerr << std::endl;
	return false;
}

bool GxsDataProxy::convertMsgToMetaData(void *groupData, RsMsgMetaData &meta)
{
	std::cerr << "GxsDataProxy::convert fn ... please implement!";
	std::cerr << std::endl;
	return false;
}

        /* extract Data */
bool GxsDataProxy::getGroupSummary(const std::string &groupId, RsGroupMetaData &groupSummary)
{
	RsStackMutex stack(mDataMtx); /***** LOCKED *****/

       	std::map<std::string, RsGroupMetaData>::iterator mit;
	mit = mGroupMetaData.find(groupId);

	if (mit == mGroupMetaData.end())
	{
		// error.
		std::cerr << "GxsDataProxy::getGroupMetaData() Error Finding GroupId: " << groupId;
		std::cerr << std::endl;
		return false;
	}
	else
	{
		groupSummary = mit->second;
	}
	return true;
}


bool GxsDataProxy::getMsgSummary(const std::string &msgId, RsMsgMetaData &msgSummary)
{
	RsStackMutex stack(mDataMtx); /***** LOCKED *****/

        std::map<std::string, RsMsgMetaData>::iterator mit;
	mit = mMsgMetaData.find(msgId);

	if (mit == mMsgMetaData.end())
	{
		// error.
		std::cerr << "GxsDataProxy::getMsgSummary() Error Finding MsgId: " << msgId;
		std::cerr << std::endl;
	}
	else
	{
		msgSummary = (mit->second);
	}
	return true;
}


        /* extract Data */
bool GxsDataProxy::getGroupSummary(const std::list<std::string> &groupIds, std::list<RsGroupMetaData> &groupSummary)
{
	std::list<std::string>::const_iterator it;
	for(it = groupIds.begin(); it != groupIds.end(); it++)
	{
		RsStackMutex stack(mDataMtx); /***** LOCKED *****/

        	std::map<std::string, RsGroupMetaData>::iterator mit;
		mit = mGroupMetaData.find(*it);

		if (mit == mGroupMetaData.end())
		{
			// error.
			std::cerr << "GxsDataProxy::getGroupMetaData() Error Finding GroupId: " << *it;
			std::cerr << std::endl;
		}
		else
		{
			groupSummary.push_back(mit->second);
		}
	}
	return true;
}


bool GxsDataProxy::getMsgSummary(const std::list<std::string> &msgIds, std::list<RsMsgMetaData> &msgSummary)
{
	std::list<std::string>::const_iterator it;
	for(it = msgIds.begin(); it != msgIds.end(); it++)
	{
		RsStackMutex stack(mDataMtx); /***** LOCKED *****/

        	std::map<std::string, RsMsgMetaData>::iterator mit;
		mit = mMsgMetaData.find(*it);

		if (mit == mMsgMetaData.end())
		{
			// error.
			std::cerr << "GxsDataProxy::getMsgSummary() Error Finding MsgId: " << *it;
			std::cerr << std::endl;
		}
		else
		{
			msgSummary.push_back(mit->second);
		}
	}
	return true;
}


bool GxsDataProxy::getGroupData(const std::string &groupId, void * &groupData)
{
	RsStackMutex stack(mDataMtx); /***** LOCKED *****/

        std::map<std::string, void *>::iterator mit;
	mit = mGroupData.find(groupId);

	if (mit == mGroupData.end())
	{
		// error.
		std::cerr << "GxsDataProxy::getGroupData() Error Finding GroupId: " << groupId;
		std::cerr << std::endl;
		return false;
	}
	else
	{
		groupData = mit->second;
	}
	return true;
}
		
bool GxsDataProxy::getMsgData(const std::string &msgId, void * &msgData)
{
	RsStackMutex stack(mDataMtx); /***** LOCKED *****/

        std::map<std::string, void *>::iterator mit;
	mit = mMsgData.find(msgId);

	if (mit == mMsgData.end())
	{
		// error.
		std::cerr << "GxsDataProxy::getMsgData() Error Finding MsgId: " << msgId;
		std::cerr << std::endl;
		return false;
	}
	else
	{
		msgData = mit->second;
	}
	return true;
}

#if 0
bool GxsDataProxy::getGroupData(const std::list<std::string> &groupIds, std::list<void *> &groupData)
{
	std::list<std::string>::const_iterator it;
	for(it = groupIds.begin(); it != groupIds.end(); it++)
	{
		RsStackMutex stack(mDataMtx); /***** LOCKED *****/

        	std::map<std::string, void *>::iterator mit;
		mit = mGroupData.find(*it);

		if (mit == mGroupData.end())
		{
			// error.
			std::cerr << "GxsDataProxy::getGroupData() Error Finding GroupId: " << *it;
			std::cerr << std::endl;
		}
		else
		{
			groupData.push_back(mit->second);
		}
	}
	return true;
}
		
bool GxsDataProxy::getMsgData(const std::list<std::string> &msgIds, std::list<void *> &msgData)
{
	std::list<std::string>::const_iterator it;
	for(it = msgIds.begin(); it != msgIds.end(); it++)
	{
		RsStackMutex stack(mDataMtx); /***** LOCKED *****/

        	std::map<std::string, void *>::iterator mit;
		mit = mMsgData.find(*it);

		if (mit == mMsgData.end())
		{
			// error.
			std::cerr << "GxsDataProxy::getMsgData() Error Finding MsgId: " << *it;
			std::cerr << std::endl;
		}
		else
		{
			msgData.push_back(mit->second);
		}
	}
	return true;
}
#endif

bool GxsDataProxy::isUniqueMsg(const std::string &msgId)
{
	RsStackMutex stack(mDataMtx); /***** LOCKED *****/

        std::map<std::string, void *>::iterator mit;
	mit = mMsgData.find(msgId);

	return (mit == mMsgData.end());
}

		
bool GxsDataProxy::isUniqueGroup(const std::string &groupId)
{
	RsStackMutex stack(mDataMtx); /***** LOCKED *****/

        std::map<std::string, void *>::iterator mit;
	mit = mGroupData.find(groupId);

	return (mit == mGroupData.end());
}



/*********************************************************************************************************/
/*********************************************************************************************************/
/*********************************************************************************************************/
/*********************************************************************************************************/
/*********************************************************************************************************/


p3GxsDataService::p3GxsDataService(uint16_t type, GxsDataProxy *proxy)
	:p3GxsService(type), mProxy(proxy)
{
	return;
}





bool 	p3GxsDataService::fakeprocessrequests()
{
	std::list<uint32_t> toClear;
	std::list<uint32_t>::iterator cit;
	time_t now = time(NULL);

	{ RsStackMutex stack(mReqMtx); /******* LOCKED *******/

	std::map<uint32_t, gxsRequest>::iterator it;
	
	for(it = mRequests.begin(); it != mRequests.end(); it++)
	{
                //std::cerr << "p3GxsDataService::fakeprocessrequests() Token: " << it->second.token << " Status: " << it->second.status << " ReqType: " << it->second.reqType << " Age: " << now - it->second.reqTime << std::endl;

                if (it->second.status == GXS_REQUEST_STATUS_PENDING)
                {
                	std::cerr << "p3GxsDataService::fakeprocessrequests() Processing Token: " << it->second.token << " Status: " << it->second.status << " ReqType: " << it->second.reqType << " Age: " << now - it->second.reqTime << std::endl;
                        it->second.status = GXS_REQUEST_STATUS_PARTIAL;
			/* PROCESS REQUEST! */
			switch(it->second.reqType)
			{
				case  GXS_REQUEST_TYPE_GROUPS:
					mProxy->getGroupList(it->second.token, it->second.Options, it->second.inList, it->second.outList);
					break;
				case  GXS_REQUEST_TYPE_MSGS:
					mProxy->getMsgList(it->second.token, it->second.Options, it->second.inList, it->second.outList);
					break;
				case  GXS_REQUEST_TYPE_MSGRELATED:
					mProxy->getMsgRelatedList(it->second.token, it->second.Options, it->second.inList, it->second.outList);
					break;
				default:
                        		it->second.status = GXS_REQUEST_STATUS_FAILED;
					break;
			}
                }
                else if (it->second.status == GXS_REQUEST_STATUS_PARTIAL)
                {
                        it->second.status = GXS_REQUEST_STATUS_COMPLETE;
                }
                else if (it->second.status == GXS_REQUEST_STATUS_DONE)
                {
                        std::cerr << "p3GxsDataService::fakeprocessrequests() Clearing Done Request Token: " << it->second.token;
                        std::cerr << std::endl;
			toClear.push_back(it->second.token);
                }
                else if (now - it->second.reqTime > MAX_REQUEST_AGE)
                {
                        std::cerr << "p3GxsDataService::fakeprocessrequests() Clearing Old Request Token: " << it->second.token;
                        std::cerr << std::endl;
			toClear.push_back(it->second.token);
                }
        }

	} // END OF MUTEX.

	for(cit = toClear.begin(); cit != toClear.end(); cit++)
	{
		clearRequest(*cit);
	}

        return true;
}


std::ostream &operator<<(std::ostream &out, const RsGroupMetaData &meta)
{
	out << "[ GroupId: " << meta.mGroupId << " Name: " << meta.mGroupName << " ]";
	return out;
}

std::ostream &operator<<(std::ostream &out, const RsMsgMetaData &meta)
{
	out << "[ GroupId: " << meta.mGroupId << " MsgId: " << meta.mMsgId << " Name: " << meta.mMsgName << " ]";
	out << " OrigMsgId: " << meta.mOrigMsgId;
	out << " ThreadId: " << meta.mThreadId;
	out << " ParentId: " << meta.mParentId;
	out << " AuthorId: " << meta.mAuthorId;
	out << " Name: " << meta.mMsgName << " ]";
	return out;
}


