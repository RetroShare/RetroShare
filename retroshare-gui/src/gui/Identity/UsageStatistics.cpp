/*******************************************************************************
 * retroshare-gui/src/gui/Identity/UsageStatistics.cpp                         *
 *                                                                             *
 * Copyright (C) 2026 by Retroshare Team     <retroshare.project@gmail.com>    *
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

#include <unistd.h>
#include <memory>

#include "UsageStatistics.h"
#include "ui_UsageStatistics.h"

#include "gui/RetroShareLink.h"
#include "gui/gxs/GxsIdDetails.h"
#include "util/DateTime.h"
#include "util/rstime.h"

#include "retroshare/rsgxsflags.h"
#include "retroshare/rschats.h"
#include "retroshare/rspeers.h"
#include "retroshare/rsservicecontrol.h"
#include "retroshare/rsgxschannels.h"
#include "retroshare/rsgxsforums.h"
#include "retroshare/rsposted.h"

#include <iostream>
#include <algorithm>
#include <memory>

/******
 * #define ID_DEBUG 1
 *****/

/** Constructor */
UsageStatistics::UsageStatistics(QWidget *parent)
    : QWidget(parent), ui(new Ui::UsageStatistics)
{
	ui->setupUi(this);

}

/** Destructor */
UsageStatistics::~UsageStatistics()
{
    delete ui;
}

static QString getHumanReadableDuration(uint32_t seconds)
{
    if(seconds < 60)
        return QString(QObject::tr("%1 seconds ago")).arg(seconds) ;
    else if(seconds < 120)
        return QString(QObject::tr("%1 minute ago")).arg(seconds/60) ;
    else if(seconds < 3600)
        return QString(QObject::tr("%1 minutes ago")).arg(seconds/60) ;
    else if(seconds < 7200)
        return QString(QObject::tr("%1 hour ago")).arg(seconds/3600) ;
    else if(seconds < 24*3600)
        return QString(QObject::tr("%1 hours ago")).arg(seconds/3600) ;
    else if(seconds < 2*24*3600)
        return QString(QObject::tr("%1 day ago")).arg(seconds/86400) ;
    else
        return QString(QObject::tr("%1 days ago")).arg(seconds/86400) ;
}

