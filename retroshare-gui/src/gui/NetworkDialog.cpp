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

#include <QTreeWidget>
#include <QDebug>
#include <QTimer>
#include <QTime>
#include <QMenu>

#include <retroshare/rsiface.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsdisc.h>
#include <retroshare/rsmsgs.h>

#include "common/vmessagebox.h"
#include "common/RSTreeWidgetItem.h"
#include <gui/common/FriendSelectionDialog.h>
#include "NetworkDialog.h"
//#include "TrustView.h"
#include "NetworkView.h"
#include "GenCertDialog.h"
#include "connect/ConfCertDialog.h"
#include "settings/rsharesettings.h"
#include "RetroShareLink.h"

#include <time.h>

/* Images for context menu icons */
#define IMAGE_LOADCERT       ":/images/loadcert16.png"
#define IMAGE_PEERDETAILS    ":/images/peerdetails_16x16.png"
#define IMAGE_AUTH           ":/images/encrypted16.png"
#define IMAGE_CLEAN_UNUSED   ":/images/deletemail24.png"
#define IMAGE_MAKEFRIEND     ":/images/user/add_user16.png"
#define IMAGE_EXPORT         ":/images/exportpeers_16x16.png"
#define IMAGE_COPYLINK       ":/images/copyrslink.png"

/* Images for Status icons */
#define IMAGE_AUTHED         ":/images/accepted16.png"
#define IMAGE_DENIED         ":/images/denied16.png"
#define IMAGE_TRUSTED        ":/images/rs-2.png"

#define COLUMN_PEERNAME    1
#define COLUMN_PEERID      4
#define COLUMN_LAST_USED   5

RsCertId getNeighRsCertId(QTreeWidgetItem *i);

/******
 * #define NET_DEBUG 1
 *****/

