/*
 * libretroshare/src/serialiser: rsserviceserialiser.cc
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2013-2013 by Cyril Soler & Robert Fernie
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

#include "rsserviceserialiser.h"

uint32_t    RsServiceSerialiser::size(RsItem *i)
{
	RsRawItem *item = dynamic_cast<RsRawItem *>(i);

	if (item)
	{
		return item->getRawLength();
	}
	return 0;
}

/* serialise the data to the buffer */
bool    RsServiceSerialiser::serialise(RsItem *i, void *data, uint32_t *pktsize)
{
        RsRawItem *item = dynamic_cast<RsRawItem *>(i);
	if (!item)
	{
		return false;
	}

        #ifdef RSSERIAL_DEBUG
                std::cerr << "RsServiceSerialiser::serialise() serializing raw item. pktsize : " << *pktsize;
        #endif

	uint32_t tlvsize = item->getRawLength();
        #ifdef RSSERIAL_DEBUG
                std::cerr << "tlvsize : " << tlvsize << std::endl;
        #endif

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	if (tlvsize > getRsPktMaxSize())
    	{
	    std::cerr << "(EE) Serialised packet is too big. Maximum allowed size is " << getRsPktMaxSize() << ". Serialised size is " << tlvsize << ". Please tune your service to correctly split packets" << std::endl;
	    return false; /* packet too big */
    	}

	*pktsize = tlvsize;

	/* its serialised already!!! */
	memcpy(data, item->getRawData(), tlvsize);

	return true;
}

RsItem *RsServiceSerialiser::deserialise(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	if (RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
		return NULL; /* not enough data */

	if (rssize > getRsPktMaxSize())
		return NULL; /* packet too big */

	/* set the packet length */
	*pktsize = rssize;

	RsRawItem *item = new RsRawItem(rstype, rssize);
	void *item_data = item->getRawData();

	memcpy(item_data, data, rssize);

	return item;
}


