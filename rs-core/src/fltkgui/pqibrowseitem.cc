/*
 * "$Id: pqibrowseitem.cc,v 1.11 2007-02-18 21:46:49 rmf24 Exp $"
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



#include "fltkgui/pqibrowseitem.h"

#include "pqi/pqichannel.h" /* for mode bits */


// for a helper fn.
#include "fltkgui/pqistrings.h"
#include <sstream>


	// a couple of functions that do the work.
int FileDisItem::ndix() // Number of Indices.
{
	return 4;
}

static const int FI_COL_SOURCE_NAME = 0;
static const int FI_COL_SEARCH_TERMS = 1;
static const int FI_COL_NAME = 2;
static const int FI_COL_SIZE = 3;

std::string FileDisItem::txt(int col)
{
	char numstr[100]; // this will be used regularly.... 
	std::string out;
	switch(col)
	{
		case FI_COL_SOURCE_NAME:
			if ((item -> p) != NULL)
			{
				out = item -> p -> Name();
	//std::cerr << "FDI::txt() Returning sn: " << out << std::endl;
				return out;
			}
			return std::string("Local");
	//std::cerr << "FDI::txt() person = NULL" << std::endl;
			break;
		case FI_COL_SEARCH_TERMS:
			if (terms != NULL)
			{
				out = terms -> data;
	//std::cerr << "FDI::txt() Returning terms: " << out << std::endl;
				return out;
			}
	//std::cerr << "FDI::txt() terms = NULL" << std::endl;
			break;
		case FI_COL_NAME:
			out = item -> name;
	//std::cerr << "FDI::txt() Returning name: " << out << std::endl;
			return out;
			break;
		case FI_COL_SIZE:
			sprintf(numstr, "%d kB", item -> size / 1000);
			out = numstr;
	//std::cerr << "FDI::txt() Returning size: " << out << std::endl;
			return out;
			break;
		default:
			break;
	}
	out = "";
	return out;
}

int FileDisItem::cmp(int col, DisplayData *fdi)
{
	FileDisItem *other = (FileDisItem *) fdi;
	int ret = 0;
	switch(col)
	{
		case FI_COL_SOURCE_NAME:
	//	std::cerr << "FileDisItem::cmp() SRC NAME" << std::endl;
			if (item -> p == 
					other -> item -> p)
			{
				ret = 0;
				break;
			}
			if (item -> p == NULL)
			{
				ret = -1;
				break;
			}
			if (other -> item -> p == NULL)
			{
				ret = 1;
				break;
			}
			ret = strcmp((item -> p -> Name()).c_str(),
			    (other -> item -> p -> Name()).c_str());
			break;
		case FI_COL_SEARCH_TERMS:
	//	std::cerr << "FileDisItem::cmp() TERMS" << std::endl;
			if (terms == other -> terms)
			{
				ret = 0;
				break;
			}
			if (terms == NULL)
			{
				ret = -1;
				break;
			}
			if (other -> terms == NULL)
			{
				ret = 1;
				break;
			}
			ret = strcmp((terms -> data).c_str(),
			    (other -> terms -> data).c_str());
			break;
		case FI_COL_NAME:
	//	std::cerr << "FileDisItem::cmp() NAME" << std::endl;
			ret = strcmp((item -> name).c_str(),
			    (other -> item -> name).c_str());
			break;
		case FI_COL_SIZE:
	//	std::cerr << "FileDisItem::cmp() SIZE" << std::endl;
			if (item -> size == other -> item -> size)
			{
				ret =  0;
				break;
			}
			if (item -> size > other -> item -> size)
			{
				ret = 1;
				break;
			}
			ret = -1;
			break;
		default:
	//	std::cerr << "FileDisItem::cmp() Default" << std::endl;
			ret = 0;
			break;
	}
	//std::cerr << "FileDisItem::cmp() ret: " << ret << std::endl;
	return ret;
}



	// a couple of functions that do the work.
int PersonDisItem::ndix() // Number of Indices.
{
	return 5;
}

static const int PI_COL_NAME = 0;
static const int PI_COL_STATUS = 1;
static const int PI_COL_AUTOCONNECT = 2;
static const int PI_COL_TRUST = 3;
static const int PI_COL_SERVER = 4;

