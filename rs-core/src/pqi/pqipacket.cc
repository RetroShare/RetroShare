/*
 * "$Id: pqipacket.cc,v 1.11 2007-03-21 18:45:41 rmf24 Exp $"
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




#include "pqi/pqipacket.h"

// get 
#include "pqi/pqi.h"

// discItem functions...
//#include "discItem.h"

#include "pqi/pqitunnel.h"


#include "pqi/pqidebug.h"
#include <sstream>
const int pqipktzone = 38422;


void 	pqipkt_print(pqipkt *pkt);

/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS // ie UNIX

	// NOTHING
#else
// typedefs to generate the types...
// this must be checked!!!

typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/


// internal functions.
// PQI Functions.
void pqipkt_setPQITag(pqipkt *pkt);
void	pqipkt_setPQIType(void *pkt, int type);
int	pqipkt_getPQIType(void *pkt);
void 	pqipkt_setPQISubType(void *pkt, int subtype);
int 	pqipkt_getPQISubType(void *pkt);
void 	pqipkt_setPQILen(void *pkt, int len);
int 	pqipkt_getPQILen(void *pkt);
void 	pqipkt_setPQIsid(void *pkt, int sid);
int 	pqipkt_getPQIsid(void *pkt);
void 	pqipkt_setPQIFlags(void *pkt, int flags);
int 	pqipkt_getPQIFlags(void *pkt);

// SI Functions.
pqipkt *pqipkt_makesipkt(SearchItem *si);
SearchItem *pqipkt_makesiobj(pqipkt *pkt);
void	pqipkt_setSIdatatype(void *pkt, int datatype);
int	pqipkt_getSIdatatype(void *pkt);
void	pqipkt_setSIdata(void *pkt, const char *in);
std::string	pqipkt_getSIdata(void *pkt);

// FI Functions.
pqipkt *pqipkt_makefipkt(PQFileItem *fi);
PQFileItem *pqipkt_makefiobj(pqipkt *pkt);
void	pqipkt_setFIhash(void *pkt, const char *in);
std::string	pqipkt_getFIhash(void *pkt);
void	pqipkt_setFIname(void *pkt, const char *in);
std::string	pqipkt_getFIname(void *pkt);
void	pqipkt_setFIext(void *pkt, const char *in);
std::string	pqipkt_getFIext(void *pkt);
void	pqipkt_setFIlen(void *pkt, int len);
int	pqipkt_getFIlen(void *pkt);

void	pqipkt_setFIoffset(void *pkt, int offset);
int 	pqipkt_getFIoffset(void *pkt);
void	pqipkt_setFIchunksize(void *pkt, int size);
int 	pqipkt_getFIchunksize(void *pkt);

void	pqipkt_setFIreqtype(void *pkt, int in);
int 	pqipkt_getFIreqtype(void *pkt);
void	pqipkt_setFIexttype(void *pkt, int in);
int	pqipkt_getFIexttype(void *pkt);
void	pqipkt_setFIpath(void *pkt, const char *in);
std::string	pqipkt_getFIpath(void *pkt);

// (S)MI Functions.
pqipkt *pqipkt_makecipkt(ChatItem *ci);
ChatItem *pqipkt_makeciobj(pqipkt *pkt);
void 	pqipkt_setMIuint32(void *pkt, int offset, unsigned long val);
unsigned long pqipkt_getMIuint32(void *pkt, int offset);

void 	pqipkt_setMIstring(void *pkt, int offset, std::string name);
std::string 	pqipkt_setMIstring(void *pkt, int offset, int len);

// FD Functions.
pqipkt *pqipkt_makefdpkt(PQFileData *fd);
PQFileData *pqipkt_makefdobj(pqipkt *pkt);

void	pqipkt_setFDlen(void *pkt, int len);
int	pqipkt_getFDlen(void *pkt);
void	pqipkt_setFDflags(void *pkt, int flags);
int	pqipkt_getFDflags(void *pkt);
void	pqipkt_setFDdata(void *pkt, void *data, int datalen);
void	pqipkt_getFDdata(void *pkt, void *data, int datalen);

static std::map<int, PQTunnel *(*)(void *d, int n)> types_fn;
static std::map<int, PQTunnelInit *(*)(void *d, int n)> types_fninit;


int registerTunnelType(int subtype, PQTunnel *(*fn)(void *d, int n))
{
	// check it don't exist already.
	std::map<int, PQTunnel *(*)(void *d, int n)>::iterator it;
	it = types_fn.find(subtype);
	if (it != types_fn.end())
		return -1;

	types_fn[subtype] = fn;
	return 1;
}

PQTunnel *createTunnelType(int subtype, void *d, int n)
{
	// check		 it don't exist already.
	std::map<int, PQTunnel *(*)(void *d, int n)>::iterator it;
	it = types_fn.find(subtype);
	if (it != types_fn.end())
	{
	  	std::ostringstream out;
	  	out << "Created Tunnel Class:" << std::endl;

	  	PQTunnel *pkt = (it -> second)(d, n);
		// build the packet.
		pkt -> in(d, n);

		pkt -> print(out);
	  	out << std::endl;
	  	pqioutput(PQL_DEBUG_BASIC, pqipktzone, out.str());
		return pkt;
	}

	{
	std::ostringstream out;
	for(it = types_fn.begin(); it != types_fn.end(); it++)
	{
	  	out << "TYPE_FN IDX: " << it -> first << std::endl;
	}
	pqioutput(PQL_DEBUG_BASIC, pqipktzone, out.str());
	}
		

	pqioutput(PQL_DEBUG_BASIC, pqipktzone, 
	  	  "createTunnelType() returning Null");
	return NULL;
}


int registerTunnelInitType(int subtype, PQTunnelInit *(*fn)(void *d, int n))
{
	// check it don't exist already.
	std::map<int, PQTunnelInit *(*)(void *d, int n)>::iterator it;
	it = types_fninit.find(subtype);
	if (it != types_fninit.end())
		return -1;

	types_fninit[subtype] = fn;
	return 1;
}

PQTunnelInit *createTunnelInitType(int subtype, void *d, int n)
{
	// check		 it don't exist already.
	std::map<int, PQTunnelInit *(*)(void *d, int n)>::iterator it;
	it = types_fninit.find(subtype);
	if (it != types_fninit.end())
	{
	  	std::ostringstream out;
	  	out << "Created Init Tunnel Class:" << std::endl;

	  	PQTunnelInit *pkt = (it -> second)(d, n);
		// build the packet.
		pkt -> in(d, n);

		pkt -> print(out);
	  	out << std::endl;
	  	pqioutput(PQL_DEBUG_BASIC, pqipktzone, out.str());
		return pkt;
	}

	{
	std::ostringstream out;
	for(it = types_fninit.begin(); it != types_fninit.end(); it++)
	{
	  	out << "TYPE_FN IDX: " << it -> first << std::endl;
	}
	pqioutput(PQL_DEBUG_BASIC, pqipktzone, out.str());
	}
		

	pqioutput(PQL_DEBUG_BASIC, pqipktzone, 
	  	  "createTunnelInitType() returning Null");
	return NULL;
}


// Tunnel Functions.
pqipkt *pqipkt_maketunnelpkt(PQTunnel *item);
PQTunnel *pqipkt_maketunnelobj(pqipkt *pkt);

// Init Tunnel
pqipkt *pqipkt_maketunnelinitpkt(PQTunnelInit *item);
PQTunnelInit *pqipkt_maketunnelinitobj(pqipkt *pkt);

void	pqipkt_setTunnellen(void *pkt, int len);
int	pqipkt_getTunnellen(void *pkt);
void *pqipkt_getTunneldata(void *pkt);



int	pqipkt_basesize()
{
	//fixme("pqipkt_basesize()", 5);
	return 276;
}

	int	pqipkt_maxsize()
{
	//fixme("pqipkt_maxsize()", 5);
	return 16 * 1024;
}


// determine properties. (possible from incomplete pkt).
int	pqipkt_rawlen(pqipkt *pkt)
{
	return pqipkt_getPQILen(pkt);
}

bool 	pqipkt_check(pqipkt *pkt, int size)
{
	std::ostringstream out;
	out << "pqipkt_check() RawLen: " << pqipkt_rawlen(pkt);
	out << " is ?=? to size: " << size;
	pqioutput(PQL_DEBUG_BASIC, pqipktzone, out.str());

	return (pqipkt_rawlen(pkt) <= size);
}


PQItem *pqipkt_create(pqipkt *pkt)
{
	PQItem *pqi = NULL;

	{
	  std::ostringstream out;
	  out << "pqipkt_create() Making PQItem Based on Type:";
	  out << pqipkt_getPQIType(pkt);
	  pqioutput(PQL_DEBUG_BASIC, pqipktzone, out.str());
	}

	//pqipkt_print(pkt);

	switch(pqipkt_getPQIType(pkt))
	{
		case PQI_ITEM_TYPE_SEARCHITEM:
			pqi = pqipkt_makesiobj(pkt);
			break;

		case PQI_ITEM_TYPE_FILEITEM:
			pqi = pqipkt_makefiobj(pkt);
			break;


		case PQI_ITEM_TYPE_CHATITEM:
			pqi = pqipkt_makeciobj(pkt);
			break;

		case PQI_ITEM_TYPE_TUNNELITEM:
			pqi = pqipkt_maketunnelobj(pkt);
			break;

		case PQI_ITEM_TYPE_TUNNELINITITEM:
			pqi = pqipkt_maketunnelinitobj(pkt);
			break;


		//case PQI_ITEM_TYPE_AUTODISCITEM:
		//	pqi = pqipkt_makeadobj(pkt);
		//	break;

		case PQI_ITEM_TYPE_INFOITEM: // fall through as none
		case PQI_ITEM_TYPE_COMMANDITEM: // of these should happen.
		default:
			{
	  		  std::ostringstream out;
			  out << "pqipkt_create() UNKNOWN TYPE :";
			  out << pqipkt_getPQIType(pkt);
	  		  pqioutput(PQL_DEBUG_BASIC, pqipktzone, out.str());
			}
			return NULL;
			break;
	}
	if (pqi != NULL)
	{
		//pqi -> print(std::cerr);
	}
	else
	{
	  	pqioutput(PQL_ALERT, pqipktzone,
			"pqipkt_create() Bad Packet");
	}
	return pqi;
}


// Outgoing Pkts.... (These are raw packets....(a chunk of data))
pqipkt *pqipkt_makepkt(PQItem *pqi)
{
	pqipkt *pkt = NULL;
	{
	  std::ostringstream out;
	  out << "pqipkt_makepkt() Making pqipkt Based on Type:";
	  out << pqi -> type << std::endl;
	  pqi -> print(out);
	  pqioutput(PQL_DEBUG_BASIC, pqipktzone, out.str());
	}

	switch(pqi -> type)
	{
		case PQI_ITEM_TYPE_SEARCHITEM:
			pkt = pqipkt_makesipkt((SearchItem *) pqi);
			break;

		case PQI_ITEM_TYPE_FILEITEM:
			pkt = pqipkt_makefipkt((PQFileItem *) pqi);
			break;

		case PQI_ITEM_TYPE_CHATITEM:
			pkt = pqipkt_makecipkt((ChatItem *) pqi);
			break;

		case PQI_ITEM_TYPE_TUNNELITEM:
			pkt = pqipkt_maketunnelpkt((PQTunnel *) pqi);
			break;

		case PQI_ITEM_TYPE_TUNNELINITITEM:
			pkt = pqipkt_maketunnelinitpkt((PQTunnelInit *) pqi);
			break;

		//case PQI_ITEM_TYPE_AUTODISCITEM:
		//	pkt = pqipkt_makeadpkt((DiscItem *) pqi);
		//	return pkt;
		//	break;

		case PQI_ITEM_TYPE_INFOITEM: // fall through as none
		case PQI_ITEM_TYPE_COMMANDITEM: // of these should happen.
		default:
			{
			  std::ostringstream out;
			  out << "pqipkt_makepkt() UNKNOWN TYPE :";
			  out << pqi->type << "/" << pqi->subtype;
	  		  pqioutput(PQL_ALERT, pqipktzone, out.str());
			}
			return NULL;
			break;
	}

	if (pkt != NULL)
	{
		//pqipkt_print(pkt);
	}
	else
	{
		std::ostringstream out;
		out << "pqipkt_makepkt() NULL pkt";
		pqioutput(PQL_DEBUG_BASIC, pqipktzone, out.str());
	}
	return pkt;
}

// delete the raw packet (allocated using above).
void 	pqipkt_delete(pqipkt *pkt)
{
	free(pkt);
	return;
}


// delete the raw packet (allocated using above).
void 	pqipkt_print(pqipkt *pkt)
{
	int len = pqipkt_getPQILen(pkt);
	std::ostringstream out;

	out << "pqipkt_print(pkt): Type(";
	out << pqipkt_getPQIType(pkt) << "/";
	out << pqipkt_getPQISubType(pkt) << ") - len:";
	out << len << std::endl;
	for(int i = 0; i < len; i++)
	{
		if ((i > 0) && (i % 20 == 0))
			out << std::endl;

		out << std::hex;
		int val = (int) ((unsigned char *) pkt)[i];
		if (val < 16)
			out << "0" << val << " ";
		else
			out << val << " ";

	}
	out << std::endl;
	pqioutput(PQL_DEBUG_BASIC, pqipktzone, out.str());

	return;
}










/************ PQI PACKET DESCRIPTION ******************
 *
 * VERY BASIC at the moment.
 *
 * 4 bytes -> PQI Tag + Version.
 * 2 bytes -> PKT_TYPE
 * 2 bytes -> PKT_SUBTYPE
 * 4 bytes -> PKT_LEN
 * 4 bytes -> PKT_SID
 * 4 bytes -> PKT_FLAGS
 *
 * Total (20) bytes.
 **************************************************/


