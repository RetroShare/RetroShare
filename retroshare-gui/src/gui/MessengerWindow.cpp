/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006, crypton
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

#include <QFile>
#include <QFileInfo>
#include <QWidgetAction>
#include <QTimer>

#include "common/vmessagebox.h"
#include "common/StatusDefs.h"

#include <retroshare/rsiface.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsmsgs.h>
#include <retroshare/rsstatus.h>
#include <retroshare/rsnotify.h>

#include "rshare.h"
#include "MessengerWindow.h"
#include "RsAutoUpdatePage.h"

#ifndef MINIMAL_RSGUI
#include "MainWindow.h"
#include "chat/PopupChatDialog.h"
#include "msgs/MessageComposer.h"
#include "ShareManager.h"
#include "notifyqt.h"
#include "connect/ConnectFriendWizard.h"
#endif // MINIMAL_RSGUI
#include "FriendsDialog.h"
#include "connect/ConfCertDialog.h"
#include "util/PixmapMerging.h"
#include "LogoBar.h"
#include "util/Widget.h"
#include "util/misc.h"
#include "settings/rsharesettings.h"
#include "common/RSTreeWidgetItem.h"

#include "RetroShareLink.h"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <set>

/* Images for context menu icons */
#define IMAGE_REMOVEFRIEND       ":/images/removefriend16.png"
#define IMAGE_EXPIORTFRIEND      ":/images/exportpeers_16x16.png"
#define IMAGE_CHAT               ":/images/chat.png"
#define IMAGE_MSG                ":/images/message-mail.png"
#define IMAGE_CONNECT            ":/images/connect_friend.png"
#define IMAGE_PEERINFO           ":/images/peerdetails_16x16.png"
#define IMAGE_AVAIBLE            ":/images/user/identityavaiblecyan24.png"
#define IMAGE_CONNECT2           ":/images/reload24.png"
#define IMAGE_PASTELINK          ":/images/pasterslink.png"

#define COLUMN_COUNT    3
#define COLUMN_NAME     0
#define COLUMN_STATE    1
#define COLUMN_INFO     2

#define COLUMN_DATA     0 // column for storing the userdata id

#define ROLE_SORT  Qt::UserRole
#define ROLE_ID    Qt::UserRole + 1

/******
 * #define MSG_DEBUG 1
 *****/

MessengerWindow* MessengerWindow::_instance = NULL;
static std::set<std::string> *expandedPeers = NULL;

/*static*/ void MessengerWindow::showYourself ()
{
    if (_instance == NULL) {
        _instance = new MessengerWindow();
    }

    _instance->show();
    _instance->activateWindow();
}

MessengerWindow* MessengerWindow::getInstance()
{
    return _instance;
}

void MessengerWindow::releaseInstance()
{
    if (_instance) {
        delete _instance;
    }
    if (expandedPeers) {
        /* delete saved expanded peers */
        delete(expandedPeers);
        expandedPeers = NULL;
    }
}

/** Constructor */
MessengerWindow::MessengerWindow(QWidget* parent, Qt::WFlags flags)
    : 	RWindow("MessengerWindow", parent, flags)
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);

    setAttribute ( Qt::WA_DeleteOnClose, true );
#ifdef MINIMAL_RSGUI
    setAttribute (Qt::WA_QuitOnClose, true);
#endif // MINIMAL_RSGUI

    m_compareRole = new RSTreeWidgetItemCompareRole;
    m_compareRole->setRole(COLUMN_NAME, ROLE_SORT);

    connect( ui.messengertreeWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( messengertreeWidgetCostumPopupMenu( QPoint ) ) );
#ifndef MINIMAL_RSGUI
    connect( ui.messengertreeWidget, SIGNAL(itemDoubleClicked ( QTreeWidgetItem *, int)), this, SLOT(chatfriend(QTreeWidgetItem *)));

    connect( ui.avatarButton, SIGNAL(clicked()), SLOT(getAvatar()));
    connect( ui.shareButton, SIGNAL(clicked()), SLOT(openShareManager()));
    connect( ui.addIMAccountButton, SIGNAL(clicked( bool ) ), this , SLOT( addFriend() ) );
#endif // MINIMAL_RSGUI
    connect( ui.actionHide_Offline_Friends, SIGNAL(triggered()), this, SLOT(insertPeers()));
    connect( ui.actionSort_by_State, SIGNAL(triggered()), this, SLOT(insertPeers()));
    connect(ui.clearButton, SIGNAL(clicked()), this, SLOT(clearFilter()));

    connect(ui.messagelineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(savestatusmessage()));
    connect(ui.filterPatternLineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(filterRegExpChanged()));

#ifndef MINIMAL_RSGUI
    connect(NotifyQt::getInstance(), SIGNAL(friendsChanged()), this, SLOT(updateMessengerDisplay()));
    connect(NotifyQt::getInstance(), SIGNAL(ownAvatarChanged()), this, SLOT(updateAvatar()));
    connect(NotifyQt::getInstance(), SIGNAL(ownStatusMessageChanged()), this, SLOT(loadmystatusmessage()));
    connect(NotifyQt::getInstance(), SIGNAL(peerStatusChanged(QString,int)), this, SLOT(updateOwnStatus(QString,int)));