std::string PersonDisItem::txt(int col)
{
	char numstr[100]; // this will be used regularly.... 
	std::string out;
	if (item == NULL)
		return out;

	switch(col)
	{
		case PI_COL_NAME:
			out = item -> Name();
			break;
		case PI_COL_STATUS:
			out = get_status_string(item -> Status());
			break;
		case PI_COL_AUTOCONNECT:
			out = get_autoconnect_string(item);
			break;
		case PI_COL_TRUST:
			out = get_trust_string(item);
			break;
		case PI_COL_SERVER:
			out = get_server_string(item);
			break;
		default:
			break;
	}
	return out;
}

int PersonDisItem::cmp(int col, DisplayData *fdi)
{
	PersonDisItem *other = (PersonDisItem *) fdi;
	switch(col)
	{
		case PI_COL_NAME:
			return strcmp((item -> Name()).c_str(),
			    (other -> item -> Name()).c_str());
			break;
		case PI_COL_STATUS:
			if (item -> Connected() == 
					other -> item -> Connected())
			{
				if (item -> Accepted() == 
					other -> item -> Accepted())
				{
					return 0;
				}
				if (item -> Accepted())
					return -1;
				return 1;
			}
			if (item -> Connected())
				return -1;
			return 1;
			break;

		case PI_COL_AUTOCONNECT:
			return 0;
			break;
		case PI_COL_TRUST:
			return other->item->trustLvl - item->trustLvl;
			break;
		case PI_COL_SERVER:
			return 0;
			break;
		default:
			return 0;
			break;
	}
	return 0;
}


	// a couple of functions that do the work.
int NeighDisItem::ndix() // Number of Indices.
{
	return 3;
}

static const int NI_COL_STATUS = 0;
static const int NI_COL_TRUST = 1;
static const int NI_COL_CONNECT = 2;
static const int NI_COL_NAME = 3;

std::string NeighDisItem::txt(int col)
{
	char numstr[100]; // this will be used regularly.... 
	std::string out;
	if (item == NULL)
		return out;

	switch(col)
	{
		
		case NI_COL_STATUS:
		        if (item->Accepted())
			{
				out = "Accept";
			}
			else
			{
				out = "Deny";
			}
			break;
		case NI_COL_TRUST:
			out = get_trust_string(item);
			break;
		case NI_COL_CONNECT:
			out = get_lastconnecttime_string(item);
			break;
		case NI_COL_NAME:
			out = item -> Name();
			break;
		default:
			break;
	}
	return out;
}

int NeighDisItem::cmp(int col, DisplayData *fdi)
{
	NeighDisItem *other = (NeighDisItem *) fdi;
	switch(col)
	{
		case NI_COL_STATUS:
			if (item -> Accepted() == 
				other -> item -> Accepted())
			{
				return 0;
			}
			if (item -> Accepted())
				return 1;
			return -1;
			break;

		case NI_COL_TRUST:
			return other->item->trustLvl - item->trustLvl;
			break;
		case NI_COL_CONNECT:
			return get_lastconnecttime(item)
				- get_lastconnecttime(other->item);
			break;
		case NI_COL_NAME:
			return strcmp((item -> Name()).c_str(),
			    (other -> item -> Name()).c_str());
			break;
		default:
			return 0;
			break;
	}
	return 0;
}



	// a couple of functions that do the work.
int MsgDisItem::ndix() // Number of Indices.
{
	return 5;
}

static const int MI_COL_NAME = 0;
static const int MI_COL_MSG = 1;
static const int MI_COL_DATE = 2;
static const int MI_COL_REC_NAME = 3;
static const int MI_COL_REC_SIZE = 4;

std::string MsgDisItem::txt(int col)
{
	char numstr[100]; // this will be used regularly.... 
	std::string out;
	switch(col)
	{
		case MI_COL_NAME:
			if ((item -> p) != NULL)
			{
				out = item -> p -> Name();
				return out;
			}
			return std::string("Local");
			break;
		case MI_COL_MSG:
			out = "";
			// remove \n and \t...
			for(unsigned int i =0; i < item -> msg.length(); i++)
			{
				if ((item -> msg[i] == '\n') ||
					(item -> msg[i] == '\t'))
				{
					out += " ";
				}
				else
				{
					out += item -> msg[i];
				}
			}
			return out;
			break;
		case MI_COL_DATE:
			out = timeFormat(item->epoch, TIME_FORMAT_OLDVAGUE);
			//out = "Today!";
			return out;
			break;
		case MI_COL_REC_NAME:
			out = item -> recommendname;
			return out;
			break;
		case MI_COL_REC_SIZE:
			//sprintf(numstr, "%d kB", item -> recommendsize / 1000);
			//out = numstr;
			out = "N/A";
			return out;
			break;
		default:
			break;
	}
	out = "";
	return out;
}

