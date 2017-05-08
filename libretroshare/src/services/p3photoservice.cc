#include "p3photoservice.h"
#include "rsitems/rsphotoitems.h"
#include "retroshare/rsgxsflags.h"

RsPhoto *rsPhoto = NULL;


const uint32_t RsPhoto::FLAG_MSG_TYPE_MASK = 0x000f;
const uint32_t RsPhoto::FLAG_MSG_TYPE_PHOTO_POST = 0x0001;
const uint32_t RsPhoto::FLAG_MSG_TYPE_PHOTO_COMMENT = 0x0002;




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
        data = (uint8_t *) rs_malloc(size);
        
        if(data == NULL)
            return false ;
        
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

RsPhotoComment::RsPhotoComment(const RsGxsPhotoCommentItem &comment)
    : mComment(""), mCommentFlag(0) {

    *this = comment.comment;
    (*this).mMeta = comment.meta;

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

p3PhotoService::p3PhotoService(RsGeneralDataService* gds, RsNetworkExchangeService* nes, RsGixs* gixs)
    : RsGenExchange(gds, nes, new RsGxsPhotoSerialiser(), RS_SERVICE_GXS_TYPE_PHOTO, gixs, photoAuthenPolicy()),
    mPhotoMutex(std::string("Photo Mutex"))
{
}

const std::string GXS_PHOTO_APP_NAME = "gxsphoto";
const uint16_t GXS_PHOTO_APP_MAJOR_VERSION  =       1;
const uint16_t GXS_PHOTO_APP_MINOR_VERSION  =       0;
const uint16_t GXS_PHOTO_MIN_MAJOR_VERSION  =       1;
const uint16_t GXS_PHOTO_MIN_MINOR_VERSION  =       0;

RsServiceInfo p3PhotoService::getServiceInfo()
{
        return RsServiceInfo(RS_SERVICE_GXS_TYPE_PHOTO,
                GXS_PHOTO_APP_NAME,
                GXS_PHOTO_APP_MAJOR_VERSION,
                GXS_PHOTO_APP_MINOR_VERSION,
                GXS_PHOTO_MIN_MAJOR_VERSION,
                GXS_PHOTO_MIN_MINOR_VERSION);
}



uint32_t p3PhotoService::photoAuthenPolicy()
{
        uint32_t policy = 0;
        uint8_t flag = 0;

        flag = GXS_SERV::MSG_AUTHEN_ROOT_PUBLISH_SIGN | GXS_SERV::MSG_AUTHEN_CHILD_AUTHOR_SIGN;
        RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::PUBLIC_GRP_BITS);

        flag |= GXS_SERV::MSG_AUTHEN_CHILD_PUBLISH_SIGN;
        RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::RESTRICTED_GRP_BITS);
        RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::PRIVATE_GRP_BITS);

        flag = GXS_SERV::GRP_OPTION_AUTHEN_AUTHOR_SIGN;
        RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::GRP_OPTION_BITS);

        return policy;
}

bool p3PhotoService::updated()
{
    RsStackMutex stack(mPhotoMutex);

    bool changed =  (!mGroupChange.empty() || !mMsgChange.empty());

    return changed;
}

void p3PhotoService::service_tick()
{

}



void p3PhotoService::groupsChanged(std::list<RsGxsGroupId>& grpIds)
{
    RsStackMutex stack(mPhotoMutex);

    while(!mGroupChange.empty())
    {
            RsGxsGroupChange* gc = mGroupChange.back();
            std::list<RsGxsGroupId>& gList = gc->mGrpIdList;
            std::list<RsGxsGroupId>::iterator lit = gList.begin();
            for(; lit != gList.end(); ++lit)
                    grpIds.push_back(*lit);

            mGroupChange.pop_back();
            delete gc;
    }
}


