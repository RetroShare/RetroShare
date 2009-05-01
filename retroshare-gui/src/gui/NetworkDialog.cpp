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

#include "rshare.h"
#include "common/vmessagebox.h"
#include "util/rsversion.h"
#include "NetworkDialog.h"
#include "NetworkView.h"
#include "TrustView.h"
#include "connect/ConnectDialog.h"
#include "rsiface/rsiface.h"
#include "rsiface/rspeers.h"
#include <sstream>

#include <QTimer>
#include <QTime>
#include <QContextMenuEvent>
#include <QMenu>
#include <QCursor>
#include <QPoint>
#include <QMouseEvent>
#include <QPixmap>
#include <QHeaderView>

/* Images for context menu icons */
#define IMAGE_LOADCERT       ":/images/loadcert16.png"
#define IMAGE_PEERDETAILS    ":/images/peerdetails_16x16.png"
#define IMAGE_AUTH           ":/images/encrypted16.png"
#define IMAGE_MAKEFRIEND     ":/images/user/add_user16.png"
/* Images for Status icons */
#define IMAGE_AUTHED         ":/images/accepted16.png"
#define IMAGE_DENIED         ":/images/denied16.png"
#define IMAGE_TRUSTED        ":/images/rs-2.png"

RsCertId getNeighRsCertId(QTreeWidgetItem *i);

/******
 * #define NET_DEBUG 1
 *****/

/** Constructor */
NetworkDialog::NetworkDialog(QWidget *parent)
: MainPage(parent), connectdialog(NULL)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

  connect( ui.connecttreeWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( connecttreeWidgetCostumPopupMenu( QPoint ) ) );

  /* create a single connect dialog */
  connectdialog = new ConnectDialog();
  
  connect(ui.infoLog, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(displayInfoLogMenu(const QPoint&)));
  
  /* hide the Tree +/- */
  ui.connecttreeWidget -> setRootIsDecorated( false );
  
  /* Set header resize modes and initial section sizes */
	QHeaderView * _header = ui.connecttreeWidget->header () ;   
	_header->setResizeMode (0, QHeaderView::Custom);
	_header->setResizeMode (1, QHeaderView::Interactive);
	_header->setResizeMode (2, QHeaderView::Interactive);
	_header->setResizeMode (3, QHeaderView::Interactive);
	_header->setResizeMode (4, QHeaderView::Interactive);
	_header->setResizeMode (5, QHeaderView::Interactive);
	_header->setResizeMode (6, QHeaderView::Interactive);
	_header->setResizeMode (7, QHeaderView::Interactive);
	_header->setResizeMode (8, QHeaderView::Interactive);
	_header->setResizeMode (9, QHeaderView::Interactive);
    
	_header->resizeSection ( 0, 25 );
	_header->resizeSection ( 1, 100 );
	_header->resizeSection ( 2, 100 );
	_header->resizeSection ( 3, 100 );
	_header->resizeSection ( 4, 100 );
	_header->resizeSection ( 5, 200);
	_header->resizeSection ( 6, 100 );
	_header->resizeSection ( 7, 100 );
	_header->resizeSection ( 8, 100 );
	_header->resizeSection ( 9, 100 );
	
	// set header text aligment
	QTreeWidgetItem * headerItem = ui.connecttreeWidget->headerItem();
	headerItem->setTextAlignment(0, Qt::AlignHCenter | Qt::AlignVCenter);
	headerItem->setTextAlignment(1, Qt::AlignHCenter | Qt::AlignVCenter);
  headerItem->setTextAlignment(2, Qt::AlignHCenter | Qt::AlignVCenter);
	headerItem->setTextAlignment(3, Qt::AlignHCenter | Qt::AlignVCenter);
	headerItem->setTextAlignment(4, Qt::AlignHCenter | Qt::AlignVCenter);
	headerItem->setTextAlignment(5, Qt::AlignHCenter | Qt::AlignVCenter);
	headerItem->setTextAlignment(6, Qt::AlignHCenter | Qt::AlignVCenter);
	headerItem->setTextAlignment(7, Qt::AlignHCenter | Qt::AlignVCenter);
	headerItem->setTextAlignment(8, Qt::AlignHCenter | Qt::AlignVCenter);
	headerItem->setTextAlignment(9, Qt::AlignHCenter | Qt::AlignVCenter);
	
	networkview = new NetworkView(ui.networkviewTab);
	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(networkview);
	ui.networkviewTab->setLayout(layout);
	layout->setSpacing( 0 );
	layout->setMargin( 0 );

	ui.networkTab->addTab(new TrustView(),QString(tr("Trust matrix")));
     
    // Set Log infos
    setLogInfo(tr("RetroShare %1 started.", "e.g: RetroShare v0.x started.").arg(retroshareVersion()));
    
    setLogInfo(tr("Welcome to RetroShare."), QString::fromUtf8("blue"));
      
    QMenu *menu = new QMenu(tr("View"));
    menu->addAction(ui.actionTabsright); 
    menu->addAction(ui.actionTabswest);
    menu->addAction(ui.actionTabssouth); 
    menu->addAction(ui.actionTabsnorth);
    menu->addSeparator();
    menu->addAction(ui.actionTabsTriangular); 
    menu->addAction(ui.actionTabsRounded);
    ui.viewButton->setMenu(menu);
    
    QTimer *timer = new QTimer(this);
    timer->connect(timer, SIGNAL(timeout()), this, SLOT(getNetworkStatus()));
    timer->start(100000);
    
    //getNetworkStatus();
    

  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

