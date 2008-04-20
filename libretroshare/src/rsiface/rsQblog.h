#ifndef RSQBLOG_H_
#define RSQBLOG_H_

/*
 * libretroshare/src/rsiface: rsmsgs.h
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2007-2008 by Robert Fernie.
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
 #include <strings>
 #include <list>
 #include <time.h>
 #include <map>
 
 
 /*!
  * will contain profile information
  */
 class Profile;
 
/*!
 * blog records to be accessed only by user!
 */
 class rsQBlogMsgs
 {
 	public:
 	
 	std::list< std::pair<time_t, std::string> > blogs; // blogs recorded
 };
 	

 
 /*!
  * contains information that defines rsQblogs attribute space 
  */
 class rsQBlogInfo
 {
 	public:
 	
 	
 	list<std::string> filterList; /// contains the list of ids usr only wants to see
 	bool filterSwitch; /// determines whether filter is activated or not
 	std::string Status; /// the status of the user  
 	list<Profile> UsrProfiles; /// contains list to users friends profile
 	
 	std::map< std::string, rsQblogMsgs> usrBlogSet; /// contain usr and frineds blogs
 	// std::string favSong; ///usrs latest fav song
 };
 
 
 	
 	
 class rsQblogs
 {
 	public:
 	
 	
	rsQblogs() { return; }
virtual ~rsQblogs() { return; }
 	
 	/**
 	 * allow user to set his status
 	 * @param status The status of the user 
 	 */
 	virtual bool setStatus(std::string &status) = 0;
 	
 	/**
 	 * get status of users friends
 	 *
 	 **/
 	 virtual std::string* getStatus(void) = 0;
 	 
 	/**
 	 * choose whether to filter or not
 	 * @param filterSwitch
 	 **/
 	virtual bool setFilterSwitch(bool filterSwitch) = 0;
 	
 	/**
 	 * retrieve usrs filterSwitch status
 	 **/
 	virtual bool getFilterSwitch(void) = 0;
 	
 	/**
 	 * add user id to filter list 
 	 * @param usr id to add to filter list
 	 **/
 	virtual bool addToFilter(std::string &id) = 0
 	
 	/**
 	 * remove friend from filter list
 	 * @param id The user's frined's id
 	 **/
 	 virtual bool removeFiltFriend(std::string &id) = 0;
 	 
 	/**
 	  * get users profile
 	  */
 	  virtual Profile* getProfile(std::string &id) = 0;
 	  
 	  /**
 	   * set profile info
 	   */
 	   virtual bool setProfile(Profile &profile) = 0;
 	   
 	   /**
 	    * send blog info to usr blog list
 	    * @param msg The msg of the usr wants to send
 	    */
 	   virtual bool sendBlog(std::string &msg) = 0
 	   
 	   /**
 	    * retrieve blog of usr
 	    * @param usr the user to return bloginfo for
 	    * @return rsQblog the blog information of usr
 	    */
 	   virtual rsQBlog* getBlogs(std::string &usr) = 0;
 	   
 };

#endif /*RSQBLOG_H_*/
