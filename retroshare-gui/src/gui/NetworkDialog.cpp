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
#include <QCursor>

#include "rshare.h"
#include "common/vmessagebox.h"
#include "NetworkDialog.h"
#include "NetworkView.h"
#include "TrustView.h"
#include "connect/ConnectDialog.h"
#include "GenCertDialog.h"
#include "rsiface/rsiface.h"
#include "rsiface/rspeers.h"
#include "rsiface/rsdisc.h"
#include <algorithm>

/* for GPGME */
#include "rsiface/rsinit.h"

#include <gpgme.h>

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

#include "connect/ConfCertDialog.h"

/* Images for context menu icons */
#define IMAGE_LOADCERT       ":/images/loadcert16.png"
#define IMAGE_PEERDETAILS    ":/images/peerdetails_16x16.png"
#define IMAGE_AUTH           ":/images/encrypted16.png"
#define IMAGE_MAKEFRIEND     ":/images/user/add_user16.png"
#define IMAGE_EXPIORT      ":/images/exportpeers_16x16.png"

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
: RsAutoUpdatePage(10000,parent), 	// updates every 10 sec.
	connectdialog(NULL)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);
  
  /* Create RshareSettings object */
  _settings = new RshareSettings();

  connect( ui.connecttreeWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( connecttreeWidgetCostumPopupMenu( QPoint ) ) );
  connect( ui.connecttreeWidget, SIGNAL( itemSelectionChanged()), ui.unvalidGPGkeyWidget, SLOT( clearSelection() ) );
  connect( ui.unvalidGPGkeyWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( connecttreeWidgetCostumPopupMenu( QPoint ) ) );
  connect( ui.unvalidGPGkeyWidget, SIGNAL( itemSelectionChanged()), ui.connecttreeWidget, SLOT( clearSelection() ) );

  /* create a single connect dialog */
  connectdialog = new ConnectDialog();
  
  connect(ui.infoLog, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(displayInfoLogMenu(const QPoint&)));

  connect(ui.showUnvalidKeys, SIGNAL(clicked()), this, SLOT(insertConnect()));


  /* hide the Tree +/- */
  ui.connecttreeWidget -> setRootIsDecorated( false );
      ui.connecttreeWidget->setColumnCount(5);
    ui.unvalidGPGkeyWidget->setColumnCount(5);

  /* Set header resize modes and initial section sizes */
    QHeaderView * _header = ui.connecttreeWidget->header () ;
    _header->setResizeMode (0, QHeaderView::Custom);
    _header->setResizeMode (1, QHeaderView::Interactive);
    _header->setResizeMode (2, QHeaderView::Interactive);
    _header->setResizeMode (3, QHeaderView::Interactive);
    _header->setResizeMode (4, QHeaderView::Interactive);

    _header->resizeSection ( 0, 25 );
    _header->resizeSection ( 1, 200 );
    _header->resizeSection ( 2, 200 );
    _header->resizeSection ( 3, 200 );

    // set header text aligment
    QTreeWidgetItem * headerItem = ui.connecttreeWidget->headerItem();
    headerItem->setTextAlignment(0, Qt::AlignHCenter | Qt::AlignVCenter);
    headerItem->setTextAlignment(1, Qt::AlignHCenter | Qt::AlignVCenter);
    headerItem->setTextAlignment(2, Qt::AlignHCenter | Qt::AlignVCenter);
    headerItem->setTextAlignment(3, Qt::AlignHCenter | Qt::AlignVCenter);
    headerItem->setTextAlignment(4, Qt::AlignVCenter);

      /* hide the Tree +/- */
    ui.unvalidGPGkeyWidget -> setRootIsDecorated( false );

    /* Set header resize modes and initial section sizes */
    ui.unvalidGPGkeyWidget->header()->setResizeMode (0, QHeaderView::Custom);
    ui.unvalidGPGkeyWidget->header()->setResizeMode (1, QHeaderView::Interactive);
    ui.unvalidGPGkeyWidget->header()->setResizeMode (2, QHeaderView::Interactive);
    ui.unvalidGPGkeyWidget->header()->setResizeMode (3, QHeaderView::Interactive);
    ui.unvalidGPGkeyWidget->header()->setResizeMode (4, QHeaderView::Interactive);

    ui.unvalidGPGkeyWidget->header()->resizeSection ( 0, 25 );
    ui.unvalidGPGkeyWidget->header()->resizeSection ( 1, 200 );
    ui.unvalidGPGkeyWidget->header()->resizeSection ( 2, 200 );
    ui.unvalidGPGkeyWidget->header()->resizeSection ( 3, 200 );

    // set header text aligment
    ui.unvalidGPGkeyWidget->headerItem()->setTextAlignment(0, Qt::AlignHCenter | Qt::AlignVCenter);
    ui.unvalidGPGkeyWidget->headerItem()->setTextAlignment(1, Qt::AlignHCenter | Qt::AlignVCenter);
    ui.unvalidGPGkeyWidget->headerItem()->setTextAlignment(2, Qt::AlignHCenter | Qt::AlignVCenter);
    ui.unvalidGPGkeyWidget->headerItem()->setTextAlignment(3, Qt::AlignHCenter | Qt::AlignVCenter);
    ui.unvalidGPGkeyWidget->headerItem()->setTextAlignment(4, Qt::AlignVCenter);

    ui.connecttreeWidget->sortItems( 1, Qt::AscendingOrder );
    ui.unvalidGPGkeyWidget->sortItems( 1, Qt::AscendingOrder );

    //ui.networkTab->addTab(new NetworkView(),QString(tr("Network View")));
    ui.networkTab->addTab(new TrustView(),QString(tr("Trust matrix")));
     
    QString version = "-";
    std::map<std::string, std::string>::iterator vit;
    std::map<std::string, std::string> versions;
    bool retv = rsDisc->getDiscVersions(versions);
    if (retv && versions.end() != (vit = versions.find(rsPeers->getOwnId()))) {
    	version	= QString::fromStdString(vit->second);
    }

    // Set Log infos
    setLogInfo(tr("RetroShare %1 started.").arg(version));
    
    setLogInfo(tr("Welcome to RetroShare."), QString::fromUtf8("blue"));
      
    QMenu *menu = new QMenu(tr("Menu"));
    //menu->addAction(ui.actionAddFriend); 
    //menu->addAction(ui.actionCopyKey);
    //menu->addAction(ui.actionExportKey);
    //menu->addAction(ui.actionCreate_New_Profile);
    //menu->addSeparator();
    menu->addAction(ui.actionTabsright); 
    menu->addAction(ui.actionTabswest);
    menu->addAction(ui.actionTabssouth); 
    menu->addAction(ui.actionTabsnorth);
    menu->addSeparator();
    menu->addAction(ui.actionTabsTriangular); 
    menu->addAction(ui.actionTabsRounded);
    ui.viewButton->setMenu(menu);
    
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(getNetworkStatus()));
    timer->start(100000);
    
    QTimer *timer2 = new QTimer(this);
    connect(timer2, SIGNAL(timeout()), this, SLOT(updateNetworkStatus()));
    timer2->start(1000);
    
    getNetworkStatus();
    updateNetworkStatus();
    //load();
    loadtabsettings();

    #ifdef RS_RELEASE_VERSION
    ui.tabBottom->removeTab(0); //hide the logs tab
    #endif
    

  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