void NetworkDialog::connecttreeWidgetCostumPopupMenu( QPoint point )
{

    QTreeWidgetItem *wi = getCurrentNeighbour();
    if (!wi)
    	return;

//		 return ;

      QMenu contextMnu( this );
      QMouseEvent *mevent = new QMouseEvent( QEvent::MouseButtonPress, point, Qt::RightButton, Qt::RightButton, Qt::NoModifier );
      contextMnu.clear();

		std::string peer_id = wi->text(9).toStdString() ;

			// That's what context menus are made for
		RsPeerDetails detail;
		if(!rsPeers->getPeerDetails(peer_id, detail))		// that is not suppose to fail.
			return ;

		if(peer_id != rsPeers->getOwnId())
			if(detail.state & RS_PEER_STATE_FRIEND)
			{
				denyFriendAct = new QAction(QIcon(IMAGE_DENIED), tr( "Deny friend" ), this );

				connect( denyFriendAct , SIGNAL( triggered() ), this, SLOT( denyFriend() ) );
				contextMnu.addAction( denyFriendAct);
			}
			else	// not a friend
			{
				if(detail.trustLvl > RS_TRUST_LVL_MARGINAL)		// it's a denied old friend.
					makefriendAct = new QAction(QIcon(IMAGE_MAKEFRIEND), tr( "Accept friend" ), this );
				else
					makefriendAct = new QAction(QIcon(IMAGE_MAKEFRIEND), tr( "Make friend" ), this );

				connect( makefriendAct , SIGNAL( triggered() ), this, SLOT( makeFriend() ) );
				contextMnu.addAction( makefriendAct);
#ifdef TODO
				if(detail.trustLvl > RS_TRUST_LVL_MARGINAL)		// it's a denied old friend.
				{
					deleteCertAct = new QAction(QIcon(IMAGE_PEERDETAILS), tr( "Delete certificate" ), this );
					connect( deleteCertAct, SIGNAL( triggered() ), this, SLOT( deleteCert() ) );
					contextMnu.addAction( deleteCertAct );
				}
#endif
			}

		peerdetailsAct = new QAction(QIcon(IMAGE_PEERDETAILS), tr( "Peer details..." ), this );
		connect( peerdetailsAct , SIGNAL( triggered() ), this, SLOT( peerdetails() ) );
		contextMnu.addAction( peerdetailsAct);

		contextMnu.exec( mevent->globalPos() );
}

