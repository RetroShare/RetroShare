


#ifndef P3BLOG_H_
#define P3BLOG_H_

/*
 * libretroshare/src/rsserver: p3blog.h
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

#include "rsiface/rsQblog.h"
#include "services/p3Qblog.h"

/*!
 * Interface class using composition (p3Qblog is an attribute)
 *  See derived class for documentation of derived functions
 */
class p3Blog : public RsQblog
{
	public:
	
		p3Blog(p3Qblog* qblog);
		virtual ~p3Blog();
	
		virtual bool setStatus(const std::string &status);
		virtual bool getStatus(std::map<std::string, std::string> &usrStatus);
		virtual bool setFilterSwitch(bool &filterSwitch);
		virtual bool getFilterSwitch(void);
		virtual bool addToFilter(std::string &usrId);
		virtual bool removeFiltFriend(std::string &usrId);
		virtual bool getProfile(std::map<std::string, std::string> &profile);
		virtual bool setProfile(const std::string &favSong);
		virtual bool sendBlog(const std::string &msg);
		virtual bool getBlogs(std::map< std::string, std::multimap<long int, std:: string> > &blogs);
		
		virtual bool getPeerLatestBlog(std::string id, uint32_t &ts, std::wstring &post);
		virtual bool getPeerProfile(std::string id, std::list< std::pair<std::wstring, std::wstring> > &entries);
		virtual bool getPeerFavourites(std::string id, std::list<FileInfo> &favs);
		
	private:
		
		/// to make rsCore blog-service calls
		p3Qblog* mQblog;		
};


#endif /*P3BLOG_H_*/
