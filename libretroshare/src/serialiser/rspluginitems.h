/*
 * libretroshare/src/services: p3turtle.h
 *
 * Services for RetroShare.
 *
 * Copyright 2009 by Cyril Soler
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
 * Please report all bugs and problems to "csoler@users.sourceforge.net".
 *
 */


#pragma once

#include "serialiser/rsserial.h"
#include "serialiser/rstlvbase.h"
#include "serialiser/rstlvtypes.h"
#include "serialiser/rsconfigitems.h"
#include "serialiser/rsbaseserial.h"

const uint8_t RS_PKT_CLASS_PLUGIN_SUBTYPE_HASHSET = 0x01 ;

class RsPluginItem: public RsItem
{
	public:
		RsPluginItem(uint8_t plugin_item_subtype): RsItem(RS_PKT_VERSION1,RS_PKT_CLASS_CONFIG,RS_PKT_TYPE_PLUGIN_CONFIG,plugin_item_subtype) {}
		virtual ~RsPluginItem() {}

		virtual bool serialise(void *data,uint32_t& size) = 0 ;	// Isn't it better that items can serialise themselves ?
		virtual uint32_t serial_size() = 0 ; 							// deserialise is handled using a constructor

		virtual void clear() {} 
};

class RsPluginSerialiser: public RsSerialType
{
	public:
		RsPluginSerialiser() : RsSerialType(RS_PKT_VERSION1, RS_PKT_CLASS_CONFIG, RS_PKT_TYPE_PLUGIN_CONFIG) {}

		virtual uint32_t 	size (RsItem *item) 
		{ 
			return dynamic_cast<RsPluginItem *>(item)->serial_size() ;
		}
		virtual bool serialise(RsItem *item, void *data, uint32_t *size) 
		{ 
			return dynamic_cast<RsPluginItem *>(item)->serialise(data,*size) ;
		}
		virtual RsItem *deserialise (void *data, uint32_t *size) ;
};

class RsPluginHashSetItem: public RsPluginItem
{
	public:
		RsPluginHashSetItem() : RsPluginItem(RS_PKT_CLASS_PLUGIN_SUBTYPE_HASHSET) {}
		RsPluginHashSetItem(void *data,uint32_t size) ;

		RsTlvHashSet hashes ;

		virtual std::ostream& print(std::ostream& o, uint16_t) ;

	protected:
		virtual bool serialise(void *data,uint32_t& size) ;
		virtual uint32_t serial_size() ;
};


