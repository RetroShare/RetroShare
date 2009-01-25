/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006,2007 crypton
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

#include "rshare.h"
#include "PeersDialog.h"
#include "rsiface/rsiface.h"
#include "rsiface/rspeers.h"
#include "rsiface/rsstatus.h"
#include "rsiface/rsmsgs.h"
#include "rsiface/rsnotify.h"

#include "chat/PopupChatDialog.h"
#include "msgs/ChanMsgDialog.h"
#include "ChatDialog.h"
#include "connect/ConfCertDialog.h"
#include "profile/ProfileView.h"


#include "gui/Preferences/rsharesettings.h"

#include <iostream>
#include <sstream>

#include <QTextCodec>
#include <QTextEdit>
#include <QTextCursor>
#include <QTextList>
#include <QTextStream>
#include <QTextDocumentFragment>

#include <QContextMenuEvent>
#include <QMenu>
#include <QCursor>
#include <QPoint>
#include <QMouseEvent>
#include <QPixmap>
#include <QMessageBox>
#include <QHeaderView>
#include <QtGui/QKeyEvent>


/* Images for context menu icons */
#define IMAGE_REMOVEFRIEND       ":/images/removefriend16.png"
#define IMAGE_EXPIORTFRIEND      ":/images/exportpeers_16x16.png"
#define IMAGE_PEERINFO           ":/images/peerdetails_16x16.png"
#define IMAGE_CHAT               ":/images/chat.png"
#define IMAGE_MSG                ":/images/message-mail.png"
#define IMAGE_CONNECT            ":/images/connect_friend.png"
/* Images for Status icons */
#define IMAGE_ONLINE             ":/images/user/identity24.png"
#define IMAGE_OFFLINE            ":/images/user/identityoffline24.png"
#define IMAGE_OFFLINE2           ":/images/user/identitylightgrey24.png"


/******
 * #define PEERS_DEBUG 1
 *****/


