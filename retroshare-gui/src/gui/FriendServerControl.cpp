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
#include <QMessageBox>
#include <QTcpSocket>
#include <QMenu>

#include "retroshare/rsfriendserver.h"
#include "retroshare/rstor.h"

#include "util/qtthreadsutils.h"
#include "util/misc.h"
#include "gui/common/FilesDefs.h"

#include "FriendServerControl.h"

#include <iostream>

#define ICON_STATUS_UNKNOWN ":/images/ledoff1.png"
#define ICON_STATUS_OK      ":/images/ledon1.png"

#define NAME_COLUMN 0
#define NODE_COLUMN 1
#define ADDR_COLUMN 2
#define STAT_COLUMN 3

/** Constructor */
FriendServerControl::FriendServerControl(QWidget *parent)
    : MainPage(parent)
{
    /* Invoke the Qt Designer generated object setup routine */
    setupUi(this);

    friendServerOnOff_CB->setEnabled(false); // until FS is connected.
    mCurrentlyCheckingServerAddress = false;

    if(!rsFriendServer)
    {
        setEnabled(false);
        return;
    }

    int H = QFontMetricsF(torServerAddress_LE->font()).height();

    QString help_str = tr("\
                          <h1><img width=\"%1\" src=\":/icons/help_64.png\">&nbsp;&nbsp;Friend Server</h1>                           \
                          <p>This configuration panel allows you to specify the onion address of a                                   \
                            friend server. Retroshare will talk to that server anonymously through Tor                               \
                            and use it to acquire a fixed number of friends.</p>                                                     \
                          <p>The friend server will continue supplying new friends until that number is reached                      \
                            in particular if you add your own friends manually, the friend server may become useless                 \
                            and you will save bandwidth disabling it. When disabling it, you will keep existing friends.</p>                                                            \
                          <p>The friend server only knows your peer ID and profile public key. It doesn't know your IP address.</p>  \
                          "
                          ).arg(QString::number(2*H), QString::number(2*H)) ;

    registerHelpButton(helpButton,help_str,"Friend Server") ;

    mConnectionCheckTimer = new QTimer;

    whileBlocking(autoAccept_CB)->setChecked(rsFriendServer->autoAddFriends());

    // Init values

    whileBlocking(torServerFriendsToRequest_SB)->setValue(rsFriendServer->friendsToRequest());
    whileBlocking(torServerAddress_LE)->setText(QString::fromStdString(rsFriendServer->friendsServerAddress().c_str()));
    whileBlocking(torServerPort_SB)->setValue(rsFriendServer->friendsServerPort());

    // Connect slignals/slots

    QObject::connect(friendServerOnOff_CB,SIGNAL(toggled(bool)),this,SLOT(onOnOffClick(bool)));
    QObject::connect(torServerFriendsToRequest_SB,SIGNAL(valueChanged(int)),this,SLOT(onFriendsToRequestChanged(int)));
    QObject::connect(torServerAddress_LE,SIGNAL(textEdited(const QString&)),this,SLOT(onOnionAddressEdit(const QString&)));
    QObject::connect(torServerPort_SB,SIGNAL(valueChanged(int)),this,SLOT(onOnionPortEdit(int)));
    QObject::connect(autoAccept_CB,SIGNAL(toggled(bool)),this,SLOT(onAutoAddFriends(bool)));

    QObject::connect(mConnectionCheckTimer,SIGNAL(timeout()),this,SLOT(checkServerAddress()));
    QObject::connect(status_TW, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(launchStatusContextMenu(QPoint)));

    mCheckingServerMovie = new QMovie(":/images/loader/circleball-16.gif");

    updateFriendServerStatusIcon(false);

    makeFriend_ACT             = new QAction(tr("Make friend"));			// makes SSL-only friend with the peer

    QObject::connect(makeFriend_ACT,SIGNAL(triggered()),this,SLOT(makeFriend()));

    mEventHandlerId_fs = 0;

    rsEvents->registerEventsHandler( [this](std::shared_ptr<const RsEvent> event)
    {
        RsQThreadUtils::postToObject([=](){ handleEvent_main_thread(event); }, this );
    }, mEventHandlerId_fs, RsEventType::FRIEND_SERVER );

    mEventHandlerId_peer = 0;

    rsEvents->registerEventsHandler( [this](std::shared_ptr<const RsEvent> event)
    {
        RsQThreadUtils::postToObject([=](){ handleEvent_main_thread(event); }, this );
    }, mEventHandlerId_peer, RsEventType::PEER_CONNECTION );
}

