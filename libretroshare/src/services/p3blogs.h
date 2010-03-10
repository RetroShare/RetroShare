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


/**
 * check if any of the blogs has been changed
 * @param blogIds list blogs by ids to be checked
 * @return false if changed and vice versa
 */
virtual bool blogsChanged(std::list<std::string> &blogIds);

/**
 * creates a new blog
 */
virtual std::string createBlog(std::wstring blogName, std::wstring blogDesc, uint32_t blogFlags);

/**
 * @param cId the id for the blog
 * @param BloogInfo blog information is stored here
 */
virtual bool getBlogInfo(std::string cId, BlogInfo &ci);

/**
 * get list of blogs from the group peer belongs to
 */
virtual bool getBlogList(std::list<BlogInfo> &blogList);

/**
 * Returns a list of all the messages sent
 * @param cId group id
 * @param msgs
 */
virtual bool getBlogMsgList(std::string cId, std::list<BlogMsgSummary> &msgs);
virtual bool getBlogMessage(std::string cId, std::string mId, BlogMsgInfo &msg);

virtual bool BlogMessageSend(BlogMsgInfo &info);

/**
 * subscribes or unsubscribes peer to a blog message
 * @param cId the id of the blog message peer wants to subscribe
 * @param subscribe set to true to subscribe, false otherwise
 */
virtual bool blogSubscribe(std::string cId, bool subscribe);


/**
 * send a reply to a blog msg, ensure msgIdReply has valid id to a message
 */
virtual bool BlogMessageReply(BlogMsgInfo &info);

/**
 * check if a particular blog is a reply
 * @return false if not a reply, true otherwise
 */
virtual bool isReply(BlogMsgInfo& info);

/**
 * Deletes a blog by group id
 *
 */
virtual bool deleteBlog(std::string cId);

/**
 * Delete a blog message
 * @param cId the group id the blog message belongs to
 * @param mId the message id within the group
 */
virtual bool deleteBlogMsg(std::string cId, std::string mId);

virtual bool isBlogDeleted(std::string cId);

virtual bool isBlogMsgDeleted(std::string cId, std::string mId);
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
