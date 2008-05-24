#ifndef RSQBLOG_H_
#define RSQBLOG_H_

/*
 * libretroshare/src/rsiface: rsQblog.h
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2007-2008 by Chris Parker, Robert Fernie.
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
 
 #include <iostream>
 #include <string>
 #include <list>
 #include <map>
 
 
/* delcare interafce for everyone o use */
class RsQblog;
extern RsQblog *rsQblog;

 /*! allows gui to interface with the rsQblogs service */
 class RsQblog
 {
 	public:
 	
 	
		RsQblog() { return; }
		virtual ~RsQblog() { return; }
	 	
	 	/**
	 	 * allows user to set his status
	 	 * @param status The status of the user 
	 	 */
	 	virtual bool setStatus(const std::string &status) = 0;
	 	
	 	/**
	 	 * returns reference to map of usrs and their status
	 	 * @param usrStatus returns map to usr and their status
	 	 */
	 	virtual bool getStatus(std::map<std::string, std::string> &usrStatus) = 0;
	 	 
	 	/**
	 	 * choose whether to filter or not
	 	 * @param filterSwitch
	 	 */
	 	virtual bool setFilterSwitch(bool &filterSwitch) = 0;
	 	
	 	/**
	 	 * get usrs friend list
	 	 * @ param usrList
	 	 */
	 	virtual bool getFriendList(std::list<std::string> &friendList) =0;
	 	
	 	/**
	 	 * retrieve usrs filterSwitch status
	 	 */
	 	virtual bool getFilterSwitch(void) = 0;
	 	
	 	/**
	 	 * add user id to filter list 
	 	 * @param usr id to add to filter list
	 	 */
	 	virtual bool addToFilter(std::string &usrId) = 0;
	 	
	 	/**
	 	 * remove friend from filter list
	 	 * @param id The user's frined's id
	 	 */
	 	virtual bool removeFiltFriend(std::string &usrId) = 0;
	 	 
	 	/**
	 	 * get users fav song
	 	 * @param usrId the usr whose fav song you want
	 	 * @param favSong puts ref for fav song here
	 	 */
	 	virtual bool getProfile(std::map<std::string, std::string> &profile) = 0;
	 	  
	 	  
	 	  
	 	/**
	 	 * for now just fav song, TODO: must find way to link to rs profile
	 	 */
	 	virtual bool setProfile(const std::string &favSong) = 0;
	 	   
	 	/**
	 	 * send blog info, will send to a data structure for transmission
	     * @param msg The msg the usr wants to send
	     */
	    virtual bool sendBlog(const std::string &msg) = 0;
	 	   
	 	/**
	 	  * retrieve blog of a usr
	 	  * @param blogs contains the blog msgs of usr along with time posted for sorting
	 	  */
	 	virtual bool getBlogs(std::map< std::string, std::multimap<long int, std:: string> > &blogs) = 0;
 	   
 };

#endif /*RSQBLOG_H_*/