/** Constructor */
NetworkDialog::NetworkDialog(QWidget *parent)
: RsAutoUpdatePage(10000,parent) 	// updates every 10 sec.
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);
  
    connect( ui.connectTreeWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( connectTreeWidgetCostumPopupMenu( QPoint ) ) );
    connect( ui.connectTreeWidget, SIGNAL( itemSelectionChanged()), ui.unvalidGPGKeyWidget, SLOT( clearSelection() ) );
    connect( ui.connectTreeWidget, SIGNAL( itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT( peerdetails () ) );
    connect( ui.unvalidGPGKeyWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( connectTreeWidgetCostumPopupMenu( QPoint ) ) );
    connect( ui.unvalidGPGKeyWidget, SIGNAL( itemSelectionChanged()), ui.connectTreeWidget, SLOT( clearSelection() ) );
    connect( ui.unvalidGPGKeyWidget, SIGNAL( itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT( peerdetails () ) );

    connect( ui.filterLineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(filterItems(QString)));
    connect( ui.filterLineEdit, SIGNAL(filterChanged(int)), this, SLOT(filterColumnChanged(int)));

    connect( ui.showUnvalidKeys, SIGNAL(clicked()), this, SLOT(securedUpdateDisplay()));

    /* hide the Tree +/- */
    ui.connectTreeWidget -> setRootIsDecorated( false );
    ui.connectTreeWidget -> setColumnCount(6);
    ui.unvalidGPGKeyWidget-> setColumnCount(6);

    /* Set header resize modes and initial section sizes */
    QHeaderView * _header = ui.connectTreeWidget->header () ;
    _header->setResizeMode (0, QHeaderView::Custom);
    _header->setResizeMode (1, QHeaderView::Interactive);
    _header->setResizeMode (2, QHeaderView::Interactive);
    _header->setResizeMode (3, QHeaderView::Interactive);
    _header->setResizeMode (4, QHeaderView::Interactive);
    _header->setResizeMode (5, QHeaderView::Interactive);

    _header->resizeSection ( 0, 25 );
    _header->resizeSection ( 1, 200 );
    _header->resizeSection ( 2, 200 );
    _header->resizeSection ( 3, 200 );
    _header->resizeSection ( 5, 75 );

    // set header text aligment
    QTreeWidgetItem * headerItem = ui.connectTreeWidget->headerItem();
    headerItem->setTextAlignment(0, Qt::AlignHCenter | Qt::AlignVCenter);
    headerItem->setTextAlignment(1, Qt::AlignHCenter | Qt::AlignVCenter);
    headerItem->setTextAlignment(2, Qt::AlignHCenter | Qt::AlignVCenter);
    headerItem->setTextAlignment(3, Qt::AlignHCenter | Qt::AlignVCenter);
    headerItem->setTextAlignment(4, Qt::AlignVCenter);
    headerItem->setTextAlignment(5, Qt::AlignVCenter);

      /* hide the Tree +/- */
    ui.unvalidGPGKeyWidget -> setRootIsDecorated( false );

    /* Set header resize modes and initial section sizes */
    ui.unvalidGPGKeyWidget->header()->setResizeMode (0, QHeaderView::Custom);
    ui.unvalidGPGKeyWidget->header()->setResizeMode (1, QHeaderView::Interactive);
    ui.unvalidGPGKeyWidget->header()->setResizeMode (2, QHeaderView::Interactive);
    ui.unvalidGPGKeyWidget->header()->setResizeMode (3, QHeaderView::Interactive);
    ui.unvalidGPGKeyWidget->header()->setResizeMode (4, QHeaderView::Interactive);
    ui.unvalidGPGKeyWidget->header()->setResizeMode (5, QHeaderView::Interactive);

    ui.unvalidGPGKeyWidget->header()->resizeSection ( 0, 25 );
    ui.unvalidGPGKeyWidget->header()->resizeSection ( 1, 200 );
    ui.unvalidGPGKeyWidget->header()->resizeSection ( 2, 200 );
    ui.unvalidGPGKeyWidget->header()->resizeSection ( 3, 200 );
    ui.unvalidGPGKeyWidget->header()->resizeSection ( 5, 75 );

    // set header text aligment
    ui.unvalidGPGKeyWidget->headerItem()->setTextAlignment(0, Qt::AlignHCenter | Qt::AlignVCenter);
    ui.unvalidGPGKeyWidget->headerItem()->setTextAlignment(1, Qt::AlignHCenter | Qt::AlignVCenter);
    ui.unvalidGPGKeyWidget->headerItem()->setTextAlignment(2, Qt::AlignHCenter | Qt::AlignVCenter);
    ui.unvalidGPGKeyWidget->headerItem()->setTextAlignment(3, Qt::AlignHCenter | Qt::AlignVCenter);
    ui.unvalidGPGKeyWidget->headerItem()->setTextAlignment(4, Qt::AlignVCenter);
    ui.unvalidGPGKeyWidget->headerItem()->setTextAlignment(5, Qt::AlignVCenter);

    ui.connectTreeWidget->sortItems( 1, Qt::AscendingOrder );
    ui.unvalidGPGKeyWidget->sortItems( 1, Qt::AscendingOrder );

//    ui.networkTab->addTab(new TrustView(),QString(tr("Authentication matrix")));
//    ui.networkTab->addTab(networkview = new NetworkView(),QString(tr("Network View")));
    
    ui.showUnvalidKeys->setMinimumWidth(20);
     
    QString version = "-";
    std::map<std::string, std::string>::iterator vit;
    std::map<std::string, std::string> versions;
    bool retv = rsDisc->getDiscVersions(versions);
    if (retv && versions.end() != (vit = versions.find(rsPeers->getOwnId()))) {
    	version	= QString::fromStdString(vit->second);
    }
      
    QMenu *menu = new QMenu();
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
    //ui.viewButton->setMenu(menu);
    
    QTimer *timer2 = new QTimer(this);
    connect(timer2, SIGNAL(timeout()), this, SLOT(updateNetworkStatus()));
    timer2->start(1000);
    
    /* add filter actions */
    ui.filterLineEdit->addFilter(QIcon(), tr("Name"), COLUMN_PEERNAME, tr("Search Name"));
    ui.filterLineEdit->addFilter(QIcon(), tr("Peer ID"), COLUMN_PEERID, tr("Search Peer ID"));
    ui.filterLineEdit->setCurrentFilter(COLUMN_PEERNAME);

    //updateNetworkStatus();
    //loadtabsettings();
    
  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

void NetworkDialog::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::StyleChange:
        securedUpdateDisplay();
        break;
    default:
        // remove compiler warnings
        break;
    }
}

