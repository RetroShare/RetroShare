/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2010 Christopher Evi-Parker
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

#include "CreateLobbyDialog.h"

#include <QMessageBox>
#include <algorithm>

#include <retroshare/rsmsgs.h>
#include <retroshare/rspeers.h>

#include "gui/common/PeerDefs.h"
#include "gui/chat/PopupChatDialog.h"

CreateLobbyDialog::CreateLobbyDialog(const std::list<std::string>& peer_list,QWidget *parent, Qt::WFlags flags, std::string grpId, int grpType) :
        QDialog(parent, flags), mGrpId(grpId), mGrpType(grpType)
{
	ui = new Ui::CreateLobbyDialog() ;
    ui->setupUi(this);

	 std::string default_nick ;
	 rsMsgs->getDefaultNickNameForChatLobby(default_nick) ;

#if QT_VERSION >= 0x040700
	 ui->lobbyName_LE->setPlaceholderText(tr("Put a sensible lobby name here")) ;
	 ui->nickName_LE->setPlaceholderText(tr("Your nickname for this lobby (Change default name in options->chat)")) ;
#endif
	 ui->nickName_LE->setText(QString::fromStdString(default_nick)) ;

    connect( ui->shareButton, SIGNAL( clicked ( bool ) ), this, SLOT( createLobby( ) ) );
    connect( ui->cancelButton, SIGNAL( clicked ( bool ) ), this, SLOT( cancel( ) ) );
    connect( ui->lobbyName_LE, SIGNAL( textChanged ( QString ) ), this, SLOT( checkTextFields( ) ) );
    connect( ui->nickName_LE, SIGNAL( textChanged ( QString ) ), this, SLOT( checkTextFields( ) ) );

    connect(ui->keyShareList, SIGNAL(itemChanged( QTreeWidgetItem *, int ) ),
            this, SLOT(togglePersonItem( QTreeWidgetItem *, int ) ));

    setShareList(peer_list);
	 checkTextFields() ;
}


CreateLobbyDialog::~CreateLobbyDialog()
{
    delete ui;
}

void CreateLobbyDialog::closeEvent (QCloseEvent * event)
{
 QWidget::closeEvent(event);
}

void CreateLobbyDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void CreateLobbyDialog::checkTextFields()
{
	if(ui->lobbyName_LE->text() == "" || ui->nickName_LE->text() == "")
		ui->shareButton->setEnabled(false) ;
	else
		ui->shareButton->setEnabled(true) ;
}
void CreateLobbyDialog::createLobby()
{
    if(mShareList.empty())
    {
        QMessageBox::warning(this, tr("RetroShare"),tr("Please select at least one peer"),
        QMessageBox::Ok, QMessageBox::Ok);

        return;
    }

	 // create chat lobby !!
	 std::string lobby_name = ui->lobbyName_LE->text().toUtf8().constData() ;

    // add to group

	 int lobby_privacy_type = (ui->security_CB->currentIndex() == 0)?RS_CHAT_LOBBY_PRIVACY_LEVEL_PUBLIC:RS_CHAT_LOBBY_PRIVACY_LEVEL_PRIVATE ;

    ChatLobbyId id = rsMsgs->createChatLobby(lobby_name, mShareList, lobby_privacy_type);

	 std::cerr << "gui: Created chat lobby " << std::hex << id << std::endl ;

	 // set nick name !

	 rsMsgs->setNickNameForChatLobby(id,ui->nickName_LE->text().toStdString()) ;

	 // open chat window !!
	 std::string vpid ;
	 
	 if(rsMsgs->getVirtualPeerId(id,vpid))
		 PopupChatDialog::chatFriend(vpid) ; 

    close();
}

void CreateLobbyDialog::cancel()
{
      close();
}

void CreateLobbyDialog::setShareList(const std::list<std::string>& friend_list)
{
    if (!rsPeers)
    {
            /* not ready yet! */
            return;
    }

    std::list<std::string> peers;
    std::list<std::string>::iterator it;

	 mShareList.clear() ;
    rsPeers->getFriendList(peers);

    /* get a link to the table */
    QTreeWidget *shareWidget = ui->keyShareList;

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

		 item -> setText(0, PeerDefs::nameWithLocation(detail));
		 if (detail.state & RS_PEER_STATE_CONNECTED) {
			 item -> setTextColor(0,(Qt::darkBlue));
		 }
		 item -> setSizeHint(0,  QSize( 17,17 ) );
		 item -> setText(1, QString::fromStdString(detail.id));
		 item -> setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);

		 item -> setCheckState(0, Qt::Unchecked);

		 for(std::list<std::string>::const_iterator it2(friend_list.begin());it2!=friend_list.end();++it2)
			 if(*it == *it2)
			 {
				 item -> setCheckState(0, Qt::Checked);
				 mShareList.push_back(*it) ;
				 break ;
			 }

		 /* add to the list */
		 items.append(item);
	 }

    /* remove old items */
    shareWidget->clear();
    shareWidget->setColumnCount(1);

    /* add the items in! */
    shareWidget->insertTopLevelItems(0, items);

    shareWidget->update(); /* update display */
}

void CreateLobbyDialog::togglePersonItem( QTreeWidgetItem *item, int /*col*/ )
{
	/* extract id */
	std::string id = (item -> text(1)).toStdString();

	/* get state */
	bool checked = (Qt::Checked == item -> checkState(0)); /* alway column 0 */

	/* call control fns */
	std::list<std::string>::iterator lit = std::find(mShareList.begin(), mShareList.end(), id);

	if(checked && (lit == mShareList.end()))
		mShareList.push_back(id);						 // make sure ids not added already
	else if(lit != mShareList.end())
		mShareList.erase(lit);

	return;
}

