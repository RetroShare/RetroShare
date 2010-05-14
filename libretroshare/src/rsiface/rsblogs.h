#ifndef RS_BLOG_GUI_INTERFACE_H
#define RS_BLOG_GUI_INTERFACE_H

/*
 * libretroshare/src/rsiface: rsblogs.h
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

class BlogInfo 
{
	public:
	BlogInfo() {}
	std::string  blogId;
	std::wstring blogName;
	std::wstring blogDesc;

	uint32_t blogFlags;
	uint32_t pop;

	unsigned char* pngChanImage;
	uint32_t pngImageLen;

	time_t lastPost;
};

class BlogMsgInfo 
{
	public:
	BlogMsgInfo() {}
	std::string blogId;
	std::string msgId;
	/// this has a value if replying to another msg
	std::string msgIdReply;

	unsigned int msgflags;

	std::wstring subject;
	std::wstring msg;
	time_t ts;

	std::list<FileInfo> files;
	uint32_t count;
	uint64_t size;
};


class BlogMsgSummary 
{
	public:
	BlogMsgSummary() {}
	std::string blogId;
	std::string msgId;

	uint32_t msgflags;

	std::wstring subject;
	std::wstring msg;
	std::string msgIdReply;
	uint32_t count; /* file count     */
	time_t ts;

};

std::ostream &operator<<(std::ostream &out, const BlogInfo &info);
std::ostream &operator<<(std::ostream &out, const BlogMsgSummary &info);
std::ostream &operator<<(std::ostream &out, const BlogMsgInfo &info);

class RsBlogs;
extern RsBlogs   *rsBlogs;

class RsBlogs 
{
	public:

	RsBlogs() { return; }
virtual ~RsBlogs() { return; }

/****************************************/

/*!
 * Checks if the group a blod id belongs to has changed
 */
virtual bool blogsChanged(std::list<std::string> &blogIds) = 0;


virtual std::string createBlog(std::wstring blogName, std::wstring blogDesc, uint32_t blogFlags,
		unsigned char* pngImageData, uint32_t imageSize) = 0;

virtual bool getBlogInfo(std::string cId, BlogInfo &ci) = 0;
virtual bool getBlogList(std::list<BlogInfo> &chanList) = 0;
virtual bool getBlogMsgList(std::string cId, std::list<BlogMsgSummary> &msgs) = 0;

/*!
 * Retrieves a specific blog Msg based on group Id and message Id
 */
virtual bool getBlogMessage(std::string cId, std::string mId, BlogMsgInfo &msg) = 0;

/*!
 * Can send blog message to user
 * @param info the message
 */
virtual bool BlogMessageSend(BlogMsgInfo &info)                 = 0;

/*!
 * Allows user to subscribe to a blog via group ID
 * @param cId group id
 * @param subscribe determine subscription based on value
 */
virtual bool blogSubscribe(std::string cId, bool subscribe)	= 0;

/*!
 * Commenting on other user's blogs, ensure field info has a valid info.msgIdReply has valid msg id, this
 * points to which message the blog reply is replying to
 */
virtual bool BlogMessageReply(BlogMsgInfo &info) = 0;

/*!
 *
 */
virtual bool isReply(BlogMsgInfo &info) = 0;
/****************************************/

};


#endif