/** Constructor */
PeersDialog::PeersDialog(QWidget *parent)
: MainPage(parent), chatDialog(NULL)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

  connect( ui.peertreeWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( peertreeWidgetCostumPopupMenu( QPoint ) ) );
  connect( ui.peertreeWidget, SIGNAL( itemDoubleClicked ( QTreeWidgetItem *, int)), this, SLOT(chatfriend()));

  /* hide the Tree +/- */
  ui.peertreeWidget -> setRootIsDecorated( false );
  
    /* Set header resize modes and initial section sizes */
	QHeaderView * _header = ui.peertreeWidget->header () ;
   	_header->setResizeMode (0, QHeaderView::Custom);
	_header->setResizeMode (1, QHeaderView::Interactive);
	_header->setResizeMode (2, QHeaderView::Interactive);
	/*_header->setResizeMode (3, QHeaderView::Interactive);
	_header->setResizeMode (4, QHeaderView::Interactive);
	_header->setResizeMode (5, QHeaderView::Interactive);
	_header->setResizeMode (6, QHeaderView::Interactive);
	_header->setResizeMode (7, QHeaderView::Interactive);*/

    
	_header->resizeSection ( 0, 25 );
	_header->resizeSection ( 1, 100 );
	_header->resizeSection ( 2, 100 );
	/*_header->resizeSection ( 3, 120 );
	_header->resizeSection ( 4, 100 );
	_header->resizeSection ( 5, 230 );
	_header->resizeSection ( 6, 120 );
	_header->resizeSection ( 7, 220 );*/
	
    // set header text aligment
	QTreeWidgetItem * headerItem = ui.peertreeWidget->headerItem();
	headerItem->setTextAlignment(0, Qt::AlignHCenter | Qt::AlignVCenter);
	headerItem->setTextAlignment(1, Qt::AlignHCenter | Qt::AlignVCenter);
	headerItem->setTextAlignment(2, Qt::AlignHCenter | Qt::AlignVCenter);

	
	  
  loadEmoticonsgroupchat();
  
  //setWindowIcon(QIcon(QString(":/images/rstray3.png")));

  connect(ui.lineEdit, SIGNAL(textChanged ( ) ), this, SLOT(checkChat( ) ));
  connect(ui.Sendbtn, SIGNAL(clicked()), this, SLOT(sendMsg()));
  connect(ui.emoticonBtn, SIGNAL(clicked()), this, SLOT(smileyWidgetgroupchat()));

   
  //connect( ui.msgSendList, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( msgSendListCostumPopupMenu( QPoint ) ) );
  connect( ui.msgText, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(displayInfoChatMenu(const QPoint&)));
 
  connect(ui.textboldChatButton, SIGNAL(clicked()), this, SLOT(setFont()));  
  connect(ui.textunderlineChatButton, SIGNAL(clicked()), this, SLOT(setFont()));  
  connect(ui.textitalicChatButton, SIGNAL(clicked()), this, SLOT(setFont()));
  connect(ui.fontsButton, SIGNAL(clicked()), this, SLOT(getFont()));  
  connect(ui.colorChatButton, SIGNAL(clicked()), this, SLOT(setColor()));
   
  ui.fontsButton->setIcon(QIcon(QString(":/images/fonts.png")));
  
  _currentColor = Qt::black;
  QPixmap pxm(16,16);
  pxm.fill(_currentColor);
  ui.colorChatButton->setIcon(pxm);
  
  mCurrentFont = QFont("Comic Sans MS", 12);
  ui.lineEdit->setFont(mCurrentFont);
  
  setChatInfo(tr("Welcome to RetroShare's group chat."), QString::fromUtf8("blue"));
  
  QMenu * grpchatmenu = new QMenu();
  grpchatmenu->addAction(ui.actionClearChat);
  ui.menuButton->setMenu(grpchatmenu);
  
  _underline = false;

  QTimer *timer = new QTimer(this);
  timer->connect(timer, SIGNAL(timeout()), this, SLOT(insertChat()));
  timer->start(500); /* half a second */

	ui.peertreeWidget->sortItems( 1, Qt::AscendingOrder );


  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

void PeersDialog::setChatDialog(ChatDialog *cd)
{
  chatDialog = cd;
}


void PeersDialog::peertreeWidgetCostumPopupMenu( QPoint point )
{

      QMenu contextMnu( this );
      QMouseEvent *mevent = new QMouseEvent( QEvent::MouseButtonPress, point, Qt::RightButton, Qt::RightButton, Qt::NoModifier );

      chatAct = new QAction(QIcon(IMAGE_CHAT), tr( "Chat" ), this );
      connect( chatAct , SIGNAL( triggered() ), this, SLOT( chatfriend() ) );

      msgAct = new QAction(QIcon(IMAGE_MSG), tr( "Message Friend" ), this );
      connect( msgAct , SIGNAL( triggered() ), this, SLOT( msgfriend() ) );

      connectfriendAct = new QAction(QIcon(IMAGE_CONNECT), tr( "Connect To Friend" ), this );
      connect( connectfriendAct , SIGNAL( triggered() ), this, SLOT( connectfriend() ) );
      
      configurefriendAct = new QAction(QIcon(IMAGE_PEERINFO), tr( "Peer Details" ), this );
      connect( configurefriendAct , SIGNAL( triggered() ), this, SLOT( configurefriend() ) );

      profileviewAct = new QAction(QIcon(IMAGE_PEERINFO), tr( "Profile View" ), this );
      connect( profileviewAct , SIGNAL( triggered() ), this, SLOT( viewprofile() ) );
      
      exportfriendAct = new QAction(QIcon(IMAGE_EXPIORTFRIEND), tr( "Export Friend" ), this );
      connect( exportfriendAct , SIGNAL( triggered() ), this, SLOT( exportfriend() ) );
      
      removefriendAct = new QAction(QIcon(IMAGE_REMOVEFRIEND), tr( "Remove Friend" ), this );
      connect( removefriendAct , SIGNAL( triggered() ), this, SLOT( removefriend() ) );


      contextMnu.clear();
      contextMnu.addAction( chatAct);
      contextMnu.addAction( msgAct);
      contextMnu.addSeparator(); 
      contextMnu.addAction( configurefriendAct);
      //contextMnu.addAction( profileviewAct);
      contextMnu.addSeparator();
      contextMnu.addAction( connectfriendAct); 
      contextMnu.addAction( exportfriendAct);
      contextMnu.addAction( removefriendAct);
      contextMnu.exec( mevent->globalPos() );
}



