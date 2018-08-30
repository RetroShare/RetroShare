/*******************************************************************************
 * libresapi/api/ChannelsHandler.cpp                                           *
 *                                                                             *
 * LibResAPI: API for local socket server                                      *
 *                                                                             *
 * Copyright 2018 by Gioacchino Mazzurco <gio@eigenlab.org>                    *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include "ChannelsHandler.h"

#include <retroshare/rsgxschannels.h>
#include <util/radix64.h>
#include <util/rstime.h>
#include <algorithm>
#include <time.h>

#include "Operators.h"

namespace resource_api
{

StreamBase& operator << (StreamBase& left, RsGxsFile& file)
{
    left << makeKeyValueReference("name", file.mName)
         << makeKeyValueReference("hash", file.mHash);
    if(left.serialise())
    {
        double size = file.mSize;
        left << makeKeyValueReference("size", size);
    }
    else
    {
        double size = 0;
        left << makeKeyValueReference("size", size);
		file.mSize = size;
    }
    return left;
}

ChannelsHandler::ChannelsHandler(RsGxsChannels& channels): mChannels(channels)
{
	addResourceHandler("list_channels", this,
	                   &ChannelsHandler::handleListChannels);
	addResourceHandler("get_channel_info", this, &ChannelsHandler::handleGetChannelInfo);
	addResourceHandler("get_channel_content", this, &ChannelsHandler::handleGetChannelContent);
	addResourceHandler("toggle_subscribe", this, &ChannelsHandler::handleToggleSubscription);
	addResourceHandler("toggle_auto_download", this, &ChannelsHandler::handleToggleAutoDownload);
	addResourceHandler("toggle_read", this, &ChannelsHandler::handleTogglePostRead);
	addResourceHandler("create_channel", this, &ChannelsHandler::handleCreateChannel);
	addResourceHandler("create_post", this, &ChannelsHandler::handleCreatePost);
}

void ChannelsHandler::handleListChannels(Request& /*req*/, Response& resp)
{
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;
	uint32_t token;

	RsTokenService& tChannels = *mChannels.getTokenService();

	tChannels.requestGroupInfo(token, RS_DEPRECATED_TOKREQ_ANSTYPE, opts);

	time_t start = time(NULL);
	while((tChannels.requestStatus(token) != RsTokenService::COMPLETE)
	      &&(tChannels.requestStatus(token) != RsTokenService::FAILED)
	      &&((time(NULL) < (start+10)))) rstime::rs_usleep(500*1000);

	std::list<RsGroupMetaData> grps;
	if( tChannels.requestStatus(token) == RsTokenService::COMPLETE
	        && mChannels.getGroupSummary(token, grps) )
	{
		for( RsGroupMetaData& grp : grps )
		{
			KeyValueReference<RsGxsGroupId> id("channel_id", grp.mGroupId);
			KeyValueReference<uint32_t> vis_msg("visible_msg_count", grp.mVisibleMsgCount);
			bool own = (grp.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN);
			bool subscribed = IS_GROUP_SUBSCRIBED(grp.mSubscribeFlags);
			std::string lastPostTsStr = std::to_string(grp.mLastPost);
			std::string publishTsStr = std::to_string(grp.mPublishTs);
			resp.mDataStream.getStreamToMember()
			        << id
			        << makeKeyValueReference("name", grp.mGroupName)
			        << makeKeyValueReference("last_post_ts", lastPostTsStr)
			        << makeKeyValueReference("popularity", grp.mPop)
			        << makeKeyValueReference("publish_ts", publishTsStr)
			        << vis_msg
			        << makeKeyValueReference("group_status", grp.mGroupStatus)
			        << makeKeyValueReference("author_id", grp.mAuthorId)
			        << makeKeyValueReference("parent_grp_id", grp.mParentGrpId)
			        << makeKeyValueReference("own", own)
			        << makeKeyValueReference("subscribed", subscribed);
		}

		resp.setOk();
	}
	else resp.setFail("Cant get data from GXS!");
}