void NetworkDialog::connecttreeWidgetCostumPopupMenu( QPoint point )
{
    std::cerr << "NetworkDialog::connecttreeWidgetCostumPopupMenu( QPoint point ) called" << std::endl;
    QTreeWidgetItem *wi = getCurrentNeighbour();
    if (!wi)
    	return;

//		 return ;

      QMenu contextMnu( this );
      QMouseEvent *mevent = new QMouseEvent( QEvent::MouseButtonPress, point, Qt::RightButton, Qt::RightButton, Qt::NoModifier );
      contextMnu.clear();

                std::string peer_id = wi->text(4).toStdString() ;

                // That's what context menus are made for
		RsPeerDetails detail;
                if(!rsPeers->getGPGDetails(peer_id, detail))		// that is not suppose to fail.
			return ;

                if(peer_id != rsPeers->getGPGOwnId())
		{
                        if(detail.accept_connection)
			{
				denyFriendAct = new QAction(QIcon(IMAGE_DENIED), tr( "Deny friend" ), this );

				connect( denyFriendAct , SIGNAL( triggered() ), this, SLOT( denyFriend() ) );
				contextMnu.addAction( denyFriendAct);
			}
			else	// not a friend
			{
				makefriendAct = new QAction(QIcon(IMAGE_MAKEFRIEND), tr( "Make friend" ), this );

				connect( makefriendAct , SIGNAL( triggered() ), this, SLOT( makeFriend() ) );
				contextMnu.addAction( makefriendAct);
#ifdef TODO
				if(detail.validLvl > RS_TRUST_LVL_MARGINAL)		// it's a denied old friend.
				{
					deleteCertAct = new QAction(QIcon(IMAGE_PEERDETAILS), tr( "Delete certificate" ), this );
					connect( deleteCertAct, SIGNAL( triggered() ), this, SLOT( deleteCert() ) );
					contextMnu.addAction( deleteCertAct );
				}

#endif
			}
		}
                if (  peer_id == rsPeers->getGPGOwnId())
		{
		    exportcertAct = new QAction(QIcon(IMAGE_EXPIORT), tr( "Export my Cert" ), this );
		    connect( exportcertAct , SIGNAL( triggered() ), this, SLOT( on_actionExportKey_activated() ) );
		    contextMnu.addAction( exportcertAct);
		}

		peerdetailsAct = new QAction(QIcon(IMAGE_PEERDETAILS), tr( "Peer details..." ), this );
		connect( peerdetailsAct , SIGNAL( triggered() ), this, SLOT( peerdetails() ) );
		contextMnu.addAction( peerdetailsAct);


		contextMnu.exec( mevent->globalPos() );
}