/* get the list of peers from the RsIface.  */
void  PeersDialog::insertPeers()
{
	std::list<std::string> peers;
	std::list<std::string>::iterator it;

	if (!rsPeers)
        {
                /* not ready yet! */
                return;
        }

	rsPeers->getFriendList(peers);

        /* get a link to the table */
        QTreeWidget *peerWidget = ui.peertreeWidget;
        QTreeWidgetItem *oldSelect = getCurrentPeer();
        QTreeWidgetItem *newSelect = NULL;
	time_t now = time(NULL);

        std::string oldId;
        if (oldSelect)
        {
                oldId = (oldSelect -> text(7)).toStdString();
        }

        /* remove old items ??? */
	peerWidget->clear();
	peerWidget->setColumnCount(3);
	

        QList<QTreeWidgetItem *> items;
	for(it = peers.begin(); it != peers.end(); it++)
	{
		RsPeerDetails detail;
		if (!rsPeers->getPeerDetails(*it, detail))
		{
			continue; /* BAD */
		}
		
		/* make a widget per friend */
           	QTreeWidgetItem *item = new QTreeWidgetItem((QTreeWidget*)0);

		/* add all the labels */
		/* First 5 (1-5) Key Items */
		/* () Status Icon */
		item -> setText(0, "");
		
		/* (0) Status */		
		QString status = QString::fromStdString(RsPeerStateString(detail.state));

		/* Append additional status info from status service */
		StatusInfo statusInfo;
		if ((rsStatus) && (rsStatus->getStatus(*it, statusInfo)))
		{
			status.append(QString::fromStdString("/" + RsStatusString(statusInfo.status)));
		}

		//item -> setText(1, status);
		item -> setText(1, QString::fromStdString(detail.autoconnect));
		item -> setTextAlignment(1, Qt::AlignCenter | Qt::AlignVCenter );

		/* (1) Person */
		item -> setText(2, QString::fromStdString(detail.name));

		/* (2) Auto Connect */
		//item -> setText(3, QString::fromStdString(detail.autoconnect));
		item -> setText(3, status);

		/* (3) Trust Level */
                item -> setText(4,QString::fromStdString(
				RsPeerTrustString(detail.trustLvl)));
		
		/* (4) Peer Address */
		{
			std::ostringstream out;
			out << detail.localAddr << ":";
			out << detail.localPort << "/";
			out << detail.extAddr << ":";
			out << detail.extPort;
			item -> setText(5, QString::fromStdString(out.str()));
		}
		
		/* less important ones */
		/* () Last Contact */
                item -> setText(6,QString::fromStdString(
				RsPeerLastConnectString(now - detail.lastConnect)));

		/* () Org */
		//item -> setText(7, QString::fromStdString(detail.org));
		/* () Location */
		//item -> setText(8, QString::fromStdString(detail.location));
		/* () Email */
		//item -> setText(9, QString::fromStdString(detail.email));
	
		/* Hidden ones: RsCertId */
		{
			item -> setText(7, QString::fromStdString(detail.id));
                        if ((oldSelect) && (oldId == detail.id))
                        {
                                newSelect = item;
                        }
		}

		/* ()  AuthCode */	
        //        item -> setText(11, QString::fromStdString(detail.authcode));

		/* change background */
		int i;
		if (detail.state & RS_PEER_STATE_CONNECTED)
		{
			/* bright green */
			for(i = 1; i < 8; i++)
			{
				// CsoLer: I uncommented the color because it's really confortable
				// to be able to see at some distance that people are connected.
				// The blue/gray icons need a close look indeed.
			  item -> setBackground(i,QBrush(Qt::green));
			  item -> setIcon(0,(QIcon(IMAGE_ONLINE)));
			}
		}
		else if (detail.state & RS_PEER_STATE_UNREACHABLE)
		{
			/* bright green */
			for(i = 1; i < 8; i++)
			{
			  item -> setBackground(i,QBrush(Qt::red));
			  item -> setIcon(0,(QIcon(IMAGE_OFFLINE)));
			}
		}
		else if (detail.state & RS_PEER_STATE_ONLINE)
		{
			/* bright green */
			for(i = 1; i < 8; i++)
			{
			  item -> setBackground(i,QBrush(Qt::cyan));
			  item -> setIcon(0,(QIcon(IMAGE_OFFLINE)));
			}
		}
		else 
		{
                	if (now - detail.lastConnect < 3600)
			{
				for(i = 1; i < 8; i++)
				{
				  //item -> setBackground(i,QBrush(Qt::lightGray));
				  item -> setIcon(0,(QIcon(IMAGE_OFFLINE)));
				}
			}
			else
			{
				for(i = 1; i < 8; i++)
				{
				  //item -> setBackground(i,QBrush(Qt::gray));
				  item -> setIcon(0,(QIcon(IMAGE_OFFLINE2)));
				}
			}
		}
			
		/* add to the list */
		items.append(item);
	}

	/* add the items in! */
	peerWidget->insertTopLevelItems(0, items);
        if (newSelect)
        {
                peerWidget->setCurrentItem(newSelect);
        }

	peerWidget->update(); /* update display */
}