void NetworkDialog::connectTreeWidgetCostumPopupMenu( QPoint /*point*/ )
{
	//std::cerr << "NetworkDialog::connectTreeWidgetCostumPopupMenu( QPoint point ) called" << std::endl;
	QTreeWidgetItem *wi = getCurrentNeighbour();
	if (!wi)
		return;

	QMenu *contextMnu = new QMenu;

	std::string peer_id = wi->text(COLUMN_PEERID).toStdString() ;

	// That's what context menus are made for
	RsPeerDetails detail;
	if(!rsPeers->getGPGDetails(peer_id, detail))		// that is not suppose to fail.
		return ;

	if(peer_id != rsPeers->getGPGOwnId())
	{
		if(detail.accept_connection)
			contextMnu->addAction(QIcon(IMAGE_DENIED), tr("Deny friend"), this, SLOT(denyFriend()));
		else	// not a friend
			contextMnu->addAction(QIcon(IMAGE_MAKEFRIEND), tr("Make friend"), this, SLOT(makeFriend()));
	}
	if(peer_id == rsPeers->getGPGOwnId())
		contextMnu->addAction(QIcon(IMAGE_EXPORT), tr("Export my certificate..."), this, SLOT(on_actionExportKey_activated()));

	contextMnu->addAction(QIcon(IMAGE_PEERDETAILS), tr("Peer details..."), this, SLOT(peerdetails()));
	contextMnu->addAction(QIcon(IMAGE_COPYLINK), tr("Copy RetroShare Link"), this, SLOT(copyLink()));
	contextMnu->addSeparator() ;
	contextMnu->addAction(QIcon(IMAGE_CLEAN_UNUSED), tr("Remove unused keys..."), this, SLOT(removeUnusedKeys()));

	contextMnu->exec(QCursor::pos());
}

void NetworkDialog::removeUnusedKeys()
{
	std::list<std::string> pre_selected ;
	std::list<std::string> ids ;

	rsPeers->getGPGAllList(ids) ;
	RsPeerDetails details ;
	time_t now = time(NULL) ;
	time_t THREE_MONTHS = 86400*31*3 ;

	for(std::list<std::string>::const_iterator it(ids.begin());it!=ids.end();++it)
	{
		rsPeers->getPeerDetails(*it,details) ;

		if(rsPeers->haveSecretKey(*it))
		{
			std::cerr << "Skipping public/secret key pair " << *it << std::endl;
			continue ;
		}
		if(now > (time_t) (THREE_MONTHS + details.lastUsed))
		{
			std::cerr << "Adding " << *it << " to pre-selection." << std::endl;
			pre_selected.push_back(*it) ;
		}
	}

	std::list<std::string> selected = FriendSelectionDialog::selectFriends(NULL,
			tr("Clean keyring"),
			tr("The selected keys below haven't been used in the last 3 months. \nDo you want to delete them permanently ? \n\nNotes: Your old keyring will be backed up.\n    The removal may fail when running multiple Retroshare instances on the same machine."),FriendSelectionWidget::MODUS_CHECK,FriendSelectionWidget::SHOW_GPG | FriendSelectionWidget::SHOW_NON_FRIEND_GPG,
			FriendSelectionWidget::IDTYPE_GPG, pre_selected) ;
	
	std::cerr << "Removing these keys from the keyring: " << std::endl;
	for(std::list<std::string>::const_iterator it(selected.begin());it!=selected.end();++it)
		std::cerr << "  " << *it << std::endl;

	std::string backup_file ;
	uint32_t error_code ;

	if(selected.empty())
		return ;

	if( rsPeers->removeKeysFromPGPKeyring(selected,backup_file,error_code) )
		QMessageBox::information(NULL,tr("Keyring info"),tr("%1 keys have been deleted from your keyring. \nFor security, your keyring was previously backed-up to file \n\n").arg(selected.size())+QString::fromStdString(backup_file) ) ;
	else
	{
		QString error_string ;

		switch(error_code)
		{
			default:
			case PGP_KEYRING_REMOVAL_ERROR_NO_ERROR: error_string = tr("Unknown error") ;
																  break ;
			case PGP_KEYRING_REMOVAL_ERROR_CANT_REMOVE_SECRET_KEYS: error_string = tr("Cannot delete secret keys") ;
																					  break ;
			case PGP_KEYRING_REMOVAL_ERROR_CANNOT_WRITE_BACKUP:
			case PGP_KEYRING_REMOVAL_ERROR_CANNOT_CREATE_BACKUP: error_string = tr("Cannot create backup file. Check for permissions in pgp directory, disk space, etc.") ;
																				  break ;
			case PGP_KEYRING_REMOVAL_ERROR_DATA_INCONSISTENCY: 	error_string = tr("Data inconsistency in the keyring. This is most probably a bug. Please contact the developers.") ;
																				  break ;

		}
		QMessageBox::warning(NULL,tr("Keyring info"),tr("Key removal has failed. Your keyring remains intact.\n\nReported error: ")+error_string ) ;
	}
	insertConnect() ;
}

