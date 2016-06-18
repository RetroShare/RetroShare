#include "ChannelsHandler.h"

#include <retroshare/rsgxschannels.h>
#include <util/radix64.h>
#include <algorithm>
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

ChannelsHandler::ChannelsHandler(RsGxsChannels *channels):
    mChannels(channels)
{
    addResourceHandler("create_post", this, &ChannelsHandler::handleCreatePost);
}

ResponseTask* ChannelsHandler::handleCreatePost(Request &req, Response &resp)
{
    RsGxsChannelPost post;

    req.mStream << makeKeyValueReference("group_id", post.mMeta.mGroupId);
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
        return 0;
    }
    if(post.mMeta.mMsgName.empty())
    {
        resp.setFail("subject is empty");
        return 0;
    }
    if(post.mMsg.empty())
    {
        resp.setFail("msg text is empty");
        return 0;
    }
    // empty file list is ok, but files have to be valid
    for(std::list<RsGxsFile>::iterator lit = post.mFiles.begin(); lit != post.mFiles.end(); ++lit)
    {
        if(lit->mHash.isNull())
        {
            resp.setFail("at least one file hash is empty");
            return 0;
        }
        if(lit->mName.empty())
        {
            resp.setFail("at leats one file name is empty");
            return 0;
        }
        if(lit->mSize == 0)
        {
            resp.setFail("at least one file size is empty");
            return 0;
        }
    }

    std::vector<uint8_t> png_data = Radix64::decode(thumbnail_base64);
    if(!png_data.empty())
    {
        if(png_data.size() < 8)
        {
            resp.setFail("Decoded thumbnail_base64_png is smaller than 8 byte. This can't be a valid png file!");
            return 0;
        }
        uint8_t png_magic_number[] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
        if(!std::equal(&png_magic_number[0],&png_magic_number[8],png_data.begin()))
        {
            resp.setFail("Decoded thumbnail_base64_png does not seem to be a png file. (Header is missing magic number)");
            return 0;
        }
        post.mThumbnail.copy(png_data.data(), png_data.size());
    }

    uint32_t token;
    mChannels->createPost(token, post);
    // TODO: grp creation acknowledge
    return 0;
}

} // namespace resource_api