/* Utility Fns */
std::string getPeerRsCertId(QTreeWidgetItem *i)
{
	std::string id = (i -> text(7)).toStdString();
	return id;
}

/** Open a QFileDialog to browse for export a file. */
void PeersDialog::exportfriend()
{
        QTreeWidgetItem *c = getCurrentPeer();

#ifdef PEERS_DEBUG 
        std::cerr << "PeersDialog::exportfriend()" << std::endl;
#endif
	if (!c)
	{
#ifdef PEERS_DEBUG 
        	std::cerr << "PeersDialog::exportfriend() Noone Selected -- sorry" << std::endl;
#endif
		return;
	}

	std::string id = getPeerRsCertId(c);
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save Certificate"), "",
	                                                     tr("Certificates (*.pqi)"));

	std::string file = fileName.toStdString();
	if (file != "")
	{
#ifdef PEERS_DEBUG 
        	std::cerr << "PeersDialog::exportfriend() Saving to: " << file << std::endl;
        	std::cerr << std::endl;
#endif
		if (rsPeers)
		{
			rsPeers->SaveCertificateToFile(id, file);
		}
	}

}

void PeersDialog::chatfriend()
{
    QTreeWidgetItem *i = getCurrentPeer();

    if (!i)
	return;

    std::string name = (i -> text(2)).toStdString();
    std::string id = (i -> text(7)).toStdString();
    
    RsPeerDetails detail;
    if (!rsPeers->getPeerDetails(id, detail))
    {
    	return;
    }

    if (detail.state & RS_PEER_STATE_CONNECTED)
    {
    	getPrivateChat(id, name, RS_CHAT_REOPEN);
    }
    else
    {
    	/* info dialog */
       QMessageBox::StandardButton sb = QMessageBox::question ( NULL, 
			"Friend Not Online", 
	"Your Friend is offline \nDo you want to send them a Message instead",
			        (QMessageBox::Yes | QMessageBox::No));
	if (sb == QMessageBox::Yes)
	{
		msgfriend();
	}
    }
    return;
}


