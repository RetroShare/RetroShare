#include "p3photoserviceV2.h"
#include "serialiser/rsphotov2items.h"

RsPhotoV2 *rsPhotoV2 = NULL;

p3PhotoServiceV2::p3PhotoServiceV2(RsGeneralDataService* gds, RsNetworkExchangeService* nes)
	: RsGenExchange(gds, nes, new RsGxsPhotoSerialiser(), RS_SERVICE_TYPE_PHOTO)
{

}

bool p3PhotoServiceV2::updated()
{
	return false;
}


void p3PhotoServiceV2::groupsChanged(std::list<std::string>& grpIds) {
}


void p3PhotoServiceV2::msgsChanged(
		std::map<std::string, std::vector<std::string> >& msgs)
{

}


RsTokenServiceV2* p3PhotoServiceV2::getTokenService() {

	return RsGenExchange::getTokenService();
}


bool p3PhotoServiceV2::getGroupList(const uint32_t& token,
		std::list<std::string>& groupIds)
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
	return false;
}



bool p3PhotoServiceV2::submitPhoto(RsPhotoPhoto& photo)
{
	return false;
}