#endif // MINIMAL_RSGUI

    timer = new QTimer(this);
    timer->connect(timer, SIGNAL(timeout()), this, SLOT(updateMessengerDisplay()));
    timer->setInterval(1000); /* one second */
    timer->setSingleShot(true);

    /* to hide the header  */
    ui.messengertreeWidget->header()->hide();

    /* Set header resize modes and initial section sizes */
    ui.messengertreeWidget->setColumnCount(COLUMN_COUNT);
    ui.messengertreeWidget->setColumnHidden ( COLUMN_INFO, true);
    ui.messengertreeWidget->sortItems( COLUMN_NAME, Qt::AscendingOrder );

    QHeaderView * _header = ui.messengertreeWidget->header () ;
    _header->setResizeMode (COLUMN_NAME, QHeaderView::Stretch);
    _header->setResizeMode (COLUMN_STATE, QHeaderView::Custom);
    _header->setStretchLastSection(false);

    _header->resizeSection ( COLUMN_NAME, 200 );
    _header->resizeSection ( COLUMN_STATE, 42 );

    //LogoBar
    _rsLogoBarmessenger = NULL;
    _rsLogoBarmessenger = new LogoBar(ui.logoframe);
    Widget::createLayout(ui.logoframe)->addWidget(_rsLogoBarmessenger);

    ui.messagelineEdit->setMinimumWidth(20);

    itemFont = QFont("ARIAL", 10);
    itemFont.setBold(true);

    displayMenu();

    // load settings
    processSettings(true);

    // add self nick
    RsPeerDetails pd;
    std::string ownId = rsPeers->getOwnId();
    if (rsPeers->getPeerDetails(ownId, pd)) {
        /* calculate only once */
        m_nickName = QString::fromUtf8(pd.name.c_str());
#ifdef MINIMAL_RSGUI
         ui.statusButton->setText(m_nickName);
#endif
    }

#ifndef MINIMAL_RSGUI
    /* Show nick and current state */
    StatusInfo statusInfo;
    rsStatus->getOwnStatus(statusInfo);
    updateOwnStatus(QString::fromStdString(ownId), statusInfo.status);

    MainWindow *pMainWindow = MainWindow::getInstance();
    if (pMainWindow) {
        QMenu *pStatusMenu = new QMenu();
        pMainWindow->initializeStatusObject(pStatusMenu, true);
        ui.statusButton->setMenu(pStatusMenu);
    }

    updateAvatar();
    loadmystatusmessage();
#endif // MINIMAL_RSGUI

    insertPeers();

    ui.clearButton->hide();

    updateMessengerDisplay();

    /* Hide platform specific features */
#ifdef Q_WS_WIN
#endif
}

MessengerWindow::~MessengerWindow ()
{
    // save settings
    processSettings(false);

#ifndef MINIMAL_RSGUI
    MainWindow *pMainWindow = MainWindow::getInstance();
    if (pMainWindow) {
        pMainWindow->removeStatusObject(ui.statusButton);
    }
#endif // MINIMAL_RSGUI

    delete(m_compareRole);

    _instance = NULL;
}

void MessengerWindow::processSettings(bool bLoad)
{
    QHeaderView *header = ui.messengertreeWidget->header ();

    Settings->beginGroup(_name);

    if (bLoad) {
        // load settings

        // state of messenger tree
        header->restoreState(Settings->value("MessengerTree").toByteArray());

        // state of actionHide_Offline_Friends
        ui.actionHide_Offline_Friends->setChecked(Settings->value("hideOfflineFriends", false).toBool());

        // state of actionSort_by_State
        ui.actionSort_by_State->setChecked(Settings->value("sortByState", false).toBool());

        // state of actionRoot_is_decorated
        ui.actionRoot_is_decorated->setChecked(Settings->value("rootIsDecorated", true).toBool());
        on_actionRoot_is_decorated_activated();
    } else {
        // save settings

        // state of messenger tree
        Settings->setValue("MessengerTree", header->saveState());

        // state of actionSort_by_State
        Settings->setValue("sortByState", ui.actionSort_by_State->isChecked());

        // state of actionHide_Offline_Friends
        Settings->setValue("hideOfflineFriends", ui.actionHide_Offline_Friends->isChecked());

        // state of actionRoot_is_decorated
        Settings->setValue("rootIsDecorated", ui.actionRoot_is_decorated->isChecked());
    }

    Settings->endGroup();
}

