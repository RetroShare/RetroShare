/*******************************************************************************
 * gui/common/PeerDefs.cpp                                                     *
 *                                                                             *
 * Copyright (C) 2010, Retroshare Team <retroshare.project@gmail.com>          *
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

#include <QCoreApplication>
#include <retroshare/rspeers.h>
#include <retroshare/rsmsgs.h>
#include <retroshare/rsidentity.h>

#include "PeerDefs.h"

const QString PeerDefs::nameWithLocation(const RsPeerDetails &details)
{
    QString name = QString::fromUtf8(details.name.c_str());
    if (details.location.empty() == false) {
        name += " (" + QString::fromUtf8(details.location.c_str()) + ")";
    }

    return name;
}
const QString PeerDefs::nameWithLocation(const RsIdentityDetails &details)
{
    return QString::fromUtf8(details.mNickname.c_str()) + " (" + QString::fromStdString(details.mId.toStdString()) + ")";
}

const QString PeerDefs::nameWithId(const RsIdentityDetails &details)
{
     return QString::fromUtf8(details.mNickname.c_str()) + " <" + QString::fromUtf8(details.mNickname.c_str()) + "@" + QString::fromStdString(details.mId.toStdString()) + ">";
}

const QString PeerDefs::rsid(const std::string &name, const RsPgpId &id)
{
    if (name.empty()) {
        return qApp->translate("PeerDefs", "Unknown") + "@" + QString::fromStdString(id.toStdString());
    }

    return QString::fromUtf8(name.c_str()) + "@" + QString::fromStdString(id.toStdString());
}
const QString PeerDefs::rsid(const std::string &name, const RsGxsId &id)
{
    if (name.empty()) {
        return qApp->translate("PeerDefs", "Unknown") + "@" + QString::fromStdString(id.toStdString());
    }

    return QString::fromUtf8(name.c_str()) + "@" + QString::fromStdString(id.toStdString());
}
const QString PeerDefs::rsid(const std::string &name, const RsPeerId &id)
{
    if (name.empty()) {
        return qApp->translate("PeerDefs", "Unknown") + "@" + QString::fromStdString(id.toStdString());
    }

    return QString::fromUtf8(name.c_str()) + "@" + QString::fromStdString(id.toStdString());
}

const QString PeerDefs::rsid(const RsPeerDetails &details)
{
    return rsid(details.name, details.id);
}
const QString PeerDefs::rsidFromId(const RsGxsId &id, QString *name /* = NULL*/)
{
    QString rsid;

	 // Check own GXS ids first, since they can be obtained faster.
	 //
    std::list<RsGxsId> gxs_ids ;
    rsIdentity->getOwnIds(gxs_ids) ;

    for(std::list<RsGxsId>::const_iterator it(gxs_ids.begin());it!=gxs_ids.end();++it)
        if(*it == id)
        {
            // not a real peer. Try from hash for distant messages

            RsIdentityDetails details ;
            if(!rsIdentity->getIdDetails(*it,details))
                continue ;

				std::string peerName = details.mNickname ;

            rsid = PeerDefs::rsid(peerName, *it);
            if(name)
                *name = QString::fromUtf8(peerName.c_str());

				return rsid ;
        }

	 RsIdentityDetails details ;

	 if(rsIdentity->getIdDetails(id,details))
	 {
		 std::string peerName = details.mNickname ;
		 rsid = PeerDefs::rsid(peerName, id);

		 if(name)
			 *name = QString::fromUtf8(peerName.c_str());

		 return rsid ;
	 }
    rsid = PeerDefs::rsid("", id);

    if (name)
        *name = qApp->translate("PeerDefs", "Unknown");

    return rsid;
}
const QString PeerDefs::rsidFromId(const RsPeerId &id, QString *name /* = NULL*/)
{
    QString rsid;

    std::string peerName = rsPeers->getPeerName(id);

    if(!peerName.empty())
    {
        rsid = PeerDefs::rsid(peerName, id);

        if (name)
            *name = QString::fromUtf8(peerName.c_str());

        return rsid ;

    }

    std::list<RsGxsId> gxs_ids ;
    rsIdentity->getOwnIds(gxs_ids) ;

    for(std::list<RsGxsId>::const_iterator it(gxs_ids.begin());it!=gxs_ids.end();++it)
        if(RsPeerId(*it) == id)
        {
            // not a real peer. Try from hash for distant messages

            RsIdentityDetails details ;
            if(!rsIdentity->getIdDetails(*it,details))
                continue ;

            peerName = details.mNickname ;

            rsid = PeerDefs::rsid(peerName, *it);
            if(name)
                *name = QString::fromUtf8(peerName.c_str());

				return rsid ;
        }

    rsid = PeerDefs::rsid("", id);

    if (name)
        *name = qApp->translate("PeerDefs", "Unknown");

    return rsid;
}
const QString PeerDefs::rsidFromId(const RsPgpId &id, QString *name /* = NULL*/)
{
    QString rsid;

    std::string peerName = rsPeers->getGPGName(id);

	 if(!peerName.empty())	
	 {
        rsid = PeerDefs::rsid(peerName, id);

        if (name) {
            *name = QString::fromUtf8(peerName.c_str());
        }
    }
	 else
    {
        rsid = PeerDefs::rsid("", id);

        if (name) 
            *name = qApp->translate("PeerDefs", "Unknown");
    } 

    return rsid;
}

RsPeerId PeerDefs::idFromRsid(const QString &rsid, bool check)
{
    // search for cert id in string
    RsPeerId id;

    int index = rsid.indexOf("@");
    if (index >= 0) {
        // found "@", extract cert id from string
        id = RsPeerId(rsid.mid(index + 1).toStdString());
    } else {
        // maybe its only the cert id
        id = RsPeerId(rsid.toStdString());
    }

    if (check) {
        RsPeerDetails detail;
        if (rsPeers->getPeerDetails(id, detail) == false) {
            id.clear();
        }
    }

    return id;
}
