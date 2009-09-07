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
#include "common/vmessagebox.h"

#include "rsiface/rsiface.h"
#include "rsiface/rspeers.h"
#include "rsiface/rsmsgs.h"

#include "rshare.h"
#include "MessengerWindow.h"

#include "chat/PopupChatDialog.h"
#include "msgs/ChanMsgDialog.h"
#include "PeersDialog.h"
#include "connect/ConfCertDialog.h"
#include "util/PixmapMerging.h"
#include "LogoBar.h"
#include "util/Widget.h"
//#include "gui/connect/InviteDialog.h"
//#include "gui/connect/AddFriendDialog.h"
#include "gui/connect/ConnectFriendWizard.h"

#include "MessagesPopupDialog.h"
#include "ShareManager.h"

#include <iostream>
#include <sstream>

#include <QContextMenuEvent>
#include <QMenu>
#include <QCursor>
#include <QPoint>
#include <QPixmap>
#include <QMouseEvent>
#include <QHeaderView>


/* Images for context menu icons */
#define IMAGE_REMOVEFRIEND       ":/images/removefriend16.png"
#define IMAGE_EXPIORTFRIEND      ":/images/exportpeers_16x16.png"
#define IMAGE_CHAT               ":/images/chat.png"
#define IMAGE_SENDMESSAGE		 ":/images/message-mail.png"
/* Images for Status icons */
#define IMAGE_ONLINE             ":/images/donline.png"
#define IMAGE_OFFLINE            ":/images/dhidden.png"
/* Images for Status icons */
#define IMAGE_ON                 ":/images/contract_hover.png"
#define IMAGE_OFF                ":/images/expand_hover.png"

/******
 * #define MSG_DEBUG 1
 *****/


/** Constructor */
MessengerWindow::MessengerWindow(QWidget* parent, Qt::WFlags flags)
: RWindow("MessengerWindow", parent, flags)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);
  
	//RshareSettings config;
	//config.loadWidgetInformation(this);

	connect( ui.messengertreeWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( messengertreeWidgetCostumPopupMenu( QPoint ) ) );

	connect( ui.avatarButton, SIGNAL(clicked()), SLOT(getAvatar()));
	connect( ui.shareButton, SIGNAL(clicked()), SLOT(openShareManager()));

    	connect( ui.addIMAccountButton, SIGNAL(clicked( bool ) ), this , SLOT( addFriend2() ) );

    	connect( ui.messengertreeWidget, SIGNAL(itemDoubleClicked ( QTreeWidgetItem *, int)), this, SLOT(chatfriend2()));


  
	/* to hide the header  */
	ui.messengertreeWidget->header()->hide(); 
 
        /* Set header resize modes and initial section sizes */
	ui.messengertreeWidget->setColumnCount(1);

	QHeaderView * _header = ui.messengertreeWidget->header () ;   
	_header->setResizeMode (0, QHeaderView::Interactive);
	//_header->setResizeMode (1, QHeaderView::Interactive);
	//_header->setResizeMode (2, QHeaderView::Interactive);
	//_header->setResizeMode (3, QHeaderView::Interactive);

	_header->resizeSection ( 0, 200 );   

    	/* Create all the dialogs of which we only want one instance */
    	//_mainWindow = new MainWindow();
 
	//LogoBar
	_rsLogoBarmessenger = NULL;
	_rsLogoBarmessenger = new LogoBar(ui.logoframe);
	Widget::createLayout(ui.logoframe)->addWidget(_rsLogoBarmessenger);
  

	ui.statuscomboBox->setMinimumWidth(20);
	ui.messagecomboBox->setMinimumWidth(20);
	ui.searchlineEdit->setMinimumWidth(20);

  	updateAvatar();

  
  /* Hide platform specific features */
#ifdef Q_WS_WIN
#endif
}