void MessengerWindow::messengertreeWidgetCostumPopupMenu( QPoint /*point*/ )
{
      QTreeWidgetItem *c = getCurrentPeer();

      QMenu contextMnu( this );

      QAction* expandAll = new QAction(tr( "Expand all" ), &contextMnu );
      connect( expandAll , SIGNAL( triggered() ), ui.messengertreeWidget, SLOT (expandAll()) );

      QAction* collapseAll = new QAction(tr( "Collapse all" ), &contextMnu );
      connect( collapseAll , SIGNAL( triggered() ), ui.messengertreeWidget, SLOT(collapseAll()) );

#ifndef MINIMAL_RSGUI
      QAction* chatAct = new QAction(QIcon(IMAGE_CHAT), tr( "Chat" ), &contextMnu );
      if (c) {
          connect( chatAct , SIGNAL( triggered() ), this, SLOT( chatfriendproxy() ) );
      } else {
          chatAct->setDisabled(true);
      }

      QAction* sendMessageAct = new QAction(QIcon(IMAGE_MSG), tr( "Message Friend" ), &contextMnu );
      if (c) {
          connect( sendMessageAct , SIGNAL( triggered() ), this, SLOT( sendMessage() ) );
      } else {
          sendMessageAct->setDisabled(true);
      }
#endif // MINIMAL_RSGUI

      QAction* connectfriendAct = new QAction(QIcon(IMAGE_CONNECT), tr( "Connect To Friend" ), &contextMnu );
      if (c) {
          connect( connectfriendAct , SIGNAL( triggered() ), this, SLOT( connectfriend() ) );
      } else {
          connectfriendAct->setDisabled(true);
      }

#ifndef MINIMAL_RSGUI
      QAction* configurefriendAct = new QAction(QIcon(IMAGE_PEERINFO), tr( "Peer Details" ), &contextMnu );
      if (c) {
          connect( configurefriendAct , SIGNAL( triggered() ), this, SLOT( configurefriend() ) );
      } else {
          configurefriendAct->setDisabled(true);
      }

      QAction* recommendfriendAct = new QAction(QIcon(IMAGE_EXPIORTFRIEND), tr( "Recomend this Friend to..." ), &contextMnu );
      if (c && c->type() == 0) {
          connect( recommendfriendAct , SIGNAL( triggered() ), this, SLOT( recommendfriend() ) );
      } else {
          recommendfriendAct->setDisabled(true);
      }

      QAction* pastePersonAct = new QAction(QIcon(IMAGE_PASTELINK), tr( "Paste RetroShare Link" ), &contextMnu );
      if(!RSLinkClipboard::empty(RetroShareLink::TYPE_PERSON)) {
          connect( pastePersonAct , SIGNAL( triggered() ), this, SLOT( pastePerson() ) );
      } else {
          pastePersonAct->setDisabled(true);
      }

      //QAction* profileviewAct = new QAction(QIcon(IMAGE_PEERINFO), tr( "Profile View" ), &contextMnu );
      //connect( profileviewAct , SIGNAL( triggered() ), this, SLOT( viewprofile() ) );

      QAction* exportfriendAct = new QAction(QIcon(IMAGE_EXPIORTFRIEND), tr( "Export Friend" ), &contextMnu );
      if (c) {
          connect( exportfriendAct , SIGNAL( triggered() ), this, SLOT( exportfriend() ) );
      } else {
          exportfriendAct->setDisabled(true);
      }

      QAction* removefriendAct = new QAction(QIcon(IMAGE_REMOVEFRIEND), tr( "Deny Friend" ), &contextMnu );
      if (c) {
          if (c->type() == 1) {
              //this is a SSL key
              removefriendAct->setText(tr( "Remove Friend Location"));
          }
          connect( removefriendAct , SIGNAL( triggered() ), this, SLOT( removefriend() ) );
      } else {
          removefriendAct->setDisabled(true);
      }
#endif // MINIMAL_RSGUI

      QWidget *widget = new QWidget();
      widget->setStyleSheet( ".QWidget{background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1,stop:0 #FEFEFE, stop:1 #E8E8E8); border: 1px solid #CCCCCC;}");  
      
      QHBoxLayout *hbox = new QHBoxLayout(&contextMnu);
      hbox->setMargin(0);
      hbox->setSpacing(6);
    
      QLabel *iconLabel = new QLabel(&contextMnu);
      iconLabel->setPixmap(QPixmap(":/images/user/friends24.png"));
      iconLabel->setMaximumSize( iconLabel->frameSize().height() + 24, 24 );
      hbox->addWidget(iconLabel);

      QLabel *textLabel;
      textLabel = new QLabel( tr("<strong>RetroShare instance</strong>"), widget );
      if (c && c->type() == 0) {
          //this is a GPG key
          textLabel->setText(tr("<strong>GPG Key</strong>"));
      }

      hbox->addWidget(textLabel);

      QSpacerItem *spacerItem = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
      hbox->addItem(spacerItem); 
       
      widget->setLayout( hbox );
    
      QWidgetAction *widgetAction = new QWidgetAction(this); 
      widgetAction->setDefaultWidget(widget); 

      contextMnu.addAction( widgetAction);
#ifndef MINIMAL_RSGUI
      contextMnu.addAction( chatAct);
      contextMnu.addAction( sendMessageAct);
      contextMnu.addAction( configurefriendAct);
      //contextMnu.addAction( profileviewAct);
      contextMnu.addAction( recommendfriendAct);
#endif // MINIMAL_RSGUI
      contextMnu.addAction( connectfriendAct);
#ifndef MINIMAL_RSGUI
      contextMnu.addAction(pastePersonAct);
      contextMnu.addAction( removefriendAct);
#endif // MINIMAL_RSGUI
      //contextMnu.addAction( exportfriendAct);
      contextMnu.addSeparator();
      contextMnu.addAction( expandAll);
      contextMnu.addAction( collapseAll);
      contextMnu.exec(QCursor::pos());
}

void MessengerWindow::updateMessengerDisplay()
{
    if (RsAutoUpdatePage::eventsLocked() == false) {
        insertPeers();
    }

    timer->start();
}