void pqipkt_setPQITag(pqipkt *pkt)
{
	char *str = (char *) pkt;
	strncpy(str, "PQ11", 4);
	return;
}

void	pqipkt_setPQIType(void *pkt, int type)
{
	uint16_t *ptype = (uint16_t *) &(((char *) pkt)[4]);
	ptype[0] = htons(type);
	return;
}


int	pqipkt_getPQIType(void *pkt)
{
	uint16_t *ptype = (uint16_t *) &(((char *) pkt)[4]);
	return ntohs(ptype[0]);
}

void 	pqipkt_setPQISubType(void *pkt, int subtype)
{
	uint16_t *ptype = (uint16_t *) &(((char *) pkt)[6]);
	ptype[0] = htons(subtype);
	return;
}

int 	pqipkt_getPQISubType(void *pkt)
{
	uint16_t *ptype = (uint16_t *) &(((char *) pkt)[6]);
	return ntohs(ptype[0]);
}

void 	pqipkt_setPQILen(void *pkt, int len)
{
	uint32_t *ptype = (uint32_t *) &(((char *) pkt)[8]);
	ptype[0] = htonl(len);
	return;
}

int 	pqipkt_getPQILen(void *pkt)
{
	uint32_t *ptype = (uint32_t *) &(((char *) pkt)[8]);
	return ntohl(ptype[0]);
}