void MessengerWindow::messengertreeWidgetCostumPopupMenu( QPoint point )
{

      QMenu contextMnu( this );
      QMouseEvent *mevent = new QMouseEvent( QEvent::MouseButtonPress, point, Qt::RightButton, Qt::RightButton, Qt::NoModifier );

      chatAct = new QAction(QIcon(IMAGE_CHAT), tr( "Chat" ), this );
      connect( chatAct , SIGNAL( triggered() ), this, SLOT( chatfriend2() ) );
      
      sendMessageAct = new QAction(QIcon(IMAGE_SENDMESSAGE), tr( "Send Message" ), this );
      connect( sendMessageAct , SIGNAL( triggered() ), this, SLOT( sendMessage() ) );

      connectfriendAct = new QAction( tr( "Connect To Friend" ), this );
      connect( connectfriendAct , SIGNAL( triggered() ), this, SLOT( connectfriend2() ) );
     
     /************ Do we want these options here??? 
      *
      *
      configurefriendAct = new QAction( tr( "Configure Friend" ), this );
      connect( configurefriendAct , SIGNAL( triggered() ), this, SLOT( configurefriend2() ) );
      
      exportfriendAct = new QAction(QIcon(IMAGE_EXPIORTFRIEND), tr( "Export Friend" ), this );
      connect( exportfriendAct , SIGNAL( triggered() ), this, SLOT( exportfriend2() ) );
      *
      *
      *********/
      
      removefriend2Act = new QAction(QIcon(IMAGE_REMOVEFRIEND), tr( "Remove Friend" ), this );
      connect( removefriend2Act , SIGNAL( triggered() ), this, SLOT( removefriend2() ) );


      contextMnu.clear();
      contextMnu.addAction( chatAct);
      contextMnu.addAction( sendMessageAct);
      contextMnu.addSeparator(); 
      contextMnu.addAction( connectfriendAct);
      contextMnu.addAction( removefriend2Act);

      /**** Do we want these options here??? 
       *
       *
      contextMnu.addAction( configurefriendAct);
      contextMnu.addAction( exportfriendAct);
       *
       *
      ****/

      contextMnu.exec( mevent->globalPos() );
}



/* get the list of peers from the RsIface.  */
void  MessengerWindow::insertPeers()
{
        if (!rsPeers)
        {
                /* not ready yet! */
                return;
        }

        std::list<std::string> peers;
        std::list<std::string>::iterator it;

        rsPeers->getFriendList(peers);

        /* get a link to the table */
        QTreeWidget *peerWidget = ui.messengertreeWidget;

	/* have two lists: online / offline */
        QList<QTreeWidgetItem *> online_items;
        QList<QTreeWidgetItem *> offline_items;

	for(it = peers.begin(); it != peers.end(); it++)
	{

                RsPeerDetails details;
                if (!rsPeers->getPeerDetails(*it, details))
                {
                        continue; /* BAD */
                }

		/* make a widget per friend */
           	QTreeWidgetItem *item = new QTreeWidgetItem((QTreeWidget*)0);

		/* add all the labels */
		/* (0) Person */
		item -> setText(0, QString::fromStdString(details.name));
		/* (1) Org */
		//item -> setText(1, QString::fromStdString(details.org));
		/* (2) Location */
		//item -> setText(2, QString::fromStdString(details.location));
		/* (3) Email */
		//item -> setText(3, QString::fromStdString(details.email));
		
		/* Hidden ones: RsCertId */
		item -> setText(4, QString::fromStdString(details.id));

		/* add to the list */
                if (details.state & RS_PEER_STATE_CONNECTED)
		{
		   online_items.append(item);
		   item -> setIcon(0,(QIcon(IMAGE_ONLINE)));	   
		}
		else
		{
		   offline_items.append(item);
		   item -> setIcon(0,(QIcon(IMAGE_OFFLINE)));
		}
	}

        /* remove old items */
	peerWidget->clear();
	peerWidget->setColumnCount(1);

	if (online_items.size() > 0)
	{
           	QTreeWidgetItem *item = new QTreeWidgetItem((QTreeWidget*)0);

		/* add all the labels */
		/* (0) Person */
		item -> setText(0, "Online");
		item -> addChildren(online_items);
		item -> setIcon(0,(QIcon(IMAGE_ON)));
	        peerWidget->addTopLevelItem(item);
		peerWidget->expandItem(item);
	}

	if (offline_items.size() > 0)
	{
           	QTreeWidgetItem *item = new QTreeWidgetItem((QTreeWidget*)0);

		/* add all the labels */
		/* (0) Person */
		item -> setText(0, "Offline");
		item -> addChildren(offline_items);
		
	        peerWidget->addTopLevelItem(item);
		peerWidget->expandItem(item);
		item -> setIcon(0,(QIcon(IMAGE_OFF)));
	}

	peerWidget->update(); /* update display */
}