void NetworkDialog::denyFriend()
{
	QTreeWidgetItem *wi = getCurrentNeighbour();
        std::string peer_id = wi->text(4).toStdString() ;
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
        ConfCertDialog::showTrust(getCurrentNeighbour()->text(4).toStdString());
}

/** Shows Peer Information/Auth Dialog */
void NetworkDialog::peerdetails()
{
         ConfCertDialog::show(getCurrentNeighbour()->text(4).toStdString());
}

/** Shows Peer Information/Auth Dialog */
void NetworkDialog::showpeerdetails(std::string id)
{
#ifdef NET_DEBUG 
    std::cerr << "NetworkDialog::showpeerdetails()" << std::endl;
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

void NetworkDialog::updateDisplay()
{
	insertConnect() ;
}

/* get the list of Neighbours from the RsIface.  */
void NetworkDialog::insertConnect()
{
	if (!rsPeers)
	{
		return;
	}

        std::list<std::string> neighs; //these are GPG ids
	std::list<std::string>::iterator it;
        rsPeers->getGPGAllList(neighs);

	/* get a link to the table */
        QTreeWidget *connectWidget = ui.connecttreeWidget;

        //remove items
        int index = 0;
        while (index < connectWidget->topLevelItemCount()) {
            std::string gpg_widget_id = (connectWidget->topLevelItem(index))->text(4).toStdString();
            RsPeerDetails detail;
            if (!rsPeers->getGPGDetails(gpg_widget_id, detail) || detail.validLvl < 3) {
                connectWidget->takeTopLevelItem(index);
            } else {
                index++;
            }
        }
        index = 0;
        while (index < ui.unvalidGPGkeyWidget->topLevelItemCount()) {
            std::string gpg_widget_id = (ui.unvalidGPGkeyWidget->topLevelItem(index))->text(4).toStdString();
            RsPeerDetails detail;
            if (!rsPeers->getGPGDetails(gpg_widget_id, detail) || detail.validLvl >= 3) {
                ui.unvalidGPGkeyWidget->takeTopLevelItem(index);
            } else {
                index++;
            }
        }

        QList<QTreeWidgetItem *> validItems;
        QList<QTreeWidgetItem *> unvalidItems;
	for(it = neighs.begin(); it != neighs.end(); it++)
	{
                #ifdef NET_DEBUG
                std::cerr << "NetworkDialog::insertConnect() inserting gpg key : " << *it << std::endl;
                #endif
                if (*it == rsPeers->getGPGOwnId()) {
                    continue;
                }

                RsPeerDetails detail;
                if (!rsPeers->getGPGDetails(*it, detail))
                {
                        continue; /* BAD */
                }

                /* make a widget per friend */
                QTreeWidgetItem *item;
                QList<QTreeWidgetItem *> list = connectWidget->findItems(QString::fromStdString(*it), Qt::MatchExactly, 4);
                if (list.size() == 1) {
                    item = list.front();
                } else {
                    list = ui.unvalidGPGkeyWidget->findItems(QString::fromStdString(*it), Qt::MatchExactly, 4);
                    if (list.size() == 1) {
                        item = list.front();
                    } else {
                        //create new item
                        #ifdef NET_DEBUG
                        std::cerr << "NetworkDialog::insertConnect() creating new tree widget item : " << *it << std::endl;
                        #endif
                        item = new QTreeWidgetItem(0);
                        item->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);

                        /* (1) Person */
                        item -> setText(1, QString::fromStdString(detail.name));

                        /* (4) key id */
                        item -> setText(4, QString::fromStdString(detail.id));
                    }
                }

                /* (2) Key validity */
                if (detail.ownsign) {
                    item -> setText(2, tr("Authenticated"));
                    item -> setToolTip(2, tr("GPG key signed"));
                } else {
                    item -> setText(2, tr("Not Authenticated"));
                    item -> setToolTip(2, tr("GPG key not signed"));
                }

                /* (3) has me auth */
                if (detail.hasSignedMe)
                        item -> setText(3, tr("Has authenticated me"));
		else
                        item -> setText(3, tr("Unknown"));
		

		/**
		* Determinated the Background Color
		*/
		QColor backgrndcolor;
		
                if (detail.accept_connection)
		{
                    if (detail.ownsign) {
                        item -> setText(0, "0");
			item -> setIcon(0,(QIcon(IMAGE_AUTHED)));
                        backgrndcolor=QColor("#45ff45");//bright green
                    } else {
                        item -> setText(0, "0");
                        item -> setIcon(0,(QIcon(IMAGE_AUTHED)));
                        backgrndcolor=QColor("#43C043");//light green
                    }
                } else {
                        item -> setText(0, "1");
                        if (detail.hasSignedMe)
			{
                                backgrndcolor=QColor("#42B2B2"); //kind of darkCyan
                                item -> setIcon(0,(QIcon(IMAGE_DENIED)));
				for(int k=0;k<8;++k)
                                        item -> setToolTip(k,QString::fromStdString(detail.name) + QString(tr(" has authenticated you. \nRight-click and select 'make friend' to be able to connect."))) ;
			}
			else
			{
                                backgrndcolor=Qt::lightGray;
				item -> setIcon(0,(QIcon(IMAGE_DENIED)));
			}
		}

		// Color each Background column in the Network Tab except the first one => 1-9
		// whith the determinated color
                for(int i = 0; i <10; i++)
			item -> setBackground(i,QBrush(backgrndcolor));

		/* add to the list */
                if (detail.accept_connection || detail.validLvl >= 3) {
                    /* add gpg item to the list. If item is already in the list, it won't be duplicated thanks to Qt */
                    connectWidget->addTopLevelItem(item);
                } else {
                    ui.unvalidGPGkeyWidget->addTopLevelItem(item);
                }

	}

	// add self to network.
        RsPeerDetails ownGPGDetails;
        rsPeers->getGPGDetails(rsPeers->getGPGOwnId(), ownGPGDetails);
        /* make a widget per friend */
        QTreeWidgetItem *self_item;
        QList<QTreeWidgetItem *> list = connectWidget->findItems(QString::fromStdString(ownGPGDetails.gpg_id), Qt::MatchExactly, 4);
        if (list.size() == 1) {
            self_item = list.front();
        } else {
            self_item = new QTreeWidgetItem(0);
            self_item->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
        }
        self_item -> setText(0, "0");
        self_item->setIcon(0,(QIcon(IMAGE_AUTHED)));
        self_item->setText(1,QString::fromStdString(ownGPGDetails.name) + " (yourself)") ;
        self_item->setText(2,"N/A");
        self_item->setText(4, QString::fromStdString(ownGPGDetails.id));

        // Color each Background column in the Network Tab except the first one => 1-9
        for(int i=0;i<10;++i)
        {
                self_item->setBackground(i,QBrush(QColor("#45ff45")));
        }
        connectWidget->addTopLevelItem(self_item);

        if (ui.showUnvalidKeys->isChecked()) {
            ui.unvalidGPGkeyWidget->show();
        } else {
            ui.unvalidGPGkeyWidget->hide();
        }
        connectWidget->update(); /* update display */
        ui.unvalidGPGkeyWidget->update(); /* update display */

}

