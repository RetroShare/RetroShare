#include "p3photoserviceV2.h"
#include "serialiser/rsphotov2items.h"

RsPhotoV2 *rsPhotoV2 = NULL;

p3PhotoServiceV2::p3PhotoServiceV2(RsGeneralDataService* gds, RsNetworkExchangeService* nes)
	: RsGenExchange(gds, nes, new RsGxsPhotoSerialiser(), RS_SERVICE_TYPE_PHOTO)
{

}

bool p3PhotoServiceV2::updated()
{
        bool changed =  (!mGroupChange.empty() || !mMsgChange.empty());

        std::list<RsGxsGroupId> gL;
        std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > msgs;

        groupsChanged(gL);
        msgsChanged(msgs);

        return changed;
}



void p3PhotoServiceV2::groupsChanged(std::list<RsGxsGroupId>& grpIds)
{
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
					delete *vit;
				}
			}
		}
	}

	return ok;
}


bool p3PhotoServiceV2::submitAlbumDetails(RsPhotoAlbum& album)
{
	RsGxsPhotoAlbumItem* albumItem = new RsGxsPhotoAlbumItem();
	albumItem->album = album;
	albumItem->meta = album.mMeta;
	return RsGenExchange::publishGroup(albumItem);
}



void p3PhotoServiceV2::notifyChanges(std::vector<RsGxsNotify*>& changes)
{
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

bool p3PhotoServiceV2::submitPhoto(RsPhotoPhoto& photo)
{
	RsGxsPhotoPhotoItem* photoItem = new RsGxsPhotoPhotoItem();
	photoItem->photo = photo;
	photoItem->meta = photo.mMeta;

	return RsGenExchange::publishMsg(photoItem);
}


