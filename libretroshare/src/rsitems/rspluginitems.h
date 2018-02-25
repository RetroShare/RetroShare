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

#include "rsitems/rsitem.h"
#include "rsitems/rsconfigitems.h"

#include "serialiser/rstypeserializer.h"

const uint8_t RS_PKT_CLASS_PLUGIN_SUBTYPE_HASHSET = 0x01 ;

class RsPluginItem: public RsItem
{
	public:
		explicit RsPluginItem(uint8_t plugin_item_subtype): RsItem(RS_PKT_VERSION1,RS_PKT_CLASS_CONFIG,RS_PKT_TYPE_PLUGIN_CONFIG,plugin_item_subtype) {}
		virtual ~RsPluginItem() {}

		virtual void clear() {} 
};

class RsPluginHashSetItem: public RsPluginItem
{
	public:
		RsPluginHashSetItem() : RsPluginItem(RS_PKT_CLASS_PLUGIN_SUBTYPE_HASHSET) {}
		RsPluginHashSetItem(void *data,uint32_t size) ;

		virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
        {
            RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,hashes,"hashes");
        }

		RsTlvHashSet hashes ;
};

class RsPluginSerialiser: public RsConfigSerializer
{
	public:
		RsPluginSerialiser() : RsConfigSerializer(RS_PKT_CLASS_CONFIG, RS_PKT_TYPE_PLUGIN_CONFIG) {}

		virtual RsItem *create_item(uint8_t class_type, uint8_t item_type) const
        {
            if(class_type == RS_PKT_TYPE_PLUGIN_CONFIG && item_type == RS_PKT_CLASS_PLUGIN_SUBTYPE_HASHSET)
                return new RsPluginHashSetItem() ;

            return NULL ;
        }
};


