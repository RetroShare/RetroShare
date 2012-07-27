/*
 * libretroshare/src/gxp: gxp_apps.h
 *
 * General Exchange Protocol interface for RetroShare.
 *
 * Copyright 2011-2011 by Robert Fernie.
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

#ifndef RS_GXP_APPS_H
#define RS_GXP_APPS_H



/************************************************************************
 * This File describes applications that will use the GXP protocols.
 *
 *****/


/************************************************************************
 * Forums / Channels.
 *
 * The existing experiences of Forums & Channels have highlighted some
 * significant limitations of the Cache-Based exchange system. The new
 * and improved system will be based on GMXP.
 *
 * Existing Issues to deal with:
 * 1) GPG Signatures take too long to verify (GPGme/gpg.exe issue)
 * 2) Signatures are re-verified each startup.
 * 3) Forum Messages are broadcast to all peers - excessive traffic/disk space.
 * 4) Impossible to Edit Messages, or Comment on Channel Posts.
 *
 * Most of these issues (1-3) will be dealt with via GMXP GIXP system
 *
 * The data structures below are closely modelled on existing types.
 *
 *****/

class gxp::forum::msg: public gmxp::msg
{
	/**** PROVIDED BY PARENT ***/
	//gxp::id groupId;
	//gxp::id msgId;

	//gxp::id parentId; 
	//gxp::id threadId;

	//gxp::id origMsgId;
	//gxp::id replacingMsgId;

	//uint32_t timestamp;
	//uint32_t type;
	//uint32_t flags;

	//gpp::permissions msgPermissions;
	//gxp::signset signatures;

	/**** SPECIFIC FOR FORUM MSG ****/

	std::string srcId;
	std::string title;
	std::string msg;

};

class gxp::channel::msg: public gmxp::msg
{
	/**** PROVIDED BY PARENT ***/
	//gxp::id groupId;
	//gxp::id msgId;

	//gxp::id parentId; // NOT USED.
	//gxp::id threadId; // NOT USED.

	//gxp::id origMsgId;
	//gxp::id replacingMsgId;

	//uint32_t timestamp;
	//uint32_t type;
	//uint32_t flags;

	//gpp::permissions msgPermissions;
	//gxp::signset signatures;

	/**** SPECIFIC FOR CHANNEL MSG ****/

        std::wstring subject;
        std::wstring message;

        RsTlvFileSet attachment;
        RsTlvImage thumbnail;

};


/************************************************************************
 * Events.
 *
 * It is well known that Events & Photos are the two primary uses for
 * Facebook. It is imperative that these are implemented in GXP.
 *
 *****/

class gxp::events::event: public gmxp::msg
{
	/**** PROVIDED BY PARENT ***/
	//gxp::id groupId;
	//gxp::id msgId;

	//gxp::id parentId; 
	//gxp::id threadId;

	//gxp::id origMsgId;
	//gxp::id replacingMsgId;

	//uint32_t timestamp;
	//uint32_t type;
	//uint32_t flags;

	//gpp::permissions msgPermissions;
	//gxp::signset signatures;

	/**** SPECIFIC FOR EVENT MSG ****/

	location
	time
	repeats
	invite list
	number of places.
	host


};

class gxp::events::reply: public gmxp::msg
{
	/**** PROVIDED BY PARENT ***/
	//gxp::id groupId;
	//gxp::id msgId;

	//gxp::id parentId; 
	//gxp::id threadId;

	//gxp::id origMsgId;
	//gxp::id replacingMsgId;

	//uint32_t timestamp;
	//uint32_t type;
	//uint32_t flags;

	//gpp::permissions msgPermissions;
	//gxp::signset signatures;

	/**** SPECIFIC FOR REPLY MSG ****/

	bool amcoming;

};


/************************************************************************
 * Photos.
 *
 *****/

class gxp::PhotoAlbum: public gmxp::group
{
	Location
	Period
	Type
	Photographer
	Comments
};



class gxp::photos::photo: public gmxp::msg
{
	/**** PROVIDED BY PARENT ***/
	//gxp::id groupId;
	//gxp::id msgId;

	//gxp::id parentId; 
	//gxp::id threadId;

	//gxp::id origMsgId;
	//gxp::id replacingMsgId;

	//uint32_t timestamp;
	//uint32_t type;
	//uint32_t flags;

	//gpp::permissions msgPermissions;
	//gxp::signset signatures;

	/**** SPECIFIC FOR PHOTO MSG ****/

	Location
	Time
	Type
	Photographer
	Comment

	FileLink.
};


/************************************************************************
 * Wiki.
 *
 * Wiki pages are very similar to Forums... Just they are editable
 * but anyone, and will replace the earlier version.
 *
 *****/


class gxp::wiki::topic: public gmxp::group
{


}


class gxp::wiki::page: public gmxp::msg
{
	/**** PROVIDED BY PARENT ***/
	//gxp::id groupId;
	//gxp::id msgId;

	//gxp::id parentId; 
	//gxp::id threadId;

	//gxp::id origMsgId;
	//gxp::id replacingMsgId;

	//uint32_t timestamp;
	//uint32_t type;
	//uint32_t flags;

	//gpp::permissions msgPermissions;
	//gxp::signset signatures;

	/**** SPECIFIC FOR FORUM MSG ****/

	Title
	Format
	Page

	Links
	References

};


/************************************************************************
 * Links.
 *
 *****/

class gxp::Link
{




};



/************************************************************************
 * Comments.
 *
 *****/

class gxp::Comment
{




};


/************************************************************************
 * Vote.
 *
 *****/

class gxp::Vote
{




};


/************************************************************************
 * Status
 *
 *****/

class gxp::Status
{




};



/************************************************************************
 * Tasks
 *
 *****/

class gxp::Task
{




};


/************************************************************************
 * Tweet
 *
 *****/

class gxp::Tweet
{
	HashTags

	Content

	Links
};


/************************************************************************
 * Library.
 *
 *****/

class gxp::Paper
{
	KeyWords

	Journal

	Authors

	Abstract

	References

	Similar Papers
};





#endif /* RS_GXP_H */


