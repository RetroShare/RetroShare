#ifndef RS_P3_BLOGS_INTERFACE_H
#define RS_P3_BLOGS_INTERFACE_H

/*
 * libretroshare/src/services: p3blogs.h
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

#include "rsiface/rsblogs.h"
#include "rsiface/rsfiles.h"
#include "services/p3distrib.h"

#include "serialiser/rstlvtypes.h"
#include "serialiser/rsblogitems.h"


class p3Blogs: public p3GroupDistrib, public RsBlogs 
{
	public:

	p3Blogs(uint16_t type, CacheStrapper *cs, CacheTransfer *cft, RsFiles *files,
                std::string srcdir, std::string storedir, std::string blogsdir);
virtual ~p3Blogs();

/****************************************/
/********* rsBlogs Interface ***********/

virtual bool blogsChanged(std::list<std::string> &blogIds);

virtual std::string createBlog(std::wstring blogName, std::wstring blogDesc, uint32_t blogFlags);

virtual bool getBlogInfo(std::string cId, BlogInfo &ci);
virtual bool getBlogList(std::list<BlogInfo> &chanList);
virtual bool getBlogMsgList(std::string cId, std::list<BlogMsgSummary> &msgs);
virtual bool getBlogMessage(std::string cId, std::string mId, BlogMsgInfo &msg);

virtual bool BlogMessageSend(BlogMsgInfo &info);


virtual bool blogSubscribe(std::string cId, bool subscribe);

virtual bool BlogMessageReply(BlogMsgInfo &info);

virtual bool isReply(BlogMsgInfo& info);

/***************************************************************************************/
/****************** Event Feedback (Overloaded form p3distrib) *************************/
/***************************************************************************************/

	protected:
virtual void locked_notifyGroupChanged(GroupInfo &info, uint32_t flags);
virtual bool locked_eventNewMsg(GroupInfo *, RsDistribMsg *, std::string);
virtual bool locked_eventDuplicateMsg(GroupInfo *, RsDistribMsg *, std::string);


/****************************************/
/********* Overloaded Functions *********/

virtual RsSerialType *createSerialiser();

virtual bool    locked_checkDistribMsg(RsDistribMsg *msg);
virtual RsDistribGrp *locked_createPublicDistribGrp(GroupInfo &info);
virtual RsDistribGrp *locked_createPrivateDistribGrp(GroupInfo &info);


/****************************************/

	private:

	RsFiles *mRsFiles;
	std::string mBlogsDir;

};


#endif