void p3PhotoService::msgsChanged(
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


RsTokenService* p3PhotoService::getTokenService() {

	return RsGenExchange::getTokenService();
}


bool p3PhotoService::getGroupList(const uint32_t& token,
		std::list<RsGxsGroupId>& groupIds)
{
	return RsGenExchange::getGroupList(token, groupIds);
}


bool p3PhotoService::getMsgList(const uint32_t& token,
		GxsMsgIdResult& msgIds)
{

	return RsGenExchange::getMsgList(token, msgIds);
}


bool p3PhotoService::getGroupSummary(const uint32_t& token,
		std::list<RsGroupMetaData>& groupInfo)
{
	return RsGenExchange::getGroupMeta(token, groupInfo);
}


bool p3PhotoService::getMsgSummary(const uint32_t& token,
		MsgMetaResult& msgInfo)
{
	return RsGenExchange::getMsgMeta(token, msgInfo);
}


bool p3PhotoService::getAlbum(const uint32_t& token, std::vector<RsPhotoAlbum>& albums)
{
	std::vector<RsGxsGrpItem*> grpData;
	bool ok = RsGenExchange::getGroupData(token, grpData);

	if(ok)
	{
		std::vector<RsGxsGrpItem*>::iterator vit = grpData.begin();

		for(; vit != grpData.end(); ++vit)
		{
			RsGxsPhotoAlbumItem* item = dynamic_cast<RsGxsPhotoAlbumItem*>(*vit);
			if (item)
			{
				RsPhotoAlbum album = item->album;
				item->album.mMeta = item->meta;
				album.mMeta = item->album.mMeta;
				delete item;
				albums.push_back(album);
			}
			else
			{
				std::cerr << "Not a RsGxsPhotoAlbumItem, deleting!" << std::endl;
				delete *vit;
			}
		}
	}

	return ok;
}

bool p3PhotoService::getPhoto(const uint32_t& token, PhotoResult& photos)
{
	GxsMsgDataMap msgData;
	bool ok = RsGenExchange::getMsgData(token, msgData);

	if(ok)
	{
		GxsMsgDataMap::iterator mit = msgData.begin();

		for(; mit != msgData.end(); ++mit)
		{
			RsGxsGroupId grpId = mit->first;
			std::vector<RsGxsMsgItem*>& msgItems = mit->second;
			std::vector<RsGxsMsgItem*>::iterator vit = msgItems.begin();

			for(; vit != msgItems.end(); ++vit)
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

bool p3PhotoService::getPhotoComment(const uint32_t &token, PhotoCommentResult &comments)
{
    GxsMsgDataMap msgData;
    bool ok = RsGenExchange::getMsgData(token, msgData);

    if(ok)
    {
        GxsMsgDataMap::iterator mit = msgData.begin();

        for(; mit != msgData.end(); ++mit)
        {
            RsGxsGroupId grpId = mit->first;
            std::vector<RsGxsMsgItem*>& msgItems = mit->second;
            std::vector<RsGxsMsgItem*>::iterator vit = msgItems.begin();

            for(; vit != msgItems.end(); ++vit)
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

RsPhotoComment& RsPhotoComment::operator=(const RsGxsPhotoCommentItem& comment)
{
    *this = comment.comment;
    return *this;
}

bool p3PhotoService::getPhotoRelatedComment(const uint32_t &token, PhotoRelatedCommentResult &comments)
{

    return RsGenExchange::getMsgRelatedDataT<RsGxsPhotoCommentItem, RsPhotoComment>(token, comments);

}

bool p3PhotoService::submitAlbumDetails(uint32_t& token, RsPhotoAlbum& album)
{
    RsGxsPhotoAlbumItem* albumItem = new RsGxsPhotoAlbumItem();
    albumItem->album = album;
    albumItem->meta = album.mMeta;
    RsGenExchange::publishGroup(token, albumItem);
    return true;
}



void p3PhotoService::notifyChanges(std::vector<RsGxsNotify*>& changes)
{

    RsStackMutex stack(mPhotoMutex);

    std::vector<RsGxsNotify*>::iterator vit = changes.begin();

    for(; vit != changes.end(); ++vit)
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

bool p3PhotoService::submitPhoto(uint32_t& token, RsPhotoPhoto& photo)
{
	RsGxsPhotoPhotoItem* photoItem = new RsGxsPhotoPhotoItem();
	photoItem->photo = photo;
	photoItem->meta = photo.mMeta;
        photoItem->meta.mMsgFlags = FLAG_MSG_TYPE_PHOTO_POST;

        RsGenExchange::publishMsg(token, photoItem);
        return true;
}

bool p3PhotoService::submitComment(uint32_t &token, RsPhotoComment &comment)
{
    RsGxsPhotoCommentItem* commentItem = new RsGxsPhotoCommentItem();
    commentItem->comment = comment;
    commentItem->meta = comment.mMeta;
    commentItem->meta.mMsgFlags = FLAG_MSG_TYPE_PHOTO_COMMENT;

    RsGenExchange::publishMsg(token, commentItem);
    return true;
}

bool p3PhotoService::acknowledgeMsg(const uint32_t& token,
		std::pair<RsGxsGroupId, RsGxsMessageId>& msgId)
{
	return RsGenExchange::acknowledgeTokenMsg(token, msgId);
}


bool p3PhotoService::acknowledgeGrp(const uint32_t& token,
		RsGxsGroupId& grpId)
{
	return RsGenExchange::acknowledgeTokenGrp(token, grpId);
}

bool p3PhotoService::subscribeToAlbum(uint32_t &token, const RsGxsGroupId &grpId, bool subscribe)
{
    if(subscribe)
        RsGenExchange::setGroupSubscribeFlags(token, grpId, GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED, GXS_SERV::GROUP_SUBSCRIBE_MASK);
    else
        RsGenExchange::setGroupSubscribeFlags(token, grpId, 0, GXS_SERV::GROUP_SUBSCRIBE_MASK);

    return true;
}


