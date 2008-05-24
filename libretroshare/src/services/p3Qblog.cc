/*
 * libretroshare/src/services: p3Qblog.cc
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2007-2008 by Chris Evi-Parker
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

#include "services/p3Qblog.h"
#include <utility>
#include <ctime>

RsQblog *rsQblog = NULL;

p3Qblog::p3Qblog()
: FilterSwitch(false)
{
	loadDummy(); // load dummy data
	return;
}

p3Qblog::~p3Qblog()
{
	return;
}


bool p3Qblog::setStatus(const std::string &status)
{
	FriendStatusSet[usr] = status;
	return true;
}

bool p3Qblog::getFilterSwitch(void)
{
	return FilterSwitch;
}

bool p3Qblog::setFilterSwitch(bool &filterSwitch)
{
	FilterSwitch = filterSwitch;	
	return true;
}

bool p3Qblog::getFriendList(std::list<std::string> &friendList)
{
	if(FriendList.empty())
	{
		std::cerr << "FriendList empty!" << std::endl;
		return false;
	}
	
	friendList = FriendList;
	return true;
}

bool p3Qblog::getStatus(std::map<std::string, std::string> &usrStatus)
{
	usrStatus = FriendStatusSet;
	return true;
}

bool p3Qblog::removeFiltFriend(std::string &usrId)
{
	std::list<std::string>::iterator it;
	
	/* search through list to remove friend */
	for(it = FriendList.begin(); it != FriendList.end(); it++)
	{
		if(it->compare(usrId))
		{
			FriendList.erase(it); // remove friend from list
			return true; 
		}
	}
	
	std::cerr << "usr could not be found!" << std::endl;
	return false; // could not find friend 
}

bool p3Qblog::addToFilter(std::string& usrId)
{
	std::list<std::string>::iterator it;
	
	/* search through list to remove friend */
	for(it = FriendList.begin(); it != FriendList.end(); it++)
	{
		if(it->compare(usrId))
		{
			std::cerr << "usr already in list!" << std::endl;
			return false; // user already in list, not added
		}
	}
	
	FilterList.push_back(usrId);
	return true;
}
	
bool p3Qblog::getBlogs(std::map< std::string, std::multimap<long int, std:: string> > &blogs)
{
	if(UsrBlogSet.empty()) // return error blogs are empty
	{
		std::cerr << "usr blog set empty!" << std::endl;
		return false;
	}
	
	blogs = UsrBlogSet;
	return true;
}
	
bool p3Qblog::sendBlog(const std::string &msg)
{
	time_t msgCreatedTime;
	UsrBlogSet["Usr1"].insert(std::make_pair(msgCreatedTime, msg));
	return true;
}

bool p3Qblog::getProfile(std::map<std::string, std::string> &profile)
{	
	/* return error is set empty */
	if(FriendSongset.empty())
	{
		std::cerr << "friend song set empty!" << std::endl;
		return false;
	} 
	
	profile = FriendSongset;
	return true;
}

bool p3Qblog::setProfile(const std::string &favSong)
{
	FriendSongset[usr] = favSong;
	return true;
} 

void p3Qblog::loadDummy(void)
{
	/* load usr list */
	FriendList.push_back("Usr1"); // home usr/server
	FriendList.push_back("Mike2");
	FriendList.push_back("Mike3");
	FriendList.push_back("Mike4");
	FriendList.push_back("Mike5");
	
	/* set usr status: need to create usr/status set or just add to profile object */
	//TODO
	
	/* set favsong: will be made part of profile */
	FavSong = "DeathOfAthousandSuns"; 
	
	/* load friend song set */
	FriendSongset.insert(std::make_pair("Usr1", FavSong)); // home usr/server
	FriendSongset.insert(std::make_pair("Mike2", "yowsers"));
	FriendSongset.insert(std::make_pair("Mike3", "destroyers"));
	FriendSongset.insert(std::make_pair("Mike4", "revolvers"));
	FriendSongset.insert(std::make_pair("Mike5", "pepolvers"));
	
	/* load usr blogs */
	
	/* the usr dummy usr blogs */	
	std::string usrBlog = "I think we should eat more cheese";
	std::string Blog2 = "today was so cool, i got attacked by fifty ninja while buying a loaf so i used my paper bag to suffocate each of them to death at hyper speed";
	std::string Blog3 = "Nuthins up";
	std::string Blog4 = "stop bothering me";
	std::string Blog5 = "I'm really a boring person and having nothin interesting to say";
		
	
	time_t  time1, time2, time3, time4, time5; // times of blogs
	
	/*** populate time/blog multimaps and usrBlog map ****/	
	std::multimap<long int, std::string> timeBlog1, timeBlog2, timeBlog3, timeBlog4, timeBlog5;
	timeBlog1.insert(std::make_pair(time1, usrBlog));
	timeBlog2.insert(std::make_pair(time2, Blog2));
	timeBlog3.insert(std::make_pair(time3, Blog3));
	timeBlog4.insert(std::make_pair(time4, Blog4));
	timeBlog5.insert(std::make_pair(time5, Blog5));
	
	UsrBlogSet.insert(std::make_pair("Usr1", timeBlog1));
	UsrBlogSet.insert(std::make_pair("Mike2", timeBlog2));
	UsrBlogSet.insert(std::make_pair("Mike3", timeBlog2));
	UsrBlogSet.insert(std::make_pair("Mike4", timeBlog2));
	UsrBlogSet.insert(std::make_pair("Mike5", timeBlog5));
	
}	
	




	
