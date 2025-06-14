/*******************************************************************************
 * gui/common/StatusDefs.h                                                     *
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

#ifndef _STATUSDEFS_H
#define _STATUSDEFS_H

#include <QColor>
#include <QFont>

#include "retroshare/rsstatus.h"

struct RsPeerDetails;

class StatusDefs
{
public:
    static QString     name(RsStatusValue status);
    static const char* imageIM(RsStatusValue status);
    static const char* imageUser(RsStatusValue status);
    static const char* imageStatus(RsStatusValue status);
    static QString     tooltip(RsStatusValue status);

    static QFont       font(RsStatusValue status);

	static QString     peerStateString(int peerState);
	static QString     connectStateString(RsPeerDetails &details);
	static QString     connectStateWithoutTransportTypeString(RsPeerDetails &details);
	static QString     connectStateIpString(const RsPeerDetails &details);
};

#endif