QString UsageStatistics::createUsageString(const RsIdentityUsage& u) const
{
    QString service_name;
    RetroShareLink::enumType service_type = RetroShareLink::TYPE_UNKNOWN;

    switch(u.mServiceId)
	{
	case RsServiceType::CHANNELS:  service_name = tr("Channels") ;service_type = RetroShareLink::TYPE_CHANNEL   ; break ;
	case RsServiceType::FORUMS:    service_name = tr("Forums") ;  service_type = RetroShareLink::TYPE_FORUM     ; break ;
	case RsServiceType::POSTED:    service_name = tr("Boards") ;  service_type = RetroShareLink::TYPE_POSTED    ; break ;
	case RsServiceType::CHAT:      service_name = tr("Chat")   ;  service_type = RetroShareLink::TYPE_CHAT_ROOM ; break ;

	case RsServiceType::GXS_TRANS: return tr("GxsMail author ");

	case RsServiceType::GXSCIRCLE: service_name = tr("GxsCircles");  service_type = RetroShareLink::TYPE_CIRCLES; break ;
	//case RsServiceType::TYPE_WIRE: service_name = tr("Wire");  service_type = RetroShareLink::TYPE_WIRE; break ;

    default:
        service_name = tr("Unknown (service=")+QString::number((int)u.mServiceId,16)+")"; service_type = RetroShareLink::TYPE_UNKNOWN ;
    }

    switch(u.mUsageCode)
    {
	case RsIdentityUsage::UNKNOWN_USAGE:
        	return tr("[Unknown]") ;
	case RsIdentityUsage::GROUP_ADMIN_SIGNATURE_CREATION:       // These 2 are normally not normal GXS identities, but nothing prevents it to happen either.
        	return tr("Admin signature in service %1").arg(service_name);
    case RsIdentityUsage::GROUP_ADMIN_SIGNATURE_VALIDATION:
        	return tr("Admin signature verification in service %1").arg(service_name);
    case RsIdentityUsage::GROUP_AUTHOR_SIGNATURE_CREATION:      // not typically used, since most services do not require group author signatures
        	return tr("Creation of author signature in service %1").arg(service_name);
    case RsIdentityUsage::MESSAGE_AUTHOR_SIGNATURE_CREATION:    // most common use case. Messages are signed by authors in e.g. forums.
    {
        RetroShareLink l;
        QString groupName;
        bool groupFound = false;

        // Prepare the list containing the single Group ID for the lookup
        std::list<RsGxsGroupId> groupIds;
        groupIds.push_back(u.mGrpId);

        // Fetch the Group Name from the appropriate service
        if (service_type == RetroShareLink::TYPE_CHANNEL && rsGxsChannels) {
            std::vector<RsGxsChannelGroup> groups;
            if (rsGxsChannels->getChannelsInfo(groupIds, groups) && !groups.empty()) {
                groupName = QString::fromUtf8(groups[0].mMeta.mGroupName.c_str());
                groupFound = !groupName.isEmpty();
            }
        } 
        else if (service_type == RetroShareLink::TYPE_FORUM && rsGxsForums) {
            std::vector<RsGxsForumGroup> groups;
            if (rsGxsForums->getForumsInfo(groupIds, groups) && !groups.empty()) {
                groupName = QString::fromUtf8(groups[0].mMeta.mGroupName.c_str());
                groupFound = !groupName.isEmpty();
            }
        }
        else if (service_type == RetroShareLink::TYPE_POSTED && rsPosted) {
            std::vector<RsPostedGroup> groups;
            if (rsPosted->getBoardsInfo(groupIds, groups) && !groups.empty()) {
                groupName = QString::fromUtf8(groups[0].mMeta.mGroupName.c_str());
                groupFound = !groupName.isEmpty();
            }
        }

        // Prepare the label: 
        QString label;
        if (groupFound) {
            label = groupName;
        } else {
            label = QString::fromStdString(u.mGrpId.toStdString());
        }

        // Create the GXS Group Link
        l = RetroShareLink::createGxsGroupLink(service_type, u.mGrpId, label);

        // Return the formatted string
        return tr("Message signature creation in group %1 of service %2").arg(l.toHtml(), service_name);
    }
    case RsIdentityUsage::GROUP_AUTHOR_KEEP_ALIVE:               // Identities are stamped regularly by crawlign the set of messages for all groups. That helps keepign the useful identities in hand.
    case RsIdentityUsage::GROUP_AUTHOR_SIGNATURE_VALIDATION:
    {
        RetroShareLink l;
        QString groupName;
        bool groupFound = false;

        // Prepare a list containing the single Group ID we want to look up
        std::list<RsGxsGroupId> groupIds;
        groupIds.push_back(u.mGrpId);

        // Fetch the Group Name from the appropriate service
        if (service_type == RetroShareLink::TYPE_CHANNEL && rsGxsChannels) {
            std::vector<RsGxsChannelGroup> groups;
            // API requires std::list input and std::vector output
            if (rsGxsChannels->getChannelsInfo(groupIds, groups) && !groups.empty()) {
                groupName = QString::fromUtf8(groups[0].mMeta.mGroupName.c_str());
                groupFound = !groupName.isEmpty();
            }
        }
        else if (service_type == RetroShareLink::TYPE_FORUM && rsGxsForums) {
            std::vector<RsGxsForumGroup> groups;
            // Forums API requires a list of IDs and a vector for results
            if (rsGxsForums->getForumsInfo(groupIds, groups) && !groups.empty()) {
                groupName = QString::fromUtf8(groups[0].mMeta.mGroupName.c_str());
                groupFound = !groupName.isEmpty();
            }
        }
        else if (service_type == RetroShareLink::TYPE_POSTED && rsPosted) {
            std::vector<RsPostedGroup> groups;
            // Posted/Boards API requires a list of IDs and a vector for results
            if (rsPosted->getBoardsInfo(groupIds, groups) && !groups.empty()) {
                groupName = QString::fromUtf8(groups[0].mMeta.mGroupName.c_str());
                groupFound = !groupName.isEmpty();
            }
        }

        QString label;
        if (groupFound) {
            label = groupName;
        } else {
            // If not found, we use the raw ID string as the original code did
            label = QString::fromStdString(u.mGrpId.toStdString());
        }

        // Create the GXS Group Link
        l = RetroShareLink::createGxsGroupLink(service_type, u.mGrpId, label);

        // Return the final formatted string
        return tr("Group author for group %1 in service %2").arg(l.toHtml(), service_name);
    }
    case RsIdentityUsage::MESSAGE_AUTHOR_SIGNATURE_VALIDATION:
    case RsIdentityUsage::MESSAGE_AUTHOR_KEEP_ALIVE:             // Identities are stamped regularly by crawling the set of messages for all groups. That helps keepign the useful identities in hand.
	{
        RetroShareLink l;
        QString title;
        bool titleFound = false;

        // Create a set containing only the ID we are looking for
        std::set<RsGxsMessageId> msgIds;
        msgIds.insert(u.mMsgId);

        if (service_type == RetroShareLink::TYPE_CHANNEL && rsGxsChannels) {
            std::vector<RsGxsChannelPost> posts;
            std::vector<RsGxsComment> cmts; // Local variable to avoid rvalue error
            std::vector<RsGxsVote> vots;    // Local variable to avoid rvalue error
            
            if (rsGxsChannels->getChannelContent(u.mGrpId, msgIds, posts, cmts, vots)) {
                if (!posts.empty()) {
                    title = QString::fromUtf8(posts[0].mMeta.mMsgName.c_str());
                    titleFound = !title.isEmpty();
                }
            }
        } 
        else if (service_type == RetroShareLink::TYPE_FORUM && rsGxsForums) {
            std::vector<RsGxsForumMsg> msgs;
            // getForumContent only needs 3 arguments, so no extra vectors needed
            if (rsGxsForums->getForumContent(u.mGrpId, msgIds, msgs)) {
                if (!msgs.empty()) {
                    title = QString::fromUtf8(msgs[0].mMeta.mMsgName.c_str());
                    titleFound = !title.isEmpty();
                }
            }
        }
        else if (service_type == RetroShareLink::TYPE_POSTED && rsPosted) {
            std::vector<RsPostedPost> posts;
            std::vector<RsGxsComment> cmts;
            std::vector<RsGxsVote> vots;
            
            if (rsPosted->getBoardContent(u.mGrpId, msgIds, posts, cmts, vots)) {
                if (!posts.empty()) {
                    title = QString::fromUtf8(posts[0].mMeta.mMsgName.c_str());
                    titleFound = !title.isEmpty();
                }
            }
        }
        
        // Prepare the label
        QString label;
        if (titleFound) {
            label = title;
        } else {
            // Use raw Group ID string if it's a group validation case or total failure
            label = QString::fromStdString(u.mGrpId.toStdString());
        }


        // Generate the link
        if ((service_type == RetroShareLink::TYPE_CHANNEL || service_type == RetroShareLink::TYPE_POSTED) && !u.mThreadId.isNull()) {
            l = RetroShareLink::createGxsMessageLink(service_type, u.mGrpId, u.mThreadId, label);
        }
        else {
            l = RetroShareLink::createGxsMessageLink(service_type, u.mGrpId, u.mMsgId, label);
        }
        
        // Determine the suffix based on the service type
        QString suffix;
        if (service_type == RetroShareLink::TYPE_CHANNEL || service_type == RetroShareLink::TYPE_POSTED) {
            suffix = tr("Vote/comment");
        } else {
            suffix = tr("Message");
        }

            return tr("%2 in %3 service %1").arg(l.toHtml(), suffix, service_name);
        }
    case RsIdentityUsage::CHAT_LOBBY_MSG_VALIDATION:             // Chat lobby msgs are signed, so each time one comes, or a chat lobby event comes, a signature verificaiton happens.
    {
		ChatId id = ChatId(ChatLobbyId(u.mAdditionalId));
		ChatLobbyInfo linfo ;
        rsChats->getChatLobbyInfo(ChatLobbyId(u.mAdditionalId),linfo);
		RetroShareLink l = RetroShareLink::createChatRoom(id, QString::fromUtf8(linfo.lobby_name.c_str()));
		return tr("Message in chat room %1").arg(l.toHtml()) ;
    }
    case RsIdentityUsage::GLOBAL_ROUTER_SIGNATURE_CHECK:         // Global router message validation
    {
		return tr("Distant message signature validation.");
    }
    case RsIdentityUsage::GLOBAL_ROUTER_SIGNATURE_CREATION:      // Global router message signature
    {
		return tr("Distant message signature creation.");
    }
    case RsIdentityUsage::GXS_TUNNEL_DH_SIGNATURE_CHECK:         //
    {
		return tr("Signature validation in distant tunnel system.");
    }
    case RsIdentityUsage::GXS_TUNNEL_DH_SIGNATURE_CREATION:      //
    {
		return tr("Signature in distant tunnel system.");
    }
    case RsIdentityUsage::IDENTITY_NEW_FROM_GXS_SYNC:            // Group update on that identity data. Can be avatar, name, etc.
    {
		return tr("Received from GXS sync.");
    }
    case RsIdentityUsage::IDENTITY_NEW_FROM_DISCOVERY:           // Own friend sended his own ids
    {
		return tr("Friend node identity received through discovery.");
    }
    case RsIdentityUsage::IDENTITY_GENERIC_SIGNATURE_CHECK:      // Any signature verified for that identity
    {
		return tr("Generic signature validation.");
    }
    case RsIdentityUsage::IDENTITY_GENERIC_SIGNATURE_CREATION:   // Any signature made by that identity
    {
		return tr("Generic signature creation (e.g. chat room message, global router,...).");
    }
	case RsIdentityUsage::IDENTITY_GENERIC_ENCRYPTION: return tr("Generic encryption.");
	case RsIdentityUsage::IDENTITY_GENERIC_DECRYPTION: return tr("Generic decryption.");
	case RsIdentityUsage::CIRCLE_MEMBERSHIP_CHECK:
    {
        RetroShareLink l;
        RsGxsCircleDetails det;
        bool circleFound = false;

        // Try to fetch circle details to get the name
        if (rsGxsCircles && rsGxsCircles->getCircleDetails(RsGxsCircleId(u.mGrpId), det)) {
            circleFound = true;
        }

        // Prepare the label: 
        QString label;
        if (circleFound && !det.mCircleName.empty()) {
            label = QString::fromUtf8(det.mCircleName.c_str()) ;
        } else {
            label = QString::fromStdString(u.mGrpId.toStdString());
        }

        // Create the RetroShareLink for the circle
        l = RetroShareLink::createCircle(RsGxsCircleId(u.mGrpId), label);

        // Return the formatted string with the clickable link
        if (circleFound) {
            return tr("Membership verification in circle %1.").arg(l.toHtml());
        } else {
            return tr("Membership verification in circle (ID=%1).").arg(l.toHtml());
        }
    }

#warning TODO! csoler 2017-01-03: Add the different strings and translations here.
	default:
		return QString("Undone yet");
    }
    return QString("Unknown");
}

void UsageStatistics::setUsageData(RsGxsIdGroup data)
{
    time_t now = time(NULL);
    RsIdentityDetails det;
    rsIdentity->getIdDetails(RsGxsId(data.mMeta.mGroupId), det);

    QString usage_txt;
    std::map<rstime_t, RsIdentityUsage> rmap;
    for(auto it(det.mUseCases.begin()); it != det.mUseCases.end(); ++it)
        rmap.insert(std::make_pair(it->second, it->first));

    for(auto it(rmap.begin()); it != rmap.end(); ++it)
        usage_txt += QString("<b>") + getHumanReadableDuration(now - data.mLastUsageTS) + "</b> \t: " + createUsageString(it->second) + "<br/>";

    if(usage_txt.isEmpty()) // .isNull() can sometimes be tricky, .isEmpty() is safer here
        usage_txt = tr("<b>[No record in current session]</b>");

    ui->usageStatistics_TB->setText(usage_txt);
}