QTreeWidgetItem *NetworkDialog::getCurrentNeighbour()
{ 
        if (ui.connecttreeWidget->selectedItems().size() != 0)  {
            return ui.connecttreeWidget -> currentItem();
        } else if (ui.unvalidGPGkeyWidget->selectedItems().size() != 0) {
            return ui.unvalidGPGkeyWidget->currentItem();
        }

        return NULL;
}   

/* Utility Fns */
RsCertId getNeighRsCertId(QTreeWidgetItem *i)
{
        RsCertId id = (i -> text(4)).toStdString();
        return id;
}   
  
/* So from the Neighbours Dialog we can call the following control Functions:
 * (1) Load Certificate             NeighLoadCertificate(std::string file)
 * (2) Neigh  Auth                  NeighAuthFriend(id, code)
 * (4) Neigh  Add                   NeighAddFriend(id)
 *
 * All of these rely on the finding of the current Id.
 */

std::string NetworkDialog::loadneighbour()
{
#ifdef NET_DEBUG 
        std::cerr << "NetworkDialog::loadneighbour()" << std::endl;
#endif
        QString fileName = QFileDialog::getOpenFileName(this, tr("Select Certificate"), "",
	                                             tr("Certificates (*.pqi *.pem)"));

	std::string file = fileName.toStdString();
	std::string id;
        std::string gpg_id;
	if (file != "")
	{
                rsPeers->loadCertificateFromFile(file, id, gpg_id);
	}
	return id;
}

