/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2010, RetroShare Team
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#include <QCoreApplication>
#include <retroshare/rspeers.h>
#include <retroshare/rsmsgs.h>

#include "PeerDefs.h"

const QString PeerDefs::nameWithLocation(const RsPeerDetails &details)
{
    QString name = QString::fromUtf8(details.name.c_str());
    if (details.location.empty() == false) {
        name += " (" + QString::fromUtf8(details.location.c_str()) + ")";
    }

    return name;
}

const QString PeerDefs::rsid(const std::string &name, const std::string &id)
{
    if (name.empty()) {
        return qApp->translate("PeerDefs", "Unknown") + "@" + QString::fromStdString(id);
    }

    return QString::fromUtf8(name.c_str()) + "@" + QString::fromStdString(id);
}

const QString PeerDefs::rsid(const RsPeerDetails &details)
{
    return rsid(details.name, details.id);
}

const QString PeerDefs::rsidFromId(const std::string &id, QString *name /* = NULL*/)
{
    QString rsid;

    std::string peerName = rsPeers->getPeerName(id);
	 std::string hash ;

	 if(!peerName.empty())	
	 {
        rsid = PeerDefs::rsid(peerName, id);

        if (name) {
            *name = QString::fromUtf8(peerName.c_str());
        }
    }
	 else if(rsMsgs->getDistantMessageHash(rsPeers->getGPGOwnId(),hash) && hash == id)
	 {
		 // not a real peer. Try from hash for distant messages
	
		 peerName = rsPeers->getGPGName(rsPeers->getGPGOwnId()) ;
		 rsid = PeerDefs::rsid(peerName, rsPeers->getGPGOwnId());
		 if(name)
			 *name = QString::fromUtf8(peerName.c_str());
	 }
	 else
    {
        rsid = PeerDefs::rsid("", id);

        if (name) 
            *name = qApp->translate("PeerDefs", "Unknown");
    } 

    return rsid;
}

const std::string PeerDefs::idFromRsid(const QString &rsid, bool check)
{
    // search for cert id in string
    std::string id;

    int index = rsid.indexOf("@");
    if (index >= 0) {
        // found "@", extract cert id from string
        id = rsid.mid(index + 1).toStdString();
    } else {
        // maybe its only the cert id
        id = rsid.toStdString();
    }

    if (check) {
        RsPeerDetails detail;
        if (rsPeers->getPeerDetails(id, detail) == false) {
            id.clear();
        }
    }

    return id;
}
