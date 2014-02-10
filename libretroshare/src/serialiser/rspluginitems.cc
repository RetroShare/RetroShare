#include "rspluginitems.h"

#ifndef WINDOWS_SYS
#include <stdexcept>
#endif

bool RsPluginHashSetItem::serialise(void *data,uint32_t& pktsize) 
{
	uint32_t tlvsize = serial_size();
	uint32_t offset = 0;

#ifdef P3TURTLE_DEBUG
	std::cerr << "RsPluginSerialiser::serialising HashSet packet (size=" << tlvsize << ")" << std::endl;
#endif
	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data,tlvsize,PacketId(), tlvsize);

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */

	ok &= hashes.SetTlv(data,tlvsize,&offset) ;

	if (offset != tlvsize)
	{
		ok = false;
#ifdef P3TURTLE_DEBUG
		std::cerr << "RsPluginHashSetItem::serialise() Size Error! (offset=" << offset << ", tlvsize=" << tlvsize << ")" << std::endl;
#endif
	}

	return ok ;
}

RsPluginHashSetItem::RsPluginHashSetItem(void *data,uint32_t size)
	: RsPluginItem(RS_PKT_CLASS_PLUGIN_SUBTYPE_HASHSET)
{
	uint32_t offset = 8; // skip the header 
	uint32_t rssize = getRsItemSize(data);
	bool ok = true ;

	hashes.ids.clear() ;

	ok &= hashes.GetTlv(data,size,&offset) ;

	if (offset != rssize)
	{
#ifdef TLV_DEBUG
		std::cerr << "RsTlvPeerIdSet::GetTlv() Warning extra bytes at end of item";
		std::cerr << std::endl;
#endif
		ok = false ;
	}

#ifdef WINDOWS_SYS // No Exceptions in Windows compile. (drbobs).
	UNREFERENCED_LOCAL_VARIABLE(rssize);
#else
	if (!ok)
		throw std::runtime_error("Unknown error while deserializing.") ;
#endif

}

RsItem *RsPluginSerialiser::deserialise(void *data, uint32_t *size) 
{
	// look what we have...

	/* get the type */
	uint32_t rstype = getRsItemId(data);
#ifdef P3TURTLE_DEBUG
	std::cerr << "p3turtle: deserialising packet: " << std::endl ;
#endif
	if (	(RS_PKT_VERSION1 				!= getRsItemVersion(rstype)) 
			|| (RS_PKT_CLASS_CONFIG 		!= getRsItemClass(rstype)) 
			|| (RS_PKT_TYPE_PLUGIN_CONFIG != getRsItemType(rstype))) 
	{
#ifdef P3TURTLE_DEBUG
		std::cerr << "  Wrong type !!" << std::endl ;
#endif
		return NULL; /* wrong type */
	}

#ifndef WINDOWS_SYS
	try
	{
#endif
		switch(getRsItemSubType(rstype))
		{
			case RS_PKT_CLASS_PLUGIN_SUBTYPE_HASHSET	:	return new RsPluginHashSetItem(data,*size) ;

			default:
																		std::cerr << "Unknown packet type in RsPluginSerialiser. Type = " << rstype << std::endl;
																		return NULL ;
		}
#ifndef WINDOWS_SYS
	}
	catch(std::exception& e)
	{
		std::cerr << "Exception raised: " << e.what() << std::endl ;
		return NULL ;
	}
#endif
}

uint32_t RsPluginHashSetItem::serial_size() 
{
	uint32_t size = 8 ;

	size += hashes.TlvSize() ;

	return size ;
}

std::ostream& RsPluginHashSetItem::print(std::ostream& o, uint16_t) 
{
	o << "Item type: RsPluginHashSetItem" << std::endl;
	o << "  Hash list: " << std::endl;

    for(std::list<Sha1CheckSum>::const_iterator it(hashes.ids.begin());it!=hashes.ids.end();++it)
		o << "       " << *it << std::endl;

	return o ;
}


