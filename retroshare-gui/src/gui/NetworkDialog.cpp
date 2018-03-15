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

//following defined in model
/*#define IMAGE_AUTHED         ":/images/accepted16.png"
#define IMAGE_DENIED         ":/images/denied16.png"
#define IMAGE_TRUSTED        ":/images/rs-2.png" */

// Defines for key list columns

//following defined in model
/*#define COLUMN_CHECK 0
#define COLUMN_PEERNAME    1
#define COLUMN_I_AUTH_PEER 2
#define COLUMN_PEER_AUTH_ME 3
#define COLUMN_PEERID      4
#define COLUMN_LAST_USED   5
#define COLUMN_COUNT 6 */

//RsPeerId getNeighRsCertId(QTreeWidgetItem *i);

/******
 * #define NET_DEBUG 1
 *****/

//static const unsigned int ROLE_SORT = Qt::UserRole + 1 ;

/** Constructor */
NetworkDialog::NetworkDialog(QWidget */*parent*/)
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);
  
    connect( ui.filterLineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(filterItems(QString)));
    connect( ui.filterLineEdit, SIGNAL(filterChanged(int)), this, SLOT(filterColumnChanged(int)));


    //list data model
    float f = QFontMetricsF(font()).height()/14.0 ;

    PGPIdItemModel = new pgpid_item_model(neighs, f, this);
    PGPIdItemProxy = new pgpid_item_proxy(this);
    connect(ui.onlyTrustedKeys, SIGNAL(toggled(bool)), PGPIdItemProxy, SLOT(use_only_trusted_keys(bool)));
    PGPIdItemProxy->setSourceModel(PGPIdItemModel);
    PGPIdItemProxy->setFilterKeyColumn(COLUMN_PEERNAME);
    PGPIdItemProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    PGPIdItemProxy->setSortRole(Qt::EditRole); //use edit role to get raw data since we do not have edit for this model.
    ui.connectTreeWidget->setModel(PGPIdItemProxy);
    ui.connectTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    ui.connectTreeWidget->verticalHeader()->hide();
    ui.connectTreeWidget->setShowGrid(false);
    ui.connectTreeWidget->setUpdatesEnabled(true);
    ui.connectTreeWidget->setSortingEnabled(true);
    ui.connectTreeWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui.connectTreeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(ui.connectTreeWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( connectTreeWidgetCostumPopupMenu( QPoint ) ) );
    connect(ui.connectTreeWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(peerdetails()));

    /* Set header resize modes and initial section sizes */
/*    QHeaderView * _header = ui.connectTreeWidget->header () ;
    QHeaderView_setSectionResizeModeColumn(_header, COLUMN_CHECK, QHeaderView::Custom);
    QHeaderView_setSectionResizeModeColumn(_header, COLUMN_PEERNAME, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(_header, COLUMN_I_AUTH_PEER, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(_header, COLUMN_PEER_AUTH_ME, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(_header, COLUMN_PEERID, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(_header, COLUMN_LAST_USED, QHeaderView::Interactive); */


    ui.onlyTrustedKeys->setMinimumWidth(20*f);

/*    QMenu *menu = new QMenu();
    menu->addAction(ui.actionTabsright); 
    menu->addAction(ui.actionTabswest);
    menu->addAction(ui.actionTabssouth); 
    menu->addAction(ui.actionTabsnorth);
    menu->addSeparator();
    menu->addAction(ui.actionTabsTriangular); 
    menu->addAction(ui.actionTabsRounded);  */
    
    /* add filter actions */
    ui.filterLineEdit->addFilter(QIcon(), tr("Name"), COLUMN_PEERNAME, tr("Search name"));
    ui.filterLineEdit->addFilter(QIcon(), tr("Peer ID"), COLUMN_PEERID, tr("Search peer ID"));
    ui.filterLineEdit->setCurrentFilter(COLUMN_PEERNAME);
    connect(ui.filterLineEdit, SIGNAL(textChanged(QString)), PGPIdItemProxy, SLOT(setFilterWildcard(QString)));

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

	QModelIndexList l = ui.connectTreeWidget->selectionModel()->selection().indexes();
	if(l.empty())
	{
		return;
	}

	QMenu *contextMnu = new QMenu;

	RsPgpId peer_id(ui.connectTreeWidget->model()->data(ui.connectTreeWidget->model()->index(l.begin()->row(), COLUMN_PEERID)).toString().toStdString()) ;

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
		if(now > (time_t) (THREE_MONTHS + details.lastUsed) && !details.accept_connection)
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
    updateDisplay();
//	insertConnect() ;
}

void NetworkDialog::denyFriend()
{
    QModelIndexList l = ui.connectTreeWidget->selectionModel()->selection().indexes();
    if(l.empty())
        return;

    RsPgpId peer_id(ui.connectTreeWidget->model()->data(ui.connectTreeWidget->model()->index(l.begin()->row(), COLUMN_PEERID)).toString().toStdString());
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

    QModelIndexList l = ui.connectTreeWidget->selectionModel()->selection().indexes();
    if(l.empty())
        return;

    PGPKeyDialog::showIt(RsPgpId(ui.connectTreeWidget->model()->data(ui.connectTreeWidget->model()->index(l.begin()->row(), COLUMN_PEERID)).toString().toStdString()), PGPKeyDialog::PageDetails);
}

/** Shows Peer Information/Auth Dialog */
void NetworkDialog::peerdetails()
{
    QModelIndexList l = ui.connectTreeWidget->selectionModel()->selection().indexes();
    if(l.empty())
        return;

    PGPKeyDialog::showIt(RsPgpId(ui.connectTreeWidget->model()->data(ui.connectTreeWidget->model()->index(l.begin()->row(), COLUMN_PEERID)).toString().toStdString()), PGPKeyDialog::PageDetails);
}

void NetworkDialog::copyLink()
{
    QModelIndexList l = ui.connectTreeWidget->selectionModel()->selection().indexes();
    if(l.empty())
        return;


    RsPgpId peer_id (ui.connectTreeWidget->model()->data(ui.connectTreeWidget->model()->index(l.begin()->row(), COLUMN_PEERID)).toString().toStdString()) ;

	QList<RetroShareLink> urls;
	RetroShareLink link = RetroShareLink::createPerson(peer_id);
	if (link.valid()) {
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



/* Utility Fns */
/*RsPeerId getNeighRsCertId(QTreeWidgetItem *i)
{
	RsPeerId id ( (i -> text(COLUMN_PEERID)).toStdString() );
	return id;
} */
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

void NetworkDialog::filterColumnChanged(int col)
{
    if(PGPIdItemProxy)
        PGPIdItemProxy->setFilterKeyColumn(col);
    //filterItems(ui.filterLineEdit->text());
}



void NetworkDialog::updateDisplay()
{
    if (!rsPeers)
        return;
    //update ids list
    std::list<RsPgpId> new_neighs;
    rsPeers->getGPGAllList(new_neighs);
    //refresh model
    PGPIdItemModel->data_updated(new_neighs);
}