void ChannelsHandler::handleGetChannelInfo(Request& req, Response& resp)
{
	std::string chanIdStr;
	req.mStream << makeKeyValueReference("channel_id", chanIdStr);
	if(chanIdStr.empty())
	{
		resp.setFail("channel_id required!");
		return;
	}

	RsGxsGroupId chanId(chanIdStr);
	if(chanId.isNull())
	{
		resp.setFail("Invalid channel_id:" + chanIdStr);
		return;
	}

	bool wantThumbnail = true;
	req.mStream << makeKeyValueReference("want_thumbnail", wantThumbnail);

	std::list<RsGxsGroupId> groupIds; groupIds.push_back(chanId);
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;
	uint32_t token;

	RsTokenService& tChannels = *mChannels.getTokenService();
	tChannels.requestGroupInfo( token, RS_DEPRECATED_TOKREQ_ANSTYPE,
	                            opts, groupIds );

	time_t start = time(NULL);
	while((tChannels.requestStatus(token) != RsTokenService::COMPLETE)
	      &&(tChannels.requestStatus(token) != RsTokenService::FAILED)
	      &&((time(NULL) < (start+10)))) rstime::rs_usleep(500*1000);

	std::vector<RsGxsChannelGroup> grps;
	if( tChannels.requestStatus(token) == RsTokenService::COMPLETE
	        && mChannels.getGroupData(token, grps) )
	{
		for( RsGxsChannelGroup& grp : grps )
		{
			KeyValueReference<RsGxsGroupId> id("channel_id", grp.mMeta.mGroupId);
			KeyValueReference<uint32_t> vis_msg("visible_msg_count", grp.mMeta.mVisibleMsgCount);
			bool own = (grp.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN);
			bool subscribed = IS_GROUP_SUBSCRIBED(grp.mMeta.mSubscribeFlags);
			std::string lastPostTsStr = std::to_string(grp.mMeta.mLastPost);
			std::string publishTsStr = std::to_string(grp.mMeta.mPublishTs);
			StreamBase& rgrp(resp.mDataStream.getStreamToMember());
			rgrp << id
			     << makeKeyValueReference("name", grp.mMeta.mGroupName)
			     << makeKeyValueReference("last_post_ts", lastPostTsStr)
			     << makeKeyValueReference("popularity", grp.mMeta.mPop)
			     << makeKeyValueReference("publish_ts", publishTsStr)
			     << vis_msg
			     << makeKeyValueReference("group_status", grp.mMeta.mGroupStatus)
			     << makeKeyValueReference("author_id", grp.mMeta.mAuthorId)
			     << makeKeyValueReference("parent_grp_id", grp.mMeta.mParentGrpId)
			     << makeKeyValueReference("description", grp.mDescription)
			     << makeKeyValueReference("own", own)
			     << makeKeyValueReference("subscribed", subscribed)
			     << makeKeyValueReference("auto_download", grp.mAutoDownload);

			if(wantThumbnail)
			{
				std::string thumbnail_base64;
				Radix64::encode(grp.mImage.mData, grp.mImage.mSize, thumbnail_base64);
				rgrp << makeKeyValueReference("thumbnail_base64_png", thumbnail_base64);
			}
		}

		resp.setOk();
	}
	else resp.setFail("Cant get data from GXS!");
}

