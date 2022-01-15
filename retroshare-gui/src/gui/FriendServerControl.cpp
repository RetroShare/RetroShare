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
#include "retroshare/rstor.h"

#include "util/qtthreadsutils.h"
#include "gui/common/FilesDefs.h"

#include "FriendServerControl.h"

#include <iostream>

#define ICON_STATUS_UNKNOWN ":/images/ledoff1.png"
#define ICON_STATUS_OK      ":/images/ledon1.png"

/** Constructor */
FriendServerControl::FriendServerControl(QWidget *parent)
    : QWidget(parent)
{
    /* Invoke the Qt Designer generated object setup routine */
    setupUi(this);

    if(!rsFriendServer)
    {
        setEnabled(false);
        return;
    }

    mConnectionCheckTimer = new QTimer;

    // init values

    torServerFriendsToRequest_SB->setValue(rsFriendServer->friendsToRequest());
    torServerAddress_LE->setText(QString::fromStdString(rsFriendServer->friendsServerAddress().c_str()));
    torServerPort_SB->setValue(rsFriendServer->friendsServerPort());

    // connect slignals/slots

    QObject::connect(friendServerOnOff_CB,SIGNAL(toggled(bool)),this,SLOT(onOnOffClick(bool)));
    QObject::connect(torServerFriendsToRequest_SB,SIGNAL(valueChanged(int)),this,SLOT(onFriendsToRequestChanged(int)));
    QObject::connect(torServerAddress_LE,SIGNAL(textChanged(const QString&)),this,SLOT(onOnionAddressEdit(const QString&)));
    QObject::connect(torServerPort_SB,SIGNAL(valueChanged(int)),this,SLOT(onOnionAddressEdit(int)));

    QObject::connect(mConnectionCheckTimer,SIGNAL(timeout()),this,SLOT(checkServerAddress()));

    mCheckingServerMovie = new QMovie(":/images/loader/circleball-16.gif");
    serverStatusCheckResult_LB->setMovie(mCheckingServerMovie);

    updateFriendServerStatusIcon(false);
    updateTorProxyInfo();
}

void FriendServerControl::updateTorProxyInfo()
{
    std::string friend_proxy_address;
    uint16_t friend_proxy_port;

    RsTor::getProxyServerInfo(friend_proxy_address,friend_proxy_port);

    torProxyPort_SB->setValue(friend_proxy_port);
    torProxyAddress_LE->setText(QString::fromStdString(friend_proxy_address));
}

FriendServerControl::~FriendServerControl()
{
    delete mCheckingServerMovie;
    delete mConnectionCheckTimer;
}

void FriendServerControl::onOnOffClick(bool b)
{
    if(b)
        rsFriendServer->startServer();
    else
        rsFriendServer->stopServer();
}
void FriendServerControl::onOnionPortEdit(int)
{
    // Setup timer to auto-check the friend server address

    mConnectionCheckTimer->stop();
    mConnectionCheckTimer->setSingleShot(true);
    mConnectionCheckTimer->setInterval(5000); // check in 5 secs unless something is changed in the mean time.

    mConnectionCheckTimer->start();

    if(mCheckingServerMovie->fileName() != QString(":/images/loader/circleball-16.gif" ))
    {
        mCheckingServerMovie->setFileName(":/images/loader/circleball-16.gif");
        mCheckingServerMovie->start();
    }
}

void FriendServerControl::onOnionAddressEdit(const QString&)
{
    // Setup timer to auto-check the friend server address

    mConnectionCheckTimer->stop();
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
    rsFriendServer->checkServerAddress_async(torServerAddress_LE->text().toStdString(),torServerPort_SB->value(),5000,
                                       [this](const std::string& address,uint16_t port,bool test_result)
                                        {
                                            if(test_result)
                                                rsFriendServer->setServerAddress(address,port);

                                            RsQThreadUtils::postToObject( [=]()  {  updateFriendServerStatusIcon(test_result); },this);
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



