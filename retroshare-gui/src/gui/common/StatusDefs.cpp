/*******************************************************************************
 * gui/common/StatusDefs.cpp                                                   *
 *                                                                             *
 * Copyright (c) 2010, RetroShare Team <retroshare.project@gmail.com>          *
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
#include <retroshare/rsstatus.h>
#include <retroshare/rspeers.h>

#include "StatusDefs.h"

QString StatusDefs::name(RsStatusValue status)
{
	switch (status) {
    default:
    case RsStatusValue::RS_STATUS_OFFLINE:
		return qApp->translate("StatusDefs", "Offline");
    case RsStatusValue::RS_STATUS_AWAY:
		return qApp->translate("StatusDefs", "Away");
    case RsStatusValue::RS_STATUS_BUSY:
		return qApp->translate("StatusDefs", "Busy");
    case RsStatusValue::RS_STATUS_ONLINE:
		return qApp->translate("StatusDefs", "Online");
    case RsStatusValue::RS_STATUS_INACTIVE:
		return qApp->translate("StatusDefs", "Idle");
	}

    RsErr() << "StatusDefs::name: Unknown status requested " << (int)status;
	return "";
}

const char *StatusDefs::imageIM(RsStatusValue status)
{
	switch (status) {
    default:
    case RsStatusValue::RS_STATUS_OFFLINE:
		return ":/images/im-user-offline.png";
    case RsStatusValue::RS_STATUS_AWAY:
		return ":/images/im-user-away.png";
    case RsStatusValue::RS_STATUS_BUSY:
		return ":/images/im-user-busy.png";
    case RsStatusValue::RS_STATUS_ONLINE:
		return ":/images/im-user.png";
    case RsStatusValue::RS_STATUS_INACTIVE:
		return ":/images/im-user-inactive.png";
	}

    RsErr() << "StatusDefs::imageIM: Unknown status requested " << (int)status;
	return "";
}

const char *StatusDefs::imageUser(RsStatusValue status)
{
	switch (status) {
    default:
    case RsStatusValue::RS_STATUS_OFFLINE:
		return ":/images/user/identityoffline24.png";
    case RsStatusValue::RS_STATUS_AWAY:
		return ":/images/user/identity24away.png";
    case RsStatusValue::RS_STATUS_BUSY:
		return ":/images/user/identity24busy.png";
    case RsStatusValue::RS_STATUS_ONLINE:
		return ":/images/user/identity24.png";
    case RsStatusValue::RS_STATUS_INACTIVE:
		return ":/images/user/identity24idle.png";
	}

    RsErr() << "StatusDefs::imageUser: Unknown status requested " << (int)status;
	return "";
}

const char *StatusDefs::imageStatus(RsStatusValue status)
{
	switch (status) {
    default:
    case RsStatusValue::RS_STATUS_OFFLINE:
		return ":/icons/user-offline_64.png";
    case RsStatusValue::RS_STATUS_AWAY:
		return ":/icons/user-away_64.png";
    case RsStatusValue::RS_STATUS_BUSY:
		return ":/icons/user-busy_64.png";
    case RsStatusValue::RS_STATUS_ONLINE:
		return ":/icons/user-online_64.png";
    case RsStatusValue::RS_STATUS_INACTIVE:
		return ":/icons/user-away-extended_64.png";
	}

    RsErr() << "StatusDefs::imageUser: Unknown status requested " << (int)status;
	return "";
}

QString StatusDefs::tooltip(RsStatusValue status)
{
	switch (status) {
    default:
    case RsStatusValue::RS_STATUS_OFFLINE:
		return qApp->translate("StatusDefs", "Friend is offline");
    case RsStatusValue::RS_STATUS_AWAY:
		return qApp->translate("StatusDefs", "Friend is away");
    case RsStatusValue::RS_STATUS_BUSY:
		return qApp->translate("StatusDefs", "Friend is busy");
    case RsStatusValue::RS_STATUS_ONLINE:
		return qApp->translate("StatusDefs", "Friend is online");
    case RsStatusValue::RS_STATUS_INACTIVE:
		return qApp->translate("StatusDefs", "Friend is idle");
	}

    RsErr() << "StatusDefs::tooltip: Unknown status requested " << (int)status;
	return "";
}

QFont StatusDefs::font(RsStatusValue status)
{
	QFont font;

	switch (status) {
    default:
    case RsStatusValue::RS_STATUS_AWAY:
    case RsStatusValue::RS_STATUS_BUSY:
    case RsStatusValue::RS_STATUS_ONLINE:
    case RsStatusValue::RS_STATUS_INACTIVE:
		font.setBold(true);
		return font;
    case RsStatusValue::RS_STATUS_OFFLINE:
		font.setBold(false);
		return font;
	}

    RsErr() << "StatusDefs::font: Unknown status requested " << (int)status;
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
	bool isConnected = false;

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
		isConnected = true;
		break;
	case RS_PEER_CONNECTSTATE_CONNECTED_UDP:
		stateString = qApp->translate("StatusDefs", "Connected: UDP");
		isConnected = true;
		break;
	case RS_PEER_CONNECTSTATE_CONNECTED_TOR:
		stateString = qApp->translate("StatusDefs", "Connected: Tor");
		isConnected = true;
		break;
	case RS_PEER_CONNECTSTATE_CONNECTED_I2P:
		stateString = qApp->translate("StatusDefs", "Connected: I2P");
		isConnected = true;
		break;
	case RS_PEER_CONNECTSTATE_CONNECTED_UNKNOWN:
		stateString = qApp->translate("StatusDefs", "Connected: Unknown");
		isConnected = true;
		break;
	}

	if(isConnected) {
		stateString += " ";
		if(details.actAsServer)
			stateString += qApp->translate("StatusDefs", "inbound connection");
		else
			stateString += qApp->translate("StatusDefs", "outbound connection");
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

QString StatusDefs::connectStateWithoutTransportTypeString(RsPeerDetails &details)
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
	case RS_PEER_CONNECTSTATE_CONNECTED_UDP:
	case RS_PEER_CONNECTSTATE_CONNECTED_TOR:
	case RS_PEER_CONNECTSTATE_CONNECTED_I2P:
	case RS_PEER_CONNECTSTATE_CONNECTED_UNKNOWN:
		stateString = qApp->translate("StatusDefs", "Connected");
		break;
	}

	return stateString;
}

QString StatusDefs::connectStateIpString(const RsPeerDetails &details)
{
    QString stateString = QString("");

	switch (details.connectState) {
	case 0:
		stateString = peerStateString(details.state);
		break;
	case RS_PEER_CONNECTSTATE_TRYING_TCP:
	case RS_PEER_CONNECTSTATE_CONNECTED_TCP:
		stateString += QString(details.actAsServer ? qApp->translate("StatusDefs", "TCP-in") : qApp->translate("StatusDefs", "TCP-out"));
		break;
	case RS_PEER_CONNECTSTATE_TRYING_UDP:
	case RS_PEER_CONNECTSTATE_CONNECTED_UDP:
		stateString += qApp->translate("StatusDefs", "UDP");
		break;
	case RS_PEER_CONNECTSTATE_CONNECTED_TOR:
		stateString += QString(details.actAsServer ? qApp->translate("StatusDefs", "Tor-in") : qApp->translate("StatusDefs", "Tor-out"));
		break;
	case RS_PEER_CONNECTSTATE_CONNECTED_I2P:
		stateString += QString(details.actAsServer ? qApp->translate("StatusDefs", "I2P-in") : qApp->translate("StatusDefs", "I2P-out"));
		break;
	case RS_PEER_CONNECTSTATE_CONNECTED_UNKNOWN:
		stateString += qApp->translate("StatusDefs", "unkown");
		break;
	}
    stateString += QString(" : ");

    stateString += QString(details.connectAddr.c_str()) ;

	return stateString;
}