/* Utility Fns */
std::string getMessengerPeerRsCertId(QTreeWidgetItem *i)
{
	std::string id = (i -> text(4)).toStdString();
	return id;
}


/** Open a QFileDialog to browse for export a file. */
void MessengerWindow::exportfriend2()
{

}


void MessengerWindow::allowfriend2()
{
	
}


void MessengerWindow::connectfriend2()
{
    bool isOnline;
    QTreeWidgetItem *i = getCurrentPeer(isOnline);
    if (!i)
      return;

    std::string id = (i -> text(4)).toStdString();
    if (rsPeers->isOnline(id))
    {
#ifdef MSG_DEBUG 
	std::cerr << "MessengerWindow::connectfriend2() Already online" << std::endl;
#endif
    }
    else
    {
#ifdef MSG_DEBUG 
	std::cerr << "MessengerWindow::connectfriend2() Trying" << std::endl;
#endif
	rsPeers->connectAttempt(getMessengerPeerRsCertId(i));
    }
}

void MessengerWindow::setaddressfriend2()
{
	
}

void MessengerWindow::trustfriend2()
{
	
}



/* GUI stuff -> don't do anything directly with Control */
void MessengerWindow::configurefriend2()
{
	
}


/** Overloads the default show  */
void MessengerWindow::show()
{

  if (!this->isVisible()) {
    QWidget::show();
  } else {
    QWidget::activateWindow();
    setWindowState(windowState() & ~Qt::WindowMinimized | Qt::WindowActive);
    QWidget::raise();
  }
}

void MessengerWindow::closeEvent (QCloseEvent * event)
{
	//RshareSettings config;
	//config.saveWidgetInformation(this);

    hide();
    event->ignore();
}

/** Shows MessagesPopupDialog */
void MessengerWindow::showMessagesPopup()
{
    static MessagesPopupDialog* messagespopup = new MessagesPopupDialog(this);
    messagespopup->show();
}

/** Shows Share Manager */
void MessengerWindow::openShareManager()
{
	ShareManager::showYourself();

}

/** Shows Messages */
/*void MessengerWindow::showMessages(MainWindow::Page page)
{
    showWindow(page);
}*/


void MessengerWindow::setChatDialog(PeersDialog *cd)
{
  chatDialog = cd;
}

void MessengerWindow::chatfriend2()
{
    bool isOnline;
    QTreeWidgetItem *i = getCurrentPeer(isOnline);
    if (!i)
      return;

    std::string name = (i -> text(0)).toStdString();
    std::string id = (i -> text(4)).toStdString();

    if (!(rsPeers->isOnline(id)))
    {
    	/* info dialog */
        QMessageBox::StandardButton sb = QMessageBox::question ( NULL, 
			"Friend Not Online", 
	"Your Friend is offline \nDo You want to send them a Message instead",
	(QMessageBox::Yes | QMessageBox::No ));
	if (sb == QMessageBox::Yes)
	{
    		rsicontrol -> ClearInMsg();
    		rsicontrol -> SetInMsg(id, true);

    		/* create a message */
    		ChanMsgDialog *nMsgDialog = new ChanMsgDialog(true);

    		nMsgDialog->newMsg();
    		nMsgDialog->show();
	}
	return;
    }

    /* must reference ChatDialog */
    if (chatDialog)
    {
    	chatDialog->getPrivateChat(id, name, true);
    }
}