void FriendServerControl::onAutoAddFriends(bool b)
{
    rsFriendServer->setAutoAddFriends(b);
}
void FriendServerControl::handleEvent_main_thread(std::shared_ptr<const RsEvent> event)
{
    {
        const RsFriendServerEvent *fe = dynamic_cast<const RsFriendServerEvent*>(event.get());

        if(fe)
            switch(fe->mFriendServerEventType)
            {
            case RsFriendServerEventCode::PEER_INFO_CHANGED: updateContactsStatus();
                break;

            case RsFriendServerEventCode::FRIEND_SERVER_STATUS_CHANGED: updateFriendServerStatusIcon(fe->mFriendServerStatus == RsFriendServerStatus::ONLINE);
                break;

            default:
            case RsFriendServerEventCode::UNKNOWN: break;
            }
    }

    {
        const RsConnectionEvent *pe = dynamic_cast<const RsConnectionEvent*>(event.get());

        if(pe)
            switch(pe->mConnectionInfoCode)
            {
            case RsConnectionEventCode::PEER_ADDED:
            case RsConnectionEventCode::PEER_REMOVED:
            case RsConnectionEventCode::PEER_CONNECTED: updateContactsStatus();
                break;

            default: ;
            }
    }
}

FriendServerControl::~FriendServerControl()
{
    delete mCheckingServerMovie;
    delete mConnectionCheckTimer;

    rsEvents->unregisterEventsHandler(mEventHandlerId_fs);
    rsEvents->unregisterEventsHandler(mEventHandlerId_peer);
}

void FriendServerControl::launchStatusContextMenu(QPoint p)
{
    RsPeerId peer_id = getCurrentPeer();

    RsPeerDetails det;
    if(rsPeers->getPeerDetails(peer_id,det) && det.accept_connection)
        return;

    QMenu contextMnu(this);
    contextMnu.addAction(makeFriend_ACT);

    contextMnu.exec(QCursor::pos());
}
void FriendServerControl::onOnOffClick(bool b)
{
    if(b)
    {
        if(passphrase_LE->text().isNull())
        {
            QMessageBox::critical(nullptr,tr("Missing profile passphrase."),tr("Your profile passphrase is missing. Please enter is in the field below before enabling the friend server."));
            whileBlocking(friendServerOnOff_CB)->setCheckState(Qt::Unchecked);
            return;
        }
        rsFriendServer->setProfilePassphrase(passphrase_LE->text().toStdString());
        rsFriendServer->startServer();
    }
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
    while(mCurrentlyCheckingServerAddress)
    {
        std::cerr << "  waiting for free slot" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));	// wait for ongoing check to finish.
    }

    std::cerr << "Resetting connection proxy timer" << std::endl;

    // Setup timer to auto-check the friend server address
    mConnectionCheckTimer->setSingleShot(true);
    mConnectionCheckTimer->setInterval(5000); // check in 5 secs unless something is changed in the mean time.
    mConnectionCheckTimer->start();
}

void FriendServerControl::checkServerAddress()
{
    std::cerr << "In checkServerAddress() ..." << std::endl;

    mCurrentlyCheckingServerAddress = true;

    serverStatusCheckResult_LB->setMovie(mCheckingServerMovie);
    serverStatusCheckResult_LB->setToolTip(tr("Trying to contact friend server\nThis may take up to 1 min.")) ;
    mCheckingServerMovie->setFileName(":/images/loader/circleball-16.gif");
    mCheckingServerMovie->start();

    RsThread::async( [this]()
    {
        auto port = torServerPort_SB->value();
        auto addr = torServerAddress_LE->text().toStdString();

        std::cerr << "calling sync test..." << std::endl;
        bool succeed = rsFriendServer->checkServerAddress(addr,port,5000);
        std::cerr << "result is " << succeed << std::endl;

        RsQThreadUtils::postToObject( [addr,port,succeed,this]()
        {
            if(succeed)
                rsFriendServer->setServerAddress(addr,port);

            mCheckingServerMovie->stop();
            updateFriendServerStatusIcon(succeed);
            mCurrentlyCheckingServerAddress = false;

        },this);
    });
}