int MsgDisItem::cmp(int col, DisplayData *fdi)
{
	MsgDisItem *other = (MsgDisItem *) fdi;
	switch(col)
	{
		case MI_COL_NAME:
			if (item -> p == 
					other -> item -> p)
			{
				return 0;
			}
			if (item -> p == NULL)
			{
				return -1;
			}
			if (other -> item -> p == NULL)
			{
				return 1;
			}
			return strcmp((item -> p -> Name()).c_str(),
			    (other -> item -> p -> Name()).c_str());
			break;

		case MI_COL_MSG:
			return strcmp((item -> msg).c_str(),
			    (other -> item -> msg).c_str());
			break;
		case MI_COL_DATE:
			return 0;
			break;
		case MI_COL_REC_NAME:
			return strcmp((item -> recommendname).c_str(),
			    (other -> item -> recommendname).c_str());
			break;
		case MI_COL_REC_SIZE:
			return 0;
//			if (item -> recommend_size == 
//					other -> item -> recommend_size)
//			{
//				return 0;
//			}
//			if (item -> recommend_size > 
//					other -> item -> recommend_size)
//			{
//				return 1;
//			}
//			return -1;
			break;
		default:
			return 0;
			break;
	}
	return 0;
}


	// a couple of functions that do the work.
int FTDisItem::ndix() // Number of Indices.
{
	return 5;
}

static const int FT_COL_SOURCE_NAME = 0;
static const int FT_COL_DIRECTION = 1;
static const int FT_COL_FILE_NAME = 2;
static const int FT_COL_FILE_SIZE = 3;
static const int FT_COL_FILE_RATE = 4;


static const int SEC_PER_DAY =  24 * 3600;

std::string FTDisItem::txt(int col)
{
	char numstr[100]; // this will be used regularly.... 
	std::string out;
	switch(col)
	{
		case FT_COL_SOURCE_NAME:
			if ((item -> p) != NULL)
			{
				out = item -> p -> Name();
				return out;
			}
			return std::string("Unknown");
			break;

		case FT_COL_DIRECTION: /* This has now become state.... */
			switch(item -> state)
			{
				case FT_STATE_OKAY:
					if (item->crate > 0)
					{
						out += "Downloading";
					}
					else
					{
						out += "Waiting";
					}
					break;
				case FT_STATE_FAILED:
					out += "Failed";
					break;
				case FT_STATE_COMPLETE:
					out += "Complete";
					break;
				default:
					out += "UNKNOWN";
					break;
			}
			return out;
			break;

		case FT_COL_FILE_NAME:
			out = item -> name;
			return out;
			break;

		case FT_COL_FILE_SIZE:
			{

			float trans = item -> transferred;
			float size = item -> size;
			float percent = 100 * trans / size;
			char lets[] = "kMGT";
			char let = ' ';

			// if bigger than 50 kbytes -> display 
			for(int i = 0; (i < 4) && (size / 1024 > 1); i++)
			{
				size /= 1024;
				trans /= 1024;
				let = lets[i];
			}
			sprintf(numstr, "%.2f/%.2f %cBytes (%.2f%%)", 
					trans, size, let, percent);

			out = numstr;
			return out;
			}
			break;

		case FT_COL_FILE_RATE:
		        if (item->size == item->transferred)
			{
			   sprintf(numstr, "Done");
			}
			else if (item->crate > 0.01) /* 10 bytes / sec */
			{
			   float secs = item->size - item->transferred;
			   secs /= (item -> crate * 1000.0);


			   if (secs > SEC_PER_DAY)
			   {
			   	sprintf(numstr, "%.2f kB/s, %d days to go.", item -> crate, 
					(int) (secs + SEC_PER_DAY / 2) / SEC_PER_DAY);
			   }
			   else 
			   {
			   	int hours = (int) secs / 3600;
				secs -= 3600 * hours;
				int min = (int) secs / 60;
				secs -= 60 * min;
			   	sprintf(numstr, "%.2f kB/s, %d:%02d:%02d to go.", item -> crate, 
					hours, min, (int) secs);
			   }
			}
			else  
			{
			   sprintf(numstr, "%.2f kB/s, ... Forever", item -> crate);

			}
			out = numstr;
			return out;
			break;
		default:
			break;
	}
	out = "";
	return out;
}