void MessengerWindow::sendMessage()
{
	bool isOnline;
#ifdef MSG_DEBUG 
    std::cerr << "SharedFilesDialog::msgfriend()" << std::endl;
#endif

    QTreeWidgetItem *i = getCurrentPeer(isOnline);

    if (!i)
	return;

    //std::string status = (i -> text(0)).toStdString();
    std::string name = (i -> text(0)).toStdString();
    std::string id = (i -> text(4)).toStdString();

    rsicontrol -> ClearInMsg();
    rsicontrol -> SetInMsg(id, true);

    /* create a message */
    ChanMsgDialog *nMsgDialog = new ChanMsgDialog(true);

    nMsgDialog->newMsg();
    nMsgDialog->show();
}

QTreeWidgetItem *MessengerWindow::getCurrentPeer(bool &isOnline)
{
	/* get the current, and extract the Id */

	/* get a link to the table */
        QTreeWidget *peerWidget = ui.messengertreeWidget;
        QTreeWidgetItem *item = peerWidget -> currentItem();
        if (!item)
        {
#ifdef MSG_DEBUG 
		std::cerr << "Invalid Current Item" << std::endl;
#endif
		return NULL;
	}

	/* check if parent is online or offline */
        QTreeWidgetItem *parent = item->parent();
	if ((!parent) ||
	    (parent == peerWidget->invisibleRootItem()))
	{
#ifdef MSG_DEBUG 
		std::cerr << "Selected Parent Invalid Item" << std::endl;
#endif
		return NULL;
	}

	isOnline = (parent->text(0) == "<span><strong>Online</strong></span>");

	return item;
}

void MessengerWindow::removefriend2()
{
	bool isOnline;
    QTreeWidgetItem *c = getCurrentPeer(isOnline);
#ifdef MSG_DEBUG 
    std::cerr << "MessengerWindow::removefriend2()" << std::endl;
#endif
	if (!c)
	{
#ifdef MSG_DEBUG 
        	std::cerr << "MessengerWindow::removefriend2() Noone Selected -- sorry" << std::endl;
#endif
		return;
	}
	rsPeers->removeFriend(getMessengerPeerRsCertId(c));
}

void MessengerWindow::changeAvatarClicked() 
{

	updateAvatar();
}


/** Add a Friend ShortCut */
void MessengerWindow::addFriend2()
{
    /* call load Certificate */
#if 0
    std::string id;
    if (connectionsDialog)
    {
        id = connectionsDialog->loadneighbour();
    }

    /* call make Friend */
    if (id != "")
    {
        connectionsDialog->showpeerdetails(id);
    }
    virtual int NeighLoadPEMString(std::string pem, std::string &id)  = 0;
#else
/*
    static  AddFriendDialog *addDialog2 = 
    new AddFriendDialog(networkDialog2, this);

    std::string invite = "";
    addDialog2->setInfo(invite);
    addDialog2->show();
    */
    ConnectFriendWizard* connwiz = new ConnectFriendWizard(this);

    // set widget to be deleted after close
    connwiz->setAttribute( Qt::WA_DeleteOnClose, true);


    connwiz->show();
#endif
}

LogoBar & MessengerWindow::getLogoBar() const {
	return *_rsLogoBarmessenger;
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
	QString fileName = QFileDialog::getOpenFileName(this, "Load File", QDir::homePath(), "Pictures (*.png *.xpm *.jpg)");
	if(!fileName.isEmpty())
	{
		picture = QPixmap(fileName).scaled(82,82, Qt::IgnoreAspectRatio);

		std::cerr << "Sending avatar image down the pipe" << std::endl ;

		// send avatar down the pipe for other peers to get it.
		QByteArray ba;
		QBuffer buffer(&ba);
		buffer.open(QIODevice::WriteOnly);
		picture.save(&buffer, "PNG"); // writes image into ba in PNG format

		std::cerr << "Image size = " << ba.size() << std::endl ;

		rsMsgs->setOwnAvatarData((unsigned char *)(ba.data()),ba.size()) ;	// last char 0 included.

		updateAvatar() ;
	}
}