/* get the list of peers from the RsIface.  */
void  MessengerWindow::insertPeers()
{
    std::list<std::string> gpgFriends;
    std::list<std::string>::iterator it;
    std::list<StatusInfo> statusInfo;
#ifndef MINIMAL_RSGUI
    rsStatus->getStatusList(statusInfo);
#endif // MINIMAL_RSGUI

    if (!rsPeers) {
        /* not ready yet! */
        std::cerr << "FriendsDialog::insertPeers() not ready yet : rsPeers unintialized."  << std::endl;
        return;
    }

    std::list<std::string> privateChatIds;
#ifndef MINIMAL_RSGUI
    rsMsgs->getPrivateChatQueueIds(true, privateChatIds);
#endif // MINIMAL_RSGUI

    rsPeers->getGPGAcceptedList(gpgFriends);

    std::string sOwnId = rsPeers->getGPGOwnId();

    //add own gpg id, if we have more than on location (ssl client)
    std::list<std::string> ownSslContacts;
    rsPeers->getAssociatedSSLIds(sOwnId, ownSslContacts);
    if (ownSslContacts.size() > 0) {
        gpgFriends.push_back(sOwnId);
    }

    /* get a link to the table */
    QTreeWidget *peertreeWidget = ui.messengertreeWidget;

    bool sortState = ui.actionSort_by_State->isChecked();
    bool hideOfflineFriends = ui.actionHide_Offline_Friends->isChecked();

    //remove items that are not fiends anymore
    int itemCount = peertreeWidget->topLevelItemCount();
    int index = 0;
    while (index < itemCount) {
        std::string gpg_widget_id = peertreeWidget->topLevelItem(index)->data(COLUMN_DATA, ROLE_ID).toString().toStdString();
        if (std::find(gpgFriends.begin(), gpgFriends.end(), gpg_widget_id) == gpgFriends.end()) {
            delete (peertreeWidget->takeTopLevelItem(index));
            // count again
            itemCount = peertreeWidget->topLevelItemCount();
        } else {
            index++;
        }
    }

    //add the gpg friends
    for(it = gpgFriends.begin(); it != gpgFriends.end(); it++) {
        /* make a widget per friend */
        QTreeWidgetItem *gpg_item = NULL;
        QTreeWidgetItem *gpg_item_loop = NULL;
        QString gpgid = QString::fromStdString(*it);
        itemCount = peertreeWidget->topLevelItemCount();
        for (int nIndex = 0; nIndex < itemCount; nIndex++) {
            gpg_item_loop = peertreeWidget->topLevelItem(nIndex);
            if (gpg_item_loop->data(COLUMN_DATA, ROLE_ID).toString() == gpgid) {
                gpg_item = gpg_item_loop;
                break;
            }
        }

        RsPeerDetails detail;
        if ((!rsPeers->getPeerDetails(*it, detail) || !detail.accept_connection)
            && detail.gpg_id != sOwnId) {
            //don't accept anymore connection, remove from the view
            delete (peertreeWidget->takeTopLevelItem(peertreeWidget->indexOfTopLevelItem(gpg_item)));
            continue;
        }

        if (gpg_item == NULL) {
            gpg_item = new RSTreeWidgetItem(m_compareRole, 0); //set type to 0 for custom popup menu
            gpg_item->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
        }

        gpg_item -> setTextAlignment(COLUMN_NAME, Qt::AlignLeft | Qt::AlignVCenter );
        gpg_item -> setSizeHint(COLUMN_NAME,  QSize( 40,40 ) );

        /* not displayed, used to find back the item */
        gpg_item -> setData(COLUMN_DATA, ROLE_ID, QString::fromStdString(detail.id));

        //remove items that are not friends anymore
        int childCount = gpg_item->childCount();
        int childIndex = 0;
        while (childIndex < childCount) {
            std::string ssl_id = gpg_item->child(childIndex)->data(COLUMN_DATA, ROLE_ID).toString().toStdString();
            if (!rsPeers->isFriend(ssl_id)) {
                delete (gpg_item->takeChild(childIndex));
                // count again
                childCount = gpg_item->childCount();
            } else {
                childIndex++;
            }
        }

        //update the childs (ssl certs)
        bool gpg_connected = false;
        bool gpg_online = false;
        bool gpg_hasPrivateChat = false;
        std::list<std::string> sslContacts;
        std::map<std::string, std::string> sslLocations;
        std::map<std::string, QString> sslCustomStateStrings;

        rsPeers->getAssociatedSSLIds(detail.gpg_id, sslContacts);
        for(std::list<std::string>::iterator sslIt = sslContacts.begin(); sslIt != sslContacts.end(); sslIt++) {
            QTreeWidgetItem *sslItem = NULL;

            //find the corresponding sslItem child item of the gpg item
            bool newChild = true;
            childCount = gpg_item->childCount();
            for (int childIndex = 0; childIndex < childCount; childIndex++) {
                if (gpg_item->child(childIndex)->data(COLUMN_DATA, ROLE_ID).toString().toStdString() == *sslIt) {
                    sslItem = gpg_item->child(childIndex);
                    newChild = false;
                    break;
                }
            }

            RsPeerDetails sslDetail;
            if (!rsPeers->getPeerDetails(*sslIt, sslDetail) || !rsPeers->isFriend(*sslIt)) {
                std::cerr << "Removing widget from the view : id : " << *sslIt << std::endl;
                if (sslItem) {
                    //child has disappeared, remove it from the gpg_item
                    gpg_item->removeChild(sslItem);
                }
                continue;
            }

            if (sslItem == NULL) {
                sslItem = new RSTreeWidgetItem(m_compareRole, 1); //set type to 1 for custom popup menu
            }

            /* not displayed, used to find back the item */
            sslItem -> setData(COLUMN_DATA, ROLE_ID, QString::fromStdString(sslDetail.id));

            /* store location */
            sslLocations[sslDetail.id] = sslDetail.location;

            QString sCustomString;
#ifndef MINIMAL_RSGUI
            if (sslDetail.state & RS_PEER_STATE_CONNECTED) {
                sCustomString = QString::fromUtf8(rsMsgs->getCustomStateString(sslDetail.id).c_str());
            }
#endif // MINIMAL_RSGUI
            if (sCustomString.isEmpty()) {
                sslItem -> setText( COLUMN_NAME, tr("location") + " : " + QString::fromUtf8(sslDetail.location.c_str()) + " " + StatusDefs::connectStateString(sslDetail));
                sslItem -> setToolTip( COLUMN_NAME, tr("location") + " : " + QString::fromUtf8(sslDetail.location.c_str()));
            } else {
                sslItem -> setText( COLUMN_NAME, tr("location") + " : " + QString::fromUtf8(sslDetail.location.c_str()) + " " + StatusDefs::connectStateString(sslDetail));
                sslItem -> setToolTip( COLUMN_NAME, tr("location") + " : " + QString::fromUtf8(sslDetail.location.c_str()) + " - " + sCustomString);

                /* store custom state string */
                sslCustomStateStrings[sslDetail.id] = sCustomString;
            }

            QIcon sslIcon;
            QFont sslFont;
            QColor sslColor;
            if (sslDetail.state & RS_PEER_STATE_CONNECTED) {
                sslItem->setHidden(false);
                gpg_connected = true;

#ifdef MINIMAL_RSGUI
// to show the gpg as online, remove it with MINIMAL_RSGUI
                QFont font1;
                font1.setBold(true);

                gpg_item->setToolTip(COLUMN_NAME, StatusDefs::tooltip(RS_STATUS_ONLINE));
                gpg_item->setData(COLUMN_NAME, ROLE_SORT, BuildStateSortString(sortState, gpg_item->text(COLUMN_NAME), PEER_STATE_ONLINE));

                QColor textColor = StatusDefs::textColor(RS_STATUS_ONLINE);
                QFont font = StatusDefs::font(RS_STATUS_ONLINE);
                for(int i = 0; i < COLUMN_COUNT; i++) {
                    gpg_item -> setTextColor(i, textColor);
                    gpg_item -> setFont(i, font);
                }
#endif // MINIMAL_RSGUI

                /* change color and icon */
                sslIcon = QIcon(":/images/connect_established.png");
                sslFont.setBold(true);
                sslColor = Qt::darkBlue;
            } else if (sslDetail.state & RS_PEER_STATE_ONLINE) {
                sslItem->setHidden(hideOfflineFriends);
                gpg_online = true;

                if (sslDetail.connectState) {
                    sslIcon = QIcon(":/images/connect_creating.png");
                } else {
                    sslIcon = QIcon(":/images/connect_no.png");
                }

                sslFont.setBold(true);
                sslColor = Qt::black;
            } else {
                sslItem->setHidden(hideOfflineFriends);
                if (sslDetail.connectState) {
                    sslIcon = QIcon(":/images/connect_creating.png");
                } else {
                    sslIcon = QIcon(":/images/connect_no.png");
                }

                sslFont.setBold(false);
                sslColor = Qt::black;
            }

            if (std::find(privateChatIds.begin(), privateChatIds.end(), sslDetail.id) != privateChatIds.end()) {
                // private chat is available
                sslIcon = QIcon(":/images/chat.png");
                gpg_hasPrivateChat = true;
            }
            sslItem -> setIcon(COLUMN_NAME, sslIcon);

            for (int i = 0; i < COLUMN_COUNT; i++) {
                sslItem -> setTextColor(i, sslColor);
                sslItem -> setFont(i, sslFont);
            }

#ifdef PEERS_DEBUG
            std::cerr << "FriendsDialog::insertPeers() inserting sslItem." << std::endl;
#endif
            /* add sl child to the list. If item is already in the list, it won't be duplicated thanks to Qt */
            gpg_item->addChild(sslItem);
            if (newChild) {
                gpg_item->setExpanded(true);
            }
        }

        int i = 0;
        QIcon gpgIcon;

        if (gpg_connected) {
            gpg_item->setHidden(false);

#ifdef MINIMAL_RSGUI
            gpgIcon = QIcon(StatusDefs::imageIM(RS_STATUS_ONLINE));
#else
            int bestPeerState = 0;        // for gpg item
            std::string bestSslId;        // for gpg item
            unsigned int bestRSState = 0; // for gpg item

            std::list<StatusInfo>::iterator it = statusInfo.begin();
            for(; it != statusInfo.end() ; it++){

                // don't forget the kids
                std::list<std::string>::iterator cont_it = sslContacts.begin();
                for(; cont_it != sslContacts.end(); cont_it++){

                    if((it->id == *cont_it) && (rsPeers->isOnline(*cont_it))) {

                        int peerState = 0;

                        gpg_item -> setText(COLUMN_STATE, StatusDefs::name(it->status));

                        unsigned char *data = NULL;
                        int size = 0 ;
                        rsMsgs->getAvatarData(it->id ,data,size);

                        if(size != 0){
                            QPixmap avatar ;
                            avatar.loadFromData(data,size,"PNG") ;
                            QIcon avatar_icon(avatar);
                            gpg_item-> setIcon(COLUMN_STATE, avatar_icon);
                            delete[] data;

                        } else {
                            gpg_item -> setIcon(COLUMN_STATE,(QIcon(":/images/no_avatar_70.png")));
                        }

                        switch (it->status) {
                        case RS_STATUS_INACTIVE:
                            peerState = PEER_STATE_INACTIVE;
                            break;

                        case RS_STATUS_ONLINE:
                            peerState = PEER_STATE_ONLINE;
                            break;

                        case RS_STATUS_AWAY:
                            peerState = PEER_STATE_AWAY;
                            break;

                        case RS_STATUS_BUSY:
                            peerState = PEER_STATE_BUSY;
                            break;
                        }

                        /* find the best ssl contact for the gpg item */
                        if (bestPeerState == 0) {
                            /* first ssl contact */
                            bestPeerState = peerState;
                            bestSslId = *cont_it;
                            bestRSState = it->status;
                        } else if (peerState < bestPeerState) {
                            /* higher state */
                            bestPeerState = peerState;
                            bestSslId = *cont_it;
                            bestRSState = it->status;
                        } else if (peerState == bestPeerState) {
                            /* equal state */

                            /* use the ssl id with existing custom state string */
                            std::map<std::string, QString>::iterator it1 = sslCustomStateStrings.find(bestSslId);
                            std::map<std::string, QString>::iterator it2 = sslCustomStateStrings.find(*cont_it);

                            if (it1 == sslCustomStateStrings.end()) {
                                if (it2 == sslCustomStateStrings.end()) {
                                    /* both with no custom state string ... use first */
                                } else {
                                    /* second with a custom state string ... use second */
                                    bestPeerState = peerState;
                                    bestSslId = *cont_it;
                                }
                            } else {
                                /* use first */
                            }
                        }
                    }
                }
            }

            if (bestPeerState == 0) {
                // show as online
                bestPeerState = PEER_STATE_ONLINE;
                bestRSState = RS_STATUS_ONLINE;
            }

            QColor textColor = StatusDefs::textColor(bestRSState);
            QFont font = StatusDefs::font(bestRSState);
            for(i = 0; i < COLUMN_COUNT; i++) {
                gpg_item -> setTextColor(i, textColor);
                gpg_item -> setFont(i, font);
            }

            gpgIcon = QIcon(StatusDefs::imageIM(bestRSState));

            gpg_item->setText(COLUMN_NAME, QString::fromUtf8(detail.name.c_str()));
            gpg_item -> setToolTip(COLUMN_NAME, StatusDefs::tooltip(bestRSState));

            std::map<std::string, QString>::iterator customStateString = sslCustomStateStrings.find(bestSslId);
            if (customStateString == sslCustomStateStrings.end()) {
//                std::map<std::string, std::string>::iterator location = sslLocations.find(bestSslId);
//                if (location == sslLocations.end()) {
//                    /* show only the name */
//                    gpg_item->setText(COLUMN_NAME, QString::fromStdString(detail.name));
//                } else {
//                    /* show the name with location */
//                    gpg_item->setText(COLUMN_NAME, QString::fromStdString(detail.name) + "\n" + QString::fromStdString(location->second));
//                }

                /* use state string for location */
                QString stateString;
                stateString = StatusDefs::name(bestRSState);

                if (stateString.isEmpty()) {
                    /* show only the name */
                    gpg_item->setText(COLUMN_NAME, QString::fromUtf8(detail.name.c_str()));
                } else {
                    /* show the name with location */
                    gpg_item->setText(COLUMN_NAME, QString::fromUtf8(detail.name.c_str()) + "\n" + stateString);
                }
            } else {
                /* show the name with custom state string */
                gpg_item->setText(COLUMN_NAME, QString::fromUtf8(detail.name.c_str()) + "\n" + customStateString->second);
            }

            gpg_item->setData(COLUMN_NAME, ROLE_SORT, BuildStateSortString(sortState, gpg_item->text(COLUMN_NAME), bestPeerState));

#endif // MINIMAL_RSGUI
        } else if (gpg_online) {
            gpg_item->setHidden(hideOfflineFriends);
            gpg_item->setText(COLUMN_NAME, QString::fromUtf8(detail.name.c_str()));
            gpgIcon = QIcon(IMAGE_AVAIBLE);
            gpg_item->setData(COLUMN_NAME, ROLE_SORT, BuildStateSortString(sortState, gpg_item->text(COLUMN_NAME), PEER_STATE_AVAILABLE));
            QFont font;
            font.setBold(true);
            for(i = 0; i < COLUMN_COUNT; i++) {
                gpg_item -> setTextColor(i,(Qt::black));
                gpg_item -> setFont(i,font);
            }
        } else {
            gpg_item->setHidden(hideOfflineFriends);
            gpg_item->setText(COLUMN_NAME, QString::fromUtf8(detail.name.c_str()));
            gpgIcon = QIcon(StatusDefs::imageIM(RS_STATUS_OFFLINE));
            gpg_item->setData(COLUMN_NAME, ROLE_SORT, BuildStateSortString(sortState, gpg_item->text(COLUMN_NAME), PEER_STATE_OFFLINE));

            QColor textColor = StatusDefs::textColor(RS_STATUS_OFFLINE);
            QFont font = StatusDefs::font(RS_STATUS_OFFLINE);
            for(i = 0; i < COLUMN_COUNT; i++) {
                gpg_item -> setTextColor(i, textColor);
                gpg_item -> setFont(i, font);
            }
        }

        if (gpg_hasPrivateChat) {
            gpgIcon = QIcon(":/images/chat.png");
        }

        gpg_item -> setIcon(COLUMN_NAME, gpgIcon);

        /* add gpg item to the list. If item is already in the list, it won't be duplicated thanks to Qt */
        peertreeWidget->addTopLevelItem(gpg_item);

        if (expandedPeers && expandedPeers->find(detail.gpg_id) != expandedPeers->end()) {
            /* we have information about expanded peers and the peer was expanded */
            gpg_item->setExpanded(true);
        }
    }

    if (ui.filterPatternLineEdit->text().isEmpty() == false) {
        FilterItems();
    }

    if (expandedPeers) {
        /* we don't need the informations anymore */
        delete(expandedPeers);
        expandedPeers = NULL;
    }

    QTreeWidgetItem *c = getCurrentPeer();
    if (c && c->isHidden()) {
        // active item is hidden, deselect it
        ui.messengertreeWidget->setCurrentItem(NULL);
    }
}

