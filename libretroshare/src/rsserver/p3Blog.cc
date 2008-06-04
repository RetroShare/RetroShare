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
 
#include "rsserver/p3Blog.h"

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
 
bool p3Blog::addToFilter(std::string &usrId)
{
	return mQblog->addToFilter(usrId);
}

bool p3Blog::getBlogs(std::map< std::string, std::multimap<long int, std:: string> > &blogs)
{
	return mQblog->getBlogs(blogs);
}

bool p3Blog::getFilterSwitch(void)
{
	return mQblog->getFilterSwitch();
}

bool p3Blog::getProfile(std::map<std::string, std::string> &profile)
{
	return mQblog->getProfile(profile);
}

bool p3Blog::getStatus(std::map<std::string, std::string> &usrStatus)
{
	return mQblog->getStatus(usrStatus);
}

bool p3Blog::sendBlog(const std::string &msg)
{
	return mQblog->sendBlog(msg);
}

bool p3Blog::setFilterSwitch(bool &filterSwitch)
{
	return mQblog->setFilterSwitch(filterSwitch);
}

bool p3Blog::setProfile(const std::string &favSong)
{
	return mQblog->setProfile(favSong);
}

bool p3Blog::setStatus(const std::string &status)
{
	return mQblog->setStatus(status);
}

bool p3Blog::removeFiltFriend(std::string &usrId)
{
	return mQblog->removeFiltFriend(usrId);
}