int FTDisItem::cmp(int col, DisplayData *fdi)
{
	FTDisItem *other = (FTDisItem *) fdi;
	switch(col)
	{
		case FT_COL_SOURCE_NAME:
			if (item -> p == 
					other -> item -> p)
			{
				return 0;
			}
			if (item -> p == NULL)
			{
				return -1;
			}
			if (other -> item -> p == NULL)
			{
				return 1;
			}
			return strcmp((item -> p -> Name()).c_str(),
			    (other -> item -> p -> Name()).c_str());
			break;
		case FT_COL_DIRECTION:
			if (item -> in == other -> item -> in)
			{
				return 0;
			}
			if (item -> in == true)
			{
				return 1;
			}
			return -1;
			break;

		case FT_COL_FILE_NAME:
			return strcmp((item -> name).c_str(),
			    (other -> item -> name).c_str());
			break;
		case FT_COL_FILE_SIZE:
			if (item -> size == other -> item -> size)
			{
				return 0;
			}
			if (item -> size > other -> item -> size)
			{
				return 1;
			}
			return -1;
			break;

		case FT_COL_FILE_RATE:
			if (item -> crate == other -> item -> crate)
			{
				return 0;
			}
			if (item -> crate > other -> item -> crate)
			{
				return 1;
			}
			return -1;
			break;
		default:
			return 0;
			break;
	}
	return 0;
}


	// a couple of functions that do the work.
int ChanDisItem::ndix() // Number of Indices.
{
	return 5;
}


static const int CDI_COL_MODE = 0;
static const int CDI_COL_RANK = 1;
static const int CDI_COL_TITLE = 2;
static const int CDI_COL_COUNT = 3;
static const int CDI_COL_SIGN = 4;

ChanDisItem::ChanDisItem(pqichannel *i)
	:mode(0), name("NULL CHANNEL")
{
	if (!i)
	{
		mode = -1;
		return;
	}

        cs = i->getSign();
        mode = i->getMode();
        name = i->getName();
        ranking = i->getRanking();
        msgcount = i->getMsgCount();
        return;
}


std::string ChanDisItem::txt(int col)
{
	char numstr[100]; // this will be used regularly.... 
	std::ostringstream out;
	switch(col)
	{
		case CDI_COL_MODE:
			{
				if (mode == -1)
				{
					out << "Invalid";
				}
				else if (mode & 0x040)
				{
					out << "Publisher";
				}
				else if (mode & 0x020)
				{
					out << "Subscriber";
				}
				else if (mode & 0x010)
				{
					out << "Listener";
				}
				else 
				{
					out << "Other";
				}

				return out.str();
			}
			break;

		case CDI_COL_RANK:
			{
				out << ranking;
				return out.str();
			}
			break;

		case CDI_COL_TITLE:
			{
				return name;
			}
			break;


		case CDI_COL_COUNT:
			{
				out << msgcount;
				return out.str();
			}
			break;

		case CDI_COL_SIGN:
			{
				cs.printHex(out);
				return out.str();
			}
			return std::string("<BAD CHAN>");
			break;


		default:
			break;
	}
	return out.str();
}


int ChanDisItem::cmp(int col, DisplayData *fdi)
{
	ChanDisItem *other = (ChanDisItem *) fdi;
	switch(col)
	{
		case CDI_COL_MODE:
		{
			return mode - other->mode;
		}
		case CDI_COL_RANK:
		{
			return (int) (100.0 * (ranking - other->ranking));
		}
		case CDI_COL_TITLE:
		{
			return strcmp(name.c_str(), other->name.c_str());
		}
		case CDI_COL_COUNT:
		{
			return msgcount-other->msgcount;
		}
		break;
		case CDI_COL_SIGN:
		{
			if (cs == other->cs)
				return 0;
			if (cs < other->cs)
				return -1;
			return 1;
		}
		break;

		default:
		break;
	}
	return 0;
}


	// a couple of functions that do the work.
