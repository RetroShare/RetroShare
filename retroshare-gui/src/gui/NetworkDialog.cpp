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
#include <QSortFilterProxyModel>

#include <retroshare/rsiface.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsdisc.h>
#include <retroshare/rsmsgs.h>

#include "common/vmessagebox.h"
#include "common/RSTreeWidgetItem.h"
#include <gui/common/FriendSelectionDialog.h>
#include "gui/msgs/MessageComposer.h"
#include "gui/profile/ProfileManager.h"
#include "NetworkDialog.h"
//#include "TrustView.h"
#include "NetworkView.h"
#include "GenCertDialog.h"
#include "connect/PGPKeyDialog.h"
#include "settings/rsharesettings.h"
#include "RetroShareLink.h"
#include "util/QtVersion.h"

#include <time.h>

/* Images for context menu icons */
#define IMAGE_LOADCERT       ":/images/loadcert16.png"
#define IMAGE_PEERDETAILS    ":/images/info16.png"
#define IMAGE_AUTH           ":/images/encrypted16.png"
#define IMAGE_CLEAN_UNUSED   ":/images/deletemail24.png"
#define IMAGE_MAKEFRIEND     ":/images/user/add_user16.png"
#define IMAGE_EXPORT         ":/images/exportpeers_16x16.png"
#define IMAGE_COPYLINK       ":/images/copyrslink.png"
#define IMAGE_MESSAGE         ":/images/mail_new.png"

/* Images for Status icons */
#define IMAGE_AUTHED         ":/images/accepted16.png"
#define IMAGE_DENIED         ":/images/denied16.png"
#define IMAGE_TRUSTED        ":/images/rs-2.png"

// Defines for key list columns
#define COLUMN_CHECK 0
#define COLUMN_PEERNAME    1
#define COLUMN_I_AUTH_PEER 2
#define COLUMN_PEER_AUTH_ME 3
#define COLUMN_PEERID      4
#define COLUMN_LAST_USED   5
#define COLUMN_COUNT 6

RsPeerId getNeighRsCertId(QTreeWidgetItem *i);

/******
 * #define NET_DEBUG 1
 *****/

static const unsigned int ROLE_SORT = Qt::UserRole + 1 ;