void NetworkDialog::addneighbour()
{
        QTreeWidgetItem *c = getCurrentNeighbour();
#ifdef NET_DEBUG 
        std::cerr << "NetworkDialog::addneighbour()" << std::endl;
#endif
        /*
        rsServer->NeighAddFriend(getNeighRsCertId(c));
        */
}

void NetworkDialog::authneighbour()
{
        QTreeWidgetItem *c = getCurrentNeighbour();
#ifdef NET_DEBUG 
        std::cerr << "NetworkDialog::authneighbour()" << std::endl;
#endif
        /*
	RsAuthId code;
        rsServer->NeighAuthFriend(getNeighRsCertId(c), code);
        */
}

/** Open a QFileDialog to browse for a pem/pqi file. */
void NetworkDialog::on_actionAddFriend_activated()
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


void NetworkDialog::on_actionExportKey_activated()
{
    qDebug() << "  exportcert";

    QString qdir = QFileDialog::getSaveFileName(this,
                                                "Please choose a filename",
                                                QDir::homePath(),
                                                "RetroShare Certificate (*.pqi)");

    if ( rsPeers->saveCertificateToFile(rsPeers->getOwnId(), qdir.toStdString()) )
    {
        QMessageBox::information(this, tr("RetroShare"),
                         tr("Certificate file successfully created"),
                         QMessageBox::Ok, QMessageBox::Ok);
    }
    else
    {
        QMessageBox::information(this, tr("RetroShare"),
                         tr("Sorry, certificate file creation failed"),
                         QMessageBox::Ok, QMessageBox::Ok);
    }
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

void NetworkDialog::on_actionCreate_New_Profile_activated()
{
    static GenCertDialog *gencertdialog = new GenCertDialog();
    gencertdialog->show();
    

}

void NetworkDialog::displayInfoLogMenu(const QPoint& pos) {
  // Log Menu
  QMenu myLogMenu(this);
  myLogMenu.addAction(ui.actionClearLog);
  // XXX: Why mapToGlobal() is not enough?
  // myLogMenu.exec(mapToGlobal(pos)+QPoint(0,320));
  // No. Simple use QCursor::pos() to retrieve the position of the cursor.
  myLogMenu.exec(QCursor::pos());
}

void NetworkDialog::getNetworkStatus()
{
    rsiface->lockData(); /* Lock Interface */

    /* now the extra bit .... switch on check boxes */
    const RsConfig &config = rsiface->getConfig();

    /****** Log Tab **************************/
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
      setLogInfo(tr("DHT OK."), QString::fromUtf8("green"));
    }
    else 
    {
      setLogInfo(tr("DHT is not working (down)."), QString::fromUtf8("red"));
    }
        
    if(config.netStunOk)
    {
      setLogInfo(tr("Stun external address detection is working."), QString::fromUtf8("green"));
    }
    else
    {
      setLogInfo(tr("Stun is not working."), QString::fromUtf8("red"));
    }
    
    if (config.netLocalOk)
    {
      setLogInfo(tr("Local network detected"), QString::fromUtf8("magenta"));
    }
    else
    {
      setLogInfo(tr("No local network detected"), QString::fromUtf8("red"));
    }

    if (config.netExtraAddressOk)
    {
      setLogInfo(tr("ip found via external address finder"), QString::fromUtf8("magenta"));
    }
    else
    {
      setLogInfo(tr("external address finder didn't found anything"), QString::fromUtf8("red"));
    }

    rsiface->unlockData(); /* UnLock Interface */
}