void FriendServerControl::onNbFriendsToRequestsChanged(int n)
{
    rsFriendServer->setFriendsToRequest(n);
}

void FriendServerControl::updateFriendServerStatusIcon(bool ok)
{
    std::cerr << "updating proxy status icon" << std::endl;

    if(ok)
    {
        serverStatusCheckResult_LB->setToolTip(tr("Friend server is currently reachable.")) ;
        serverStatusCheckResult_LB->setPixmap(QPixmap(ICON_STATUS_OK));
        friendServerOnOff_CB->setEnabled(true);
        status_TW->setEnabled(true);
    }
    else
    {
        rsFriendServer->stopServer();
        serverStatusCheckResult_LB->setToolTip(tr("The proxy is not enabled or broken.\nAre all services up and running fine??\nAlso check your ports!")) ;
        serverStatusCheckResult_LB->setPixmap(QPixmap(ICON_STATUS_UNKNOWN));
        friendServerOnOff_CB->setChecked(false);
        friendServerOnOff_CB->setEnabled(false);
        status_TW->setEnabled(false);
    }
}

void FriendServerControl::updateContactsStatus()
{
    std::map<RsPeerId,RsFriendServer::RsFsPeerInfo> pinfo = rsFriendServer->getPeersInfo();

    status_TW->clear();
    int row = 0;
    status_TW->setRowCount(pinfo.size());
    status_TW->setColumnCount(4);

    status_TW->setHorizontalHeaderItem(NAME_COLUMN,new QTableWidgetItem(QObject::tr("Name")));
    status_TW->setHorizontalHeaderItem(NODE_COLUMN,new QTableWidgetItem(QObject::tr("Node")));
    status_TW->setHorizontalHeaderItem(ADDR_COLUMN,new QTableWidgetItem(QObject::tr("Address")));
    status_TW->setHorizontalHeaderItem(STAT_COLUMN,new QTableWidgetItem(QObject::tr("Status")));

    for(auto it:pinfo)
    {
        uint32_t err_code=0;
        RsPeerDetails details;

        rsPeers->parseShortInvite(it.second.mInvite,details,err_code);

        status_TW->setItem(row,NAME_COLUMN,new QTableWidgetItem(QString::fromStdString(details.name)));
        status_TW->setItem(row,NODE_COLUMN,new QTableWidgetItem(QString::fromStdString(details.id.toStdString())));
        status_TW->setItem(row,ADDR_COLUMN,new QTableWidgetItem(QString::fromStdString(details.hiddenNodeAddress)+":"+QString::number(details.hiddenNodePort)));

        QString status_string;
        if(details.accept_connection)
            status_string += QString("Friend");
        else
            status_string += QString("Not friend");

        status_string += QString(" / ");

        switch(it.second.mPeerLevel)
        {
        case RsFriendServer::PeerFriendshipLevel::NO_KEY:   status_string += "Doesn't have my key" ; break;
        case RsFriendServer::PeerFriendshipLevel::HAS_KEY:  status_string += "Has my key" ; break;
        case RsFriendServer::PeerFriendshipLevel::HAS_ACCEPTED_KEY:  status_string += "Has friended me" ; break;
        default:
        case RsFriendServer::PeerFriendshipLevel::UNKNOWN:  status_string += "Unkn" ; break;
        }

        status_TW->setItem(row,STAT_COLUMN,new QTableWidgetItem(status_string));

        row++;
    }
}

RsPeerId FriendServerControl::getCurrentPeer()
{
    QTableWidgetItem *item = status_TW->currentItem();

    if(!item)
        return RsPeerId();

    return RsPeerId(status_TW->item(item->row(),NODE_COLUMN)->text().toStdString());
}
void FriendServerControl::makeFriend()
{
    RsPeerId peer_id = getCurrentPeer();

    rsFriendServer->allowPeer(peer_id);
}






