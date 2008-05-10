#ifndef P3QBLOG_H_
#define P3QBLOG_H_

/*
 * libretroshare/src/rsiface: p3Qblog.h
 *
 * RetroShare Blog Service.
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

#include "rsiface/rsQblog.h"

#include "pqi/pqi.h"
#include "pqi/pqiindic.h"
#include "dbase/cachestrapper.h"
#include "services/p3service.h"
#include "util/rsthreads.h"


/*!
 * contains definitions of the interface and blog information to be manipulated
 */
 class p3Qblog : public RsQblog 
 {
 	public:
 	
 		p3Qblog();
 		virtual ~p3Qblog ();
 	
 		virtual bool setStatus(std::string &status);
    	virtual bool getStatus(std::string &status);
		virtual bool setFilterSwitch(bool &filterSwitch);
    	virtual bool getFriendList(std::list<std::string> &friendList);
		virtual bool getFilterSwitch(void);
		virtual bool addToFilter(std::string &usrId);
    	virtual bool removeFiltFriend(std::string &usrId);
    	virtual bool getProfile(std::string &usrId, std::string &favSong);	  
 		virtual bool setProfile(std::string &favSong);   
    	virtual bool sendBlog(std::string &msg);
    	virtual bool getBlogs(std::map< std::string, std::multimap<long int, std:: string> > &blogs);
 	
  	private:
  	
  	std::list<std::string> FilterList; /// contains the list of ids usr only wants to see
 	bool FilterSwitch; /// determines whether filter is activated or not
 	std::string Status; /// the status of the user  
 	std::string FavSong; /// favorite song of usr, consider sending pathfile to d/l 
 	std::list<std::string> FriendList; /// list of friends
 	std::map<std::string, std::string> FriendSongset; //
 	//std::multimap<long int, std::string> blogs; /// blogs recorded
 	std::map< std::string, std::multimap<long int, std:: string> > UsrBlogSet; /// contain usr and their blogs
 	
 	void loadDummy(void); /// loads dummy data for testing
 	
 };
  

#endif /*P3QBLOG_H_*/