void PeersDialog::msgfriend()
{
#ifdef PEERS_DEBUG 
    std::cerr << "SharedFilesDialog::msgfriend()" << std::endl;
#endif

    QTreeWidgetItem *i = getCurrentPeer();

    if (!i)
	return;

    std::string status = (i -> text(1)).toStdString();
    std::string name = (i -> text(2)).toStdString();
    std::string id = (i -> text(7)).toStdString();

    rsicontrol -> ClearInMsg();
    rsicontrol -> SetInMsg(id, true);

    /* create a message */
    ChanMsgDialog *nMsgDialog = new ChanMsgDialog(true);

    nMsgDialog->newMsg();
    nMsgDialog->show();
}


QTreeWidgetItem *PeersDialog::getCurrentPeer()
{
	/* get the current, and extract the Id */

	/* get a link to the table */
        QTreeWidget *peerWidget = ui.peertreeWidget;
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
 

void PeersDialog::removefriend()
{
        QTreeWidgetItem *c = getCurrentPeer();
#ifdef PEERS_DEBUG 
        std::cerr << "PeersDialog::removefriend()" << std::endl;
#endif
	if (!c)
	{
#ifdef PEERS_DEBUG 
        	std::cerr << "PeersDialog::removefriend() Noone Selected -- sorry" << std::endl;
#endif
		return;
	}

	if (rsPeers)
	{
		rsPeers->removeFriend(getPeerRsCertId(c));
	}
}


void PeersDialog::allowfriend()
{
	QTreeWidgetItem *c = getCurrentPeer();
#ifdef PEERS_DEBUG 
	std::cerr << "PeersDialog::allowfriend()" << std::endl;
#endif
	/*
	bool accept = true;
	rsServer->FriendStatus(getPeerRsCertId(c), accept);
	*/
}


void PeersDialog::connectfriend()
{
	QTreeWidgetItem *c = getCurrentPeer();
#ifdef PEERS_DEBUG 
	std::cerr << "PeersDialog::connectfriend()" << std::endl;
#endif
	if (!c)
	{
#ifdef PEERS_DEBUG 
        	std::cerr << "PeersDialog::connectfriend() Noone Selected -- sorry" << std::endl;
#endif
		return;
	}

	if (rsPeers)
	{
		rsPeers->connectAttempt(getPeerRsCertId(c));
	}
}

void PeersDialog::setaddressfriend()
{
	QTreeWidgetItem *c = getCurrentPeer();
#ifdef PEERS_DEBUG 
	std::cerr << "PeersDialog::setaddressfriend()" << std::endl;
#endif

	/* need to get the input address / port */
	/*
 	std::string addr;
	unsigned short port;
	rsServer->FriendSetAddress(getPeerRsCertId(c), addr, port);
	*/
}

void PeersDialog::trustfriend()
{
	QTreeWidgetItem *c = getCurrentPeer();
#ifdef PEERS_DEBUG 
	std::cerr << "PeersDialog::trustfriend()" << std::endl;
#endif
	/*
	bool trust = true;
	rsServer->FriendTrust(getPeerRsCertId(c), trust);
	*/
}



/* GUI stuff -> don't do anything directly with Control */
void PeersDialog::configurefriend()
{
	/* display Dialog */
#ifdef PEERS_DEBUG 
	std::cerr << "PeersDialog::configurefriend()" << std::endl;
#endif
	QTreeWidgetItem *c = getCurrentPeer();


	static ConfCertDialog *confdialog = new ConfCertDialog();


	if (!c)
		return;

	/* set the Id */
	std::string id = getPeerRsCertId(c);

	confdialog -> loadId(id);
	confdialog -> show();
}


void PeersDialog::insertChat()
{
	if (!rsMsgs->chatAvailable())
	{
		return;
	}

	std::list<ChatInfo> newchat;
	if (!rsMsgs->getNewChat(newchat))
	{
		return;
	}

    QTextEdit *msgWidget = ui.msgText;
	std::list<ChatInfo>::iterator it;

        /** A RshareSettings object used for saving/loading settings */
        RshareSettings settings;
        uint chatflags = settings.getChatFlags();

	/* add in lines at the bottom */
	for(it = newchat.begin(); it != newchat.end(); it++)
	{
		std::string msg(it->msg.begin(), it->msg.end());
#ifdef PEERS_DEBUG 
		std::cerr << "PeersDialog::insertChat(): " << msg << std::endl;
#endif

		/* are they private? */
		if (it->chatflags & RS_CHAT_PRIVATE)
		{
			PopupChatDialog *pcd = getPrivateChat(it->rsid, it->name, chatflags);
			pcd->addChatMsg(&(*it));
			continue;
		}

		std::ostringstream out;
		QString currenttxt = msgWidget->toHtml();
		QString extraTxt;

        QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
        QString name = QString::fromStdString(it->name);
        QString line = "<span style=\"color:#C00000\">" + timestamp + "</span>" +			
            		"<span style=\"color:#2D84C9\"><strong>" + " " + name + "</strong></span>";
            		
        extraTxt += line;

        extraTxt += QString::fromStdWString(it->msg);
        
        /* add it everytime */
		currenttxt += extraTxt;
		
		QHashIterator<QString, QString> i(smileys);
		while(i.hasNext())
		{
			i.next();
			foreach(QString code, i.key().split("|"))
				currenttxt.replace(code, "<img src=\"" + i.value() + "\" />");
		}

		
        msgWidget->setHtml(currenttxt);
        

		QScrollBar *qsb =  msgWidget->verticalScrollBar();
		qsb -> setValue(qsb->maximum());
	}
}

void PeersDialog::checkChat()
{
	/* if <return> at the end of the text -> we can send it! */
        QTextEdit *chatWidget = ui.lineEdit;
        std::string txt = chatWidget->toPlainText().toStdString();
	if ('\n' == txt[txt.length()-1])
	{
		//std::cerr << "Found <return> found at end of :" << txt << ": should send!";
		//std::cerr << std::endl;
		if (txt.length()-1 == txt.find('\n')) /* only if on first line! */
		{
			/* should remove last char ... */
			sendMsg();
		}
	}
	else
	{
		//std::cerr << "No <return> found in :" << txt << ":";
		//std::cerr << std::endl;
	}
}

void PeersDialog::sendMsg()
{
    QTextEdit *lineWidget = ui.lineEdit;

	ChatInfo ci;
	//ci.msg = lineWidget->Text().toStdWString();
	ci.msg = lineWidget->toHtml().toStdWString();
	ci.chatflags = RS_CHAT_PUBLIC;

	std::string msg(ci.msg.begin(), ci.msg.end());
#ifdef PEERS_DEBUG 
	std::cerr << "PeersDialog::sendMsg(): " << msg << std::endl;
#endif

	rsMsgs -> ChatSend(ci);
	ui.lineEdit->clear();
	setFont();

	/* redraw send list */
	insertSendList();

}

void  PeersDialog::insertSendList()
{
	std::list<std::string> peers;
	std::list<std::string>::iterator it;

	if (!rsPeers)
	{
		/* not ready yet! */
		return;
	}

	rsPeers->getOnlineList(peers);

        /* get a link to the table */
        //QTreeWidget *sendWidget = ui.msgSendList;
	QList<QTreeWidgetItem *> items;

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

		item -> setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
		//item -> setFlags(Qt::ItemIsUserCheckable);

		item -> setCheckState(0, Qt::Checked);

		if (rsicontrol->IsInChat(*it))
		{
			item -> setCheckState(0, Qt::Checked);
		}
		else
		{
			item -> setCheckState(0, Qt::Unchecked);
		}

		/* disable for the moment */
		item -> setFlags(Qt::ItemIsUserCheckable);
		item -> setCheckState(0, Qt::Checked);

		/* add to the list */
		items.append(item);
	}

        /* remove old items */
	//sendWidget->clear();
	//sendWidget->setColumnCount(1);

	/* add the items in! */
	//sendWidget->insertTopLevelItems(0, items);

	//sendWidget->update(); /* update display */
}


