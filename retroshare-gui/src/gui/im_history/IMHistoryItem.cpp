/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2009 The RetroShare Team, Oleksiy Bilyanskyy
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

#include "IMHistoryItem.h"

//============================================================================

IMHistoryItem::IMHistoryItem()
{
    hiid = 0;
}

//============================================================================

IMHistoryItem::IMHistoryItem(int hiidIn, bool incomingIn, std::string &idIn, const QString &nameIn, const QDateTime &sendTimeIn, const QString &messageTextIn)
{
    hiid = hiidIn;
    incoming = incomingIn;
    id = idIn;
    name = nameIn;
    sendTime = sendTimeIn;
    messageText = messageTextIn;
}

//============================================================================

//! after qSort() older messages will become first
bool
IMHistoryItem::operator<(const IMHistoryItem& item) const
{
    return (sendTime < item.sendTime) ;
}
