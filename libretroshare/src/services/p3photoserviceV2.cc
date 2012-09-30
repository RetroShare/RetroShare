#include "p3photoserviceV2.h"
#include "serialiser/rsphotov2items.h"
#include "gxs/rsgxsflags.h"

RsPhotoV2 *rsPhotoV2 = NULL;


const uint32_t RsPhotoV2::FLAG_MSG_TYPE_MASK = 0x000f;
const uint32_t RsPhotoV2::FLAG_MSG_TYPE_PHOTO_POST = 0x0001;
const uint32_t RsPhotoV2::FLAG_MSG_TYPE_PHOTO_COMMENT = 0x0002;




bool RsPhotoThumbnail::copyFrom(const RsPhotoThumbnail &nail)
{
        if (data)
        {
                deleteImage();
        }

        if ((!nail.data) || (nail.size == 0))
        {
                return false;
        }

        size = nail.size;
        type = nail.type;
        data = (uint8_t *) malloc(size);
        memcpy(data, nail.data, size);

        return true;
}

bool RsPhotoThumbnail::deleteImage()
{
        if (data)
        {
                free(data);
                data = NULL;
                size = 0;
                type.clear();
        }
        return true;
}


RsPhotoPhoto::RsPhotoPhoto()
        :mSetFlags(0), mOrder(0), mMode(0), mModFlags(0)
{
        return;
}

RsPhotoAlbum::RsPhotoAlbum()
        :mMode(0), mSetFlags(0), mModFlags(0)
{
        return;
}

RsPhotoComment::RsPhotoComment()
    : mComment(""), mCommentFlag(0) {

}

std::ostream &operator<<(std::ostream &out, const RsPhotoPhoto &photo)
{
        out << "RsPhotoPhoto [ ";
        out << "Title: " << photo.mMeta.mMsgName;
        out << "]";
        return out;
}


std::ostream &operator<<(std::ostream &out, const RsPhotoAlbum &album)
{
        out << "RsPhotoAlbum [ ";
        out << "Title: " << album.mMeta.mGroupName;
        out << "]";
        return out;
}

p3PhotoServiceV2::p3PhotoServiceV2(RsGeneralDataService* gds, RsNetworkExchangeService* nes)
    : RsGenExchange(gds, nes, new RsGxsPhotoSerialiser(), RS_SERVICE_GXSV1_TYPE_PHOTO), mPhotoMutex(std::string("Photo Mutex"))
{

    // create dummy grps

    RsGxsPhotoAlbumItem* item1 = new RsGxsPhotoAlbumItem(), *item2 = new RsGxsPhotoAlbumItem();

    item1->meta.mGroupName = "Dummy Album 1";
    item2->meta.mGroupName = "Dummy Album 2";

    createDummyGroup(item1);
    createDummyGroup(item2);
}

bool p3PhotoServiceV2::updated()
{
    RsStackMutex stack(mPhotoMutex);

    bool changed =  (!mGroupChange.empty() || !mMsgChange.empty());

    return changed;
}



void p3PhotoServiceV2::groupsChanged(std::list<RsGxsGroupId>& grpIds)
{
    RsStackMutex stack(mPhotoMutex);

    while(!mGroupChange.empty())
    {
            RsGxsGroupChange* gc = mGroupChange.back();
            std::list<RsGxsGroupId>& gList = gc->grpIdList;
            std::list<RsGxsGroupId>::iterator lit = gList.begin();
            for(; lit != gList.end(); lit++)
                    grpIds.push_back(*lit);

            mGroupChange.pop_back();
            delete gc;
    }
}


void p3PhotoServiceV2::msgsChanged(
		std::map<RsGxsGroupId, std::vector<RsGxsMessageId> >& msgs)
{
    RsStackMutex stack(mPhotoMutex);

    while(!mMsgChange.empty())
    {
        RsGxsMsgChange* mc = mMsgChange.back();
        msgs = mc->msgChangeMap;
        mMsgChange.pop_back();
        delete mc;
    }
}


RsTokenServiceV2* p3PhotoServiceV2::getTokenService() {

	return RsGenExchange::getTokenService();
}


bool p3PhotoServiceV2::getGroupList(const uint32_t& token,
		std::list<RsGxsGroupId>& groupIds)
{
	return RsGenExchange::getGroupList(token, groupIds);
}


bool p3PhotoServiceV2::getMsgList(const uint32_t& token,
		GxsMsgIdResult& msgIds)
{

	return RsGenExchange::getMsgList(token, msgIds);
}


bool p3PhotoServiceV2::getGroupSummary(const uint32_t& token,
		std::list<RsGroupMetaData>& groupInfo)
{
	return RsGenExchange::getGroupMeta(token, groupInfo);
}


bool p3PhotoServiceV2::getMsgSummary(const uint32_t& token,
		MsgMetaResult& msgInfo)
{
	return RsGenExchange::getMsgMeta(token, msgInfo);
}


bool p3PhotoServiceV2::getAlbum(const uint32_t& token, std::vector<RsPhotoAlbum>& albums)
{
	std::vector<RsGxsGrpItem*> grpData;
	bool ok = RsGenExchange::getGroupData(token, grpData);

	if(ok)
	{
		std::vector<RsGxsGrpItem*>::iterator vit = grpData.begin();

		for(; vit != grpData.end(); vit++)
		{
			RsGxsPhotoAlbumItem* item = dynamic_cast<RsGxsPhotoAlbumItem*>(*vit);
			RsPhotoAlbum album = item->album;
                        item->album.mMeta = item->meta;
			album.mMeta = item->album.mMeta;
			delete item;
			albums.push_back(album);
		}
	}

	return ok;
}