/* to toggle the state */


void PeersDialog::toggleSendItem( QTreeWidgetItem *item, int col )
{
#ifdef PEERS_DEBUG 
	std::cerr << "ToggleSendItem()" << std::endl;
#endif

	/* extract id */
	std::string id = (item -> text(4)).toStdString();

	/* get state */
	bool inChat = (Qt::Checked == item -> checkState(0)); /* alway column 0 */

	/* call control fns */

	rsicontrol -> SetInChat(id, inChat);
	return;
}

PopupChatDialog *PeersDialog::getPrivateChat(std::string id, std::string name, uint chatflags)
{
   /* see if it exists already */
   PopupChatDialog *popupchatdialog = NULL;
   bool show = false;

   if (chatflags & RS_CHAT_REOPEN)
   {
  	show = true;
	std::cerr << "reopen flag so: enable SHOW popupchatdialog()";
	std::cerr << std::endl;
   }


   std::map<std::string, PopupChatDialog *>::iterator it;
   if (chatDialogs.end() != (it = chatDialogs.find(id)))
   {
   	/* exists already */
   	popupchatdialog = it->second;
   }
   else 
   {
   	popupchatdialog = new PopupChatDialog(id, name);
	chatDialogs[id] = popupchatdialog;

	if (chatflags & RS_CHAT_OPEN_NEW)
	{
		std::cerr << "new chat so: enable SHOW popupchatdialog()";
		std::cerr << std::endl;

		show = true;
	}
   }

   if (show)
   {
	std::cerr << "SHOWING popupchatdialog()";
	std::cerr << std::endl;

	popupchatdialog->show();
   }

   /* now only do these if the window is visible */
   if (popupchatdialog->isVisible())
   {
	   if (chatflags & RS_CHAT_FOCUS)
	   {
		std::cerr << "focus chat flag so: GETFOCUS popupchatdialog()";
		std::cerr << std::endl;

		popupchatdialog->getfocus();
	   }
	   else
	   {
		std::cerr << "no focus chat flag so: FLASH popupchatdialog()";
		std::cerr << std::endl;

		popupchatdialog->flash();
	   }
   }	  
   else
   {
	std::cerr << "not visible ... so leave popupchatdialog()";
	std::cerr << std::endl;
   }
	
   return popupchatdialog;
}


