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

bool p3Blog::getPeerLatestBlog(std::string id, uint32_t &ts, std::wstring &post)
{
	//return mQblog->getPeerLatestBlog(id, ts, post);
	
	// dummy info.
	
	ts = time(NULL);
	post = L"Hmmm, not much, just eating prawn crackers at the moment... but I'll post this every second if you want ;)";

	return true;
	
}

bool p3Blog::getPeerProfile(std::string id, std::list< std::pair<std::wstring, std::wstring> > &entries)
{
	//return mQblog->getPeerProfile(id, entries);
	
	std::wstring a = L"phone number";
	std::wstring b = L"0845 XXX 43639434363878735453";

	entries.push_back(make_pair(a,b));

	a = L"Favourite Film";
	b = L"Prawn Crackers revenge";

	entries.push_back(make_pair(a,b));

	a = L"Favourite Music";
	b = L"Eric Clapton, Neil Diamond, Folk, Country and Western";

	entries.push_back(make_pair(a,b));

	return true;
}

bool p3Blog::getPeerFavourites(std::string id, std::list<FileInfo> &favs)
{
	//return mQblog->getPeerFavourites(id, favs);
	
	FileInfo a;

	a.fname = "Prawn Crackers - Full Script.txt";
	a.size = 1000553;
	a.hash = "XXXXXXXXXXXXXXXXXXXXXX";
	
	favs.push_back(a);

	return true;

}


