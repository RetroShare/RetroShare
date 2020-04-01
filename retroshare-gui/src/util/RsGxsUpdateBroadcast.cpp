/*******************************************************************************
 * util/RsGxsUpdateBroadcast.cpp                                               *
 *                                                                             *
 * Copyright (C) 2014-2020 Retroshare Team <contact@retroshare.cc>             *
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

#include <QMap>

#include "RsGxsUpdateBroadcast.h"
#include "gui/notifyqt.h"

#include <retroshare/rsgxsifacehelper.h>

//#define DEBUG_GXS_BROADCAST 1

// previously gxs allowed only one event consumer to poll for changes
// this required a single broadcast instance per service
// now the update notify works through rsnotify and notifyqt
// so the single instance per service is not really needed anymore

static QMap<RsGxsIfaceHelper*, RsGxsUpdateBroadcast*> updateBroadcastMap;

RsGxsUpdateBroadcast::RsGxsUpdateBroadcast(RsGxsIfaceHelper *ifaceImpl) :
    QObject(nullptr), mIfaceImpl(ifaceImpl), mEventHandlerId(0)
{
	/* No need of postToObject here as onChangesReceived just emit signals
	 * internally */
	rsEvents->registerEventsHandler(
	            [this](std::shared_ptr<const RsEvent> event)
	{ onChangesReceived(*dynamic_cast<const RsGxsChanges*>(event.get())); },
	            mEventHandlerId, RsEventType::GXS_CHANGES );
}

RsGxsUpdateBroadcast::~RsGxsUpdateBroadcast()
{
    rsEvents->unregisterEventsHandler(mEventHandlerId);
}

void RsGxsUpdateBroadcast::cleanup()
{
	QMap<RsGxsIfaceHelper*, RsGxsUpdateBroadcast*>::iterator it;
	for (it = updateBroadcastMap.begin(); it != updateBroadcastMap.end(); ++it) {
		delete(it.value());
	}

	updateBroadcastMap.clear();
}

RsGxsUpdateBroadcast *RsGxsUpdateBroadcast::get(RsGxsIfaceHelper *ifaceImpl)
{
	QMap<RsGxsIfaceHelper*, RsGxsUpdateBroadcast*>::iterator it = updateBroadcastMap.find(ifaceImpl);
	if (it != updateBroadcastMap.end()) {
		return it.value();
	}

	RsGxsUpdateBroadcast *updateBroadcast = new RsGxsUpdateBroadcast(ifaceImpl);
	updateBroadcastMap.insert(ifaceImpl, updateBroadcast);

	return updateBroadcast;
}

void RsGxsUpdateBroadcast::onChangesReceived(const RsGxsChanges& changes)
{
#ifdef DEBUG_GXS_BROADCAST
    std::cerr << "onChangesReceived()" << std::endl;
    
     {
        std::cerr << "Received changes for service " << (void*)changes.mService << ",  expecting service " << (void*)mIfaceImpl->getTokenService() << std::endl;
        for(std::list<RsGxsGroupId>::const_iterator it(changes.mGrps.begin());it!=changes.mGrps.end();++it)
            std::cerr << "[GRP CHANGE]    grp id: " << *it << std::endl;
        for(std::list<RsGxsGroupId>::const_iterator it(changes.mGrpsMeta.begin());it!=changes.mGrpsMeta.end();++it)
            std::cerr << "[GRP CHANGE]    grp meta: " << *it << std::endl;
        for(std::map<RsGxsGroupId,std::vector<RsGxsMessageId> >::const_iterator it(changes.mMsgs.begin());it!=changes.mMsgs.end();++it) 
            for(uint32_t i=0;i<it->second.size();++i)
                std::cerr << "[MSG CHANGE]    grp id: " << it->first << ". Msg ID " << it->second[i] << std::endl;
        for(std::map<RsGxsGroupId,std::vector<RsGxsMessageId> >::const_iterator it(changes.mMsgsMeta.begin());it!=changes.mMsgsMeta.end();++it) 
            for(uint32_t i=0;i<it->second.size();++i)
                std::cerr << "[MSG CHANGE]    grp id: " << it->first << ". Msg Meta " << it->second[i] << std::endl;
    }
#endif
    if(changes.mService != mIfaceImpl->getTokenService())
    {
       // std::cerr << "(EE) Incorrect service. Dropping." << std::endl;
        
        return;
    }

    if (!changes.mMsgs.empty() || !changes.mMsgsMeta.empty())
    {
        emit msgsChanged(changes.mMsgs, changes.mMsgsMeta);
    }

    if (!changes.mGrps.empty() || !changes.mGrpsMeta.empty())
    {
        emit grpsChanged(changes.mGrps, changes.mGrpsMeta);
    }

    if(!changes.mDistantSearchReqs.empty())
        emit distantSearchResultsChanged(changes.mDistantSearchReqs) ;

    emit changed();
}
