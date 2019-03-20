/*******************************************************************************
 * libretroshare/src/rsitems: rsposteditems.h                                  *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012 by Robert Fernie <retroshare@lunamutt.com>                   *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#include "rsitems/rsposteditems.h"
#include "serialiser/rstypeserializer.h"



void RsGxsPostedPostItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_LINK,mPost.mLink,"mPost.mLink") ;
    RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_MSG ,mPost.mNotes,"mPost.mNotes") ;

	// Do not serialize mImage member if it is empty (keeps compatibility of new posts without image toward older RS)
	// and do not expect to deserialize mImage member if the data block has been consummed entirely (keeps compatibility
	// of new RS with older posts.

    if(j == RsGenericSerializer::DESERIALIZE && ctx.mOffset == ctx.mSize)
        return ;

	if((j == RsGenericSerializer::SIZE_ESTIMATE || j == RsGenericSerializer::SERIALIZE) && mImage.empty())
		return ;

	RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,mImage,"mImage") ;
}

void RsGxsPostedGroupItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
	RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_DESCR ,mGroup.mDescription,"mGroup.mDescription") ;
}

RsItem *RsGxsPostedSerialiser::create_item(uint16_t service_id,uint8_t item_subtype) const
{
    if(service_id != RS_SERVICE_GXS_TYPE_POSTED)
        return NULL ;

    switch(item_subtype)
    {
    case RS_PKT_SUBTYPE_POSTED_GRP_ITEM: return new RsGxsPostedGroupItem() ;
    case RS_PKT_SUBTYPE_POSTED_POST_ITEM: return new RsGxsPostedPostItem() ;
    default:
		return RsGxsCommentSerialiser::create_item(service_id,item_subtype) ;
    }
}

bool RsGxsPostedPostItem::fromPostedPost(RsPostedPost &post, bool moveImage)
{
	clear();
	
	mPost = post;
	meta = post.mMeta;

	if (moveImage)
	{
		mImage.binData.bin_data = post.mImage.mData;
		mImage.binData.bin_len = post.mImage.mSize;
		post.mImage.shallowClear();
	}
	else
	{
		mImage.binData.setBinData(post.mImage.mData, post.mImage.mSize);
	}

	return true;
}

bool RsGxsPostedPostItem::toPostedPost(RsPostedPost &post, bool moveImage)
{
	post = mPost;
	post.mMeta = meta;

	if (moveImage)
	{
		post.mImage.take((uint8_t *) mImage.binData.bin_data, mImage.binData.bin_len);
		// mImage doesn't have a ShallowClear at the moment!
		mImage.binData.TlvShallowClear();
	}
	else
	{
		post.mImage.copy((uint8_t *) mImage.binData.bin_data, mImage.binData.bin_len);
	}

	return true;
}

void RsGxsPostedPostItem::clear()
{
	mPost.mLink.clear();
	mPost.mNotes.clear();
	mImage.TlvClear();
}
void RsGxsPostedGroupItem::clear()
{
	mGroup.mDescription.clear();
}