void ChannelsHandler::handleGetChannelContent(Request& req, Response& resp)
{
	std::string chanIdStr;
	req.mStream << makeKeyValueReference("channel_id", chanIdStr);
	if(chanIdStr.empty())
	{
		resp.setFail("channel_id required!");
		return;
	}

	RsGxsGroupId chanId(chanIdStr);
	if(chanId.isNull())
	{
		resp.setFail("Invalid channel_id:" + chanIdStr);
		return;
	}

	std::list<RsGxsGroupId> groupIds; groupIds.push_back(chanId);
	uint32_t token;
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;

	if(! mChannels.getTokenService()->
	        requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, groupIds) )
	{
		resp.setFail("Unknown GXS error!");
		return;
	}

	time_t start = time(NULL);
	while((mChannels.getTokenService()->requestStatus(token) != RsTokenService::COMPLETE)
	      &&(mChannels.getTokenService()->requestStatus(token) != RsTokenService::FAILED)
	      &&((time(NULL) < (start+10)))) rstime::rs_usleep(500*1000);

	std::vector<RsGxsChannelPost> posts;
	std::vector<RsGxsComment> comments;
	if( mChannels.getTokenService()->requestStatus(token) ==
	        RsTokenService::COMPLETE &&
	        mChannels.getPostData(token, posts, comments) )
	{
		for( std::vector<RsGxsChannelPost>::iterator vit = posts.begin();
		     vit != posts.end(); ++vit )
		{
			RsGxsChannelPost& post = *vit;
			RsMsgMetaData& pMeta = post.mMeta;
			resp.mDataStream.getStreamToMember()
			        << makeKeyValueReference("channel_id", pMeta.mGroupId)
			        << makeKeyValueReference("name", pMeta.mMsgName)
			        << makeKeyValueReference("post_id", pMeta.mMsgId)
			        << makeKeyValueReference("parent_id", pMeta.mParentId)
			        << makeKeyValueReference("author_id", pMeta.mAuthorId)
			        << makeKeyValueReference("orig_msg_id", pMeta.mOrigMsgId)
			        << makeKeyValueReference("thread_id", pMeta.mThreadId)
			        << makeKeyValueReference("message", post.mMsg);
		}

		for( std::vector<RsGxsComment>::iterator vit = comments.begin();
		     vit != comments.end(); ++vit )
		{
			RsGxsComment& comment = *vit;
			RsMsgMetaData& cMeta = comment.mMeta;
			std::string scoreStr = std::to_string(comment.mScore);
			resp.mDataStream.getStreamToMember()
			        << makeKeyValueReference("channel_id", cMeta.mGroupId)
			        << makeKeyValueReference("name", cMeta.mMsgName)
			        << makeKeyValueReference("comment_id", cMeta.mMsgId)
			        << makeKeyValueReference("parent_id", cMeta.mParentId)
			        << makeKeyValueReference("author_id", cMeta.mAuthorId)
			        << makeKeyValueReference("orig_msg_id", cMeta.mOrigMsgId)
			        << makeKeyValueReference("thread_id", cMeta.mThreadId)
			        << makeKeyValueReference("score", scoreStr)
			        << makeKeyValueReference("message", comment.mComment);
		}

		resp.setOk();
	}
	else resp.setFail("Cant get data from GXS!");
}

void ChannelsHandler::handleToggleSubscription(Request& req, Response& resp)
{
	std::string chanIdStr;
	bool subscribe = true;

	req.mStream << makeKeyValueReference("channel_id", chanIdStr)
	            << makeKeyValueReference("subscribe", subscribe);

	if(chanIdStr.empty())
	{
		resp.setFail("channel_id required!");
		return;
	}

	RsGxsGroupId chanId(chanIdStr);
	if(chanId.isNull())
	{
		resp.setFail("Invalid channel_id:" + chanIdStr);
		return;
	}

	uint32_t token;
	if(mChannels.subscribeToGroup(token, chanId, subscribe))
	{
		RsTokenService& tChannels = *mChannels.getTokenService();

		time_t start = time(NULL);
		while((tChannels.requestStatus(token) != RsTokenService::COMPLETE)
		      &&(tChannels.requestStatus(token) != RsTokenService::FAILED)
		      &&((time(NULL) < (start+10)))) rstime::rs_usleep(500*1000);

		if(tChannels.requestStatus(token) == RsTokenService::COMPLETE)
			resp.setOk();
		else resp.setFail("Unknown GXS error!");
	}
	else resp.setFail("Unknown GXS error!");
}

