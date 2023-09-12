/*******************************************************************************
 * gui/NetworkDialog.cpp                                                       *
 *                                                                             *
 * Copyright (c) 2006 Crypton          <retroshare.project@gmail.com>          *
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
#define IMAGE_PEERDETAILS    ":/images/info16.png"
#define IMAGE_MAKEFRIEND     ":/images/user/add_user16.png"
#define IMAGE_COPYLINK       ":/images/copyrslink.png"
#define IMAGE_MESSAGE        ":/icons/mail/write-mail.png"

/******
 * #define NET_DEBUG 1
 *****/

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
    PGPIdItemProxy->setFilterKeyColumn(pgpid_item_model::PGP_ITEM_MODEL_COLUMN_PEERNAME);
    PGPIdItemProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    PGPIdItemProxy->setSortRole(Qt::EditRole); //use edit role to get raw data since we do not have edit for this model.
    ui.connectTreeWidget->setModel(PGPIdItemProxy);
    ui.connectTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    ui.connectTreeWidget->verticalHeader()->hide();
    ui.connectTreeWidget->setShowGrid(false);
    ui.connectTreeWidget->setUpdatesEnabled(true);
    ui.connectTreeWidget->setSortingEnabled(true);
    ui.connectTreeWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui.connectTreeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    connect(ui.connectTreeWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( connectTreeWidgetCostumPopupMenu( QPoint ) ) );
    connect(ui.connectTreeWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(peerdetails()));

    ui.onlyTrustedKeys->setMinimumWidth(20*f);

    /* add filter actions */
    ui.filterLineEdit->addFilter(QIcon(), tr("Name"), pgpid_item_model::PGP_ITEM_MODEL_COLUMN_PEERNAME, tr("Search name"));
    ui.filterLineEdit->addFilter(QIcon(), tr("Peer ID"), pgpid_item_model::PGP_ITEM_MODEL_COLUMN_PEERID, tr("Search peer ID"));
    ui.filterLineEdit->setCurrentFilter(pgpid_item_model::PGP_ITEM_MODEL_COLUMN_PEERNAME);
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

	contextMnu->addAction(QIcon(IMAGE_PEERDETAILS), tr("Profile details..."), this, SLOT(peerdetails()));
	contextMnu->addSeparator() ;
	contextMnu->addAction(QIcon(), tr("Remove unused keys..."), this, SLOT(removeUnusedKeys()));
	contextMnu->addAction(QIcon(), tr("Remove this key"), this, SLOT(removeSelectedKeys()));
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

	removeKeys(selected);
}

void NetworkDialog::removeSelectedKeys()
{
	QModelIndexList l = ui.connectTreeWidget->selectionModel()->selection().indexes();
	if(l.empty())
		return;
	std::set<RsPgpId> selected;

	std::set<RsPgpId> friends;
	for (int i = 0; i < l.size(); i++)
	{
		RsPgpId peer_id = RsPgpId(ui.connectTreeWidget->model()->data(ui.connectTreeWidget->model()->index(l[i].row(), COLUMN_PEERID)).toString().toStdString());
		RsPeerDetails details ;
		if(rsPeers->getGPGDetails(peer_id,details))
		{
			if(details.accept_connection)
				friends.insert(peer_id);
			else
				selected.insert(peer_id);
		}
	}
	if(!friends.empty())
	{
		if ((QMessageBox::question(this, "RetroShare", tr("You have selected %1 accepted peers among others,\n Are you sure you want to un-friend them?").arg(friends.size()), QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes)) == QMessageBox::Yes)
		{
			for(std::set<RsPgpId>::const_iterator it(friends.begin());it!=friends.end();++it)
				rsPeers->removeFriend(*it);
			selected.insert(friends.begin(),friends.end());
		}
	}
	if(!selected.empty())
		removeKeys(selected);
  
	updateDisplay();
}

void NetworkDialog::removeKeys(std::set<RsPgpId> selected)
{
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

    RsPgpId peer_id(ui.connectTreeWidget->model()->data(ui.connectTreeWidget->model()->index(l.begin()->row(), pgpid_item_model::PGP_ITEM_MODEL_COLUMN_PEERID)).toString().toStdString());
	rsPeers->removeFriend(peer_id) ;

	securedUpdateDisplay();
}

void NetworkDialog::makeFriend()
{

    QModelIndexList l = ui.connectTreeWidget->selectionModel()->selection().indexes();
    if(l.empty())
        return;

    PGPKeyDialog::showIt(RsPgpId(ui.connectTreeWidget->model()->data(ui.connectTreeWidget->model()->index(l.begin()->row(), pgpid_item_model::PGP_ITEM_MODEL_COLUMN_PEERID)).toString().toStdString()), PGPKeyDialog::PageDetails);
}

/** Shows Peer Information/Auth Dialog */
void NetworkDialog::peerdetails()
{
    QModelIndexList l = ui.connectTreeWidget->selectionModel()->selection().indexes();
    if(l.empty())
        return;

    PGPKeyDialog::showIt(RsPgpId(ui.connectTreeWidget->model()->data(ui.connectTreeWidget->model()->index(l.begin()->row(), pgpid_item_model::PGP_ITEM_MODEL_COLUMN_PEERID)).toString().toStdString()), PGPKeyDialog::PageDetails);
}

void NetworkDialog::copyLink()
{
    QModelIndexList l = ui.connectTreeWidget->selectionModel()->selection().indexes();
    if(l.empty())
        return;


    RsPgpId peer_id (ui.connectTreeWidget->model()->data(ui.connectTreeWidget->model()->index(l.begin()->row(), pgpid_item_model::PGP_ITEM_MODEL_COLUMN_PEERID)).toString().toStdString()) ;

	QList<RetroShareLink> urls;
	RetroShareLink link = RetroShareLink::createPerson(peer_id);
	if (link.valid()) {
		urls.push_back(link);
	}

	RSLinkClipboard::copyLinks(urls);
}

// void NetworkDialog::on_actionExportKey_activated()
// {
// 	ProfileManager prof ;
// 	prof.exec() ;
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