/* Utility Fns */
std::string getPeersRsCertId(QTreeWidgetItem *i)
{
    std::string id = i -> data(COLUMN_DATA, ROLE_ID).toString().toStdString();
    return id;
}

#ifndef MINIMAL_RSGUI
/** Add a Friend ShortCut */
void MessengerWindow::addFriend()
{
    ConnectFriendWizard connwiz (this);

    connwiz.exec ();
}

/** Open a QFileDialog to browse for export a file. */
void MessengerWindow::exportfriend()
{
        QTreeWidgetItem *c = getCurrentPeer();

#ifdef PEERS_DEBUG
        std::cerr << "FriendsDialog::exportfriend()" << std::endl;
#endif
	if (!c)
	{
#ifdef PEERS_DEBUG
                std::cerr << "FriendsDialog::exportfriend() None Selected -- sorry" << std::endl;
#endif
		return;
	}

	std::string id = getPeersRsCertId(c);
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save Certificate"), "",
	                                                     tr("Certificates (*.pqi)"));

	std::string file = fileName.toStdString();
	if (file != "")
	{
#ifdef PEERS_DEBUG
        	std::cerr << "FriendsDialog::exportfriend() Saving to: " << file << std::endl;
        	std::cerr << std::endl;
#endif
		if (rsPeers)
		{
                        rsPeers->saveCertificateToFile(id, file);
		}
	}

}