void PeersDialog::clearOldChats()
{
	/* nothing yet */

}

void PeersDialog::setColor()
{
	
    	bool ok;
 	QRgb color = QColorDialog::getRgba(ui.lineEdit->textColor().rgba(), &ok, this);
 	if (ok) {
 	        _currentColor = QColor(color);
 	        QPixmap pxm(16,16);
	        pxm.fill(_currentColor);
	        ui.colorChatButton->setIcon(pxm);
 	}
	setFont();
}

void PeersDialog::getFont()
{
    bool ok;
    mCurrentFont = QFontDialog::getFont(&ok, mCurrentFont, this);
    setFont();
}

void PeersDialog::setFont()
{
  mCurrentFont.setBold(ui.textboldChatButton->isChecked());
  mCurrentFont.setUnderline(ui.textunderlineChatButton->isChecked());
  mCurrentFont.setItalic(ui.textitalicChatButton->isChecked());
  ui.lineEdit->setFont(mCurrentFont);
  ui.lineEdit->setTextColor(_currentColor);

  ui.lineEdit->setFocus();
  
}

void PeersDialog::underline() 
{
 	        _underline = !_underline;
 	        ui.lineEdit->setFontUnderline(_underline);
}
 

// Update Chat Info information
void PeersDialog::setChatInfo(QString info, QColor color) 
{
  static unsigned int nbLines = 0;
  ++nbLines;
  // Check log size, clear it if too big
  if(nbLines > 200) {
    ui.msgText->clear();
    nbLines = 1;
  }
  ui.msgText->append(QString::fromUtf8("<font color='grey'>")+ QTime::currentTime().toString(QString::fromUtf8("hh:mm:ss")) + QString::fromUtf8("</font> - <font color='") + color.name() +QString::fromUtf8("'><i>") + info + QString::fromUtf8("</i></font>"));
}