void 	pqipkt_setPQIsid(void *pkt, int sid)
{
	uint32_t *loc = (uint32_t *) &(((char *) pkt)[12]);
	loc[0] = htonl(sid);
	return;
}


int 	pqipkt_getPQIsid(void *pkt)
{
	uint32_t *loc = (uint32_t *) &(((char *) pkt)[12]);
	return ntohl(loc[0]);
}


void 	pqipkt_setPQIFlags(void *pkt, int flags)
{
	uint32_t *loc = (uint32_t *) &(((char *) pkt)[16]);
	loc[0] = htonl(flags);
	return;
}

int 	pqipkt_getPQIFlags(void *pkt)
{
	uint32_t *loc = (uint32_t *) &(((char *) pkt)[16]);
	return ntohl(loc[0]);
}


// end of the PQI data fields.


/************ SI PACKET DESCRIPTION ******************
 *
 * PQI + 
 *
 * 2 bytes -> SI_DATATYPE
 * 256 bytes -> DATA
 *
 * Total (20 + 2 + 256 = 278) bytes.
 **************************************************/

// Outgoing Pkts.... (These are raw packets....(a chunk of data))
pqipkt *pqipkt_makesipkt(SearchItem *si)
{
	pqioutput(PQL_DEBUG_BASIC, pqipktzone, 
	  "pqipkt_makesipkt");

	pqipkt *pkt = (pqipkt *) malloc(278);
	pqipkt_setPQITag(pkt);
	pqipkt_setPQIType(pkt, PQI_ITEM_TYPE_SEARCHITEM);
	pqipkt_setPQISubType(pkt, si -> subtype);
	pqipkt_setPQILen(pkt, 278);
	pqipkt_setPQIFlags(pkt, si -> flags);
	pqipkt_setPQIsid(pkt, si -> sid);

	pqipkt_setSIdatatype(pkt, si -> datatype);
	pqipkt_setSIdata(pkt, (si -> data).c_str());

	return pkt;
}

SearchItem *pqipkt_makesiobj(pqipkt *pkt)
{
	pqioutput(PQL_DEBUG_BASIC, pqipktzone, 
	  "pqipkt_makesiobj");

	// only one type...
	SearchItem *si = new SearchItem();
	//pqipkt_getPQITag(pkt);
	si -> type = PQI_ITEM_TYPE_SEARCHITEM;
	si -> subtype = pqipkt_getPQISubType(pkt);
	si -> flags = pqipkt_getPQIFlags(pkt);
	si -> sid = pqipkt_getPQIsid(pkt);

	si -> datatype = pqipkt_getSIdatatype(pkt);
	si -> data = pqipkt_getSIdata(pkt);
	return si;
}

void	pqipkt_setSIdatatype(void *pkt, int datatype)
{
	uint16_t *ptype = (uint16_t *) &(((char *) pkt)[20]);
	ptype[0] = htons(datatype);
	return;
}

int	pqipkt_getSIdatatype(void *pkt)
{
	uint16_t *ptype = (uint16_t *) &(((char *) pkt)[20]);
	return ntohs(ptype[0]);
}

void	pqipkt_setSIdata(void *pkt, const char *in)
{
	char *str = &(((char *) pkt)[22]);
	strncpy(str, in, 256);
}


std::string	pqipkt_getSIdata(void *pkt)
{
	char *str = &(((char *) pkt)[22]);
	str[255] = '\0';
	return std::string(str);
}


/************ FI PACKET DESCRIPTION ******************
 *
 * PQI + 
 * 256 bytes -> hash;
 * 256 bytes -> name;
 * 16 bytes -> ext;
 * 4 bytes -> len;
 * 4 bytes -> offset; // data req
 * 4 bytes -> chunk;  // data req
 * 4 bytes -> reqtype; (now needed)
 * 4 bytes -> exttype;
 * 512 bytes -> path;
 *
 * Total (20 + 512 + 512 + 16 + 4*5 = 1080) bytes.
 **************************************************/


pqipkt *pqipkt_makefipkt(PQFileItem *fi)
{
	pqioutput(PQL_DEBUG_BASIC, pqipktzone, 
	  "pqipkt_makefipkt");

	if (fi -> subtype == PQI_FI_SUBTYPE_DATA)
	{
		return pqipkt_makefdpkt( (PQFileData *) fi);
	}

	int len = 1080;
	pqipkt *pkt = (pqipkt *) malloc(len);
	pqipkt_setPQITag(pkt);
	pqipkt_setPQIType(pkt, PQI_ITEM_TYPE_FILEITEM);
	pqipkt_setPQISubType(pkt, fi -> subtype);
	pqipkt_setPQILen(pkt, len);
	pqipkt_setPQIFlags(pkt, fi -> flags);
	pqipkt_setPQIsid(pkt, fi -> sid);

	pqipkt_setFIhash(pkt, (fi -> hash).c_str());
	pqipkt_setFIname(pkt, (fi -> name).c_str());

	pqipkt_setFIext(pkt, (fi -> ext).c_str());
	pqipkt_setFIlen(pkt, fi -> size);

	pqipkt_setFIoffset(pkt, fi -> fileoffset);
	pqipkt_setFIchunksize(pkt, fi -> chunksize);

	pqipkt_setFIreqtype(pkt, fi -> reqtype);
	pqipkt_setFIexttype(pkt, fi -> exttype);
	pqipkt_setFIpath(pkt, (fi -> path).c_str());

	return pkt;
}

PQFileItem *pqipkt_makefiobj(pqipkt *pkt)
{
	pqioutput(PQL_DEBUG_BASIC, pqipktzone, 
	  "pqipkt_makefiobj");

	if (pqipkt_getPQISubType(pkt) == PQI_FI_SUBTYPE_DATA)
	{
		return pqipkt_makefdobj(pkt);
	}

	// only one type...
	PQFileItem *fi = new PQFileItem();
	//pqipkt_getPQITag(pkt);
	fi -> type = PQI_ITEM_TYPE_FILEITEM;
	fi -> subtype = pqipkt_getPQISubType(pkt);
	fi -> flags = pqipkt_getPQIFlags(pkt);
	fi -> sid = pqipkt_getPQIsid(pkt);

	fi -> hash = pqipkt_getFIhash(pkt);
	fi -> name = pqipkt_getFIname(pkt);
	fi -> ext  = pqipkt_getFIext(pkt);
	fi -> size = pqipkt_getFIlen(pkt);

	fi -> fileoffset = pqipkt_getFIoffset(pkt);
	fi -> chunksize  = pqipkt_getFIchunksize(pkt);

	fi -> reqtype  = pqipkt_getFIreqtype(pkt);
	fi -> exttype  = pqipkt_getFIexttype(pkt);

	fi -> path = pqipkt_getFIpath(pkt);

	return fi;
}