void NetworkDialog::denyFriend()
{
	QTreeWidgetItem *wi = getCurrentNeighbour();
	std::string peer_id = wi->text(COLUMN_PEERID).toStdString() ;
	rsPeers->removeFriend(peer_id) ;

	securedUpdateDisplay();
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

	securedUpdateDisplay();
#endif
}

void NetworkDialog::makeFriend()
{
	ConfCertDialog::showIt(getCurrentNeighbour()->text(COLUMN_PEERID).toStdString(), ConfCertDialog::PageTrust);
}

/** Shows Peer Information/Auth Dialog */
void NetworkDialog::peerdetails()
{
	QTreeWidgetItem* item = getCurrentNeighbour();
	if (item == NULL) {
		return;
	}
	ConfCertDialog::showIt(item->text(COLUMN_PEERID).toStdString(), ConfCertDialog::PageDetails);
}

void NetworkDialog::copyLink()
{
	QTreeWidgetItem *wi = getCurrentNeighbour();
	if (wi == NULL) {
		return;
	}

	std::string peer_id = wi->text(COLUMN_PEERID).toStdString() ;

	QList<RetroShareLink> urls;
	RetroShareLink link;
	if (link.createPerson(peer_id)) {
		urls.push_back(link);
	}

	RSLinkClipboard::copyLinks(urls);
}

void NetworkDialog::updateDisplay()
{
	insertConnect() ;
}