void NetworkDialog::denyFriend()
{
	QTreeWidgetItem *wi = getCurrentNeighbour();
	std::string peer_id = wi->text(9).toStdString() ;
	rsPeers->removeFriend(peer_id) ;

	insertConnect() ;
}
void NetworkDialog::deleteCert()
{
#ifdef TODO
	// do whatever is needed to remove the certificate completely, hopping this
	// will eventually remove the signature we've stamped on it.
	std::cout << "Deleting friend !" << std::endl ;

	QTreeWidgetItem *wi = getCurrentNeighbour();
	std::string peer_id = wi->text(9).toStdString() ;
	rsPeers->deleteCertificate(peer_id) ;

	insertConnect() ;
#endif
}

void NetworkDialog::makeFriend()
{
	QTreeWidgetItem *wi = getCurrentNeighbour();
	std::string authId = wi->text(9).toStdString() ;

	rsPeers->AuthCertificate(authId, "");
	rsPeers->addFriend(authId);

	insertConnect() ;
}

/** Shows Peer Information/Auth Dialog */
void NetworkDialog::peerdetails()
{
#ifdef NET_DEBUG 
    std::cerr << "ConnectionsDialog::peerdetails()" << std::endl;
#endif

    QTreeWidgetItem *wi = getCurrentNeighbour();
    if (!wi)
    	return;

    RsCertId id = getNeighRsCertId(wi);
    std::ostringstream out;
    out << id;

    showpeerdetails(out.str());
}

/** Shows Peer Information/Auth Dialog */
void NetworkDialog::showpeerdetails(std::string id)
{
#ifdef NET_DEBUG 
    std::cerr << "ConnectionsDialog::showpeerdetails()" << std::endl;
#endif
    if ((connectdialog) && (connectdialog -> loadPeer(id)))
    {
    	connectdialog->show();
    }
}


/** Open a QFileDialog to browse for a pem/pqi file. */
void NetworkDialog::loadcert()
{
  /* Create a new input dialog, which allows users to create files, too */
  QFileDialog *dialog = new QFileDialog(this, tr("Select a pem/pqi File"));
  //dialog->setDirectory(QFileInfo(ui.lineTorConfig->text()).absoluteDir());
  //dialog->selectFile(QFileInfo(ui.lineTorConfig->text()).fileName());
  dialog->setFileMode(QFileDialog::AnyFile);
  dialog->setReadOnly(false);

  /* Prompt the user to select a file or create a new one */
  if (!dialog->exec() || dialog->selectedFiles().isEmpty()) {
    return;
  }
  QString filename = QDir::convertSeparators(dialog->selectedFiles().at(0));
 
  /* Check if the file exists */
  QFile torrcFile(filename);
  if (!QFileInfo(filename).exists()) {
    /* The given file does not exist. Should we create it? */
    int response = VMessageBox::question(this,
                     tr("File Not Found"),
                     tr("%1 does not exist. Would you like to create it?")
                                                            .arg(filename),
                     VMessageBox::Yes, VMessageBox::No);
    
    if (response == VMessageBox::No) {
      /* Don't create it. Just bail. */
      return;
    }
    /* Attempt to create the specified file */
    if (!torrcFile.open(QIODevice::WriteOnly)) {
      VMessageBox::warning(this,
        tr("Failed to Create File"),
        tr("Unable to create %1 [%2]").arg(filename)
                                      .arg(torrcFile.errorString()),
        VMessageBox::Ok);
      return;
    }
  }
  //ui.lineTorConfig->setText(filename);
}



#include <sstream>