void ChannelsHandler::handleCreateChannel(Request& req, Response& resp)
{
	RsGxsChannelGroup chan;
	RsGroupMetaData& cMeta = chan.mMeta;
	std::string authorIdStr;
	std::string thumbnail_base64;

	req.mStream << makeKeyValueReference("author_id", authorIdStr)
	            << makeKeyValueReference("name", cMeta.mGroupName)
	            << makeKeyValueReference("description", chan.mDescription)
	            << makeKeyValueReference("thumbnail_base64_png", thumbnail_base64);

	if(cMeta.mGroupName.empty())
	{
		resp.setFail("Channel name required!");
		return;
	}

	if(thumbnail_base64.empty()) chan.mImage.clear();
	else
	{
		std::vector<uint8_t> png_data = Radix64::decode(thumbnail_base64);
		if(!png_data.empty())
		{
			if(png_data.size() < 8)
			{
				resp.setFail("Decoded thumbnail_base64_png is smaller than 8 byte. This can't be a valid png file!");
				return;
			}
			uint8_t png_magic_number[] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
			if(!std::equal(&png_magic_number[0],&png_magic_number[8],png_data.begin()))
			{
				resp.setFail("Decoded thumbnail_base64_png does not seem to be a png file. (Header is missing magic number)");
				return;
			}
			chan.mImage.copy(png_data.data(), png_data.size());
		}
	}

	if(!authorIdStr.empty()) cMeta.mAuthorId = RsGxsId(authorIdStr);

	// ATM supports creating only public channels
	cMeta.mGroupFlags = GXS_SERV::FLAG_PRIVACY_PUBLIC;

	// I am not sure about those flags I have reversed them with the debugger
	// that gives 520 as value of this member when a channel with default
	// options is created from Qt Gui
	cMeta.mSignFlags = GXS_SERV::MSG_AUTHEN_CHILD_AUTHOR_SIGN |
	        GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_REQUIRED;

	uint32_t token;
	if(mChannels.createGroup(token, chan))
	{
		RsTokenService& tChannels = *mChannels.getTokenService();

		time_t start = time(NULL);
		while((tChannels.requestStatus(token) != RsTokenService::COMPLETE)
		      &&(tChannels.requestStatus(token) != RsTokenService::FAILED)
		      &&((time(NULL) < (start+10)))) rstime::rs_usleep(500*1000);

		if(tChannels.requestStatus(token) == RsTokenService::COMPLETE)
			resp.setOk();
		else resp.setFail("Unknown GXS error!");
	}
	else resp.setFail("Unkown GXS error!");
}

void ChannelsHandler::handleToggleAutoDownload(Request& req, Response& resp)
{

	std::string chanIdStr;
	bool autoDownload = true;

	req.mStream << makeKeyValueReference("channel_id", chanIdStr)
	            << makeKeyValueReference("auto_download", autoDownload);

	if(chanIdStr.empty())
	{
		resp.setFail("channel_id required!");
		return;
	}

	RsGxsGroupId chanId(chanIdStr);
	if(chanId.isNull())
	{
		resp.setFail("Invalid channel_id:" + chanIdStr);
		return;
	}

	if(mChannels.setChannelAutoDownload(chanId, autoDownload))
		resp.setOk();
	else resp.setFail();
}