void	pqipkt_setFIhash(void *pkt, const char *in)
{
	char *str = &(((char *) pkt)[20]);
	strncpy(str, in, 256);
}

std::string	pqipkt_getFIhash(void *pkt)
{
	char *str = &(((char *) pkt)[20]);
	str[255] = '\0';
	return std::string(str);
}

void	pqipkt_setFIname(void *pkt, const char *in)
{
	char *str = &(((char *) pkt)[276]);
	strncpy(str, in, 256);
}

std::string	pqipkt_getFIname(void *pkt)
{
	char *str = &(((char *) pkt)[276]);
	str[255] = '\0';
	return std::string(str);
}

void	pqipkt_setFIext(void *pkt, const char *in)
{
	char *str = &(((char *) pkt)[532]);
	strncpy(str, in, 16);
}

std::string	pqipkt_getFIext(void *pkt)
{
	char *str = &(((char *) pkt)[532]);
	str[15] = '\0';
	return std::string(str);
}

void	pqipkt_setFIlen(void *pkt, int len)
{
	uint32_t *loc = (uint32_t *) &(((char *) pkt)[548]);
	loc[0] = htonl(len);
	return;
}

int	pqipkt_getFIlen(void *pkt)
{
	uint32_t *loc = (uint32_t *) &(((char *) pkt)[548]);
	return ntohl(loc[0]);
}


void	pqipkt_setFIoffset(void *pkt, int offset)
{
	uint32_t *loc = (uint32_t *) &(((char *) pkt)[552]);
	loc[0] = htonl(offset);
	return;
}

int 	pqipkt_getFIoffset(void *pkt)
{
	uint32_t *loc = (uint32_t *) &(((char *) pkt)[552]);
	return ntohl(loc[0]);
}


void	pqipkt_setFIchunksize(void *pkt, int size)
{
	uint32_t *loc = (uint32_t *) &(((char *) pkt)[556]);
	loc[0] = htonl(size);
	return;
}

int 	pqipkt_getFIchunksize(void *pkt)
{
	uint32_t *loc = (uint32_t *) &(((char *) pkt)[556]);
	return ntohl(loc[0]);
}


void	pqipkt_setFIreqtype(void *pkt, int len)
{
	uint32_t *loc = (uint32_t *) &(((char *) pkt)[560]);
	loc[0] = htonl(len);
	return;
}

int	pqipkt_getFIreqtype(void *pkt)
{
	uint32_t *loc = (uint32_t *) &(((char *) pkt)[560]);
	return ntohl(loc[0]);
}

void	pqipkt_setFIexttype(void *pkt, int len)
{
	uint32_t *loc = (uint32_t *) &(((char *) pkt)[564]);
	loc[0] = htonl(len);
	return;
}

int	pqipkt_getFIexttype(void *pkt)
{
	uint32_t *loc = (uint32_t *) &(((char *) pkt)[564]);
	return ntohl(loc[0]);
}


void	pqipkt_setFIpath(void *pkt, const char *in)
{
	char *str = &(((char *) pkt)[568]);
	strncpy(str, in, 512);
}

std::string	pqipkt_getFIpath(void *pkt)
{
	char *str = &(((char *) pkt)[568]);
	str[511] = '\0';
	return std::string(str);
}


/************ SMI(Chat) PACKET DESCRIPTION ******************
 * OLD FORMATS>>>>
 *
 * PQI + 
 * 256 bytes -> msg;
 *
 * Total (20 + 256 = 276) bytes.
 **************************************************/
/************ MI PACKET DESCRIPTION ******************
 *
 * SMI + 
 * 4 bytes -> datalen; 
 * 4 bytes -> flags; // if attachment
 * 4 bytes -> size; // if attachment
 * 256 bytes -> hash;
 * 256 bytes -> name;
 * 256 bytes -> channel name;
 * n bytes -> msg.
 *
 * Total (276 + 4 + 4 + 4 + 768 + n) bytes.
 *  = 1056 + n
 **************************************************/


/************ SMI(Chat) PACKET DESCRIPTION ******************
 *
 * PQI + 
 * 4 bytes -> msgsize
 * n bytes -> msg;
 *
 * Total (20 + 4 + n = 24 + n) bytes.
 **************************************************/
/************ MI PACKET DESCRIPTION ******************
 *
 * SMI + 
 * 4 bytes -> flags;
 * 4 bytes -> sendTime;
 * 4 bytes -> titleSize;
 * n bytes -> title
 * 4 bytes -> headerSize;
 * m bytes -> header;
 * 4 bytes -> noFiles;
 * (per file)
 *    4 bytes -> size;
 *    4 bytes -> hashsize
 *    o bytes -> hash
 *    4 bytes -> namesize
 *    p bytes -> name;
 *
 * Total (SMI + 4 + 4 + 4 + n + 4 + m + 4 + files) bytes.
 **************************************************/