/* get the list of Neighbours from the RsIface.  */
void NetworkDialog::insertConnect()
{
	if (!rsPeers)
	{
		return;
	}

	std::list<std::string> neighs;
	std::list<std::string>::iterator it;
	rsPeers->getOthersList(neighs);

	/* get a link to the table */
        QTreeWidget *connectWidget = ui.connecttreeWidget;
	QTreeWidgetItem *oldSelect = getCurrentNeighbour();
	QTreeWidgetItem *newSelect = NULL;
	std::string oldId;
	if (oldSelect)
	{
		oldId = (oldSelect -> text(9)).toStdString();
	}

        QList<QTreeWidgetItem *> items;
	for(it = neighs.begin(); it != neighs.end(); it++)
	{
		RsPeerDetails detail;
		if (!rsPeers->getPeerDetails(*it, detail))
		{
			continue; /* BAD */
		}

		/* make a widget per friend */
           	QTreeWidgetItem *item = new QTreeWidgetItem((QTreeWidget*)0);

		/* add all the labels */
		
	        /* (0) Status Icon */
		item -> setText(0, "");

		/* (1) Accept/Deny */
		if (detail.state & RS_PEER_STATE_FRIEND)
			item -> setText(1, tr("Trusted"));
		else
			item -> setText(1, tr("Denied"));

		if (rsPeers->isTrustingMe(detail.id) || detail.lastConnect>0)
			item -> setText(2, tr("Is trusting me"));
		else
			item -> setText(2, tr("Unknown"));

		/* (3) Last Connect */
		{
			std::ostringstream out;
			// Show anouncement if a friend never was connected.
			if (detail.lastConnect==0 ) 
				item -> setText(3, tr("Never seen"));
			else 
			{
				// Dont Show a timestamp in RS calculate the day
				QDateTime datum = QDateTime::fromTime_t(detail.lastConnect);
				// out << datum.toString(Qt::LocalDate);
				QString stime = datum.toString(Qt::LocalDate);
				item -> setText(3, stime);
			}
		}
		
        	/* (4) Person */
		item -> setText(4, QString::fromStdString(detail.name));
		
		/* (5) Peer Address */
		{
			std::ostringstream out;
			if(detail.state & RS_PEER_STATE_FRIEND) {
				out << detail.localAddr << ":";
				out << detail.localPort << "/";
				out << detail.extAddr << ":";
				out << detail.extPort;
			} else {
				// No Trust => no IP Information
				out << "0.0.0.0:0/0.0.0.0:0";
			}
                	item -> setText(5, QString::fromStdString(out.str()));
		}

		/* Others */
		item -> setText(6, QString::fromStdString(detail.org));
		item -> setText(7, QString::fromStdString(detail.location));
		item -> setText(8, QString::fromStdString(detail.email));

		{
			item -> setText(9, QString::fromStdString(detail.id));
			if ((oldSelect) && (oldId == detail.id))
			{
				newSelect = item;
			}
		}

		//item -> setText(10, QString::fromStdString(detail.authcode));

		/**
		* Determinated the Background Color
		*/
		QColor backgrndcolor;
		
		if (detail.state & RS_PEER_STATE_FRIEND)
		{
			item -> setIcon(0,(QIcon(IMAGE_AUTHED)));
			backgrndcolor=Qt::green;
		}
		else
		{
			if (rsPeers->isTrustingMe(detail.id))
			{
				backgrndcolor=Qt::magenta;
				item -> setIcon(0,(QIcon(IMAGE_TRUSTED)));
				for(int k=0;k<8;++k)
					item -> setToolTip(k,QString::fromStdString(detail.name) + QString(tr(" is trusting you. \nRight-click and select 'make friend' to be able to connect."))) ;
			}
			else if (detail.trustLvl > RS_TRUST_LVL_MARGINAL)
			{
				backgrndcolor=Qt::cyan;
				item -> setIcon(0,(QIcon(IMAGE_DENIED)));
			}
			else if (detail.lastConnect < 10000) /* 3 hours? */
			{
				backgrndcolor=Qt::yellow;
				item -> setIcon(0,(QIcon(IMAGE_DENIED)));
			}
			else
			{
				backgrndcolor=Qt::gray;
				item -> setIcon(0,(QIcon(IMAGE_DENIED)));
			}
		}

		// Color each Background column in the Network Tab except the first one => 1-9
		// whith the determinated color
		for(int i = 1; i <10; i++)
			item -> setBackground(i,QBrush(backgrndcolor));

		/* add to the list */
		items.append(item);
	}

	// add self to network.
	RsPeerDetails pd ;
	if(rsPeers->getPeerDetails(rsPeers->getOwnId(),pd)) 
	{
		QTreeWidgetItem *self_item = new QTreeWidgetItem((QTreeWidget*)0);

		self_item->setText(1,"N/A");
		self_item->setText(2,"N/A");
		self_item->setText(3,"N/A");
		self_item->setText(4,QString::fromStdString(pd.name) + " (yourself)") ;

		std::ostringstream out;
		out << pd.localAddr << ":" << pd.localPort << "/" << pd.extAddr << ":" << pd.extPort;
		self_item->setText(5, QString::fromStdString(out.str()));
		self_item->setText(6, QString::fromStdString(pd.org));
		self_item->setText(7, QString::fromStdString(pd.location));
		self_item->setText(8, QString::fromStdString(pd.email));
		self_item->setText(9, QString::fromStdString(pd.id));

		// Color each Background column in the Network Tab except the first one => 1-9
		for(int i=1;i<10;++i)
		{
			self_item->setBackground(i,QBrush(Qt::green));
		}
		self_item->setIcon(0,(QIcon(IMAGE_AUTHED)));
		items.append(self_item);
	}

	/* remove old items ??? */
	connectWidget->clear();
	connectWidget->setColumnCount(10);	

	/* add the items in! */
	connectWidget->insertTopLevelItems(0, items);
	if (newSelect)
	{
		connectWidget->setCurrentItem(newSelect);
	}

	connectWidget->update(); /* update display */
}

