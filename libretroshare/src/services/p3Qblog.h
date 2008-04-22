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
#include <strings>
#include <list>
#include <map>

#include "rsiface/rsQblog.h"

#include "pqi/pqi.h"
#include "pqi/pqiindic.h"
#include "services/p3service.h"
//#include "serialiser/rsqblogitems"
#include "util/rsthreads.h"

/*!
 * blog records to be accessed only by user!
 */
 class rsQBlogMsgs
 {
 	public:
 	
 	std::multimap<long int, std::string> blogs; // blogs recorded
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
 
 

#endif /*P3QBLOG_H_*/
