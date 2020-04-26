/*******************************************************************************
 * libretroshare/src/retroshare: rsgxstrans.h                                  *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2016-2019  Gioacchino Mazzurco <gio@eigenlab.org>             *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License version 3 as    *
 * published by the Free Software Foundation.                                  *
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
#pragma once

#include "retroshare/rstokenservice.h"
#include "retroshare/rsgxsifacehelper.h"
#include "retroshare/rsgxscommon.h"
#include "rsitems/rsserviceids.h"

/** Subservices identifiers (like port for TCP)
 * @deprecated pay special attention fixing this as it may break
 * retro-compatibility */
enum  class RS_DEPRECATED_FOR(RsServiceType) GxsTransSubServices : uint16_t
{
	UNKNOWN         = 0x00,
	TEST_SERVICE    = 0x01,
	P3_MSG_SERVICE  = 0x02,
	P3_CHAT_SERVICE = 0x03
};

/// Values must fit into uint8_t
enum class GxsTransItemsSubtypes : uint8_t
{
	GXS_TRANS_SUBTYPE_MAIL               = 0x01,
	GXS_TRANS_SUBTYPE_RECEIPT            = 0x02,
	GXS_TRANS_SUBTYPE_GROUP              = 0x03,
	OUTGOING_RECORD_ITEM_deprecated      = 0x04,
	OUTGOING_RECORD_ITEM                 = 0x05
};

enum class GxsTransSendStatus : uint8_t
{
	UNKNOWN                       = 0x00,
	PENDING_PROCESSING            = 0x01,
	PENDING_PREFERRED_GROUP       = 0x02,
	PENDING_RECEIPT_CREATE        = 0x03,
	PENDING_RECEIPT_SIGNATURE     = 0x04,
	PENDING_SERIALIZATION         = 0x05,
	PENDING_PAYLOAD_CREATE        = 0x06,
	PENDING_PAYLOAD_ENCRYPT       = 0x07,
	PENDING_PUBLISH               = 0x08,
	/** This will be useful so the user can know if the mail reached at least
	 * some friend node, in case of internet connection interruption */
	//PENDING_TRANSFER,
	PENDING_RECEIPT_RECEIVE       = 0x09,
	/// Records with status >= RECEIPT_RECEIVED get deleted
	RECEIPT_RECEIVED              = 0x0a,
	FAILED_RECEIPT_SIGNATURE      = 0xf0,
	FAILED_ENCRYPTION             = 0xf1,
	FAILED_TIMED_OUT              = 0xf2
};

typedef uint64_t RsGxsTransId;

class RsGxsTransGroup: public RsGxsGenericGroupData
{
};

class RsGxsTransMsg
{
public:
	RsGxsTransMsg() : size(0),data(NULL) {}
	virtual ~RsGxsTransMsg() { free(data) ; }

public:
	RsMsgMetaData mMeta;

	uint32_t size ;
	uint8_t *data ;
};

struct RsGxsTransOutgoingRecord
{
	GxsTransSendStatus status;
	RsGxsId recipient;
	RsGxsTransId trans_id;

	GxsTransSubServices client_service;

	uint32_t data_size ;
	Sha1CheckSum data_hash ;
	uint32_t send_TS ;
	RsGxsGroupId group_id ;
};

class RsGxsTransGroupStatistics: public GxsGroupStatistic
{
public:
	RsGxsTransGroupStatistics()
	{
		last_publish_TS = 0;
		popularity = 0;
		subscribed = false;
	}

	void addMessageMeta(const RsGxsGroupId& grp,const RsMsgMetaData& meta)
	{
		messages_metas[meta.mMsgId] = meta ;
		last_publish_TS = std::max(last_publish_TS,meta.mPublishTs) ;
		mGrpId = grp ;
	}

	bool subscribed ;
	int  popularity ;

	rstime_t last_publish_TS;

    std::map<RsGxsMessageId,RsMsgMetaData> messages_metas ;
};


/// RetroShare GxsTrans asyncronous redundant small mail trasport on top of GXS
///
class RsGxsTrans: public RsGxsIfaceHelper
{
public:
	class GxsTransStatistics
	{
	public:
		GxsTransStatistics() {}

		RsGxsGroupId prefered_group_id ;
		std::vector<RsGxsTransOutgoingRecord> outgoing_records;
	};

	RsGxsTrans(RsGxsIface& gxs) : RsGxsIfaceHelper(gxs) {}

	virtual ~RsGxsTrans() {}

	virtual bool getDataStatistics(GxsTransStatistics& stats)=0;
	virtual bool getGroupStatistics(std::map<RsGxsGroupId,RsGxsTransGroupStatistics>& stats) =0;
};

extern RsGxsTrans *rsGxsTrans ;
