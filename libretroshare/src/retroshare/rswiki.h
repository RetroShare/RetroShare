#ifndef RETROSHARE_WIKI_GUI_INTERFACE_H
#define RETROSHARE_WIKI_GUI_INTERFACE_H

/*
 * libretroshare/src/retroshare: rswiki.h
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2012-2012 by Robert Fernie.
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

#include <inttypes.h>
#include <string>
#include <list>

#include "gxs/rstokenservice.h"
#include "gxs/rsgxsifaceimpl.h"

/* The Main Interface Class - for information about your Peers */
class RsWiki;
extern RsWiki *rsWiki;


/* so the basic idea of Wiki is a set of Collections about subjects.
 *
 * Collection: RS
 *   - page: DHT
 *       - edit
 *           - edit
 *     - official revision. (new version of thread head).
 *
 * A collection will be moderated by it creator - important to prevent stupid changes.
 * We need a way to swap out / replace / fork collections if moderator is rubbish.
 *
 * This should probably be done that the collection level.
 * and enable all the references to be modified.
 *
 * Collection1 (RS DHT)
 *  : Turtle Link: Collection 0x54e4dafc34
 *    - Page 1
 *    - Page 2
 *       - Link to Self:Page 1
 *       - Link to Turtle:Page 1
 *
 *    
 */

#define FLAG_MSG_TYPE_WIKI_SNAPSHOT	0x0001
#define FLAG_MSG_TYPE_WIKI_COMMENT	0x0002

class CollectionRef
{
	public:

	std::string KeyWord;
        std::string CollectionId;
};


class RsWikiCollection
{
	public:

	RsGroupMetaData mMeta;

	std::string mDescription;
	std::string mCategory;

	std::string mHashTags;

        //std::map<std::string, CollectionRef> linkReferences;
};


class RsWikiSnapshot
{
	public:

	RsMsgMetaData mMeta;

	std::string mPage; // all the text is stored here.
	std::string mHashTags;
};


class RsWikiComment
{
	public:

	RsMsgMetaData mMeta;
	std::string mComment; 
};


class RsWiki: public RsGxsIfaceImpl
{
	public:

	RsWiki(RsGenExchange *gxs): RsGxsIfaceImpl(gxs)  { return; }
virtual ~RsWiki() { return; }

	/* Specific Service Data */
virtual bool getCollections(const uint32_t &token, std::vector<RsWikiCollection> &collections) = 0;
virtual bool getSnapshots(const uint32_t &token, std::vector<RsWikiSnapshot> &snapshots) = 0;
virtual bool getComments(const uint32_t &token, std::vector<RsWikiComment> &comments) = 0;

virtual bool submitCollection(uint32_t &token, RsWikiCollection &collection) = 0;
virtual bool submitSnapshot(uint32_t &token, RsWikiSnapshot &snapshot) = 0;
virtual bool submitComment(uint32_t &token, RsWikiComment &comment) = 0;


};

#endif
