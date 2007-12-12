
/*
 * "$Id: rstypes.cc,v 1.2 2007-04-07 08:41:00 rmf24 Exp $"
 *
 * RetroShare C++ Interface.
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


/* Insides of RetroShare interface.
 * only prints stuff out at the moment
 */

#include "rsiface/rstypes.h"
#include <iostream>
#include <sstream>
#include <iomanip>


#if 0
/****************************************/
RsCertId::RsCertId()
{
	for(int i = 0; i < RSCERTIDLEN; i++)
	{
		data[i] = 0;
	}
}

/**********************************************************************
 * NOTE NOTE NOTE ...... XXX
 * BUG in MinGW .... %hhx in sscanf overwrites 32bits, instead of 8bits.
 * this means that scanf(.... &(data[15])) is running off the 
 * end of the buffer, and hitting data[15-18]...
 * To work around this bug we are reading into proper int32s
 * and then copying the data over...
 *
**********************************************************************/

RsCertId::RsCertId(std::string idstr)
{
        unsigned int a, b, c, d;
        /* scan the string and create an id */
        if (16 != sscanf(idstr.c_str(),
                "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx:%hhx:%hhx:%hhx:%hhx:%hhx:%hhx:%x:%x:%x:%x",
                &(data[0]), &(data[1]), &(data[2]), &(data[3]),
                &(data[4]), &(data[5]), &(data[6]), &(data[7]),
                &(data[8]), &(data[9]), &(data[10]), &(data[11]),
                &a, &b, &c, &d))
        {
                std::cerr << "RsCertId() Invalid Parsing!" << std::endl;
                /* clear it */
                for(int i = 0; i < RSCERTIDLEN; i++)
                {
                        data[i] = 0;
                }
        }
        ((unsigned char *) data)[12] = a;
        ((unsigned char *) data)[13] = b;
        ((unsigned char *) data)[14] = c;
        ((unsigned char *) data)[15] = d;
}

bool RsCertId::operator<(const RsCertId &ref) const
{
        //compare the signature.
        if (0 > memcmp(data, ref.data, RSCERTIDLEN))
                return true;
        return false;
}

bool RsCertId::operator==(const RsCertId &ref) const
{
        //compare the signature.
        return (0 == memcmp(data, ref.data, RSCERTIDLEN));
}

bool RsCertId::operator!=(const RsCertId &ref) const
{
        //compare the signature.
        return !(*this == ref);
}

std::ostream &operator<<(std::ostream &out, const RsCertId &id)
{
	std::ostringstream str;
	for(int i = 0; i < RSCERTIDLEN; i++)
	{
		if (i != 0)
		{
			str << ":";
		}
		str << std::hex << std::setw(2) << std::setfill('0')
		    << std::setprecision(2);
		str << (unsigned int) (((unsigned char *) (id.data))[i]);
	}
	out << str.str();
	return out;
}

#endif


/****************************************/
	/* Print Functions for Info Classes */
std::ostream &operator<<(std::ostream &out, const NeighbourInfo &info)
{
	out << "Neighbour Name: " << info.name;
	out << std::endl;
	out << "TrustLvl:  " << info.trustLvl;
	out << std::endl;
	out << "Status:    " << info.status;
	out << std::endl;
	out << "Auth Code: " << info.authCode;
	out << std::endl;
	return out;
}

std::ostream &operator<<(std::ostream &out, const PersonInfo &info)
{
	out << "Directory Listing for: " << info.name;
	out << std::endl;
	print(out, info.rootdir, 0);
	return out;
}

std::ostream &print(std::ostream &out, const DirInfo &info, int indentLvl)
{
	int i;
	std::ostringstream indents;
	for(i = 0; i < indentLvl; i++)
	{
		indents << "  ";
	}
	out << indents.str() << "Dir: " << info.dirname << std::endl;
	if (info.subdirs.size() > 0)
	{
	  out << indents.str() << "subdirs:" << std::endl;
	  std::list<DirInfo>::const_iterator it;
	  for(it = info.subdirs.begin(); it != info.subdirs.end(); it++)
	  {
	  	print(out, *it, indentLvl + 1);
	  }
	}
	if (info.files.size() > 0)
	{
	  out << indents.str() << "files:" << std::endl;
	  std::list<FileInfo>::const_iterator it2;
	  for(it2 = info.files.begin(); it2 != info.files.end(); it2++)
	  {
	        out << indents.str() << "  " << it2->fname;
		out << " " << it2->size << std::endl;
	  }
	}
	return out;
}


std::ostream &operator<<(std::ostream &out, const MessageInfo &info)
{
	out << "MessageInfo(TODO)";
	out << std::endl;
	return out;
}

std::ostream &operator<<(std::ostream &out, const ChannelInfo &info)
{
	out << "ChannelInfo(TODO)";
	out << std::endl;
	return out;
}

std::ostream &operator<<(std::ostream &out, const ChatInfo &info)
{
	out << "ChatInfo(TODO)";
	out << std::endl;
	return out;
}



int DirInfo::merge(const DirInfo &udir)
{
	/* add in the data from the udir */
	if (infoAge > udir.infoAge)
	{
		return -1;
	}


	std::list<DirInfo>::const_iterator it;
	for(it = udir.subdirs.begin(); it != udir.subdirs.end(); it++)
	{
		update(*it);
	}

	std::list<FileInfo>::const_iterator it2;
	for(it2 = udir.files.begin(); it2 != udir.files.end(); it2++)
	{
		add(*it2);
	}

	infoAge = udir.infoAge;
	nobytes = udir.nobytes;

	//nofiles = udir.nofiles;
	nofiles = subdirs.size() + files.size();
	return 1;
}



int DirInfo::update(const DirInfo &dir)
{
	/* add in the data from the udir */
	DirInfo *odir = existsPv(dir);
	if (odir)
	{
		// leave.. dirup -> subdirs = pd.data.subdirs;
		// leave.. dirup -> files = pd.data.files;
		odir->infoAge = dir.infoAge;
		odir->nofiles = dir.nofiles;
		odir->nobytes = dir.nobytes;
	}
	else
	{
		subdirs.push_back(dir);
	}
	return 1;
}

bool DirInfo::exists(const DirInfo &sdir)
{
	  return (existsPv(sdir) != NULL);
}



DirInfo *DirInfo::existsPv(const DirInfo &sdir)
{
	  std::list<DirInfo>::iterator it;
	  for(it = subdirs.begin(); it != subdirs.end(); it++)
	  {
	  	if (sdir.dirname == it->dirname)
		{
			return &(*it);
		}
	  }
	  return NULL;
}


bool DirInfo::exists(const FileInfo &file)
{
	  return (existsPv(file) != NULL);
}



FileInfo *DirInfo::existsPv(const FileInfo &file)
{
	  std::list<FileInfo>::iterator it;
	  for(it = files.begin(); it != files.end(); it++)
	  {
	  	if (file.fname == it->fname)
		{
			return &(*it);
		}
	  }
	  return NULL;
}


bool DirInfo::add(const DirInfo & sdir)
{
	DirInfo *entry = existsPv(sdir);
	if (entry)
	{
		*entry = sdir;
		return false;
	}
	else
	{
		subdirs.push_back(sdir);
		return true;
	}
}


bool DirInfo::add(const FileInfo & file)
{
	FileInfo *entry = existsPv(file);
	if (entry)
	{
		*entry = file;
		return false;
	}
	else
	{
		files.push_back(file);
		return true;
	}
}


