/*******************************************************************************
 * libresapi/api/ServiceControlHandler.cpp                                     *
 *                                                                             *
 * LibResAPI: API for local socket server                                      *
 *                                                                             *
 * Copyright 2018 by Retroshare Team <retroshare.project@gmail.com>            *
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
#include "ServiceControlHandler.h"

#include "retroshare/rsservicecontrol.h"

#include "Operators.h"

#include <stdlib.h>

namespace resource_api
{
// maybe move to another place later
// need more generic operators for list, vector, map
template<class T>
void setToStream(StreamBase& stream, std::set<T>& set)
{
    if(stream.serialise())
    {
        for(typename std::set<T>::iterator sit = set.begin(); sit != set.end(); sit++)
        {
            T item = *sit;
            stream << makeValueReference(item);
        }
    }
    else
    {
        while(stream.hasMore())
        {
            T item;
            stream << makeValueReference(item);
            set.insert(item);
        }
    }
}

void servicePermissionToStream(StreamBase& stream, RsServicePermissions& perm)
{
    stream << makeKeyValueReference("service_id", perm.mServiceId)
           << makeKeyValueReference("service_name", perm.mServiceName)
           << makeKeyValueReference("default_allowed", perm.mDefaultAllowed)
              ;
    setToStream(stream.getStreamToMember("peers_allowed"), perm.mPeersAllowed);
    setToStream(stream.getStreamToMember("peers_denied"), perm.mPeersDenied);
}

ServiceControlHandler::ServiceControlHandler(RsServiceControl* control):
    mRsServiceControl(control)
{
    addResourceHandler("*", this, &ServiceControlHandler::handleWildcard);
    addResourceHandler("user", this, &ServiceControlHandler::handleUser);
}

void ServiceControlHandler::handleWildcard(Request &req, Response &resp)
{
    bool ok = false;
    if(!req.mPath.empty())
    {
    }
    else
    {
        // no more path element
        if(req.isGet())
        {
            // list all peers
            ok = true;
            RsPeerServiceInfo psi;
            ok &= mRsServiceControl->getOwnServices(psi);
            for(std::map<uint32_t, RsServiceInfo>::iterator mit = psi.mServiceList.begin(); mit != psi.mServiceList.end(); mit++)
            {
                RsServicePermissions perms;
                ok &= mRsServiceControl->getServicePermissions(mit->first, perms);
                if(ok)
                {
                    servicePermissionToStream(resp.mDataStream.getStreamToMember(), perms);
                }
            }
        }
        else if(req.isPut())
        {
            // change service default

            std::string serviceidtext;
            bool enabled;

            req.mStream << makeKeyValueReference("service_id", serviceidtext)
                        << makeKeyValueReference("default_allowed", enabled);

            RsServicePermissions serv_perms ;
            //uint32_t serviceid = fromString<uint32_t>(serviceidtext);
            uint32_t serviceid  = atoi(serviceidtext.c_str());
            if (serviceid == 0) {
                resp.setFail("service_id missed");
                return;
            }

            if(!rsServiceControl->getServicePermissions(serviceid, serv_perms)){
                resp.setFail("service_id " + serviceidtext + " is invalid");
                return;
            }

            serv_perms.mDefaultAllowed = enabled;
            if(serv_perms.mDefaultAllowed)
            {
                serv_perms.mPeersDenied.clear() ;
            }
            else
            {
                serv_perms.mPeersAllowed.clear() ;
            }

            ok = rsServiceControl->updateServicePermissions(serviceid,serv_perms);
            if (!ok) {
                resp.setFail("updateServicePermissions failed");
                return;
            }
        }
    }
    if(ok)
    {
        resp.setOk();
    }
    else
    {
        resp.setFail();
    }
}


void ServiceControlHandler::handleUser(Request& req, Response& resp){
    // no get, only put (post) to allow user or delete to remove user

    std::string serviceidtext;
    std::string peeridtext;
    bool enabled;
    bool ok;

    req.mStream << makeKeyValueReference("service_id", serviceidtext)
                << makeKeyValueReference("peer_id", peeridtext)
                << makeKeyValueReference("enabled", enabled);

    RsPeerId peer_id(peeridtext);

    if (peer_id.isNull()) {
        resp.setFail("peer_id missing or not found");
        return;
    }

    RsServicePermissions serv_perms ;
    uint32_t serviceid  = atoi(serviceidtext.c_str());
    if (serviceid == 0) {
        resp.setFail("service_id missed");
        return;
    }

    if(!rsServiceControl->getServicePermissions(serviceid, serv_perms)){
        resp.setFail("service_id " + serviceidtext + " is invalid");
        return;
    }

    if(req.isPut())
    {
        if (enabled && !serv_perms.peerHasPermission(peer_id))
        {
            serv_perms.setPermission(peer_id);
        } else  if (!enabled && serv_perms.peerHasPermission(peer_id)){
            serv_perms.resetPermission(peer_id);
        } else {
            //nothing todo
            resp.setOk();
            return;
        }

    } else {
        resp.setFail("only POST supported.");
        return;
    }
    ok = rsServiceControl->updateServicePermissions(serviceid,serv_perms);
    if (!ok) {
        resp.setFail("updateServicePermissions failed");
        return;
    }

    resp.setOk();
}

} // namespace resource_api