void MessengerWindow::chatfriendproxy()
{
    chatfriend(getCurrentPeer());
}

void MessengerWindow::chatfriend(QTreeWidgetItem *pPeer)
{
    if (pPeer == NULL) {
        return;
    }

    std::string id = pPeer->data(COLUMN_DATA, ROLE_ID).toString().toStdString();
    PopupChatDialog::chatFriend(id);
}
#endif // MINIMAL_RSGUI

QTreeWidgetItem *MessengerWindow::getCurrentPeer()
{
	/* get the current, and extract the Id */

	/* get a link to the table */
        QTreeWidget *peerWidget = ui.messengertreeWidget;
        QTreeWidgetItem *item = peerWidget -> currentItem();
        if (!item)
        {
#ifdef PEERS_DEBUG
		std::cerr << "Invalid Current Item" << std::endl;
#endif
		return NULL;
	}

#ifdef PEERS_DEBUG
	/* Display the columns of this item. */
	std::ostringstream out;
        out << "CurrentPeerItem: " << std::endl;

	for(int i = 1; i < 6; i++)
	{
		QString txt = item -> text(i);
		out << "\t" << i << ":" << txt.toStdString() << std::endl;
	}
	std::cerr << out.str();
#endif
	return item;
}

/* So from the Peers Dialog we can call the following control Functions:
 * (1) Remove Current.              FriendRemove(id)
 * (2) Allow/DisAllow.              FriendStatus(id, accept)
 * (2) Connect.                     FriendConnectAttempt(id, accept)
 * (3) Set Address.                 FriendSetAddress(id, str, port)
 * (4) Set Trust.                   FriendTrustSignature(id, bool)
 * (5) Configure (GUI Only) -> 3/4
 *
 * All of these rely on the finding of the current Id.
 */