pqipkt *pqipkt_makecipkt(ChatItem *ci)
{
	pqioutput(PQL_DEBUG_BASIC, pqipktzone, 
	  "pqipkt_makecipkt");

	MsgItem *mi;
	pqipkt *pkt;
	int len;
	int offset = 0;

	int baselen = 20 + 4 + ci->msg.size();

	if ((mi = dynamic_cast<MsgItem *>(ci)) != NULL)
	{
		int extralen = 5 * 4;
		extralen += mi->title.size();
		extralen += mi->header.size();

		/* plus files */
		std::list<MsgFileItem>::iterator it;
		for(it = mi->files.begin(); it != mi->files.end(); it++)
		{
			extralen += 12;
			extralen += it -> name.size();
			extralen += it -> hash.size();
		}

		len = baselen + extralen;
		if (len > pqipkt_maxsize())
		{
			/* cannot make packet! */
			return NULL;
		}
		if (len < pqipkt_basesize())
		{
			len = pqipkt_basesize();
		}

		pkt = malloc(len);

		// fill it the base.
		pqipkt_setPQITag(pkt);
		pqipkt_setPQIType(pkt, PQI_ITEM_TYPE_CHATITEM);
		pqipkt_setPQISubType(pkt, mi -> subtype);
		pqipkt_setPQILen(pkt, len);
		pqipkt_setPQIFlags(pkt, mi -> flags);
		pqipkt_setPQIsid(pkt, mi -> sid); // id irrelevant.

		offset = 20;
		// Msg
		pqipkt_setMIuint32(pkt, offset, mi -> msg.size());
		offset += 4;
		pqipkt_setMIstring(pkt, offset, mi -> msg);
		offset += mi->msg.size();

		// Actual Msg part....
		pqipkt_setMIuint32(pkt, offset, mi -> msgflags);
		offset += 4;
		pqipkt_setMIuint32(pkt, offset, mi -> sendTime);
		offset += 4;
		// Title
		pqipkt_setMIuint32(pkt, offset, mi -> title.size());
		offset += 4;
		pqipkt_setMIstring(pkt, offset, mi -> title);
		offset += mi->title.size();
		// Header
		pqipkt_setMIuint32(pkt, offset, mi -> header.size());
		offset += 4;
		pqipkt_setMIstring(pkt, offset, mi -> header);
		offset += mi->header.size();
		// nofiles.
		pqipkt_setMIuint32(pkt, offset, mi -> files.size());
		offset += 4;
		// Now pack those files..
		for(it = mi->files.begin(); it != mi->files.end(); it++)
		{
			// size.
			pqipkt_setMIuint32(pkt, offset, it -> size);
			offset += 4;
			// hash
			pqipkt_setMIuint32(pkt, offset, it -> hash.size());
			offset += 4;
			pqipkt_setMIstring(pkt, offset, it -> hash);
			offset += it->hash.size();
			// name
			pqipkt_setMIuint32(pkt, offset, it -> name.size());
			offset += 4;
			pqipkt_setMIstring(pkt, offset, it -> name);
			offset += it->name.size();
		}

		if (offset != len)
		{
			/* error! */
		}

		return pkt;
	}
	// only chatitem.
	len = baselen;
	if (len < pqipkt_basesize())
	{
		len = pqipkt_basesize();
	}

	pkt = malloc(len);

	pqipkt_setPQITag(pkt);
	pqipkt_setPQIType(pkt, PQI_ITEM_TYPE_CHATITEM);
	pqipkt_setPQISubType(pkt, ci -> subtype);
	pqipkt_setPQILen(pkt, len);
	pqipkt_setPQIFlags(pkt, ci -> flags);
	pqipkt_setPQIsid(pkt, ci -> sid); // id irrelevant.

	// fill in the msg.
	offset = 20;
	// Msg
	pqipkt_setMIuint32(pkt, offset, ci -> msg.size());
	offset += 4;
	pqipkt_setMIstring(pkt, offset, ci -> msg);
	offset += ci->msg.size();

	return pkt;
}


ChatItem *pqipkt_makeciobj(pqipkt *pkt)
{
	pqioutput(PQL_DEBUG_BASIC, pqipktzone, 
	  "pqipkt_makeciobj");

	int len = pqipkt_getPQILen(pkt);
	int offset = 0;
	int namelen = 0;
	int i;

	// switch based on the subtype;
	if (pqipkt_getPQISubType(pkt) == PQI_MI_SUBTYPE_CHAT)
	{
		// only short pkt!
		ChatItem *ci = new ChatItem();
		//pqipkt_getPQITag(pkt);
		ci -> type = PQI_ITEM_TYPE_CHATITEM;
		ci -> subtype = pqipkt_getPQISubType(pkt);
		ci -> flags = pqipkt_getPQIFlags(pkt);
		ci -> sid = pqipkt_getPQIsid(pkt);
	
		offset = 20;
		namelen   = pqipkt_getMIuint32(pkt, offset);
		offset += 4;

		if (offset + namelen > len)
		{
			/* error */
			pqioutput(PQL_ALERT, pqipktzone, "pqipkt_makeciobj ALERT size error CI(1)");
			delete ci;
			return NULL;
		}

		ci -> msg = pqipkt_setMIstring(pkt, offset, namelen);
		return ci;
	}

	// else message
	
	MsgItem *mi = new MsgItem();
	//pqipkt_getPQITag(pkt);
	mi -> type = PQI_ITEM_TYPE_CHATITEM;
	mi -> subtype = pqipkt_getPQISubType(pkt);
	mi -> flags = pqipkt_getPQIFlags(pkt);
	mi -> sid = pqipkt_getPQIsid(pkt);

	bool pktOkay = true;

	offset = 20;
	if (offset + 4 > len) pktOkay = false;
	if (pktOkay)
	{
	  namelen   = pqipkt_getMIuint32(pkt, offset);
	  offset += 4;
	}

	if (offset + namelen > len) pktOkay = false;
	if (pktOkay)
	{
	  mi -> msg = pqipkt_setMIstring(pkt, offset, namelen);
	  offset += namelen;
	}

	/* now check the sizes */
	if (offset + 12 > len) pktOkay = false;
	if (pktOkay)
	{
	  // Actual Msg part....
	  mi -> msgflags = pqipkt_getMIuint32(pkt, offset);
	  offset += 4;
	  mi -> sendTime = pqipkt_getMIuint32(pkt, offset);
	  offset += 4;

	  // Title
	  namelen   = pqipkt_getMIuint32(pkt, offset);
	  offset += 4;
	}

	if (offset + namelen > len) pktOkay = false;
	if (pktOkay)
	{
	  mi -> title = pqipkt_setMIstring(pkt, offset, namelen);
	  offset += namelen;
	}

	// Header
	if (offset + 4 > len) pktOkay = false;
	if (pktOkay)
	{
	  namelen   = pqipkt_getMIuint32(pkt, offset);
	  offset += 4;
	}

	if (offset + namelen > len) pktOkay = false;
	if (pktOkay)
	{
	  mi -> header = pqipkt_setMIstring(pkt, offset, namelen);
	  offset += namelen;
	}

	// nofiles.
	if (offset + 4 > len) pktOkay = false;
	int nofiles = 0;
	if (pktOkay)
	{
	  nofiles = pqipkt_getMIuint32(pkt, offset);
	  offset += 4;
	}

	for(i = 0; i < nofiles; i++)
	{
		MsgFileItem mfi;

		if (offset + 8 > len) pktOkay = false;
		if (pktOkay)
		{
		  mfi.size = pqipkt_getMIuint32(pkt, offset);
		  offset += 4;

		  // Hash
		  namelen   = pqipkt_getMIuint32(pkt, offset);
		  offset += 4;
		}

		if (offset + namelen > len) pktOkay = false;
		if (pktOkay)
		{
		  mfi.hash = pqipkt_setMIstring(pkt, offset, namelen);
		  offset += namelen;
		}

		if (offset + 4 > len) pktOkay = false;
		if (pktOkay)
		{
		  // Name
		  namelen   = pqipkt_getMIuint32(pkt, offset);
		  offset += 4;
		}

		if (offset + namelen > len) pktOkay = false;
		if (pktOkay)
		{
		  mfi.name = pqipkt_setMIstring(pkt, offset, namelen);
		  offset += namelen;

		  mi -> files.push_back(mfi);
		}
	}
	if (pktOkay)
	{
		return mi;
	}
	else
	{
		/* error */
		pqioutput(PQL_ALERT, pqipktzone, "pqipkt_makeciobj ALERT size error MI(1)");

		delete mi;
		return NULL;
	}
}

void 	pqipkt_setMIuint32(void *pkt, int offset, unsigned long val)
{
	/* stick the value at the offset! */
	char *buf = (char *) pkt + offset;
	*((unsigned long *) buf) = htonl(val);
}

void 	pqipkt_setMIstring(void *pkt, int offset, std::string name)
{
	/* stick the value at the offset! */
	char *str = &(((char *) pkt)[offset]);
	for(unsigned int i = 0; i < name.size(); i++)
	{
		str[i] = name[i];
	}
}

