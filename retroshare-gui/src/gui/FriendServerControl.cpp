/*******************************************************************************
 * gui/FriendServer.cpp                                                        *
 *                                                                             *
 * Copyright (c) 2021 Retroshare Team  <retroshare.project@gmail.com>          *
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

#include <QTimer>
#include <QMovie>
#include <QTcpSocket>

#include "retroshare/rsfriendserver.h"

#include "util/qtthreadsutils.h"
#include "gui/common/FilesDefs.h"

#include "FriendServerControl.h"

#include <iostream>

RsFriendServer *rsFriendServer = new RsFriendServer;

#define ICON_STATUS_UNKNOWN ":/images/ledoff1.png"
#define ICON_STATUS_OK      ":/images/ledon1.png"

/** Constructor */
FriendServerControl::FriendServerControl(QWidget *parent)
{
    /* Invoke the Qt Designer generated object setup routine */
    setupUi(this);

    mConnectionCheckTimer = new QTimer;

    QObject::connect(mConnectionCheckTimer,SIGNAL(timeout()),this,SLOT(checkServerAddress()));
    QObject::connect(torServerAddress_LE,SIGNAL(textChanged(const QString&)),this,SLOT(onOnionAddressEdit(const QString&)));

    mCheckingServerMovie = new QMovie(":/images/loader/circleball-16.gif");
    serverStatusCheckResult_LB->setMovie(mCheckingServerMovie);

    updateFriendServerStatusIcon(false);
}

FriendServerControl::~FriendServerControl()
{
    delete mCheckingServerMovie;
    delete mConnectionCheckTimer;
}

void FriendServerControl::onOnOffClick(bool b)
{
    if(b)
        rsFriendServer->start();
    else
        rsFriendServer->stop();
}

void FriendServerControl::onOnionAddressEdit(const QString&)
{
    // Setup timer to auto-check the friend server address

    mConnectionCheckTimer->setSingleShot(true);
    mConnectionCheckTimer->setInterval(5000); // check in 5 secs unless something is changed in the mean time.

    mConnectionCheckTimer->start();

    if(mCheckingServerMovie->fileName() != QString(":/images/loader/circleball-16.gif" ))
    {
        mCheckingServerMovie->setFileName(":/images/loader/circleball-16.gif");
        mCheckingServerMovie->start();
    }
}

void FriendServerControl::checkServerAddress()
{
    rsFriendServer->checkServerAddress_async(torServerAddress_LE->text().toStdString(),torServerPort_SB->value(),
                                       [this](const std::string& address,bool test_result)
                                        {
                                            if(test_result)
                                                rsFriendServer->setServerAddress(address,1729);

                                            RsQThreadUtils::postToObject( [=]() { updateFriendServerStatusIcon(test_result); },this);
                                        }
    );
}

void FriendServerControl::onNbFriendsToRequestsChanged(int n)
{
    rsFriendServer->setFriendsToRequest(n);
}

void FriendServerControl::updateFriendServerStatusIcon(bool ok)
{
    mCheckingServerMovie->stop();

    if(ok)
    {
        torServerStatus_LB->setToolTip(tr("Friend server is currently reachable.")) ;
        mCheckingServerMovie->setFileName(ICON_STATUS_OK);
    }
    else
    {
        torServerStatus_LB->setToolTip(tr("The proxy is not enabled or broken.\nAre all services up and running fine??\nAlso check your ports!")) ;
        mCheckingServerMovie->setFileName(ICON_STATUS_UNKNOWN);
    }
    mCheckingServerMovie->start();
}