int ChanMsgDisItem::ndix() // Number of Indices.
{
	return 5;
}

static const int CMDI_COL_DATE = 0;
static const int CMDI_COL_MSG = 1;
static const int CMDI_COL_NOFILES = 2;
static const int CMDI_COL_SIZE = 3;
static const int CMDI_COL_MSGHASH = 4;


ChanMsgDisItem::ChanMsgDisItem(chanMsgSummary &s)
        :msg(s.msg), mh(s.mh), nofiles(s.nofiles),
        totalsize(s.totalsize), recvd(s.recvd)
{ 
	/* change \t & \n into " " */
	for(unsigned int i =0; i < msg.length(); i++)
	{
		if ((msg[i] == '\n') ||
			(msg[i] == '\t'))
		{
			msg[i] = ' ';
		}
	}
}


std::string ChanMsgDisItem::txt(int col)
{
	char numstr[100]; // this will be used regularly.... 
	std::ostringstream out;
	switch(col)
	{
		case CMDI_COL_DATE:
			{
				int t = time(NULL);
				int ago_sec = t - recvd;
				out << ago_sec << " secs";
				return out.str();
			}
			break;
		case CMDI_COL_MSG:
			// remove \n and \t... (done at init)
			return msg;
			break;
		case CMDI_COL_NOFILES:
			{
				out << nofiles;
				return out.str();
			}
			break;
		case CMDI_COL_SIZE:
			{
				if (totalsize > 1000000)
				{
					out << (totalsize / 1000000.0) << " MB";
				}
				else if (totalsize > 1000)
				{
					out << (totalsize / 1000.0) << " kB";
				}
				else
				{
					out << totalsize << " B";
				}

				return out.str();
			}
			break;

		case CMDI_COL_MSGHASH:
			{
				mh.printHex(out);
				return out.str();
			}
			break;

		default:
			break;
	}
	return out.str();
}

int ChanMsgDisItem::cmp(int col, DisplayData *fdi)
{
	ChanMsgDisItem *other = (ChanMsgDisItem *) fdi;
	switch(col)
	{
		case CMDI_COL_DATE:
		{
			return recvd - other->recvd;
		}
		break;

		case CMDI_COL_MSG:
		{
			return strcmp(msg.c_str(),
				other->msg.c_str());
		}
		break;

		case CMDI_COL_NOFILES:
		{
			return nofiles-other->nofiles;
		}
		break;

		case CMDI_COL_SIZE:
		{
			return totalsize-other->totalsize;
		}
		break;

		case CMDI_COL_MSGHASH:
		{
			if (mh == other->mh)
				return 0;
			if (mh < other->mh)
				return -1;
			return 1;
		}
		break;

		default:
		break;
	}
	return 0;
}


static const int CFDI_COL_SIZE = 0;
static const int CFDI_COL_NAME = 1;

	// a couple of functions that do the work.
int ChanFileDisItem::ndix() // Number of Indices.
{
	return 2;
}


ChanFileDisItem::ChanFileDisItem(std::string n, int s)
	:name(n), size(s) 
{ 
	return; 
}

std::string ChanFileDisItem::txt(int col)
{
	char numstr[100]; // this will be used regularly.... 
	std::ostringstream out;
	switch(col)
	{
		case CFDI_COL_SIZE:
			{
				if (size > 1000000)
				{
					out << (size / 1000000.0) << " MB";
				}
				else if (size > 1000)
				{
					out << (size / 1000.0) << " kB";
				}
				else
				{
					out << size << " B";
				}
				return out.str();
			}
			break;
		case CFDI_COL_NAME:
			return name;
			break;
		default:
			break;
	}
	return out.str();
}

int ChanFileDisItem::cmp(int col, DisplayData *fdi)
{
	ChanFileDisItem *other = (ChanFileDisItem *) fdi;
	switch(col)
	{
		case CFDI_COL_SIZE:
		{
			return size - other->size;
		}
		break;

		case CFDI_COL_NAME:
		{
			return strcmp(name.c_str(),
				other->name.c_str());
		}
		break;

		default:
		break;
	}
	return 0;
}