unsigned long pqipkt_getMIuint32(void *pkt, int offset)
{
	/* stick the value at the offset! */
	char *buf = (char *) pkt + offset;
	unsigned long val = ntohl(*((unsigned long *) buf));
	return val;
}

std::string 	pqipkt_setMIstring(void *pkt, int offset, int len)
{
	/* stick the value at the offset! */
	char *str = &(((char *) pkt)[offset]);
	std::string val;
	for(int i = 0; i < len; i++)
	{
		val += str[i];
	}
	return val;
}

/************ FD PACKET DESCRIPTION ******************
 *
 * PQI + 
 * (basic FI)
 * 256 bytes -> hash;
 * 256 bytes -> name;
 * 16 bytes -> ext;
 * 4 bytes -> len;
 * 4 bytes -> offset; // data req
 * 4 bytes -> chunk;  // data req
 * 4 bytes -> reqtype; (now needed)
 * 4 bytes -> exttype;
 * SubTotal (20 + 512 + 16 + 4*5 = 568) bytes.
 *
 *
 * 4 bytes -> datalen;
 * 4 bytes -> flags;
 * n bytes -> data.
 *
 * Total (568 + 4 + 4 + n) bytes.
 **************************************************/

pqipkt *pqipkt_makefdpkt(PQFileData *fd)
{
	pqioutput(PQL_DEBUG_BASIC, pqipktzone, 
	  "pqipkt_makefdpkt");

	pqipkt *pkt;
	int len;
	//is a MsgItem mi.
	//calc length of the message.
	len = 568 + 4 + 4 + fd -> datalen;
	// but len also has a minimum length of 276 bytes....
	if (len < 276)
	{
		len = 276;
	}

	pkt = malloc(len);

	// fill it in.
	pqipkt_setPQITag(pkt);
	pqipkt_setPQIType(pkt, PQI_ITEM_TYPE_FILEITEM);
	pqipkt_setPQISubType(pkt, fd -> subtype);
	pqipkt_setPQILen(pkt, len);
	pqipkt_setPQIFlags(pkt, fd -> flags);
	pqipkt_setPQIsid(pkt, fd -> sid);

	// Fi fields
	pqipkt_setFIhash(pkt, (fd -> hash).c_str());
	pqipkt_setFIname(pkt, (fd -> name).c_str());
	pqipkt_setFIext(pkt, (fd -> ext).c_str());
	pqipkt_setFIlen(pkt, fd -> size);
	pqipkt_setFIoffset(pkt, fd -> fileoffset);
	pqipkt_setFIchunksize(pkt, fd -> chunksize);
	pqipkt_setFIreqtype(pkt, fd -> reqtype);
	pqipkt_setFIexttype(pkt, fd -> exttype);

	pqipkt_setFDlen(pkt, fd -> datalen);
	pqipkt_setFDflags(pkt, fd -> fileflags);
	pqipkt_setFDdata(pkt, (fd -> data), fd -> datalen);

	return pkt;
}


PQFileData *pqipkt_makefdobj(pqipkt *pkt)
{
	pqioutput(PQL_DEBUG_BASIC, pqipktzone, 
	  "pqipkt_makefdobj");

	PQFileData *fd = new PQFileData();
	//pqipkt_getPQITag(pkt);
	fd -> type = PQI_ITEM_TYPE_FILEITEM;
	fd -> subtype = pqipkt_getPQISubType(pkt);
	fd -> flags = pqipkt_getPQIFlags(pkt);
	fd -> sid = pqipkt_getPQIsid(pkt);

	// FI Stuff.
	fd -> hash = pqipkt_getFIhash(pkt);
	fd -> name = pqipkt_getFIname(pkt);
	fd -> ext  = pqipkt_getFIext(pkt);
	fd -> size = pqipkt_getFIlen(pkt);

	fd -> fileoffset = pqipkt_getFIoffset(pkt);
	fd -> chunksize  = pqipkt_getFIchunksize(pkt);

	fd -> reqtype  = pqipkt_getFIreqtype(pkt);
	fd -> exttype  = pqipkt_getFIexttype(pkt);

	// then the fd stuff.
	fd -> datalen = pqipkt_getFDlen(pkt);
	fd -> fileflags = pqipkt_getFDflags(pkt);
	fd -> data = malloc(fd -> datalen);
	pqipkt_getFDdata(pkt, fd -> data, fd -> datalen);
	
	return fd;
}


void	pqipkt_setFDlen(void *pkt, int len)
{
	uint32_t *loc = (uint32_t *) &(((char *) pkt)[568]);
	loc[0] = htonl(len);
	return;
}

int	pqipkt_getFDlen(void *pkt)
{
	uint32_t *loc = (uint32_t *) &(((char *) pkt)[568]);
	return ntohl(loc[0]);
}



void	pqipkt_setFDflags(void *pkt, int flags)
{
	uint32_t *loc = (uint32_t *) &(((char *) pkt)[572]);
	loc[0] = htonl(flags);
	return;
}


int	pqipkt_getFDflags(void *pkt)
{
	uint32_t *loc = (uint32_t *) &(((char *) pkt)[572]);
	return ntohl(loc[0]);
}


void	pqipkt_setFDdata(void *pkt, void *data, int datalen)
{
	char *str = &(((char *) pkt)[576]);
	memcpy(str, data, datalen);
	if (datalen != pqipkt_getFDlen(pkt))
	{
	  	pqioutput(PQL_ALERT, pqipktzone,
			"pqipkt_setFDdata() len != mi.len");
	}
}


void	pqipkt_getFDdata(void *pkt, void *data, int datalen)
{
	char *str = &(((char *) pkt)[576]);
	memcpy(data, str, datalen); // copy to data.
}


// Tunnel Functions.
pqipkt *pqipkt_maketunnelpkt(PQTunnel *item)
{
	pqioutput(PQL_DEBUG_BASIC, pqipktzone, 
	  "pqipkt_maketunnelpkt");

	pqipkt *pkt;
	int len;
	len = 20 + 4 + item -> getSize();
	// but len also has a minimum length of 276 bytes....
	if (len < 276)
	{
		len = 276;
	}

	{
		std::ostringstream out;
		out << "pqipkt_maketunnelpkt len:" << len;
		pqioutput(PQL_DEBUG_BASIC, pqipktzone, out.str());
	}

	pkt = malloc(len);
	for(int i = 0; i < len; i++)
	{
		((char *)pkt)[0] = 0;
	}

	// fill it in.
	pqipkt_setPQITag(pkt);
	pqipkt_setPQIType(pkt, PQI_ITEM_TYPE_TUNNELITEM);
	pqipkt_setPQISubType(pkt, item -> subtype);
	pqipkt_setPQILen(pkt, len);
	pqipkt_setPQIFlags(pkt, item -> flags);
	pqipkt_setPQIsid(pkt, item -> sid);

	{
		std::ostringstream out;
		out << "pqipkt_maketunnelpkt datalen:" << item -> getSize();
		pqioutput(PQL_DEBUG_BASIC, pqipktzone, out.str());
	}

	pqipkt_setTunnellen(pkt, item -> getSize());
	if (item -> getSize() == 
		item -> out(pqipkt_getTunneldata(pkt), item -> getSize()))
	{
		return pkt;
	}
	// else
	free(pkt);
	return NULL;
}