/* get the list of Neighbours from the RsIface.  */
void NetworkDialog::insertConnect()
{
//	static time_t last_time = 0 ;

	if (!rsPeers)
		return;

	if (ui.showUnvalidKeys->isChecked()) {
		ui.unvalidGPGKeyWidget->show();
	} else {
		ui.unvalidGPGKeyWidget->hide();
	}

//	// Because this is called from a qt signal, there's no limitation between calls.
	time_t now = time(NULL);
//	if(last_time + 5 > now)		// never update more often then every 5 seconds.
//		return ;
//
//	last_time = now ;

	std::list<std::string> neighs; //these are GPG ids
	std::list<std::string>::iterator it;
	rsPeers->getGPGAllList(neighs);

	/* get a link to the table */
	QTreeWidget *connectWidget = ui.connectTreeWidget;

	//remove items
	int index = 0;
	while (index < connectWidget->topLevelItemCount()) {
		std::string gpg_widget_id = (connectWidget->topLevelItem(index))->text(COLUMN_PEERID).toStdString();
		RsPeerDetails detail;
		if (!rsPeers->getGPGDetails(gpg_widget_id, detail) || (detail.validLvl < RS_TRUST_LVL_MARGINAL && !detail.accept_connection)) {
			delete (connectWidget->takeTopLevelItem(index));
		} else {
			index++;
		}
	}
	index = 0;
	while (index < ui.unvalidGPGKeyWidget->topLevelItemCount()) {
		std::string gpg_widget_id = (ui.unvalidGPGKeyWidget->topLevelItem(index))->text(COLUMN_PEERID).toStdString();
		RsPeerDetails detail;
		if (!rsPeers->getGPGDetails(gpg_widget_id, detail) || detail.validLvl >= RS_TRUST_LVL_MARGINAL || detail.accept_connection) {
			delete (ui.unvalidGPGKeyWidget->takeTopLevelItem(index));
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
			list = ui.unvalidGPGKeyWidget->findItems(QString::fromStdString(*it), Qt::MatchExactly, 4);
			if (list.size() == 1) {
				item = list.front();
			} else {
				//create new item
#ifdef NET_DEBUG
				std::cerr << "NetworkDialog::insertConnect() creating new tree widget item : " << *it << std::endl;
#endif
				item = new RSTreeWidgetItem(NULL, 0);
				item -> setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
				item -> setSizeHint(0, QSize( 18,18 ) );

				/* (1) Person */
				item -> setText(COLUMN_PEERNAME, QString::fromUtf8(detail.name.c_str()));

				/* (4) key id */
				item -> setText(COLUMN_PEERID, QString::fromStdString(detail.id));
			}
		}

		QString TrustLevelString ;

		/* (2) Key validity */
		if (detail.ownsign) 
		{
			item -> setText(2, tr("Personal signature"));
			item -> setToolTip(2, tr("PGP key signed by you"));
		} 
		else 
			switch(detail.trustLvl)
			{
				case RS_TRUST_LVL_MARGINAL: item->setText(2,tr("Marginally trusted peer")) ; break;
				case RS_TRUST_LVL_FULL:
				case RS_TRUST_LVL_ULTIMATE: item->setText(2,tr("Fully trusted peer")) ; break ;
				case RS_TRUST_LVL_UNKNOWN:
				case RS_TRUST_LVL_UNDEFINED: 
				case RS_TRUST_LVL_NEVER:
				default: 							item->setText(2,tr("Untrusted peer")) ; break ;
			}

		/* (3) has me auth */
		QString PeerAuthenticationString ;

		if (detail.hasSignedMe)
			PeerAuthenticationString = tr("Has authenticated me");
		else
			PeerAuthenticationString = tr("Unknown");

		item->setText(3,PeerAuthenticationString) ;

		uint64_t last_time_used = now - detail.lastUsed ;
		QString lst_used_str ;
		
		if(last_time_used < 3600)
			lst_used_str = tr("Last hour") ;
		else if(last_time_used < 86400)
			lst_used_str = tr("Today") ;
		else if(last_time_used > 86400 * 15000)
			lst_used_str = tr("Never");
		else
			lst_used_str = tr("%1 days ago").arg((int)( last_time_used / 86400 )) ;

		item->setText(COLUMN_LAST_USED,lst_used_str) ;

		/**
		 * Determinated the Background Color
		 */
		QColor backgrndcolor;

		if (detail.accept_connection)
		{
			if (detail.ownsign) 
			{
				item -> setText(0, "0");
				item -> setIcon(0,(QIcon(IMAGE_AUTHED)));
				backgrndcolor = backgroundColorOwnSign();
			} 
			else 
			{
				item -> setText(0, "0");
				item -> setIcon(0,(QIcon(IMAGE_AUTHED)));
				backgrndcolor = backgroundColorAcceptConnection();
			}
		} 
		else 
		{
			item -> setText(0, "1");

			if (detail.hasSignedMe)
			{
				backgrndcolor = backgroundColorHasSignedMe();
				item -> setIcon(0,(QIcon(IMAGE_DENIED)));
				for(int k=0;k<8;++k)
					item -> setToolTip(k, QString::fromUtf8(detail.name.c_str()) + tr(" has authenticated you. \nRight-click and select 'make friend' to be able to connect."));
			}
			else
			{
				backgrndcolor = backgroundColorDenied();
				item -> setIcon(0,(QIcon(IMAGE_DENIED)));
			}
		}

		// Color each Background column in the Network Tab except the first one => 1-9
		// whith the determinated color
		for(int i = 0; i <10; i++)
			item -> setBackground(i,QBrush(backgrndcolor));

		/* add to the list */
		if (detail.accept_connection || detail.validLvl >= RS_TRUST_LVL_MARGINAL) 
		{
			/* add gpg item to the list. If item is already in the list, it won't be duplicated thanks to Qt */
			connectWidget->addTopLevelItem(item);
		} 
		else 
		{
			ui.unvalidGPGKeyWidget->addTopLevelItem(item);
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
		self_item = new RSTreeWidgetItem(NULL, 0);
		self_item->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
	}
	self_item -> setText(0, "0");
	self_item->setIcon(0,(QIcon(IMAGE_AUTHED)));
	self_item->setText(COLUMN_PEERNAME, QString::fromUtf8(ownGPGDetails.name.c_str()) + " (" + tr("yourself") + ")");
	self_item->setText(2,"N/A");
	self_item->setText(COLUMN_PEERID, QString::fromStdString(ownGPGDetails.id));

	// Color each Background column in the Network Tab except the first one => 1-9
	for(int i=0;i<10;++i)
	{
		self_item->setBackground(i,backgroundColorSelf()) ;
	}
	connectWidget->addTopLevelItem(self_item);

	connectWidget->update(); /* update display */
	ui.unvalidGPGKeyWidget->update(); /* update display */

	if (ui.filterLineEdit->text().isEmpty() == false) {
		filterItems(ui.filterLineEdit->text());
	}

}

QTreeWidgetItem *NetworkDialog::getCurrentNeighbour()
{ 
        if (ui.connectTreeWidget->selectedItems().size() != 0)  {
            return ui.connectTreeWidget -> currentItem();
        } else if (ui.unvalidGPGKeyWidget->selectedItems().size() != 0) {
            return ui.unvalidGPGKeyWidget->currentItem();
        }

        return NULL;
}   

/* Utility Fns */
RsCertId getNeighRsCertId(QTreeWidgetItem *i)
{
	RsCertId id = (i -> text(COLUMN_PEERID)).toStdString();
	return id;
}   
  
/* So from the Neighbours Dialog we can call the following control Functions:
 * (1) Load Certificate             NeighLoadCertificate(std::string file)
 * (2) Neigh  Auth                  NeighAuthFriend(id, code)
 * (4) Neigh  Add                   NeighAddFriend(id)
 *
 * All of these rely on the finding of the current Id.
 */

//std::string NetworkDialog::loadneighbour()
//{
//#ifdef NET_DEBUG
//        std::cerr << "NetworkDialog::loadneighbour()" << std::endl;
//#endif
// use misc::getOpenFileName
//        QString fileName = QFileDialog::getOpenFileName(this, tr("Select Certificate"), "",
//	                                             tr("Certificates (*.pqi *.pem)"));
//
//	std::string file = fileName.toStdString();
//	std::string id;
//        std::string gpg_id;
//	if (file != "")
//	{
//                rsPeers->loadCertificateFromFile(file, id, gpg_id);
//	}
//	return id;
//}

//void NetworkDialog::addneighbour()
//{
////        QTreeWidgetItem *c = getCurrentNeighbour();
//#ifdef NET_DEBUG
//        std::cerr << "NetworkDialog::addneighbour()" << std::endl;
//#endif
//        /*
//        rsServer->NeighAddFriend(getNeighRsCertId(c));
//        */
//}

//void NetworkDialog::authneighbour()
//{
////        QTreeWidgetItem *c = getCurrentNeighbour();
//#ifdef NET_DEBUG
//        std::cerr << "NetworkDialog::authneighbour()" << std::endl;
//#endif
//        /*
//	RsAuthId code;
//        rsServer->NeighAuthFriend(getNeighRsCertId(c), code);
//        */
//}

/** Open a QFileDialog to browse for a pem/pqi file. */
void NetworkDialog::on_actionAddFriend_activated()
{
//  /* Create a new input dialog, which allows users to create files, too */
// use misc::getOpenFileName
//  QFileDialog dialog (this, tr("Select a pem/pqi File"));
//  //dialog.setDirectory(QFileInfo(ui.lineTorConfig->text()).absoluteDir());
//  //dialog.selectFile(QFileInfo(ui.lineTorConfig->text()).fileName());
//  dialog.setFileMode(QFileDialog::AnyFile);
//  dialog.setReadOnly(false);
//
//  /* Prompt the user to select a file or create a new one */
//  if (!dialog.exec() || dialog.selectedFiles().isEmpty()) {
//    return;
//  }
//  QString filename = QDir::convertSeparators(dialog.selectedFiles().at(0));
//
//  /* Check if the file exists */
//  QFile torrcFile(filename);
//  if (!QFileInfo(filename).exists()) {
//    /* The given file does not exist. Should we create it? */
//    int response = VMessageBox::question(this,
//                     tr("File Not Found"),
//                     tr("%1 does not exist. Would you like to create it?")
//                                                            .arg(filename),
//                     VMessageBox::Yes, VMessageBox::No);
//
//    if (response == VMessageBox::No) {
//      /* Don't create it. Just bail. */
//      return;
//    }
//    /* Attempt to create the specified file */
//    if (!torrcFile.open(QIODevice::WriteOnly)) {
//      VMessageBox::warning(this,
//        tr("Failed to Create File"),
//        tr("Unable to create %1 [%2]").arg(filename)
//                                      .arg(torrcFile.errorString()),
//        VMessageBox::Ok);
//      return;
//    }
//  }
//  //ui.lineTorConfig->setText(filename);
}


void NetworkDialog::on_actionExportKey_activated()
{
//    qDebug() << "  exportcert";
//
//    std::string cert = rsPeers->GetRetroshareInvite();
//    if (cert.empty()) {
//        QMessageBox::information(this, tr("RetroShare"),
//                         tr("Sorry, create certificate failed"),
//                         QMessageBox::Ok, QMessageBox::Ok);
//        return;
//    }
//
// use misc::getSaveFileName
//    QString qdir = QFileDialog::getSaveFileName(this,
//                                                tr("Please choose a filename"),
//                                                QDir::homePath(),
//                                                tr("RetroShare Certificate (*.rsc );;All Files (*)"));
//    //Todo: move save to file to p3Peers::SaveCertificateToFile
//
//    if (qdir.isEmpty() == false) {
//        QFile CertFile (qdir);
//        if (CertFile.open(QIODevice::WriteOnly/* | QIODevice::Text*/)) {
//            if (CertFile.write(QByteArray(cert.c_str())) > 0) {
//                QMessageBox::information(this, tr("RetroShare"),
//                                 tr("Certificate file successfully created"),
//                                 QMessageBox::Ok, QMessageBox::Ok);
//            } else {
//                QMessageBox::information(this, tr("RetroShare"),
//                                 tr("Sorry, certificate file creation failed"),
//                                 QMessageBox::Ok, QMessageBox::Ok);
//            }
//            CertFile.close();
//        } else {
//            QMessageBox::information(this, tr("RetroShare"),
//                             tr("Sorry, certificate file creation failed"),
//                             QMessageBox::Ok, QMessageBox::Ok);
//        }
//    }
}

void NetworkDialog::on_actionCreate_New_Profile_activated()
{
//    GenCertDialog gencertdialog (this);
//    gencertdialog.exec ();
}

// void NetworkDialog::on_actionTabsnorth_activated()
// {
//   ui.networkTab->setTabPosition(QTabWidget::North);
//   
//   Settings->setValueToGroup("NetworkDialog", "TabWidget_Position",ui.networkTab->tabPosition());
// }
// 
// void NetworkDialog::on_actionTabssouth_activated()
// {
//   ui.networkTab->setTabPosition(QTabWidget::South);
// 
//   Settings->setValueToGroup("NetworkDialog", "TabWidget_Position",ui.networkTab->tabPosition());
// 
// }
// 
// void NetworkDialog::on_actionTabswest_activated()
// {
//   ui.networkTab->setTabPosition(QTabWidget::West);
// 
//   Settings->setValueToGroup("NetworkDialog", "TabWidget_Position",ui.networkTab->tabPosition());
// }
// 
// void NetworkDialog::on_actionTabsright_activated()
// {
//   ui.networkTab->setTabPosition(QTabWidget::East);
// 
//   Settings->setValueToGroup("NetworkDialog", "TabWidget_Position",ui.networkTab->tabPosition());
// }
// 
// void NetworkDialog::on_actionTabsTriangular_activated()
// {
//   ui.networkTab->setTabShape(QTabWidget::Triangular);
//   ui.tabBottom->setTabShape(QTabWidget::Triangular);
// }
// 
// void NetworkDialog::on_actionTabsRounded_activated()
// {
//   ui.networkTab->setTabShape(QTabWidget::Rounded);
//   ui.tabBottom->setTabShape(QTabWidget::Rounded);
// }
// 
// void NetworkDialog::loadtabsettings()
// {
//   Settings->beginGroup("NetworkDialog");
// 
//   if(Settings->value("TabWidget_Position","0").toInt() == 0)
//   {
//     qDebug() << "Tab North";
//     ui.networkTab->setTabPosition(QTabWidget::North);
//   }
//   else if (Settings->value("TabWidget_Position","1").toInt() == 1)
//   {
//     qDebug() << "Tab South";
//     ui.networkTab->setTabPosition(QTabWidget::South);
//   }
//   else if (Settings->value("TabWidget_Position","2").toInt() ==2)
//   {
//     qDebug() << "Tab West";
//     ui.networkTab->setTabPosition(QTabWidget::West);
//   }
//   else if(Settings->value("TabWidget_Position","3").toInt() ==3)
//   {
//     qDebug() << "Tab East";
//     ui.networkTab->setTabPosition(QTabWidget::East);
//   }
// 
//   Settings->endGroup();
// }

void NetworkDialog::filterColumnChanged(int)
{
    filterItems(ui.filterLineEdit->text());
}

void NetworkDialog::filterItems(const QString &text)
{
    int filterColumn = ui.filterLineEdit->currentFilter();

    int count = ui.connectTreeWidget->topLevelItemCount ();
    for (int index = 0; index < count; index++) {
        filterItem(ui.connectTreeWidget->topLevelItem(index), text, filterColumn);
    }
    count = ui.unvalidGPGKeyWidget->topLevelItemCount ();
    for (int nIndex = 0; nIndex < count; nIndex++) {
        filterItem(ui.unvalidGPGKeyWidget->topLevelItem(nIndex), text, filterColumn);
    }
}

bool NetworkDialog::filterItem(QTreeWidgetItem *item, const QString &text, int filterColumn)
{
    bool visible = true;

    if (text.isEmpty() == false) {
        if (item->text(filterColumn).contains(text, Qt::CaseInsensitive) == false) {
            visible = false;
        }
    }

    int visibleChildCount = 0;
    int count = item->childCount();
    for (int index = 0; index < count; index++) {
        if (filterItem(item->child(index), text, filterColumn)) {
            visibleChildCount++;
        }
    }

    if (visible || visibleChildCount) {
        item->setHidden(false);
    } else {
        item->setHidden(true);
    }

    return (visible || visibleChildCount);
}
