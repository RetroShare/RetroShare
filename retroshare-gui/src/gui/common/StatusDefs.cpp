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
#include <retroshare/rsstatus.h>
#include <retroshare/rspeers.h>

#include "StatusDefs.h"

QString StatusDefs::name(unsigned int status)
{
	switch (status) {
	case RS_STATUS_OFFLINE:
		return qApp->translate("StatusDefs", "Offline");
	case RS_STATUS_AWAY:
		return qApp->translate("StatusDefs", "Away");
	case RS_STATUS_BUSY:
		return qApp->translate("StatusDefs", "Busy");
	case RS_STATUS_ONLINE:
		return qApp->translate("StatusDefs", "Online");
	case RS_STATUS_INACTIVE:
		return qApp->translate("StatusDefs", "Idle");
	}

	std::cerr << "StatusDefs::name: Unknown status requested " << status;
	return "";
}

const char *StatusDefs::imageIM(unsigned int status)
{
	switch (status) {
	case RS_STATUS_OFFLINE:
		return ":/images/im-user-offline.png";
	case RS_STATUS_AWAY:
		return ":/images/im-user-away.png";
	case RS_STATUS_BUSY:
		return ":/images/im-user-busy.png";
	case RS_STATUS_ONLINE:
		return ":/images/im-user.png";
	case RS_STATUS_INACTIVE:
		return ":/images/im-user-inactive.png";
	}

	std::cerr << "StatusDefs::imageIM: Unknown status requested " << status;
	return "";
}

const char *StatusDefs::imageUser(unsigned int status)
{
	switch (status) {
	case RS_STATUS_OFFLINE:
		return ":/images/user/identityoffline24.png";
	case RS_STATUS_AWAY:
		return ":/images/user/identity24away.png";
	case RS_STATUS_BUSY:
		return ":/images/user/identity24busy.png";
	case RS_STATUS_ONLINE:
		return ":/images/user/identity24.png";
	case RS_STATUS_INACTIVE:
		return ":/images/user/identity24idle.png";
	}

	std::cerr << "StatusDefs::imageUser: Unknown status requested " << status;
	return "";
}

QString StatusDefs::tooltip(unsigned int status)
{
	switch (status) {
	case RS_STATUS_OFFLINE:
		return qApp->translate("StatusDefs", "Friend is offline");
	case RS_STATUS_AWAY:
		return qApp->translate("StatusDefs", "Friend is away");
	case RS_STATUS_BUSY:
		return qApp->translate("StatusDefs", "Friend is busy");
	case RS_STATUS_ONLINE:
		return qApp->translate("StatusDefs", "Friend is online");
	case RS_STATUS_INACTIVE:
		return qApp->translate("StatusDefs", "Friend is idle");
	}

	std::cerr << "StatusDefs::tooltip: Unknown status requested " << status;
	return "";
}

QFont StatusDefs::font(unsigned int status)
{
	QFont font;

	switch (status) {
	case RS_STATUS_AWAY:
	case RS_STATUS_BUSY:
	case RS_STATUS_ONLINE:
	case RS_STATUS_INACTIVE:
		font.setBold(true);
		return font;
	case RS_STATUS_OFFLINE:
		font.setBold(false);
		return font;
	}

	std::cerr << "StatusDefs::font: Unknown status requested " << status;
	return font;
}

QString StatusDefs::peerStateString(int peerState)
{
	if (peerState & RS_PEER_STATE_CONNECTED) {
		return qApp->translate("StatusDefs", "Connected");
	} else if (peerState & RS_PEER_STATE_UNREACHABLE) {
		return qApp->translate("StatusDefs", "Unreachable");
	} else if (peerState & RS_PEER_STATE_ONLINE) {
		return qApp->translate("StatusDefs", "Available");
	} else if (peerState & RS_PEER_STATE_FRIEND) {
		return qApp->translate("StatusDefs", "Offline");
	}

	return qApp->translate("StatusDefs", "Neighbor");
}

QString StatusDefs::connectStateString(RsPeerDetails &details)
{
	QString stateString;

	switch (details.connectState) {
	case 0:
		stateString = peerStateString(details.state);
		break;
	case RS_PEER_CONNECTSTATE_TRYING_TCP:
		stateString = qApp->translate("StatusDefs", "Trying TCP");
		break;
	case RS_PEER_CONNECTSTATE_TRYING_UDP:
		stateString = qApp->translate("StatusDefs", "Trying UDP");
		break;
	case RS_PEER_CONNECTSTATE_CONNECTED_TCP:
		stateString = qApp->translate("StatusDefs", "Connected: TCP");
		break;
	case RS_PEER_CONNECTSTATE_CONNECTED_UDP:
		stateString = qApp->translate("StatusDefs", "Connected: UDP");
		break;
	case RS_PEER_CONNECTSTATE_CONNECTED_UNKNOWN:
		stateString = qApp->translate("StatusDefs", "Connected: Unknown");
		break;
	}

	if (details.connectStateString.empty() == false) {
		if (stateString.isEmpty() == false) {
			stateString += ": ";
		}
		stateString += QString::fromStdString(details.connectStateString);
	}

	/* HACK to display DHT Status info too */
	if (details.foundDHT) {
		if (stateString.isEmpty() == false) {
			stateString += ", ";
		}
		stateString += qApp->translate("StatusDefs", "DHT: Contact");
	}

	return stateString;
}
