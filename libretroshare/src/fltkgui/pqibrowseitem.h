/*
 * "$Id: pqibrowseitem.h,v 1.7 2007-02-18 21:46:49 rmf24 Exp $"
 *
 * FltkGUI for RetroShare.
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



#ifndef MRKS_PQI_BROWSER_ITEMS
#define MRKS_PQI_BROWSER_ITEMS

/* My new funky browser.....
 *
 * - Designed to sort/display a tree brower 
 *   for search results....
 *
 *   First we need the basic interface class that
 *   must wrap the Data....
 */

#include "fltkgui/Fl_Funky_Browser.h"
#include "pqi/pqi.h"
#include "pqi/p3channel.h"

class FileDisItem: public DisplayData
{
	public:
	FileDisItem(PQFileItem *i, SearchItem *s) :item(i), terms(s) { return; }

	// a couple of functions that do the work.
virtual	int ndix();  // Number of Indices.
virtual std::string txt(int col);
virtual int cmp(int col, DisplayData *);

	PQFileItem *getItem() { return item;}
	void setItem(PQFileItem *i) {item = i;}
	SearchItem *getSearchItem() { return terms;}
	void setSearchItem(SearchItem *s) {terms = s;}

	private:
	PQFileItem *item;
	SearchItem *terms;
};


class PersonDisItem: public DisplayData
{
	public:
	PersonDisItem(Person *i) :item(i), checked(false) { return; }

	// a couple of functions that do the work.
virtual	int ndix();  // Number of Indices.
virtual std::string txt(int col);
virtual int cmp(int col, DisplayData *);
virtual int check(int n, int v = -1)
	{
		if (v == -1)
			return (int) checked;
		if (v == 0)
			return checked = false;
		return checked = true;
	};

	Person *getItem() { return item;}
	void setItem(Person *i) {item = i;}

	private:
	Person *item;
	bool checked;
};


class NeighDisItem: public DisplayData
{
	public:
	NeighDisItem(Person *i) :item(i) { return; }

	// a couple of functions that do the work.
virtual	int ndix();  // Number of Indices.
virtual std::string txt(int col);
virtual int cmp(int col, DisplayData *);

	Person *getItem() { return item;}
	void setItem(Person *i) {item = i;}

	private:
	Person *item;
};


class MsgDisItem: public DisplayData
{
	public:
	MsgDisItem(MsgItem *i) :item(i) { return; }

	// a couple of functions that do the work.
virtual	int ndix();  // Number of Indices.
virtual std::string txt(int col);
virtual int cmp(int col, DisplayData *);

	MsgItem *getItem() { return item;}
	void setItem(MsgItem *i) {item = i;}

	private:
	MsgItem *item;
};


class FTDisItem: public DisplayData
{
	public:
	FTDisItem(FileTransferItem *i) :item(i) { return; }

	// a couple of functions that do the work.
virtual	int ndix();  // Number of Indices.
virtual std::string txt(int col);
virtual int cmp(int col, DisplayData *);

	FileTransferItem *getItem() { return item;}
	void setItem(FileTransferItem *i) {item = i;}

	private:
	FileTransferItem *item;
};

/* NOTE The Channel classes should be changed to just
 * carry the channelSign around.... this is enough
 * to refer to the channels....
 *
 * don't want pointers to pqichannels....
 */

class ChanDisItem: public DisplayData
{
	public:
	ChanDisItem(pqichannel *i);

	// a couple of functions that do the work.
virtual	int ndix();  // Number of Indices.
virtual std::string txt(int col);
virtual int cmp(int col, DisplayData *);
virtual int check(int n, int v = -1)
	{
		if (v == -1)
			return (int) checked;
		if (v == 0)
			return checked = false;
		return checked = true;
	};

	public:

	channelSign cs;
	int mode;
	std::string name;
	float	ranking;
	int 	msgcount;

	bool checked; // subscribed..
};


class ChanMsgDisItem: public DisplayData
{
	public:
	ChanMsgDisItem(chanMsgSummary &s);

	// a couple of functions that do the work.
virtual	int ndix();  // Number of Indices.
virtual std::string txt(int col);
virtual int cmp(int col, DisplayData *);

	public:

	std::string msg;
	MsgHash		mh;
	int		nofiles;
	int		totalsize;
	long		recvd;

};

class ChanFileDisItem: public DisplayData
{
	public:
	ChanFileDisItem(std::string n, int s);

	// a couple of functions that do the work.
virtual	int ndix();  // Number of Indices.
virtual std::string txt(int col);
virtual int cmp(int col, DisplayData *);

	//private:
	std::string name;
	int size;
};


#endif

