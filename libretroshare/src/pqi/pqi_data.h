/*
 * "$Id: pqi_data.h,v 1.8 2007-04-07 08:40:55 rmf24 Exp $"
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2004-2006 by Robert Fernie.
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



#ifndef PQI_ITEM_HEADER
#define PQI_ITEM_HEADER

#include "pqi/pqi_base.h"

#include <list>
#include <string>
#include <iostream>

// Definitions of some of the commonly used classes.
// These will all be made streamable! via Boost C++

class SearchItem: public PQItem
{
public:
	SearchItem();
	SearchItem(int st);
virtual ~SearchItem();
virtual SearchItem *clone();

void 	copy(const SearchItem *si);
virtual std::ostream &print(std::ostream &out);

	// data
	int datatype;
	std::string data;

	int siflags;
};


int match(PQFileItem *fi, SearchItem *si);

class MediaItem: public PQFileItem
{
public:
	MediaItem();
virtual	~MediaItem();
virtual MediaItem *clone();

void 	copy(const MediaItem *src);
int	match(std::string term);

	// data.
std::string mimetype;

std::string title;
std::string author;
std::string collection;
std::string genre;

int 	len;

};

class ChatItem: public PQItem
{
public:
	ChatItem();
	ChatItem(int st);
virtual ~ChatItem();
virtual ChatItem *clone();
void 	copy(const ChatItem *si);
virtual std::ostream &print(std::ostream &out);

	// data
std::string msg;
	// Timestamp (Not transmitted currently!).
	// Stamped on arrival.
	int epoch;
};


class MsgFileItem
{
	public:
	std::string name; /* and dir! */
	std::string hash;
	unsigned long size;
};


class MsgItem: public ChatItem
{
public:
	MsgItem();
virtual ~MsgItem();

virtual MsgItem *clone();
void 	copy(const MsgItem *mi);
std::ostream &print(std::ostream &out);

	// Note this breaks the old client....
// Old data.....
//int	mode;
//std::string recommendname;
//std::string recommendhash;
//std::string channel;
//int recommendsize;
//int	datalen;
//void *data;

	// Flags
	unsigned long msgflags;
	unsigned long sendTime;

	std::string title;
	std::string header;

	std::list<MsgFileItem> files;
};


std::string strtolower(std::string in);
std::string fileextension(std::string in);
std::list<std::string> createtermlist(std::string in);


// IDS

const int PQI_SI_SUBTYPE_SEARCH = 1;
const int PQI_SI_SUBTYPE_CANCEL = 2;
const int PQI_SI_SUBTYPE_FILEREQ = 3;
const int PQI_SI_SUBTYPE_FILECANCEL = 4;

const int PQI_SI_DATATYPE_TERMS = 1;
const int PQI_SI_DATATYPE_HASH = 2;
const int PQI_SI_DATATYPE_NAME = 3;
const int PQI_SI_DATATYPE_DIR = 4;

const int PQI_MI_SUBTYPE_CHAT = 1;
const int PQI_MI_SUBTYPE_MSG = 2;


/* not really used here, but at higher layers */
const unsigned long PQI_MI_FLAGS_OUTGOING = 0x0001;
const unsigned long PQI_MI_FLAGS_PENDING  = 0x0002;
const unsigned long PQI_MI_FLAGS_DRAFT    = 0x0004;
const unsigned long PQI_MI_FLAGS_NEW      = 0x0010;

#endif // PQI_ITEM_HEADER