#ifndef MINIMAL_RSGUI
void MessengerWindow::removefriend()
{
        QTreeWidgetItem *c = getCurrentPeer();
#ifdef PEERS_DEBUG
        std::cerr << "FriendsDialog::removefriend()" << std::endl;
#endif
	if (!c)
	{
#ifdef PEERS_DEBUG
        	std::cerr << "FriendsDialog::removefriend() Noone Selected -- sorry" << std::endl;
#endif
		return;
	}

	if (rsPeers)
	{
		rsPeers->removeFriend(getPeersRsCertId(c));
		emit friendsUpdated() ;
	}
}
#endif // MINIMAL_RSGUI

void MessengerWindow::connectfriend()
{
    QTreeWidgetItem *c = getCurrentPeer();
#ifdef PEERS_DEBUG
    std::cerr << "FriendsDialog::connectfriend()" << std::endl;
#endif
    if (!c)
    {
#ifdef PEERS_DEBUG
        std::cerr << "FriendsDialog::connectfriend() Noone Selected -- sorry" << std::endl;
#endif
        return;
    }

    if (rsPeers)
    {
        if (c->type() == 0) {
            int childCount = c->childCount();
            for (int childIndex = 0; childIndex < childCount; childIndex++) {
                QTreeWidgetItem *item = c->child(childIndex);
                if (item->type() == 1) {
                    rsPeers->connectAttempt(getPeersRsCertId(item));
                    item->setIcon(COLUMN_NAME,(QIcon(IMAGE_CONNECT2)));
                }
            }
        } else {
            //this is a SSL key
            rsPeers->connectAttempt(getPeersRsCertId(c));
            c->setIcon(COLUMN_NAME,(QIcon(IMAGE_CONNECT2)));
        }
    }
}

#ifndef MINIMAL_RSGUI
/* GUI stuff -> don't do anything directly with Control */
void MessengerWindow::configurefriend()
{
    ConfCertDialog::showIt(getPeersRsCertId(getCurrentPeer()), ConfCertDialog::PageDetails);
}

void MessengerWindow::recommendfriend()
{
    QTreeWidgetItem *peer = getCurrentPeer();

    if (!peer)
        return;

    std::list <std::string> ids;
    ids.push_back(peer->data(COLUMN_DATA, ROLE_ID).toString().toStdString());
    MessageComposer::recommendFriend(ids);
}

void MessengerWindow::pastePerson()
{
    RSLinkClipboard::process(RetroShareLink::TYPE_PERSON, RSLINK_PROCESS_NOTIFY_ERROR);
}
#endif // MINIMAL_RSGUI

//============================================================================


void MessengerWindow::closeEvent (QCloseEvent * /*event*/)
{
    /* save the expanded peers */
    if (expandedPeers == NULL) {
        expandedPeers = new std::set<std::string>;
    } else {
        expandedPeers->clear();
    }

    for (int nIndex = 0; nIndex < ui.messengertreeWidget->topLevelItemCount(); nIndex++) {
        QTreeWidgetItem *item = ui.messengertreeWidget->topLevelItem(nIndex);
        if (item->isExpanded()) {
            expandedPeers->insert(expandedPeers->end(), item->data(COLUMN_DATA, ROLE_ID).toString().toStdString());
        }
    }

}

LogoBar & MessengerWindow::getLogoBar() const {
        return *_rsLogoBarmessenger;
}

#ifndef MINIMAL_RSGUI
/** Shows Share Manager */
void MessengerWindow::openShareManager()
{
	ShareManager::showYourself();
}

