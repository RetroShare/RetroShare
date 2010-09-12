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

#include "StatusDefs.h"

const QString StatusDefs::name(unsigned int status)
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

const QString StatusDefs::tooltip(unsigned int status)
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

const QColor StatusDefs::textColor(unsigned int status)
{
    switch (status) {
    case RS_STATUS_OFFLINE:
        return Qt::black;
    case RS_STATUS_AWAY:
        return Qt::gray;
    case RS_STATUS_BUSY:
        return Qt::gray;
    case RS_STATUS_ONLINE:
        return Qt::darkBlue;
    case RS_STATUS_INACTIVE:
        return Qt::gray;
    }

    std::cerr << "StatusDefs::textColor: Unknown status requested " << status;
    return Qt::black;
}

const QFont StatusDefs::font(unsigned int status)
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