PQTunnel *pqipkt_maketunnelobj(pqipkt *pkt)
{
	pqioutput(PQL_DEBUG_BASIC, pqipktzone, 
	  "pqipkt_maketunnelobj");

	// Check the Tunnel Length...
	int len = pqipkt_getPQILen(pkt);
	if (len < (int) 24 + pqipkt_getTunnellen(pkt))
	{
		std::ostringstream out;
		out << "Invalid Tunnel Length - Discarding Incoming Packet";
		out << "Len: " << len << " < 24 + ";
		out <<  pqipkt_getTunnellen(pkt);
	  	pqioutput(PQL_DEBUG_BASIC, pqipktzone, out.str());

		return NULL;
	}

	pqioutput(PQL_DEBUG_BASIC, pqipktzone, 
	  "creating a Tunnel Obj");

	PQTunnel *nitem = createTunnelType(pqipkt_getPQISubType(pkt),
		pqipkt_getTunneldata(pkt), pqipkt_getTunnellen(pkt));
	{
	  std::ostringstream out;
	  out << "Pkt Tunnel Incoming: st: ";
	  out << pqipkt_getPQISubType(pkt) << "DataLen: " << 
	  out << pqipkt_getTunnellen(pkt) << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqipktzone, out.str());
	}


	if (!nitem)
	{
		pqioutput(PQL_DEBUG_BASIC, pqipktzone, 
	  	  "pqipkt_maketunnelobj returning Null");
		return NULL;
	}

	nitem -> type = PQI_ITEM_TYPE_TUNNELITEM;
	nitem -> subtype = pqipkt_getPQISubType(pkt);
	nitem -> flags = pqipkt_getPQIFlags(pkt);
	nitem -> sid = pqipkt_getPQIsid(pkt);

	nitem -> in(pqipkt_getTunneldata(pkt), pqipkt_getTunnellen(pkt));
	return nitem;
}



void	pqipkt_setTunnellen(void *pkt, int len)
{
	int *loc = (int *) &(((char *) pkt)[20]);
	(*loc) = len;
}

int	pqipkt_getTunnellen(void *pkt)
{
	int *loc = (int *) &(((char *) pkt)[20]);
	return (*loc);
}

void *pqipkt_getTunneldata(void *pkt)
{
	void *loc = (void *) &(((char *) pkt)[24]);
	return loc;
}


// Tunnel Functions.
pqipkt *pqipkt_maketunnelinitpkt(PQTunnelInit *item)
{
	pqioutput(PQL_DEBUG_BASIC, pqipktzone, 
	  "pqipkt_maketunnelinitpkt");

	pqipkt *pkt;
	int len;
	len = 20 + 4 + item -> getSize();
	// but len also has a minimum length of 276 bytes....
	if (len < 276)
	{
		len = 276;
	}

	{
		std::ostringstream out;
		out << "pqipkt_maketunnelinitpkt len:" << len;
		pqioutput(PQL_DEBUG_BASIC, pqipktzone, out.str());
	}

	pkt = malloc(len);
	for(int i = 0; i < len; i++)
	{
		((char *)pkt)[0] = 0;
	}

	// fill it in.
	pqipkt_setPQITag(pkt);
	pqipkt_setPQIType(pkt, PQI_ITEM_TYPE_TUNNELINITITEM);
	pqipkt_setPQISubType(pkt, item -> subtype);
	pqipkt_setPQILen(pkt, len);
	pqipkt_setPQIFlags(pkt, item -> flags);
	pqipkt_setPQIsid(pkt, item -> sid);

	{
		std::ostringstream out;
		out << "pqipkt_maketunnelinitpkt datalen:" << item -> getSize();
		pqioutput(PQL_DEBUG_BASIC, pqipktzone, out.str());
	}

	pqipkt_setTunnellen(pkt, item -> getSize());
	if (item -> getSize() == 
		item -> out(pqipkt_getTunneldata(pkt), item -> getSize()))
	{
		return pkt;
	}
	// else
	free(pkt);
	return NULL;
}


PQTunnelInit *pqipkt_maketunnelinitobj(pqipkt *pkt)
{
	pqioutput(PQL_DEBUG_BASIC, pqipktzone, 
	  "pqipkt_maketunnelinitobj");

	// Check the Tunnel Length...
	int len = pqipkt_getPQILen(pkt);
	if (len < (int) 24 + pqipkt_getTunnellen(pkt))
	{
		std::ostringstream out;

		out << "Invalid Tunnel Length - Discarding Incoming Packet" << std::endl;
		out << "Len: " << len << " < 24 + ";
		out <<  pqipkt_getTunnellen(pkt);
	  	pqioutput(PQL_DEBUG_BASIC, pqipktzone, out.str());

		return NULL;
	}

	pqioutput(PQL_DEBUG_BASIC, pqipktzone, 
	  "creating a Tunnel Init Obj");

	PQTunnelInit *nitem = createTunnelInitType(pqipkt_getPQISubType(pkt),
		pqipkt_getTunneldata(pkt), pqipkt_getTunnellen(pkt));
	{
	  std::ostringstream out;
	  out << "Pkt Init Tunnel Incoming: st: ";
	  out << pqipkt_getPQISubType(pkt) << "DataLen: " << 
	  out << pqipkt_getTunnellen(pkt) << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqipktzone, out.str());
	}


	if (!nitem)
	{
		pqioutput(PQL_DEBUG_BASIC, pqipktzone, 
	  	  "pqipkt_maketunnelinitobj returning Null");
		return NULL;
	}

	nitem -> type = PQI_ITEM_TYPE_TUNNELINITITEM;
	nitem -> subtype = pqipkt_getPQISubType(pkt);
	nitem -> flags = pqipkt_getPQIFlags(pkt);
	nitem -> sid = pqipkt_getPQIsid(pkt);

	nitem -> in(pqipkt_getTunneldata(pkt), pqipkt_getTunnellen(pkt));
	return nitem;
}


