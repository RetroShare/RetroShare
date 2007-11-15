/*
 * "$Id: pqi.cc,v 1.8 2007-03-21 18:45:41 rmf24 Exp $"
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




#include "pqi/pqi.h"
#include "pqi/pqi_data.h"

#include <ctype.h>

// local functions.
//int pqiroute_setshift(PQItem *item, int chan);
//int pqiroute_getshift(PQItem *item);
//int	pqicid_clear(ChanId *cid);
//int	pqicid_copy(const ChanId *cid, ChanId *newcid);

// Helper functions for the PQInterface.

int fixme(char *str, int n)
{
	for(int i = 0; i < n; i++)
	{
		std::cerr << "FIXME:: " << str << std::endl;
	}
	return 1;
}

SearchItem::SearchItem()
	:PQItem(PQI_ITEM_TYPE_SEARCHITEM, PQI_SI_SUBTYPE_SEARCH), 
	datatype(PQI_SI_DATATYPE_TERMS)

{
	//std::cerr << "Creating SearchItem()" << std::endl;
	return;
}

SearchItem::SearchItem(int st)
	:PQItem(PQI_ITEM_TYPE_SEARCHITEM, st), datatype(PQI_SI_DATATYPE_TERMS)
{
	//std::cerr << "Creating SearchItem()" << std::endl;
	return;
}

SearchItem::~SearchItem()
{
	//std::cerr << "Deleting SearchItem()" << std::endl;
	return;
}


// These need to be recursive.
SearchItem *SearchItem::clone()
{
	SearchItem *nsi = new SearchItem();
	nsi -> copy(this);

	return nsi;
}

void 	SearchItem::copy(const SearchItem *si)
{
	// copy below.
	PQItem::copy(si);

	// do elements of searchItem.
	
	datatype = si -> datatype;
	data = si -> data;
}

std::ostream &SearchItem::print(std::ostream &out)
{
	out << "---- SearchItem" << std::endl;
	PQItem::print(out);

	out << "Search Type: " << datatype << std::endl;
	out << "Search: " << data << std::endl;
	return out << "----" << std::endl;
}


	ChatItem::ChatItem()
	:PQItem(PQI_ITEM_TYPE_CHATITEM, PQI_MI_SUBTYPE_CHAT)
{
	//std::cerr << "Creating ChatItem()" << std::endl;
	return;
}

	ChatItem::ChatItem(int st)
	:PQItem(PQI_ITEM_TYPE_CHATITEM, st)
{
	//std::cerr << "Creating ChatItem()" << std::endl;
	return;
}


ChatItem::~ChatItem()
{
	//std::cerr << "Deleting ChatItem()" << std::endl;
	return;
}


MsgItem::MsgItem()
	:ChatItem(PQI_MI_SUBTYPE_MSG), msgflags(0), sendTime(0)
{
	//std::cerr << "Creating MsgItem()" << std::endl;
	return;
}


MsgItem::~MsgItem()
{
	//std::cerr << "Deleting MsgItem()" << std::endl;
	return;
}


ChatItem *ChatItem::clone()
{
	ChatItem *nsi = new ChatItem();
	nsi -> copy(this);

	return nsi;
}

void 	ChatItem::copy(const ChatItem *si)
{
	// copy below.
	PQItem::copy(si);

	msg = si -> msg;

	return;
}


std::ostream &ChatItem::print(std::ostream &out)
{
	out << "---- ChatItem" << std::endl;
	PQItem::print(out);

	out << "Msg: " << msg;
	out << std::endl << "----";
	return out << std::endl;
}


MsgItem *MsgItem::clone()
{
	MsgItem *nsi = new MsgItem();
	nsi -> copy(this);

	return nsi;
}

void 	MsgItem::copy(const MsgItem *mi)
{
	// copy below.
	ChatItem::copy(mi);

	msgflags = mi -> msgflags;
	sendTime = mi -> sendTime;
	title = mi -> title;
	header = mi -> header;
	files = mi -> files;

}

std::ostream &MsgItem::print(std::ostream &out)
{
	out << "-------- MsgItem" << std::endl;
	ChatItem::print(out);

	out << "MsgFlags: " << msgflags;
	out << std::endl;
	out << "SendTime: " << sendTime;
	out << std::endl;
	out << "Title: " << title;
	out << std::endl;
	out << "Header: " << header;
	out << std::endl;
	out << "NoFiles: " << files.size();
	out << std::endl;
	std::list<MsgFileItem>::iterator it;
	for(it = files.begin(); it != files.end(); it++)
	{
		out << "File: " << it->name;
		out << " Size: " << it->size;
		out << " Hash: " << it->hash;
		out << std::endl;
	}
	out << std::endl << "--------";
	return out << std::endl;
}


//void    FileItem::fillsi(SearchItem *si)
//{
//	// first copy lowlevel stuff.
//	si -> PQItem::copyIDs(this);
//	if (hash.length() > 5)
//	{
//		// use hash.
//		si -> data = hash;
//		si -> datatype = PQI_SI_DATATYPE_HASH;
//	}
//	else
//	{
//		si -> data = name;
//		si -> datatype = PQI_SI_DATATYPE_NAME;
//	}
//	return;
//}


// This uses some helper functions for 
// string matching... these 
// functions are located at the end 
// of this file.

int     PQFileItem::match(std::string term)
{
	std::string lowerterm = strtolower(term);

	std::string haystack = strtolower(path);
	if (strstr(haystack.c_str(), lowerterm.c_str()) != NULL)
	{
		return 1;
	}

	haystack = strtolower(name);
	if (strstr(haystack.c_str(), lowerterm.c_str()) != NULL)
	{
		return 1;
	}
	return 0;
}




MediaItem::MediaItem()
{
	//std::cerr << "Creating MediaItem()" << std::endl;
	return;
}

MediaItem::~MediaItem()
{
	//std::cerr << "Deleting MediaItem()" << std::endl;
	return;
}

MediaItem *MediaItem::clone()
{
	MediaItem *nmi = new MediaItem();
	nmi -> copy(this);
	return nmi;
}


void MediaItem::copy(const MediaItem *src)
{
	// copy fileitem stuff.
	PQFileItem::copy(src);

	mimetype = src -> mimetype;
	title = src -> title;
	author = src -> author;
	collection = src -> collection;
	genre = src -> genre;
	len = src -> len;

	return;
}

int     MediaItem::match(std::string term)
{
	std::string lowerterm = strtolower(term);

	std::string haystack = strtolower(title);
	if (strstr(haystack.c_str(), lowerterm.c_str()) != NULL)
	{
		return 1;
	}

	haystack = strtolower(author);
	if (strstr(haystack.c_str(), lowerterm.c_str()) != NULL)
	{
		return 1;
	}

	haystack = strtolower(collection);
	if (strstr(haystack.c_str(), lowerterm.c_str()) != NULL)
	{
		return 1;
	}
	haystack = strtolower(genre);
	if (strstr(haystack.c_str(), lowerterm.c_str()) != NULL)
	{
		return 1;
	}

	return PQFileItem::match(term);
}


/*********************** STRING HELPERS *************************/

