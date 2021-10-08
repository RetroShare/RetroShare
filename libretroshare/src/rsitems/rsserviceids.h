/*******************************************************************************
 * libretroshare/src/rsitems: rsserviceids.h                                   *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2007-2008 Robert Fernie <retroshare@lunamutt.com>             *
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

#include "util/rsdeprecate.h"

#include <cstdint>

enum class RsServiceType : uint16_t
{
	NONE                       = 0, /// To detect non-initialized reads
	GOSSIP_DISCOVERY           = 0x0011,
	CHAT                       = 0x0012,
	MSG                        = 0x0013,
	TURTLE                     = 0x0014,
	TUNNEL                     = 0x0015,
	HEARTBEAT                  = 0x0016,
	FILE_TRANSFER              = 0x0017,
	GROUTER                    = 0x0018,
	FILE_DATABASE              = 0x0019,
	SERVICEINFO                = 0x0020,
	BANDWIDTH_CONTROL          = 0x0021,
	MAIL                       = 0x0022,
	DIRECT_MAIL                = 0x0023,
	DISTANT_MAIL               = 0x0024,
	GWEMAIL_MAIL               = 0x0025,
	SERVICE_CONTROL            = 0x0026,
	DISTANT_CHAT               = 0x0027,
	GXS_TUNNEL                 = 0x0028,
	BANLIST                    = 0x0101,
	STATUS                     = 0x0102,
    FRIEND_SERVER              = 0x0103,
    NXS                        = 0x0200,
	GXSID                      = 0x0211,
	PHOTO                      = 0x0212,
	WIKI                       = 0x0213,
	WIRE                       = 0x0214,
	FORUMS                     = 0x0215,
	POSTED                     = 0x0216,
	CHANNELS                   = 0x0217,
	GXSCIRCLE                  = 0x0218,
	/// not gxs, but used with identities.
	REPUTATION                 = 0x0219,
	GXS_RECOGN                 = 0x0220,
	GXS_TRANS                  = 0x0230,
	JSONAPI                    = 0x0240,
	FORUMS_CONFIG              = 0x0315,
	POSTED_CONFIG              = 0x0316,
	CHANNELS_CONFIG            = 0x0317,
	RTT                        = 0x1011, /// Round Trip Time


	/***************** IDS ALLOCATED FOR PLUGINS ******************/
	// 2000+
	PLUGIN_ARADO_ID            = 0x2001,
	PLUGIN_QCHESS_ID           = 0x2002,
	PLUGIN_FEEDREADER          = 0x2003,

	/// Reserved for packet slicing probes.
	PACKET_SLICING_PROBE       = 0xAABB,

	// Nabu's experimental services.
	PLUGIN_FIDO_GW             = 0xF1D0,
	PLUGIN_ZERORESERVE         = 0xBEEF
};



// TODO: Port all services types to RsServiceType

RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_TYPE_FILE_INDEX     = 0x0001;
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_TYPE_DISC           = 0x0011;
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_TYPE_CHAT           = 0x0012;
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_TYPE_MSG            = 0x0013;
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_TYPE_TURTLE         = 0x0014;
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_TYPE_TUNNEL         = 0x0015;
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_TYPE_HEARTBEAT      = 0x0016;
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_TYPE_FILE_TRANSFER  = 0x0017;
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_TYPE_GROUTER        = 0x0018;
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_TYPE_FILE_DATABASE  = 0x0019;
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_TYPE_SERVICEINFO    = 0x0020;
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_TYPE_BWCTRL         = 0x0021; /// Bandwidth Control
/// New Mail Service (replace old Msg Service)
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_TYPE_MAIL           = 0x0022;
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_TYPE_DIRECT_MAIL    = 0x0023;
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_TYPE_DISTANT_MAIL   = 0x0024;
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_TYPE_GWEMAIL_MAIL   = 0x0025;
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_TYPE_SERVICE_CONTROL= 0x0026;
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_TYPE_DISTANT_CHAT   = 0x0027;
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_TYPE_GXS_TUNNEL     = 0x0028;
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_TYPE_BANLIST        = 0x0101;
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_TYPE_STATUS         = 0x0102;
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_TYPE_FRIEND_SERVER  = 0x0103;

/// Rs Network Exchange Service
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_TYPE_NXS            = 0x0200;
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_GXS_TYPE_GXSID       = 0x0211;
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_GXS_TYPE_PHOTO       = 0x0212;
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_GXS_TYPE_WIKI        = 0x0213;
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_GXS_TYPE_WIRE        = 0x0214;
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_GXS_TYPE_FORUMS      = 0x0215;
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_GXS_TYPE_POSTED      = 0x0216;
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_GXS_TYPE_CHANNELS    = 0x0217;
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_GXS_TYPE_GXSCIRCLE   = 0x0218;
/// not gxs, but used with identities.
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_GXS_TYPE_REPUTATION  = 0x0219;
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_TYPE_GXS_RECOGN      = 0x0220;
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_TYPE_GXS_TRANS       = 0x0230;
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_TYPE_JSONAPI         = 0x0240;
/// used to save notification records in GXS and possible other service-based configuration
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_GXS_TYPE_FORUMS_CONFIG   = 0x0315;
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_GXS_TYPE_POSTED_CONFIG   = 0x0316;
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_GXS_TYPE_CHANNELS_CONFIG = 0x0317;
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_GXS_TYPE_CIRCLES_CONFIG  = 0x0318;

// Experimental Services.
/* DSDV Testing at the moment - Service Only */
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_TYPE_DSDV        = 0x1010;
/* Latency RTT Measurements */
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_TYPE_RTT         = 0x1011;


/***************** IDS ALLOCATED FOR PLUGINS ******************/
// 2000+

RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_TYPE_PLUGIN_ARADO_ID    = 0x2001;
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_TYPE_PLUGIN_QCHESS_ID   = 0x2002;
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_TYPE_PLUGIN_FEEDREADER  = 0x2003;

// Reserved for packet slicing probes.
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_TYPE_PACKET_SLICING_PROBE = 0xAABB;

// Nabu's services.
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_TYPE_PLUGIN_FIDO_GW     = 0xF1D0;
RS_DEPRECATED_FOR(RsServiceType) const uint16_t RS_SERVICE_TYPE_PLUGIN_ZERORESERVE = 0xBEEF;
