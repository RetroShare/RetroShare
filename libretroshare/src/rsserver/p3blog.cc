/*
 * libretroshare/src/rsserver: p3blog.cc
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2007-2008 by Chris Evi-Parker.
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

#include "../rsserver/p3Blog.h"

RsQblog* rsQblog =  NULL;

p3Blog::p3Blog(p3Qblog* qblog) :
mQblog(qblog)
{
	return;
}

p3Blog::~p3Blog()
{
	return;
}

bool p3Blog::getBlogs(std::map< std::string, std::multimap<long int, std::wstring> > &blogs)
{
	return mQblog->getBlogs(blogs);
}

bool p3Blog::sendBlog(const std::wstring &msg)
{
	return mQblog->sendBlog(msg);
}

bool p3Blog::getPeerLatestBlog(std::string id, uint32_t &ts, std::wstring &post)
{
	//return mQblog->getPeerLatestBlog(id, ts, post);
	ts = time(NULL);
	post = L"Hmmm, not much, just eating prawn crackers at the moment... but I'll post this every second if you want ;)";

	return true;
}

