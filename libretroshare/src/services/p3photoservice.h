/*
 * libretroshare/src/services: p3photoservice.h
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2012-2012 by Robert Fernie.
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

#ifndef P3_PHOTO_SERVICE_HEADER
#define P3_PHOTO_SERVICE_HEADER

#include "services/p3gxsservice.h"
#include "retroshare/rsphoto.h"

#include <map>
#include <string>

/* 
 * Photo Service
 *
 * This is an example service for the new cache system.
 * For the moment, it will only hold data passed to it from the GUI.
 * and spew that back when asked....
 *
 * We are doing it like this - so we can check the required interface functionality.
 *
 * Expect it won't take long before it'll be properly linked into the backend!
 *
 * This will be transformed into a Plugin Service, once the basics have been worked out.
 *
 */


class PhotoDataProxy: public GxsDataProxy
{
	public:

	bool addAlbum(const RsPhotoAlbum &album);
	bool addPhoto(const RsPhotoPhoto &photo);

	bool getAlbum(const std::string &id, RsPhotoAlbum &album);
	bool getPhoto(const std::string &id, RsPhotoPhoto &photo);

        /* These Functions must be overloaded to complete the service */
virtual bool convertGroupToMetaData(void *groupData, RsGroupMetaData &meta);
virtual bool convertMsgToMetaData(void *groupData, RsMsgMetaData &meta);

};



class p3PhotoService: public p3GxsDataService, public RsPhoto
{
	public:

	p3PhotoService(uint16_t type);

virtual int	tick();

	public:

// NEW INTERFACE.
/************* Extern Interface *******/

        /* changed? */
virtual bool updated();

       /* Data Requests */
virtual bool requestGroupInfo(     uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<std::string> &groupIds);
virtual bool requestMsgInfo(       uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<std::string> &groupIds);
virtual bool requestMsgRelatedInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<std::string> &msgIds);

        /* Generic Lists */
virtual bool getGroupList(         const uint32_t &token, std::list<std::string> &groupIds);
virtual bool getMsgList(           const uint32_t &token, std::list<std::string> &msgIds);

        /* Generic Summary */
virtual bool getGroupSummary(      const uint32_t &token, std::list<RsGroupMetaData> &groupInfo);
virtual bool getMsgSummary(        const uint32_t &token, std::list<RsMsgMetaData> &msgInfo);

        /* Actual Data -> specific to Interface */
        /* Specific Service Data */
virtual bool getAlbum(const uint32_t &token, RsPhotoAlbum &album);
virtual bool getPhoto(const uint32_t &token, RsPhotoPhoto &photo);

        /* Poll */
virtual uint32_t requestStatus(const uint32_t token);

        /* Cancel Request */
virtual bool cancelRequest(const uint32_t &token);

        //////////////////////////////////////////////////////////////////////////////
virtual bool setMessageStatus(const std::string &msgId, const uint32_t status, const uint32_t statusMask);
virtual bool setGroupStatus(const std::string &groupId, const uint32_t status, const uint32_t statusMask);
virtual bool setGroupSubscribeFlags(const std::string &groupId, uint32_t subscribeFlags, uint32_t subscribeMask);
virtual bool setMessageServiceString(const std::string &msgId, const std::string &str);
virtual bool setGroupServiceString(const std::string &grpId, const std::string &str);

virtual bool groupRestoreKeys(const std::string &groupId);
virtual bool groupShareKeys(const std::string &groupId, std::list<std::string>& peers);


/* details are updated in album - to choose Album ID, and storage path */
virtual bool submitAlbumDetails(uint32_t &token, RsPhotoAlbum &album, bool isNew);
virtual bool submitPhoto(uint32_t &token, RsPhotoPhoto &photo, bool isNew);



	private:

std::string genRandomId();

	PhotoDataProxy *mPhotoProxy;

	RsMutex mPhotoMtx;
	bool mUpdated;


};

#endif 