QTreeWidgetItem *NetworkDialog::getCurrentNeighbour()
{ 
        /* get the current, and extract the Id */
  
        /* get a link to the table */
        QTreeWidget *connectWidget = ui.connecttreeWidget;
        QTreeWidgetItem *item = connectWidget -> currentItem();
        if (!item) 
        {
#ifdef NET_DEBUG 
                std::cerr << "Invalid Current Item" << std::endl;
#endif
                return NULL;
        }
    
        /* Display the columns of this item. */

        return item;
}   

/* Utility Fns */
RsCertId getNeighRsCertId(QTreeWidgetItem *i)
{
        RsCertId id = (i -> text(9)).toStdString();
        return id;
}   
  
/* So from the Neighbours Dialog we can call the following control Functions:
 * (1) Load Certificate             NeighLoadCertificate(std::string file)
 * (2) Neigh  Auth                  NeighAuthFriend(id, code)
 * (3) Neigh  Add                   NeighAddFriend(id)
 *
 * All of these rely on the finding of the current Id.
 */

std::string NetworkDialog::loadneighbour()
{
#ifdef NET_DEBUG 
        std::cerr << "ConnectionsDialog::loadneighbour()" << std::endl;
#endif
        QString fileName = QFileDialog::getOpenFileName(this, tr("Select Certificate"), "",
	                                             tr("Certificates (*.pqi *.pem)"));

	std::string file = fileName.toStdString();
	std::string id;
	if (file != "")
	{
        	rsPeers->LoadCertificateFromFile(file, id);
	}
	return id;
}

void NetworkDialog::addneighbour()
{
        QTreeWidgetItem *c = getCurrentNeighbour();
#ifdef NET_DEBUG 
        std::cerr << "ConnectionsDialog::addneighbour()" << std::endl;
#endif
        /*
        rsServer->NeighAddFriend(getNeighRsCertId(c));
        */
}

void NetworkDialog::authneighbour()
{
        QTreeWidgetItem *c = getCurrentNeighbour();
#ifdef NET_DEBUG 
        std::cerr << "ConnectionsDialog::authneighbour()" << std::endl;
#endif
        /*
	RsAuthId code;
        rsServer->NeighAuthFriend(getNeighRsCertId(c), code);
        */
}

