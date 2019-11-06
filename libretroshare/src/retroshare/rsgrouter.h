/*******************************************************************************
 * libretroshare/src/retroshare: rsgrouter.h                                   *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2013  Cyril Soler <retroshare.project@gmail.com>              *
 * Copyright (C) 2019  Gioacchino Mazzurco <gio@eigenlab.org>                  *
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
#pragma once

#include "util/rsdir.h"
#include "util/rsdeprecate.h"
#include "retroshare/rsids.h"
#include "retroshare/rsgxsifacetypes.h"
#include "rsitems/rsserviceids.h"

RS_DEPRECATED_FOR(RsGxsId)
typedef RsGxsId GRouterKeyId;

/**
 * @note Some items use this type, when migrating to RsServiceType (2 bytes),
 * special care should be taken to do it in a way which doesn't break
 * retrocompatibility
 */
RS_DEPRECATED_FOR(RsServiceType)
typedef uint32_t GRouterServiceId;

typedef uint64_t GRouterMsgPropagationId;

class GRouterClientService ;
struct RsGRouterGenericDataItem;

class RsGRouter
{
public:
    // This is the interface file for the global router service.
    //
    struct GRouterRoutingCacheInfo
    {
        GRouterMsgPropagationId mid ;
        std::set<RsPeerId>      local_origin;
        GRouterKeyId            destination ;
        rstime_t                  routing_time;
        rstime_t                  last_tunnel_attempt_time;
        rstime_t                  last_sent_time;
        bool                    receipt_available ;
        uint32_t                duplication_factor ;
        uint32_t                data_status ;
        uint32_t                tunnel_status ;
        uint32_t                data_size ;
        Sha1CheckSum            item_hash ;
    };

	struct GRouterPublishedKeyInfo
	{
		std::string description_string;
		RsGxsId authentication_key;
		uint32_t service_id; // TODO: use RsServiceType instead
	};

    struct GRouterRoutingMatrixInfo
    {
        // Probabilities of reaching a given key for each friend.
        // This concerns all known keys.
        //
        std::map<GRouterKeyId, std::vector<float> > per_friend_probabilities ;

        // List of friend ids in the same order. Should roughly correspond to the friends that are currently online.
        //
        std::vector<RsPeerId> friend_ids ;

        // List of own published keys, with associated service ID
        //
        std::map<Sha1CheckSum,GRouterPublishedKeyInfo> published_keys ;
    };

    //===================================================//
    //                  Debugging info                   //
    //===================================================//

    virtual bool getRoutingCacheInfo(std::vector<GRouterRoutingCacheInfo>& infos) =0;
    virtual bool getRoutingMatrixInfo(GRouterRoutingMatrixInfo& info) =0;

    // retrieve the routing probabilities

    //===================================================//
    //         Communication to other services.          //
    //===================================================//

	virtual bool sendData(
	        const RsGxsId& destination, RsServiceType client_id,
	        const uint8_t* data, uint32_t data_size, const RsGxsId& signing_id,
	        GRouterMsgPropagationId& id ) = 0;
	virtual bool cancel(GRouterMsgPropagationId mid) =0;

	virtual bool registerKey(
	        const RsGxsId& authentication_id, RsServiceType client_id,
	        const std::string& description_string ) = 0;
	virtual bool unregisterKey(const RsGxsId& key_id, RsServiceType sid) = 0;

    //===================================================//
    //         Routage feedback from other services      //
    //===================================================//

    virtual void addRoutingClue(const GRouterKeyId& destination, const RsPeerId& source) =0;
};

// To access the GRouter from anywhere
//
extern RsGRouter *rsGRouter ;
