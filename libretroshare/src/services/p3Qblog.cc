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


bool p3Qblog::setStatus(std::string &status)
{
	Status = status;
	return true;
}

bool p3Qblog::getFilterSwitch()
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
	friendList = FriendList;
	return true;
}

bool p3Qblog::getStatus(std::string &status)
{
	status = Status;
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
	
bool p3Qblog::sendBlog(std::string &msg)
{
	time_t msgCreatedTime;
	UsrBlogSet.at("Usr").insert(std::make_pair(msgCreatedTime, msg));
	return true;
}

bool p3Qblog::getProfile(std::string &usrId, std::string &favSong)
{	
	/* return error is set empty */
	if(FriendSongset.empty())
	{
		std::cerr << "friend song set empty!" << std::endl;
		return false;
	} 
	
	favSong = FriendSongset.at(usrId);
	return true;
}

bool p3Qblog::setProfile(std::string &favSong)
{
	FavSong = favSong;
	return true;
} 
	
void p3Qblog::loadDummy(void)
{
	/* load usr list */
	FriendList.push_back("Usr1"); // home usr/server
	FriendList.push_back("mike2");
	FriendList.push_back("mike3");
	FriendList.push_back("mike4");
	FriendList.push_back("mike5");
	
	/* set usr status: need to create usr/status set or just add to profile object */
	Status = "I'm chilling homey";
	
	/* set favsong: will be made part of profile */
	FavSong = "DeathOfAthousandSuns"; 
	
	/* load friend song set */
	FriendSongset.insert(std::make_pair("Usr1", FavSong)); // home usr/server
	FriendSongset.insert(std::make_pair("Mike2", "yowsers"));
	FriendSongset.insert(std::make_pair("Mike3", "destroyers"));
	FriendSongset.insert(std::make_pair("Mike4", "revolvers"));
	FriendSongset.insert(std::make_pair("Mike5", "pepolvers"));
	
	/* load could usr blogs, not all tho */
	
	std::string usrBlog = "I think we should eat more cheese";
	
	std::string Blog2 = "today was so cool, i got attacked by fifty ninja while buying a loaf 
	so i used my paper bag to suffocate each of them to death at hyper speed";
	
	std::string Blog5 = "I'm really a boring person and having nothin interesting to say";
	
	UsrBlogSet.insert(std::make_pair("Usr1", usrBlog));
	UsrBlogSet.insert(std::make_pair("Mike2", Blog2));
	UsrBlogSet.insert(std::make_pair("Mike5", Blog5));
	
}	
	




	