void PeersDialog::on_actionClearChat_triggered() 
{
  ui.msgText->clear();
}

void PeersDialog::displayInfoChatMenu(const QPoint& pos) 
{
  // Log Menu
  QMenu myChatMenu(this);
  myChatMenu.addAction(ui.actionClearChat);
  // XXX: Why mapToGlobal() is not enough?
  myChatMenu.exec(mapToGlobal(pos)+QPoint(0,80));
}

void PeersDialog::loadEmoticonsgroupchat()
{
	QString sm_codes;
	QFile sm_file(QString(":/emoticons/emotes.acs"));
	if(!sm_file.open(QIODevice::ReadOnly))
	{
		std::cerr << "Could not open resouce file :/emoticons/emotes.acs" << endl ;
		return ;
	}
	sm_codes = sm_file.readAll();
	sm_file.close();
	sm_codes.remove("\n");
	sm_codes.remove("\r");
	int i = 0;
	QString smcode;
	QString smfile;
	while(sm_codes[i] != '{')
	{
		i++;
		
	}
	while (i < sm_codes.length()-2)
	{
		smcode = "";
		smfile = "";
		while(sm_codes[i] != '\"')
		{
			i++;
		}
		i++;
		while (sm_codes[i] != '\"')
		{
			smcode += sm_codes[i];
			i++;
			
		}
		i++;
		
		while(sm_codes[i] != '\"')
		{
			i++;
		}
		i++;
		while(sm_codes[i] != '\"' && sm_codes[i+1] != ';')
		{
			smfile += sm_codes[i];
			i++;
		}
		i++;
		if(!smcode.isEmpty() && !smfile.isEmpty())
			smileys.insert(smcode, smfile);
	}
}

void PeersDialog::smileyWidgetgroupchat()
{ 
	qDebug("MainWindow::smileyWidget()");
	QWidget *smWidget = new QWidget;
	smWidget->setWindowTitle("Emoticons");
	smWidget->setWindowIcon(QIcon(QString(":/images/rstray3.png")));
	smWidget->setFixedSize(256,256);

	
	
	int x = 0, y = 0;
	
	QHashIterator<QString, QString> i(smileys);
	while(i.hasNext())
	{
		i.next();
		QPushButton *smButton = new QPushButton("", smWidget);
		smButton->setGeometry(x*24, y*24, 24,24);
		smButton->setIconSize(QSize(24,24));
		smButton->setIcon(QPixmap(i.value()));
		smButton->setToolTip(i.key());
		//smButton->setFixedSize(24,24);
		++x;
		if(x > 4)
		{
			x = 0;
			y++;
		}
		connect(smButton, SIGNAL(clicked()), this, SLOT(addSmileys()));
	}
	
	smWidget->show();
}

void PeersDialog::addSmileys()
{
	ui.lineEdit->setText(ui.lineEdit->toHtml() + qobject_cast<QPushButton*>(sender())->toolTip().split("|").first());
}

/* GUI stuff -> don't do anything directly with Control */
void PeersDialog::viewprofile()
{
	/* display Dialog */

	QTreeWidgetItem *c = getCurrentPeer();


	static ProfileView *profileview = new ProfileView();


	if (!c)
		return;

	/* set the Id */
	std::string id = getPeerRsCertId(c);

	profileview -> setPeerId(id);
	profileview -> show();
}
