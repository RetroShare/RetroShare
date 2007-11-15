/*
 * "$Id: pqichannel.cc,v 1.4 2007-02-18 21:46:49 rmf24 Exp $"
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




#include "pqi/pqichannel.h"

static const int pqicizone = 449;
#include "pqi/pqidebug.h"
#include <sstream>

const int PQChan_MsgType = 0x5601;
const int PQChan_NameType = 0x5602;
const int PQChan_HashType = 0x5604;
const int PQChan_TitleType = 0x5608;
const int PQChan_FileType = 0x5610;
const int PQChan_SizeType = 0x5620;
const int PQChan_ListType = 0x5640;


PQTunnel *createChannelItems(void *d, int n)
{
	pqioutput(PQL_DEBUG_BASIC, pqicizone, 
	  "createChannelItems()");

	return new PQChanItem();
}

PQChanItem::PQChanItem()
	:PQTunnel(PQI_TUNNEL_CHANNEL_ITEM_TYPE), 
	certDER(NULL), certType(0), certLen(0), msg(""), 
	signature(NULL), signType(0), signLen(0) 
{ 
	pqioutput(PQL_DEBUG_BASIC, pqicizone, 
	  "PQChanItem::PQChanItem()");

	return; 
}


PQChanItem::~PQChanItem()
{ 
	pqioutput(PQL_DEBUG_BASIC, pqicizone, 
	  "PQChanItem::~PQChanItem()");

	clear();

	return; 
}

PQChanItem *PQChanItem::clone()
{
	pqioutput(PQL_DEBUG_BASIC, pqicizone, 
	  "PQChanItem::clone()");

	PQChanItem *di = new PQChanItem();
	di -> copy(this);
	return di;
}

void 	PQChanItem::copy(const PQChanItem *di)
{
	pqioutput(PQL_DEBUG_BASIC, pqicizone, 
	  "PQChanItem::copy()");

	PQTunnel::copy(di);

	clear();

	certType = di -> certType;
	certLen = di -> certLen;

	// allocate memory and copy.
	if (NULL == (certDER = (unsigned char *) malloc(certLen)))
	{
		pqioutput(PQL_DEBUG_BASIC, pqicizone, 
	  	"PQChanItem::in() Not enough Memory!");
		return; // cant be us.
	}

	memcpy(certDER, di -> certDER, certLen);

	msg = di -> msg;

	files = di -> files;

	// only the signature to go!.
	signType = di -> signType;
	signLen = di -> signLen;

	// allocate memory and copy.
	if (NULL == (signature = (unsigned char *) malloc(signLen)))
	{
		pqioutput(PQL_DEBUG_BASIC, pqicizone, 
	  	"PQChanItem::copy() Not enough Memory!");
		return; // cant be us.
	}

	memcpy(signature, di -> signature, signLen);
}

        // Overloaded from PQTunnel.
const int PQChanItem::getSize() const
{
	// 8 bytes for cert + certLen;
	// 8 bytes for msg  + msgLen;
	// 12 bytes for files + sum(file -> getSize())
	// 8 bytes for sign  + signLen;
	
	int files_size = 0;
	FileList::const_iterator it;
	for(it = files.begin(); it != files.end(); it++)
	{
		files_size += it -> getSize();
	}
	
	return 8 + certLen + 8 + msg.length() + 
		12 + files_size + 8 + signLen;
}

int PQChanItem::out(void *data, const int size) const
{
	{
	  std::ostringstream out;
	  out << "PQChanItem::out() Data: " << data;
	  out << " Size: " << size << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqicizone, out.str());
	}

	if (size < PQChanItem::getSize())
	{
		pqioutput(PQL_DEBUG_BASIC, pqicizone, 
		  "PQChanItem::out() Packet Too Small!");
		return -1;
	}

	// out is easy cos the getSize should be correct.
	char *dta = (char *) data;

	// first we put down the certificate;
	((int *) dta)[0] = certType;
	((int *) dta)[1] = certLen;
	dta += 8; // 4 bytes per int.
	memcpy(dta, certDER, certLen);
	dta += certLen;

	// now the message.
	((int *) dta)[0] = PQChan_MsgType;
	((int *) dta)[1] = msg.length();
	dta += 8; // 4 bytes per int.
	memcpy(dta, msg.c_str(), msg.length());
	dta += msg.length();

	// now list counter.
	((int *) dta)[0] = PQChan_ListType;
	((int *) dta)[1] = files.size();

	// calc the length.
	int files_size = 0;
	FileList::const_iterator it;
	for(it = files.begin(); it != files.end(); it++)
	{
		files_size += it -> getSize();
	}

	((int *) dta)[2] = files_size;
	dta += 12; // 4 bytes per int.

	// for each item of the list we need to put the data out.
	for(it = files.begin(); it != files.end(); it++)
	{
		int fsize = it -> getSize();
		it -> out(dta, fsize);
		dta += fsize;
	}

	// finally the signature.
	((int *) dta)[0] = signType;
	((int *) dta)[1] = signLen;
	dta += 8; // 4 bytes per int.
	memcpy(dta, signature, signLen);

	return PQChanItem::getSize();
}


void PQChanItem::clear()
{
	// first reset all our dataspaces.
	if (certDER != NULL)
	{
		free(certDER);
		certDER = NULL;
	}
	certLen = 0;
	msg.clear();
	files.clear();
	if (signature != NULL)
	{
		free(signature);
		signature = NULL;
	}
	signLen = 0;
}


int PQChanItem::in(const void *data, const int size)
{
	{
	  std::ostringstream out;
	  out << "PQChanItem::in() Data: " << data;
	  out << " Size: " << size << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqicizone, out.str());
	}

	clear();

	int i;
	int basesize = PQChanItem::getSize();
	if (size < basesize)
	{
		pqioutput(PQL_ALERT, pqicizone, 
	  	"PQChanItem::in() Not enough space for Class (Base)!");
		return -1; // cant be us.
	}

	// now get certLen.
	char *loc = ((char *) data);
	certType = ((int *) loc)[0];
	certLen = ((int *) loc)[1];
	loc += 8;
	// check size again.
	if (size < (basesize = PQChanItem::getSize()))
	{
		pqioutput(PQL_ALERT, pqicizone, 
	  	"PQChanItem::in() Not enough space for Class (Cert)!");
		return -1; // cant be us.
	}

	// allocate memory and copy.
	if (NULL == (certDER = (unsigned char *) malloc(certLen)))
	{
		pqioutput(PQL_ALERT, pqicizone, 
	  	"PQChanItem::in() Not enough Memory!");
		return -1; // cant be us.
	}

	memcpy(certDER, loc, certLen);

	loc += certLen;

	// now get msgSize.
	if (((int *) loc)[0] != PQChan_MsgType)
	{
		pqioutput(PQL_ALERT, pqicizone, 
	  	"PQChanItem::in() Bad Msg Data Type!");
		return -1; // cant be us.
	}

	int msize = ((int *) loc)[1];
	loc += 8;

	if (size < basesize + msize)
	{
		pqioutput(PQL_ALERT, pqicizone, 
	  	"PQChanItem::in() Not enough space for Class (Msg)!");
		return -1; // cant be us.
	}
	for(i = 0; i < msize; i++)
	{
		msg += loc[i];
	}
	loc += msize;

	// now the files....
	// get total size... and check.
	// now list counter.

	// now get msgSize.
	if (((int *) loc)[0] != PQChan_ListType)
	{
		pqioutput(PQL_ALERT, pqicizone, 
	  	"PQChanItem::in() Bad List Data Type!");
		return -1; // cant be us.
	}

	int fcount = ((int *) loc)[1];
	int fsize = ((int *) loc)[2];
	loc += 12;

	{
	  std::ostringstream out;
	  out << "PQChanItem::in() Size before Files: " << PQChanItem::getSize();
	  out << " FileCount: " << fcount;
	  out << " FileSize: " << fsize;
	  pqioutput(PQL_DEBUG_BASIC, pqicizone, out.str());
	}

	// check the size again.
	if (size < PQChanItem::getSize() + fsize)
	{
		pqioutput(PQL_ALERT, pqicizone, 
	  	"PQChanItem::in() Not enough space for Class (Files)!");
		return -1; // cant be us.
	}

	// Loop through the the files.
	int remain_fsize = fsize;
	for(i = 0; i < fcount; i++)
	{
		FileItem file;
		// loc, and the space left.
		if (0 >= file.in(loc, remain_fsize))
		{
			std::ostringstream out;
	  		out << "PQChanItem::in() Not enough space for Class (File2)!";
			out << std::endl;
			out << "\tWhen loading File #" << i+1 << " of " << fcount;
			out << "\tCurrent Loc: " << loc << " Remaining: " << remain_fsize;

			pqioutput(PQL_ALERT, pqicizone, out.str());
			return -1; // cant be us.
		}

		{
			std::ostringstream out;
	  		out << "PQChanItem::in() Loaded File #" << i+1;
			out << " Loc: " << loc << " size: " << file.getSize();
			pqioutput(PQL_DEBUG_BASIC, pqicizone, out.str());
		}

		loc += file.getSize();
		remain_fsize -= file.getSize();

		files.push_back(file);
	}

	if (remain_fsize != 0)
	{
		std::ostringstream out;
		out << "PQChanItem::in() Warning files size incorrect:";
		out << std::endl;
		out << "Indicated Size: " << fsize << " Indicated Count:";
		out << fcount << std::endl;
		out << "Real Size: " << fsize - remain_fsize;
		pqioutput(PQL_ALERT, pqicizone, out.str());
	}

	// only the signature to go!.
	signType = ((int *) loc)[0];
	signLen = ((int *) loc)[1];
	loc += 8;
	// check size again.
	if (size != PQChanItem::getSize())
	{
		std::ostringstream out;
		out << "PQChanItem::in() Size of Packet Incorrect!";
		out << std::endl;
		out << "getSize(): " << PQChanItem::getSize();
		out << "data size: " << size;

		pqioutput(PQL_ALERT, pqicizone, out.str());

		return -1; // cant be us.
	}

	// allocate memory and copy.
	if (NULL == (signature = (unsigned char *) malloc(signLen)))
	{
		pqioutput(PQL_ALERT, pqicizone, 
	  	"PQChanItem::in() Not enough Memory!");
		return -1; // cant be us.
	}

	memcpy(signature, loc, signLen);

	loc += signLen;

	return PQChanItem::getSize();
}






std::ostream &PQChanItem::print(std::ostream &out)
{
	out << "-------- PQChanItem" << std::endl;
	PQItem::print(out);

	out << "Cert Type/Len: " << certType << "/";
	out << certLen << std::endl;
	out << "Msg: " << msg << std::endl;
	FileList::iterator it;
	for(it = files.begin(); it != files.end(); it++)
	{
		it -> print(out);
	}
	out << "Sign Type/Len: " << signType << "/";
	out << signLen << std::endl;

	return out << "--------" << std::endl;
}


std::ostream &PQChanItem::FileItem::print(std::ostream &out)
{
	out << "-- PQChanItem::FileItem" << std::endl;
	out << "Name: " << name << std::endl;
	out << "Hash: " << hash << std::endl;
	out << "Size: " << size << std::endl;
	return out << "--" << std::endl;
}


        // Overloaded from PQTunnel.
const int PQChanItem::FileItem::getSize() const
{
	// 8 bytes 
	// 8 bytes file + size;
	// 8 bytes message + size;
	// 8 bytes size;
	
	return 8 + 8 + name.length() + 8 + hash.length() + 8;
}

int PQChanItem::FileItem::out(void *data, const int size) const
{
	{
	  std::ostringstream out;
	  out << "PQChanItem::FileItem::out() Data: " << data;
	  out << " Size: " << size << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqicizone, out.str());
	}

	if (size < PQChanItem::FileItem::getSize())
	{
		pqioutput(PQL_DEBUG_BASIC, pqicizone, 
		  "PQChanItem::FileItem::out() Packet Too Small!");
		return -1;
	}

	// out is easy cos the getSize should be correct.
	char *dta = (char *) data;

	// first we put down the total
	((int *) dta)[0] = PQChan_FileType;
	((int *) dta)[1] = PQChanItem::FileItem::getSize();
	dta += 8; // 4 bytes per int.

	((int *) dta)[0] = PQChan_NameType;
	((int *) dta)[1] = name.length();
	dta += 8; // 4 bytes per int.
	memcpy(dta, name.c_str(), name.length());
	dta += name.length();

	((int *) dta)[0] = PQChan_HashType;
	((int *) dta)[1] = hash.length();
	dta += 8; // 4 bytes per int.
	memcpy(dta, hash.c_str(), hash.length());
	dta += hash.length();

	((int *) dta)[0] = PQChan_SizeType;
	((int *) dta)[1] = this -> size;
	dta += 8; // 4 bytes per int.

	return PQChanItem::FileItem::getSize();
}


void PQChanItem::FileItem::clear()
{
	// first reset all our dataspaces.
	name.clear();
	hash.clear();
}


int PQChanItem::FileItem::in(const void *data, const int size)
{
	{
	  std::ostringstream out;
	  out << "PQChanItem::FileItem::in() Data: " << data;
	  out << " Size: " << size << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqicizone, out.str());
	}

	clear();
	int i;

	int basesize = PQChanItem::FileItem::getSize();
	if (size < basesize)
	{
		pqioutput(PQL_ALERT, pqicizone, 
	  	"PQChanItem::FileItem::in() No space for Class (Base)!");
		return -1; // cant be us.
	}


	char *loc = ((char *) data);
	if (((int *) loc)[0] != PQChan_FileType)
	{
		pqioutput(PQL_ALERT, pqicizone, 
	  	"PQChanItem::FileItem::in() Bad File Data Type!");
		return -1; // cant be us.
	}

	int real_size = ((int *) loc)[1];
	if (size < real_size)
	{
		pqioutput(PQL_ALERT, pqicizone, 
	  	"PQChanItem::FileItem::in() No space for Class (TotalSize)!");
		return -1; // cant be us.
	}
	loc += 8; // 4 bytes per int.

	// now get name;
	if (((int *) loc)[0] != PQChan_NameType)
	{
		pqioutput(PQL_ALERT, pqicizone, 
	  	"PQChanItem::FileItem::in() Bad Name Data Type!");
		return -1; // cant be us.
	}

	int msize = ((int *) loc)[1];
	loc += 8;

	if (real_size < basesize + msize)
	{
		pqioutput(PQL_ALERT, pqicizone, 
	  	"PQChanItem::FileItem::in() No space for Class (Name)!");
		return -1; // cant be us.
	}
	for(i = 0; i < msize; i++)
	{
		name += loc[i];
	}
	loc += msize;

	// now get hash;
	if (((int *) loc)[0] != PQChan_HashType)
	{
		pqioutput(PQL_ALERT, pqicizone, 
	  	"PQChanItem::FileItem::in() Bad Hash Data Type!");
		return -1; // cant be us.
	}

	int hsize = ((int *) loc)[1];
	loc += 8;

	if (real_size < basesize + msize + hsize)
	{
		pqioutput(PQL_ALERT, pqicizone, 
	  	"PQChanItem::FileItem::in() No space for Class (Hash)!");
		return -1; // cant be us.
	}
	for(i = 0; i < hsize; i++)
	{
		hash += loc[i];
	}
	loc += hsize;

	// now get size.
	if (((int *) loc)[0] != PQChan_SizeType)
	{
		pqioutput(PQL_ALERT, pqicizone, 
	  	"PQChanItem::FileItem::in() Bad Size Data Type!");
		return -1; // cant be us.
	}

	this -> size = ((int *) loc)[1];
	loc += 8;

	// finally verify that the sizes are correct.
	if (real_size != PQChanItem::FileItem::getSize())
	{
		pqioutput(PQL_ALERT, pqicizone, 
	  	"PQChanItem::FileItem::in() Size Mismatch!");
		return -1; // cant be us.
	}

	return PQChanItem::FileItem::getSize();
}