void MessengerWindow::sendMessage()
{
    QTreeWidgetItem *peer = getCurrentPeer();

    if (!peer)
        return;

    std::string id = peer->data(COLUMN_DATA, ROLE_ID).toString().toStdString();
    MessageComposer::msgFriend(id, false);
}

void MessengerWindow::updateAvatar()
{
	unsigned char *data = NULL;
	int size = 0 ;

	rsMsgs->getOwnAvatarData(data,size); 

	std::cerr << "Image size = " << size << std::endl ;

	if(size == 0)
	   std::cerr << "Got no image" << std::endl ;

	// set the image
	QPixmap pix ;
	pix.loadFromData(data,size,"PNG") ;
	ui.avatarButton->setIcon(pix); // writes image into ba in PNG format

	delete[] data ;
}

void MessengerWindow::getAvatar()
{
	QByteArray ba;
	if (misc::getOpenAvatarPicture(this, ba))
	{
#ifdef MSG_DEBUG
		std::cerr << "Avatar image size = " << ba.size() << std::endl ;
#endif
		rsMsgs->setOwnAvatarData((unsigned char *)(ba.data()), ba.size()) ;	// last char 0 included.
	}
}

/** Loads own personal status message */
void MessengerWindow::loadmystatusmessage()
{ 
    ui.messagelineEdit->setEditText( QString::fromUtf8(rsMsgs->getCustomStateString().c_str()));
}

/** Save own status message */
void MessengerWindow::savestatusmessage()
{
    rsMsgs->setCustomStateString(ui.messagelineEdit->currentText().toUtf8().constData());
}

void MessengerWindow::updateOwnStatus(const QString &peer_id, int status)
{
    // add self nick + own status
    if (peer_id.toStdString() == rsPeers->getOwnId())
    {
        // my status has changed

        ui.statusButton->setText(m_nickName + " (" + StatusDefs::name(status) + ")");

        switch (status) {
        case RS_STATUS_OFFLINE:
            ui.avatarButton->setStyleSheet("QToolButton#avatarButton{border-image:url(:/images/mystatus_bg_offline.png); }");
            break;

        case RS_STATUS_INACTIVE:
            ui.avatarButton->setStyleSheet("QToolButton#avatarButton{border-image:url(:/images/mystatus_bg_idle.png); }");
            break;

        case RS_STATUS_ONLINE:
            ui.avatarButton->setStyleSheet("QToolButton#avatarButton{border-image:url(:/images/mystatus_bg_online.png); }");
            break;

        case RS_STATUS_AWAY:
            ui.avatarButton->setStyleSheet("QToolButton#avatarButton{border-image:url(:/images/mystatus_bg_idle.png); }");
            break;

        case RS_STATUS_BUSY:
            ui.avatarButton->setStyleSheet("QToolButton#avatarButton{border-image:url(:/images/mystatus_bg_busy.png); }");
            break;
        }

        return;
    }
}

#endif // MINIMAL_RSGUI

void MessengerWindow::on_actionSort_Peers_Descending_Order_activated()
{
  ui.messengertreeWidget->sortItems ( COLUMN_NAME, Qt::DescendingOrder );
}

void MessengerWindow::on_actionSort_Peers_Ascending_Order_activated()
{
  ui.messengertreeWidget->sortItems ( COLUMN_NAME, Qt::AscendingOrder );
}

void MessengerWindow::on_actionRoot_is_decorated_activated()
{
    ui.messengertreeWidget->setRootIsDecorated(ui.actionRoot_is_decorated->isChecked());
}

void MessengerWindow::displayMenu()
{
    QMenu *lookmenu = new QMenu();
    lookmenu->addAction(ui.actionSort_Peers_Descending_Order);
    lookmenu->addAction(ui.actionSort_Peers_Ascending_Order);
    lookmenu->addAction(ui.actionSort_by_State);
    lookmenu->addAction(ui.actionHide_Offline_Friends);
    lookmenu->addAction(ui.actionRoot_is_decorated);

    ui.displaypushButton->setMenu(lookmenu);
}

/* clear Filter */
void MessengerWindow::clearFilter()
{
    ui.filterPatternLineEdit->clear();
    ui.filterPatternLineEdit->setFocus();
}

void MessengerWindow::filterRegExpChanged()
{

    QString text = ui.filterPatternLineEdit->text();

    if (text.isEmpty()) {
        ui.clearButton->hide();
    } else {
        ui.clearButton->show();
    }

    FilterItems();
}

void MessengerWindow::FilterItems()
{
    QString sPattern = ui.filterPatternLineEdit->text();

    int nCount = ui.messengertreeWidget->topLevelItemCount ();
    for (int nIndex = 0; nIndex < nCount; nIndex++) {
        FilterItem(ui.messengertreeWidget->topLevelItem(nIndex), sPattern);
    }

    QTreeWidgetItem *c = getCurrentPeer();
    if (c && c->isHidden()) {
        // active item is hidden, deselect it
        ui.messengertreeWidget->setCurrentItem(NULL);
    }
}

bool MessengerWindow::FilterItem(QTreeWidgetItem *pItem, QString &sPattern)
{
    bool bVisible = true;

    if (sPattern.isEmpty() == false) {
        if (pItem->text(0).contains(sPattern, Qt::CaseInsensitive) == false) {
            bVisible = false;
        }
    }

    int nVisibleChildCount = 0;
    int nCount = pItem->childCount();
    for (int nIndex = 0; nIndex < nCount; nIndex++) {
        if (FilterItem(pItem->child(nIndex), sPattern)) {
            nVisibleChildCount++;
        }
    }

    if (bVisible || nVisibleChildCount) {
        pItem->setHidden(false);
    } else {
        pItem->setHidden(true);
    }

    return (bVisible || nVisibleChildCount);
}