// Update Log Info information
void NetworkDialog::setLogInfo(QString info, QColor color) {
  static unsigned int nbLines = 0;
  ++nbLines;
  // Check log size, clear it if too big
  if(nbLines > 200) {
    ui.infoLog->clear();
    nbLines = 1;
  }
  ui.infoLog->append(QString::fromUtf8("<font color='grey'>")+ QTime::currentTime().toString(QString::fromUtf8("hh:mm:ss")) + QString::fromUtf8("</font> - <font color='") + color.name() +QString::fromUtf8("'><i>") + info + QString::fromUtf8("</i></font>"));
}

void NetworkDialog::on_actionClearLog_triggered() {
  ui.infoLog->clear();
}

void NetworkDialog::displayInfoLogMenu(const QPoint& pos) {
  // Log Menu
  QMenu myLogMenu(this);
  myLogMenu.addAction(ui.actionClearLog);
  // XXX: Why mapToGlobal() is not enough?
  myLogMenu.exec(mapToGlobal(pos)+QPoint(0,320));
}

void NetworkDialog::getNetworkStatus()
{
    rsiface->lockData(); /* Lock Interface */

    /* now the extra bit .... switch on check boxes */
    const RsConfig &config = rsiface->getConfig();

    //ui.check_net->setChecked(config.netOk);
    if(config.netUpnpOk)
    {
      setLogInfo(tr("UPNP is active."), QString::fromUtf8("blue"));
    }
    else
    {    
      setLogInfo(tr("UPNP NOT FOUND."), QString::fromUtf8("red"));
    }

    if(config.netDhtOk)
    {
      setLogInfo(tr("DHT OK"), QString::fromUtf8("green"));
    }
    else 
    {
      setLogInfo(tr("DHT is not working (down)."), QString::fromUtf8("red"));
    }
    
    
    if(config.netExtOk)
    {
      setLogInfo(tr("Stable External IP Address"), QString::fromUtf8("green"));
    }
    else
    {
      setLogInfo(tr("Not Found External Address"), QString::fromUtf8("red"));
    }
    
    if(config.netUdpOk)
    {
      setLogInfo(tr("UDP Port is Reacheable"), QString::fromUtf8("green"));
    }
    else
    {
      setLogInfo(tr("UDP Port isnt Reacheable"), QString::fromUtf8("red"));
    }
    
    if(config.netTcpOk)
    {
      setLogInfo(tr("TCP Port is Reacheable"), QString::fromUtf8("green"));
    }
    else
    {
      setLogInfo(tr("TCP Port is not Reacheable"), QString::fromUtf8("red"));
    }

    if (config.netExtOk)
    {
      if (config.netUpnpOk || config.netTcpOk)
      {
        setLogInfo(tr("RetroShare Server"), QString::fromUtf8("green"));
      }
      else
      {
        setLogInfo(tr("UDP Server"), QString::fromUtf8("green"));
      }
    }
    else if (config.netOk)
    {
      setLogInfo(tr("Net Limited"), QString::fromUtf8("magenta"));
    }
    else
    {
      setLogInfo(tr("No Conectivity"), QString::fromUtf8("red"));
    }
		
    rsiface->unlockData(); /* UnLock Interface */
}

void NetworkDialog::on_actionTabsright_activated()
{
  ui.networkTab->setTabPosition(QTabWidget::East);
}

void NetworkDialog::on_actionTabsnorth_activated()
{
  ui.networkTab->setTabPosition(QTabWidget::North);
}

void NetworkDialog::on_actionTabssouth_activated()
{
  ui.networkTab->setTabPosition(QTabWidget::South);
}

void NetworkDialog::on_actionTabswest_activated()
{
  ui.networkTab->setTabPosition(QTabWidget::West);
}

void NetworkDialog::on_actionTabsTriangular_activated()
{
  ui.networkTab->setTabShape(QTabWidget::Triangular);
  ui.tabBottom->setTabShape(QTabWidget::Triangular);
}

void NetworkDialog::on_actionTabsRounded_activated()
{
  ui.networkTab->setTabShape(QTabWidget::Rounded);
  ui.tabBottom->setTabShape(QTabWidget::Rounded);
}