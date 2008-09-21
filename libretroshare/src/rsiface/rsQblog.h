#ifndef RSQBLOG_H_
#define RSQBLOG_H_

/*
 * libretroshare/src/rsiface: rsQblog.h
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2007-2008 by Chris Evi-Parker, Robert Fernie.
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

 #include "../rsiface/rstypes.h"


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
	 	 * send blog info, will send to a data structure for transmission
	     * @param msg The msg the usr wants to send
	     */
	    virtual bool sendBlog(const std::wstring &msg) = 0;

	 	/**
	 	  * retrieve blog of a usr
	 	  * @param blogs contains the blog msgs of usr along with time posted for sorting
	 	  */
	 	virtual bool getBlogs(std::map< std::string, std::multimap<long int, std::wstring> > &blogs) = 0;

		/**
		 * Stuff DrBob Added for Profile View!
		 */

	 	/**
	 	 * get users Latest Blog Post.
	 	 * @param id the usr whose idetails you want to get.
	 	 * @param ts Timestamp of the Blog Post.
	 	 * @param post the actual Blog Post.
	 	 */
	 	virtual bool getPeerLatestBlog(std::string id, uint32_t &ts, std::wstring &post) = 0;

 };

#endif /*RSQBLOG_H_*/