std::string strtolower(std::string in)
{
	std::string out = in;
	for(int i = 0; i < (signed) out.length(); i++)
	{
		if (isupper(out[i]))
		{
			out[i] +=  'a' - 'A';
		}
	}
	return out;
}

std::string fileextension(std::string in)
{
	std::string out;
	bool found = false;
	for(int i = (signed) in.length() -1; (i >= 0) && (found == false); i--)
	{
		if (in[i] == '.')
		{
			found = true;
			for(int j = i+1; j < (signed) in.length(); j++)
			{
				out += in[j];
			}
		}
	}
	return strtolower(out);
}


std::list<std::string> createtermlist(std::string in)
{
	std::list<std::string> terms;
	char str[1024];
	strncpy(str, in.c_str(), 1024);
	str[1023] = '\0';

	int len = strlen(str);
	int i;

        int sword = 0;
	for(i = 0; (i < len - 1) && (str[i] != '\0'); i++)
        {
		if (isspace(str[i]))
		{
			str[i] = '\0';
			if (i - sword > 0)
			{
			terms.push_back(std::string(&(str[sword])));
			//std::cerr << "Found Search Term: (" << &(str[sword]);
			//std::cerr << ")" << std::endl;
			}
			sword = i+1;
		}
	}
	// do the final term.
	if (i - sword > 0)
	{
		terms.push_back(std::string(&(str[sword])));
		//std::cerr << "Found Search Term: (" << &(str[sword]);
		//std::cerr << ")" << std::endl;
	}
	return terms;
}







int match(PQFileItem *fi, SearchItem *si)
{
	if (si -> datatype == PQI_SI_DATATYPE_HASH)
	{
		if (0 == strcmp((fi -> hash).c_str(), (si -> data).c_str()))
			return 1;
		return 0;
	}
	else if (si -> datatype == PQI_SI_DATATYPE_NAME)
	{
		if (0 == strcmp((fi -> name).c_str(), (si -> data).c_str()))
			return 1;
		return 0;
	}
	else // try it as terms....
	{
		std::list<std::string> terms = createtermlist(si -> data);
		std::list<std::string>::iterator it;
		if (terms.size() == 0)
		{
			return 0;
		}

		for(it = terms.begin(); it != terms.end(); it++)
		{
			if (0 == fi -> match(*it))
			{
				return 0;
			}
		}
		return 1;
	}
}