bool p3PhotoServiceV2::getPhoto(const uint32_t& token, PhotoResult& photos)
{
	GxsMsgDataMap msgData;
	bool ok = RsGenExchange::getMsgData(token, msgData);

	if(ok)
	{
		GxsMsgDataMap::iterator mit = msgData.begin();

		for(; mit != msgData.end();  mit++)
		{
			RsGxsGroupId grpId = mit->first;
			std::vector<RsGxsMsgItem*>& msgItems = mit->second;
			std::vector<RsGxsMsgItem*>::iterator vit = msgItems.begin();

			for(; vit != msgItems.end(); vit++)
			{
				RsGxsPhotoPhotoItem* item = dynamic_cast<RsGxsPhotoPhotoItem*>(*vit);

				if(item)
				{
                                        RsPhotoPhoto photo = item->photo;
					photo.mMeta = item->meta;
					photos[grpId].push_back(photo);
					delete item;
				}else
				{
                                    std::cerr << "Not a photo Item, deleting!" << std::endl;
                                    delete *vit;
				}
			}
		}
	}

	return ok;
}

bool p3PhotoServiceV2::getPhotoComment(const uint32_t &token, PhotoCommentResult &comments)
{
        GxsMsgDataMap msgData;
        bool ok = RsGenExchange::getMsgData(token, msgData);

        if(ok)
        {
                GxsMsgDataMap::iterator mit = msgData.begin();

                for(; mit != msgData.end();  mit++)
                {
                        RsGxsGroupId grpId = mit->first;
                        std::vector<RsGxsMsgItem*>& msgItems = mit->second;
                        std::vector<RsGxsMsgItem*>::iterator vit = msgItems.begin();

                        for(; vit != msgItems.end(); vit++)
                        {
                                RsGxsPhotoCommentItem* item = dynamic_cast<RsGxsPhotoCommentItem*>(*vit);

                                if(item)
                                {
                                        RsPhotoComment comment = item->comment;
                                        comment.mMeta = item->meta;
                                        comments[grpId].push_back(comment);
                                        delete item;
                                }else
                                {
                                    std::cerr << "Not a comment Item, deleting!" << std::endl;
                                    delete *vit;
                                }
                        }
                }
        }

        return ok;
}

bool p3PhotoServiceV2::submitAlbumDetails(uint32_t& token, RsPhotoAlbum& album)
{
    RsGxsPhotoAlbumItem* albumItem = new RsGxsPhotoAlbumItem();
    albumItem->album = album;
    albumItem->meta = album.mMeta;
    RsGenExchange::publishGroup(token, albumItem);
    return true;
}



void p3PhotoServiceV2::notifyChanges(std::vector<RsGxsNotify*>& changes)
{

    RsStackMutex stack(mPhotoMutex);

    std::vector<RsGxsNotify*>::iterator vit = changes.begin();

    for(; vit != changes.end(); vit++)
    {
        RsGxsNotify* n = *vit;
        RsGxsGroupChange* gc;
        RsGxsMsgChange* mc;
        if((mc = dynamic_cast<RsGxsMsgChange*>(n)) != NULL)
        {
                mMsgChange.push_back(mc);
        }
        else if((gc = dynamic_cast<RsGxsGroupChange*>(n)) != NULL)
        {
                mGroupChange.push_back(gc);
        }
        else
        {
                delete n;
        }
    }
}

bool p3PhotoServiceV2::submitPhoto(uint32_t& token, RsPhotoPhoto& photo)
{
	RsGxsPhotoPhotoItem* photoItem = new RsGxsPhotoPhotoItem();
	photoItem->photo = photo;
	photoItem->meta = photo.mMeta;
        photoItem->meta.mMsgFlags = FLAG_MSG_TYPE_PHOTO_POST;

        RsGenExchange::publishMsg(token, photoItem);
        return true;
}

bool p3PhotoServiceV2::submitComment(uint32_t &token, RsPhotoComment &comment)
{
    RsGxsPhotoCommentItem* commentItem = new RsGxsPhotoCommentItem();
    commentItem->comment = comment;
    commentItem->meta = comment.mMeta;
    commentItem->meta.mMsgFlags = FLAG_MSG_TYPE_PHOTO_COMMENT;

    RsGenExchange::publishMsg(token, commentItem);
    return true;
}

bool p3PhotoServiceV2::acknowledgeMsg(const uint32_t& token,
		std::pair<RsGxsGroupId, RsGxsMessageId>& msgId)
{
	return RsGenExchange::acknowledgeTokenMsg(token, msgId);
}


bool p3PhotoServiceV2::acknowledgeGrp(const uint32_t& token,
		RsGxsGroupId& grpId)
{
	return RsGenExchange::acknowledgeTokenGrp(token, grpId);
}

bool p3PhotoServiceV2::subscribeToAlbum(uint32_t &token, const RsGxsGroupId &grpId, bool subscribe)
{
    if(subscribe)
        RsGenExchange::setGroupSubscribeFlag(token, grpId, GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED);
    else
        RsGenExchange::setGroupSubscribeFlag(token, grpId, ~GXS_SERV::GROUP_SUBSCRIBE_MASK);

    return true;
}