/** Constructor */
NetworkDialog::NetworkDialog(QWidget *parent)
: RsAutoUpdatePage(10000,parent) 	// updates every 10 sec.
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);
  
    connect( ui.connectTreeWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( connectTreeWidgetCostumPopupMenu( QPoint ) ) );
    connect( ui.connectTreeWidget, SIGNAL( itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT( peerdetails () ) );

    connect( ui.filterLineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(filterItems(QString)));
    connect( ui.filterLineEdit, SIGNAL(filterChanged(int)), this, SLOT(filterColumnChanged(int)));

    connect( ui.onlyTrustedKeys, SIGNAL(clicked()), this, SLOT(securedUpdateDisplay()));

    /* hide the Tree +/- */
    ui.connectTreeWidget -> setRootIsDecorated( false );
    ui.connectTreeWidget -> setColumnCount(6);

    compareNetworkRole = new RSTreeWidgetItemCompareRole;
    compareNetworkRole->setRole(COLUMN_LAST_USED, ROLE_SORT);

    /* Set header resize modes and initial section sizes */
    QHeaderView * _header = ui.connectTreeWidget->header () ;
    QHeaderView_setSectionResizeModeColumn(_header, COLUMN_CHECK, QHeaderView::Custom);
    QHeaderView_setSectionResizeModeColumn(_header, COLUMN_PEERNAME, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(_header, COLUMN_I_AUTH_PEER, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(_header, COLUMN_PEER_AUTH_ME, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(_header, COLUMN_PEERID, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(_header, COLUMN_LAST_USED, QHeaderView::Interactive);

    _header->model()->setHeaderData(COLUMN_CHECK, Qt::Horizontal, tr(""));
    _header->model()->setHeaderData(COLUMN_PEERNAME, Qt::Horizontal, tr("Profile"));
    _header->model()->setHeaderData(COLUMN_I_AUTH_PEER, Qt::Horizontal, tr("Trust level"));
    _header->model()->setHeaderData(COLUMN_PEER_AUTH_ME, Qt::Horizontal, tr("Has signed your key?"));
    _header->model()->setHeaderData(COLUMN_PEERID, Qt::Horizontal, tr("Id"));
    _header->model()->setHeaderData(COLUMN_LAST_USED, Qt::Horizontal, tr("Last used"));

    _header->model()->setHeaderData(COLUMN_CHECK, Qt::Horizontal, tr(" Do you accept connections signed by this profile?"),Qt::ToolTipRole);
    _header->model()->setHeaderData(COLUMN_PEERNAME, Qt::Horizontal, tr("Name of the profile"),Qt::ToolTipRole);
    _header->model()->setHeaderData(COLUMN_I_AUTH_PEER, Qt::Horizontal, tr("This column indicates trust level and whether you signed the profile PGP key"),Qt::ToolTipRole);
    _header->model()->setHeaderData(COLUMN_PEER_AUTH_ME, Qt::Horizontal, tr("Did that peer sign your own profile PGP key"),Qt::ToolTipRole);
    _header->model()->setHeaderData(COLUMN_PEERID, Qt::Horizontal, tr("PGP Key Id of that profile"),Qt::ToolTipRole);
    _header->model()->setHeaderData(COLUMN_LAST_USED, Qt::Horizontal, tr("Last time this key was used (received time, or to check connection)"),Qt::ToolTipRole);

    float f = QFontMetricsF(font()).height()/14.0 ;

    _header->resizeSection ( COLUMN_CHECK, 25*f );
    _header->resizeSection ( COLUMN_PEERNAME, 200*f );
    _header->resizeSection ( COLUMN_I_AUTH_PEER, 200*f );
    _header->resizeSection ( COLUMN_PEER_AUTH_ME, 200*f );
    _header->resizeSection ( COLUMN_LAST_USED, 75*f );

    // set header text aligment
    QTreeWidgetItem * headerItem = ui.connectTreeWidget->headerItem();
    headerItem->setTextAlignment(COLUMN_CHECK, Qt::AlignHCenter | Qt::AlignVCenter);
    headerItem->setTextAlignment(COLUMN_PEERNAME, Qt::AlignHCenter | Qt::AlignVCenter);
    headerItem->setTextAlignment(COLUMN_I_AUTH_PEER, Qt::AlignHCenter | Qt::AlignVCenter);
    headerItem->setTextAlignment(COLUMN_PEER_AUTH_ME, Qt::AlignHCenter | Qt::AlignVCenter);
    headerItem->setTextAlignment(COLUMN_PEERID, Qt::AlignVCenter);
    headerItem->setTextAlignment(COLUMN_LAST_USED, Qt::AlignVCenter);

	 headerItem->setText(0,QString()) ;

    ui.connectTreeWidget->sortItems( COLUMN_PEERNAME, Qt::AscendingOrder );

    ui.onlyTrustedKeys->setMinimumWidth(20*f);

    QMenu *menu = new QMenu();
    menu->addAction(ui.actionTabsright); 
    menu->addAction(ui.actionTabswest);
    menu->addAction(ui.actionTabssouth); 
    menu->addAction(ui.actionTabsnorth);
    menu->addSeparator();
    menu->addAction(ui.actionTabsTriangular); 
    menu->addAction(ui.actionTabsRounded);
    
    /* add filter actions */
    ui.filterLineEdit->addFilter(QIcon(), tr("Name"), COLUMN_PEERNAME, tr("Search name"));
    ui.filterLineEdit->addFilter(QIcon(), tr("Peer ID"), COLUMN_PEERID, tr("Search peer ID"));
    ui.filterLineEdit->setCurrentFilter(COLUMN_PEERNAME);
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

    RsPgpId peer_id(wi->text(COLUMN_PEERID).toStdString()) ;

	// That's what context menus are made for
	RsPeerDetails detail;
	if(!rsPeers->getGPGDetails(peer_id, detail))		// that is not suppose to fail.
		return ;

//     if(peer_id != rsPeers->getGPGOwnId())
//     {
//         if(detail.accept_connection)
//             contextMnu->addAction(QIcon(IMAGE_DENIED), tr("Deny friend"), this, SLOT(denyFriend()));
//         else	// not a friend
//             contextMnu->addAction(QIcon(IMAGE_MAKEFRIEND), tr("Make friend..."), this, SLOT(makeFriend()));
//     }
	if(peer_id == rsPeers->getGPGOwnId())
		contextMnu->addAction(QIcon(IMAGE_EXPORT), tr("Export/create a new node"), this, SLOT(on_actionExportKey_activated()));

	contextMnu->addAction(QIcon(IMAGE_PEERDETAILS), tr("Profile details..."), this, SLOT(peerdetails()));
	contextMnu->addSeparator() ;
	contextMnu->addAction(QIcon(IMAGE_CLEAN_UNUSED), tr("Remove unused keys..."), this, SLOT(removeUnusedKeys()));

	contextMnu->exec(QCursor::pos());
}

void NetworkDialog::removeUnusedKeys()
{
    std::set<RsPgpId> pre_selected ;
    std::list<RsPgpId> ids ;

	rsPeers->getGPGAllList(ids) ;
	RsPeerDetails details ;
	time_t now = time(NULL) ;
    time_t THREE_MONTHS = 3*31*24*60*60 ;//3*DayPerMonth*HoursPerDay*MinPerHour*SecPerMin

    for(std::list<RsPgpId>::const_iterator it(ids.begin());it!=ids.end();++it)
	{
		rsPeers->getGPGDetails(*it,details) ;

		if(rsPeers->haveSecretKey(*it))
		{
			std::cerr << "Skipping public/secret key pair " << *it << std::endl;
			continue ;
		}
		if(now > (time_t) (THREE_MONTHS + details.lastUsed))
		{
			std::cerr << "Adding " << *it << " to pre-selection." << std::endl;
            pre_selected.insert(*it) ;
		}
	}

    std::set<RsPgpId> selected = FriendSelectionDialog::selectFriends_PGP(NULL,
			tr("Clean keyring"),
			tr("The selected keys below haven't been used in the last 3 months. \nDo you want to delete them permanently ? \n\nNotes: Your old keyring will be backed up.\n    The removal may fail when running multiple Retroshare instances on the same machine."),FriendSelectionWidget::MODUS_CHECK,FriendSelectionWidget::SHOW_GPG | FriendSelectionWidget::SHOW_NON_FRIEND_GPG,
             pre_selected) ;
	
	std::cerr << "Removing these keys from the keyring: " << std::endl;
    for(std::set<RsPgpId>::const_iterator it(selected.begin());it!=selected.end();++it)
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
		QMessageBox::warning(NULL,tr("Keyring info"),tr("Key removal has failed. Your keyring remains intact.\n\nReported error:")+" "+error_string ) ;
	}
	insertConnect() ;
}

void NetworkDialog::denyFriend()
{
	QTreeWidgetItem *wi = getCurrentNeighbour();
    RsPgpId peer_id( wi->text(COLUMN_PEERID).toStdString() );
	rsPeers->removeFriend(peer_id) ;

	securedUpdateDisplay();
}

/*void NetworkDialog::deleteCert()
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
}*/

void NetworkDialog::makeFriend()
{
    PGPKeyDialog::showIt(RsPgpId(getCurrentNeighbour()->text(COLUMN_PEERID).toStdString()), PGPKeyDialog::PageDetails);
}

/** Shows Peer Information/Auth Dialog */
void NetworkDialog::peerdetails()
{
	QTreeWidgetItem* item = getCurrentNeighbour();
	if (item == NULL) {
		return;
	}
    PGPKeyDialog::showIt(RsPgpId(item->text(COLUMN_PEERID).toStdString()), PGPKeyDialog::PageDetails);
}

void NetworkDialog::copyLink()
{
	QTreeWidgetItem *wi = getCurrentNeighbour();
	if (wi == NULL) {
		return;
	}

    RsPgpId peer_id ( wi->text(COLUMN_PEERID).toStdString() ) ;

	QList<RetroShareLink> urls;
	RetroShareLink link;
	if (link.createPerson(peer_id)) {
		urls.push_back(link);
	}

	RSLinkClipboard::copyLinks(urls);
}

//void NetworkDialog::sendDistantMessage()
//{
//	QTreeWidgetItem *wi = getCurrentNeighbour();
//	if (wi == NULL) {
//		return;
//	}
//
//	MessageComposer *nMsgDialog = MessageComposer::newMsg();
//	if (nMsgDialog == NULL) {
//		return;
//	}
//
//    DistantMsgPeerId pid ;
//    RsPgpId mGpgId(wi->text(COLUMN_PEERID).toStdString()) ;
//
//    if(rsMsgs->getDistantMessagePeerId(mGpgId,pid))
//	{
//        nMsgDialog->addRecipient(MessageComposer::TO, pid, mGpgId);
//		nMsgDialog->show();
//		nMsgDialog->activateWindow();
//	}
//
//	/* window will destroy itself! */
//}

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

//	// Because this is called from a qt signal, there's no limitation between calls.
	time_t now = time(NULL);

    std::list<RsPgpId> neighs; //these are GPG ids
    std::list<RsPgpId>::iterator it;
	rsPeers->getGPGAllList(neighs);

	/* get a link to the table */
	QTreeWidget *connectWidget = ui.connectTreeWidget;
	/* disable sorting while editing the table */
	connectWidget->setSortingEnabled(false);

	//remove items
	int index = 0;
	while (index < connectWidget->topLevelItemCount()) 
	{
        RsPgpId gpg_widget_id( (connectWidget->topLevelItem(index))->text(COLUMN_PEERID).toStdString() );
		RsPeerDetails detail;
		if ( (!rsPeers->getGPGDetails(gpg_widget_id, detail)) || (ui.onlyTrustedKeys->isChecked() && (detail.validLvl < RS_TRUST_LVL_MARGINAL && !detail.accept_connection))) 
			delete (connectWidget->takeTopLevelItem(index));
		else 
			++index;
	}

	for(it = neighs.begin(); it != neighs.end(); ++it)
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
        QList<QTreeWidgetItem *> list = connectWidget->findItems(QString::fromStdString( (*it).toStdString() ), Qt::MatchExactly, COLUMN_PEERID);
		if (list.size() == 1) {
			item = list.front();
		} else {
				//create new item
#ifdef NET_DEBUG
				std::cerr << "NetworkDialog::insertConnect() creating new tree widget item : " << *it << std::endl;
#endif
				item = new RSTreeWidgetItem(compareNetworkRole, 0);
				item -> setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);

				int S = QFontMetricsF(font()).height() ;
                item -> setSizeHint(COLUMN_CHECK, QSize( S,S ) );

				/* (1) Person */
				item -> setText(COLUMN_PEERNAME, QString::fromUtf8(detail.name.c_str()));

				/* (4) key id */
				item -> setText(COLUMN_PEERID, QString::fromStdString(detail.gpg_id.toStdString()));
		}

        //QString TrustLevelString ;

		/* (2) Key validity */
		if (detail.ownsign) 
		{
            item -> setText(COLUMN_I_AUTH_PEER, tr("Personal signature"));
            item -> setToolTip(COLUMN_I_AUTH_PEER, tr("PGP key signed by you"));
		} 
		else 
			switch(detail.trustLvl)
			{
                case RS_TRUST_LVL_MARGINAL: item->setText(COLUMN_I_AUTH_PEER,tr("Marginally trusted peer")) ; break;
				case RS_TRUST_LVL_FULL:
                case RS_TRUST_LVL_ULTIMATE: item->setText(COLUMN_I_AUTH_PEER,tr("Fully trusted peer")) ; break ;
				case RS_TRUST_LVL_UNKNOWN:
				case RS_TRUST_LVL_UNDEFINED: 
				case RS_TRUST_LVL_NEVER:
				default: 							item->setText(2,tr("Untrusted peer")) ; break ;
			}

		/* (3) has me auth */
		QString PeerAuthenticationString ;

		if (detail.hasSignedMe)
			PeerAuthenticationString = tr("Yes");
		else
			PeerAuthenticationString = tr("No");

        item->setText(COLUMN_PEER_AUTH_ME,PeerAuthenticationString) ;

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

        QString lst_used_sort_str = QString::number(detail.lastUsed,'f',10);

		item->setText(COLUMN_LAST_USED,lst_used_str) ;
		item->setData(COLUMN_LAST_USED,ROLE_SORT,lst_used_sort_str) ;

		/**
		 * Determinated the Background Color
		 */
		QColor backgrndcolor;

		if (detail.accept_connection)
		{
            item -> setText(COLUMN_CHECK, "0");
            item -> setIcon(COLUMN_CHECK,(QIcon(IMAGE_AUTHED)));
			if (detail.ownsign) 
			{
				backgrndcolor = backgroundColorOwnSign();
			} 
			else 
			{
				backgrndcolor = backgroundColorAcceptConnection();
			}
		} 
		else 
		{
            item -> setText(COLUMN_CHECK, "1");

			if (detail.hasSignedMe)
			{
				backgrndcolor = backgroundColorHasSignedMe();
                item -> setIcon(COLUMN_CHECK,(QIcon(IMAGE_DENIED)));
                for(int k=0;k<COLUMN_COUNT;++k)
					item -> setToolTip(k, QString::fromUtf8(detail.name.c_str()) + tr(" has authenticated you. \nRight-click and select 'make friend' to be able to connect."));
			}
			else
			{
				backgrndcolor = backgroundColorDenied();
                item -> setIcon(COLUMN_CHECK,(QIcon(IMAGE_DENIED)));
			}
		}

		// Color each Background column in the Network Tab except the first one => 1-9
		// whith the determinated color
        for(int i = 0; i <COLUMN_COUNT; ++i)
			item -> setBackground(i,QBrush(backgrndcolor));

		if( (detail.accept_connection || detail.validLvl >= RS_TRUST_LVL_MARGINAL) || !ui.onlyTrustedKeys->isChecked()) 
			connectWidget->addTopLevelItem(item);
	}

	// add self to network.
	RsPeerDetails ownGPGDetails;
	rsPeers->getGPGDetails(rsPeers->getGPGOwnId(), ownGPGDetails);
	/* make a widget per friend */
	QTreeWidgetItem *self_item;
    QList<QTreeWidgetItem *> list = connectWidget->findItems(QString::fromStdString(ownGPGDetails.gpg_id.toStdString()), Qt::MatchExactly, COLUMN_PEERID);
	if (list.size() == 1) {
		self_item = list.front();
	} else {
		self_item = new RSTreeWidgetItem(compareNetworkRole, 0);
		self_item->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
	}
    self_item -> setText(COLUMN_CHECK, "0");
    self_item->setIcon(COLUMN_CHECK,(QIcon(IMAGE_AUTHED)));
	self_item->setText(COLUMN_PEERNAME, QString::fromUtf8(ownGPGDetails.name.c_str()) + " (" + tr("yourself") + ")");
    self_item->setText(COLUMN_I_AUTH_PEER,"N/A");
	self_item->setText(COLUMN_PEERID, QString::fromStdString(ownGPGDetails.gpg_id.toStdString()));

	// Color each Background column in the Network Tab except the first one => 1-9
    for(int i=0;i<COLUMN_COUNT;++i)
	{
		self_item->setBackground(i,backgroundColorSelf()) ;
	}
	connectWidget->addTopLevelItem(self_item);

	/* enable sorting */
	connectWidget->setSortingEnabled(true);
	/* update display */
	connectWidget->update();

	if (ui.filterLineEdit->text().isEmpty() == false) {
		filterItems(ui.filterLineEdit->text());
	}

}

QTreeWidgetItem *NetworkDialog::getCurrentNeighbour()
{ 
        if (ui.connectTreeWidget->selectedItems().size() != 0)  
		  {
            return ui.connectTreeWidget -> currentItem();
        } 

        return NULL;
}   

/* Utility Fns */
RsPeerId getNeighRsCertId(QTreeWidgetItem *i)
{
	RsPeerId id ( (i -> text(COLUMN_PEERID)).toStdString() );
	return id;
}   
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
	ProfileManager prof ;
	prof.exec() ;
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
    for (int index = 0; index < count; ++index) {
        filterItem(ui.connectTreeWidget->topLevelItem(index), text, filterColumn);
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
    for (int index = 0; index < count; ++index) {
        if (filterItem(item->child(index), text, filterColumn)) {
            ++visibleChildCount;
        }
    }

    if (visible || visibleChildCount) {
        item->setHidden(false);
    } else {
        item->setHidden(true);
    }

    return (visible || visibleChildCount);
}