void NetworkDialog::updateNetworkStatus()
{
    rsiface->lockData(); /* Lock Interface */

    /* now the extra bit .... switch on check boxes */
    const RsConfig &config = rsiface->getConfig();

    
       /******* Network Status Tab *******/
             
      if(config.netUpnpOk)
      {
         ui.iconlabel_upnp->setPixmap(QPixmap::QPixmap(":/images/ledon1.png"));
      }
      else
      {    
         ui.iconlabel_upnp->setPixmap(QPixmap::QPixmap(":/images/ledoff1.png"));
      }
                              
      if (config.netLocalOk)
      {
	  ui.iconlabel_netLimited->setPixmap(QPixmap::QPixmap(":/images/ledon1.png"));
      }
      else
      {          
          ui.iconlabel_netLimited->setPixmap(QPixmap::QPixmap(":/images/ledoff1.png"));
      }

      if (config.netExtraAddressOk)
      {
	  ui.iconlabel_ext->setPixmap(QPixmap::QPixmap(":/images/ledon1.png"));
      }
      else
      {
	  ui.iconlabel_ext->setPixmap(QPixmap::QPixmap(":/images/ledoff1.png"));
      }

    rsiface->unlockData(); /* UnLock Interface */
}

/*void NetworkDialog::load()
{
  //ui.check_net->setCheckable(true);
	ui.check_upnp->setCheckable(true);
	ui.check_dht->setCheckable(true);
	ui.check_ext->setCheckable(true);
	ui.check_udp->setCheckable(true);
	ui.check_tcp->setCheckable(true);

	//ui.check_net->setEnabled(false);
	ui.check_upnp->setEnabled(false);
	ui.check_dht->setEnabled(false);
	ui.check_ext->setEnabled(false);
	ui.check_udp->setEnabled(false);
	ui.check_tcp->setEnabled(false);

	ui.radio_nonet->setEnabled(false);
	ui.radio_netLimited->setEnabled(false);
	ui.radio_netUdp->setEnabled(false);
	ui.radio_netServer->setEnabled(false);
}*/

void NetworkDialog::on_actionTabsnorth_activated()
{
	_settings->beginGroup("NetworkDialog");
	
  ui.networkTab->setTabPosition(QTabWidget::North);
  
  _settings->setValue("TabWidget_Position",ui.networkTab->tabPosition());
  _settings->endGroup();
}

void NetworkDialog::on_actionTabssouth_activated()
{
	_settings->beginGroup("NetworkDialog");

  ui.networkTab->setTabPosition(QTabWidget::South);
  
  _settings->setValue("TabWidget_Position",ui.networkTab->tabPosition());  
  _settings->endGroup();
}

void NetworkDialog::on_actionTabswest_activated()
{
	_settings->beginGroup("NetworkDialog");

  ui.networkTab->setTabPosition(QTabWidget::West);
  
  _settings->setValue("TabWidget_Position",ui.networkTab->tabPosition());  
  _settings->endGroup();
}

void NetworkDialog::on_actionTabsright_activated()
{
	_settings->beginGroup("NetworkDialog");
	
  ui.networkTab->setTabPosition(QTabWidget::East);
  
  _settings->setValue("TabWidget_Position",ui.networkTab->tabPosition());  
  _settings->endGroup();
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

void NetworkDialog::loadtabsettings()
{
_settings->beginGroup("NetworkDialog");

if(_settings->value("TabWidget_Position","0").toInt() == 0)
{
qDebug() << "Tab North";
ui.networkTab->setTabPosition(QTabWidget::North);
}

else if (_settings->value("TabWidget_Position","1").toInt() == 1)
{
qDebug() << "Tab South";
ui.networkTab->setTabPosition(QTabWidget::South);
}

else if (_settings->value("TabWidget_Position","2").toInt() ==2)
{
qDebug() << "Tab West";
ui.networkTab->setTabPosition(QTabWidget::West);
}

else if(_settings->value("TabWidget_Position","3").toInt() ==3)
{
qDebug() << "Tab East";
ui.networkTab->setTabPosition(QTabWidget::East);
}

_settings->endGroup();
}