void ChannelsHandler::handleTogglePostRead(Request& req, Response& resp)
{
	std::string chanIdStr;
	std::string postIdStr;
	bool read = true;

	req.mStream << makeKeyValueReference("channel_id", chanIdStr)
	            << makeKeyValueReference("post_id", postIdStr)
	            << makeKeyValueReference("read", read);

	if(chanIdStr.empty())
	{
		resp.setFail("channel_id required!");
		return;
	}

	RsGxsGroupId chanId(chanIdStr);
	if(chanId.isNull())
	{
		resp.setFail("Invalid channel_id:" + chanIdStr);
		return;
	}

	if(postIdStr.empty())
	{
		resp.setFail("post_id required!");
		return;
	}

	RsGxsMessageId postId(postIdStr);
	if(postId.isNull())
	{
		resp.setFail("Invalid post_id:" + postIdStr);
		return;
	}

	std::cerr << __PRETTY_FUNCTION__ << " " << chanIdStr << " " << postIdStr
	          << " " << read << std::endl;

	uint32_t token;
	mChannels.setMessageReadStatus(token, std::make_pair(chanId,postId), read);

	RsTokenService& tChannels = *mChannels.getTokenService();

	time_t start = time(NULL);
	while((tChannels.requestStatus(token) != RsTokenService::COMPLETE)
	      &&(tChannels.requestStatus(token) != RsTokenService::FAILED)
	      &&((time(NULL) < (start+10)))) rstime::rs_usleep(500*1000);

	if(tChannels.requestStatus(token) == RsTokenService::COMPLETE)
		resp.setOk();
	else resp.setFail("Unknown GXS error!");
}

void ChannelsHandler::handleCreatePost(Request &req, Response &resp)
{
    RsGxsChannelPost post;

	req.mStream << makeKeyValueReference("channel_id", post.mMeta.mGroupId);
    req.mStream << makeKeyValueReference("subject", post.mMeta.mMsgName);
    req.mStream << makeKeyValueReference("message", post.mMsg);

    StreamBase& file_array = req.mStream.getStreamToMember("files");
    while(file_array.hasMore())
    {
        RsGxsFile file;
        file_array.getStreamToMember() << file;
        post.mFiles.push_back(file);
    }

    std::string thumbnail_base64;
    req.mStream << makeKeyValueReference("thumbnail_base64_png", thumbnail_base64);

    if(post.mMeta.mGroupId.isNull())
    {
		resp.setFail("groupd_id is null");
		return;
    }
    if(post.mMeta.mMsgName.empty())
    {
		resp.setFail("subject is empty");
		return;
    }
    if(post.mMsg.empty())
    {
		resp.setFail("msg text is empty");
		return;
    }
    // empty file list is ok, but files have to be valid
    for(std::list<RsGxsFile>::iterator lit = post.mFiles.begin(); lit != post.mFiles.end(); ++lit)
    {
        if(lit->mHash.isNull())
        {
			resp.setFail("at least one file hash is empty");
			return;
        }
        if(lit->mName.empty())
        {
			resp.setFail("at leats one file name is empty");
			return;
        }
        if(lit->mSize == 0)
        {
			resp.setFail("at least one file size is empty");
			return;
        }
    }

    std::vector<uint8_t> png_data = Radix64::decode(thumbnail_base64);
    if(!png_data.empty())
    {
        if(png_data.size() < 8)
        {
			resp.setFail("Decoded thumbnail_base64_png is smaller than 8 byte. This can't be a valid png file!");
			return;
        }
        uint8_t png_magic_number[] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
        if(!std::equal(&png_magic_number[0],&png_magic_number[8],png_data.begin()))
        {
			resp.setFail("Decoded thumbnail_base64_png does not seem to be a png file. (Header is missing magic number)");
			return;
        }
        post.mThumbnail.copy(png_data.data(), png_data.size());
    }

    uint32_t token;
	if(mChannels.createPost(token, post))
	{
		RsTokenService& tChannels = *mChannels.getTokenService();

		time_t start = time(NULL);
		while((tChannels.requestStatus(token) != RsTokenService::COMPLETE)
		      &&(tChannels.requestStatus(token) != RsTokenService::FAILED)
		      &&((time(NULL) < (start+10)))) rstime::rs_usleep(500*1000);

		if(tChannels.requestStatus(token) == RsTokenService::COMPLETE)
			resp.setOk();
		else resp.setFail("Unknown GXS error!");
	}
	else resp.setFail("Unknown GXS error!");
}

} // namespace resource_api