// ANOTHER TYPE DISCOVERY PACKETS...
// THIS HAS NOW BECOME A TUNNEL TYPE.
//
//void pqipkt_setADladdr(void *pkt, sockaddr_in addr);
//sockaddr_in pqipkt_getADladdr(void *pkt);
//void pqipkt_setADsaddr(void *pkt, sockaddr_in addr);
//sockaddr_in pqipkt_getADsaddr(void *pkt);
//
//int pqipkt_getADrecvTF(void *pkt); 
//void pqipkt_setADrecvTF(void *pkt, int tf);
//int pqipkt_getADconnTF(void *pkt);
//void pqipkt_setADconnTF(void *pkt, int tf);
//
//void    pqipkt_setADcertLen(void *pkt, int len);
//int     pqipkt_getADcertLen(void *pkt);
//void pqipkt_getADcertDER(void *pkt, unsigned char *pktder, int len);
//void pqipkt_setADcertDER(void *pkt, unsigned char *pktder, int len);
//
//
//pqipkt *pqipkt_makeadpkt(DiscItem *ad)
//{
//        pqipkt *pkt;
//	int len;
//
//	if (ad -> subtype == AUTODISC_RDI_SUBTYPE_PING)
//	{
//		//len = 20 + 16 + 16 (addrs) + 4 + 4 (ints) 
//		len = 20 + 16 + 16 + 4 + 4;
//		if (len < 276) 
//			len = 276;
//		pkt = malloc(len);
//		pqipkt_setPQITag(pkt);
//		pqipkt_setPQIType(pkt, PQI_ITEM_TYPE_AUTODISCITEM);
//		pqipkt_setPQISubType(pkt, ad -> subtype);
//		pqipkt_setPQILen(pkt, len);
//		pqipkt_setPQIFlags(pkt, ad -> flags);
//		pqipkt_setPQIsid(pkt, ad -> sid);
//
//		pqipkt_setADladdr(pkt, ad -> laddr);
//		pqipkt_setADsaddr(pkt, ad -> saddr);
//		pqipkt_setADrecvTF(pkt, ad -> receive_tf);
//		pqipkt_setADconnTF(pkt, ad -> connect_tf);
//	}
//	else // RPLY
//	{
//		DiscReplyItem *dri = (DiscReplyItem *) ad;
//
//		//len = 20 + 16 + 16 (addrs) + 4 + 4 (ints) + data;
//		len = 20 + 16 + 16 + 4 + 4 + 4 + dri -> certLen;
//		if (len < 276) 
//			len = 276;
//		pkt = malloc(len);
//		pqipkt_setPQITag(pkt);
//		pqipkt_setPQIType(pkt, PQI_ITEM_TYPE_AUTODISCITEM);
//		pqipkt_setPQISubType(pkt, ad -> subtype);
//		pqipkt_setPQILen(pkt, len);
//		pqipkt_setPQIFlags(pkt, ad -> flags);
//		pqipkt_setPQIsid(pkt, ad -> sid);
//
//		// connect details.
//		pqipkt_setADladdr(pkt, dri -> laddr);
//		pqipkt_setADsaddr(pkt, dri -> saddr);
//		pqipkt_setADrecvTF(pkt, dri -> receive_tf);
//		pqipkt_setADconnTF(pkt, dri -> connect_tf);
//
//		// + certificate.
//		pqipkt_setADcertLen(pkt, dri -> certLen);
//		pqipkt_setADcertDER(pkt, dri -> certDER, dri -> certLen);
//
//	}
//	return pkt;
//}
//
//
//DiscItem *pqipkt_makeadobj(pqipkt *pkt)
//{
//	DiscItem *di;
//	if (pqipkt_getPQISubType(pkt) == AUTODISC_RDI_SUBTYPE_PING)
//	{
//		DiscItem *fa = di = new DiscItem();
//		fa -> type = PQI_ITEM_TYPE_AUTODISCITEM;
//		fa -> subtype = pqipkt_getPQISubType(pkt);
//		fa -> flags = pqipkt_getPQIFlags(pkt);
//		fa -> sid = pqipkt_getPQIsid(pkt);
//
//		fa -> laddr = pqipkt_getADladdr(pkt);
//		fa -> saddr = pqipkt_getADsaddr(pkt);
//		fa -> receive_tf = pqipkt_getADrecvTF(pkt); 
//		fa -> connect_tf = pqipkt_getADconnTF(pkt);
//	}
//	else
//	{
//		DiscReplyItem *fa = new DiscReplyItem();
//		fa -> type = PQI_ITEM_TYPE_AUTODISCITEM;
//		fa -> subtype = pqipkt_getPQISubType(pkt);
//		fa -> flags = pqipkt_getPQIFlags(pkt);
//		fa -> sid = pqipkt_getPQIsid(pkt);
//	
//		fa -> laddr = pqipkt_getADladdr(pkt);
//		fa -> saddr = pqipkt_getADsaddr(pkt);
//		fa -> receive_tf = pqipkt_getADrecvTF(pkt); 
//		fa -> connect_tf = pqipkt_getADconnTF(pkt);
//		fa -> certLen = pqipkt_getADcertLen(pkt);
//		fa -> certDER = (unsigned char *) malloc(fa -> certLen);
//		pqipkt_getADcertDER(pkt, fa -> certDER, fa -> certLen);
//
//		di = fa;
//	}
//
//	return di;
//}
//
//
//void pqipkt_setADladdr(void *pkt, struct sockaddr_in addr)
//{
//	struct sockaddr_in *loc = (struct sockaddr_in *)
//			&(((char *) pkt)[20]);
//	(*loc) = addr;
//}
//
//struct sockaddr_in pqipkt_getADladdr(void *pkt)
//{
//	struct sockaddr_in *loc = (struct sockaddr_in *)
//			&(((char *) pkt)[20]);
//	return (*loc);
//}
//
//
//void pqipkt_setADsaddr(void *pkt, struct sockaddr_in addr)
//{
//	struct sockaddr_in *loc = (struct sockaddr_in *)
//			&(((char *) pkt)[36]);
//	(*loc) = addr;
//}
//
//struct sockaddr_in pqipkt_getADsaddr(void *pkt)
//{
//	struct sockaddr_in *loc = (struct sockaddr_in *)
//			&(((char *) pkt)[36]);
//	return (*loc);
//}
//
//
//int pqipkt_getADrecvTF(void *pkt)
//{
//	uint32_t *loc = (uint32_t *) &(((char *) pkt)[52]);
//	return ntohl(loc[0]);
//}
//
//void pqipkt_setADrecvTF(void *pkt, int tf)
//{
//	uint32_t *loc = (uint32_t *) &(((char *) pkt)[52]);
//	loc[0] = htonl(tf);
//	return;
//}
//
//int pqipkt_getADconnTF(void *pkt)
//{
//	uint32_t *loc = (uint32_t *) &(((char *) pkt)[56]);
//	return ntohl(loc[0]);
//}
//
//void pqipkt_setADconnTF(void *pkt, int tf)
//{
//	uint32_t *loc = (uint32_t *) &(((char *) pkt)[56]);
//	loc[0] = htonl(tf);
//	return;
//}
//
//int pqipkt_getADcertLen(void *pkt)
//{
//	uint32_t *loc = (uint32_t *) &(((char *) pkt)[60]);
//	return ntohl(loc[0]);
//}
//
//void pqipkt_setADcertLen(void *pkt, int len)
//{
//	uint32_t *loc = (uint32_t *) &(((char *) pkt)[60]);
//	loc[0] = htonl(len);
//	return;
//}
//
//void pqipkt_getADcertDER(void *pkt, unsigned char *pktder, int len)
//{
//	unsigned char *str = &(((unsigned char *) pkt)[64]);
//	memcpy(pktder, str, len);
//	if (len != pqipkt_getADcertLen(pkt))
//	{
//		std::cerr << "pqipkt_getADcertDER() len != ad.len" << std::endl;
//	}
//}
//
//
//void pqipkt_setADcertDER(void *pkt, unsigned char *pktder, int len)
//{
//	unsigned char *str = &(((unsigned char *) pkt)[64]);
//	memcpy(str, pktder, len);
//	if (len != pqipkt_getADcertLen(pkt))
//	{
//		std::cerr << "pqipkt_setADcertDER() len != ad.len" << std::endl;
//	}
//}
//
//
//
