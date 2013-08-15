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
#include <QPushButton>
#include <algorithm>

#include <retroshare/rsmsgs.h>
#include <retroshare/rspeers.h>

#include "gui/common/PeerDefs.h"
#include "ChatDialog.h"

CreateLobbyDialog::CreateLobbyDialog(const std::list<std::string>& peer_list, int privacyLevel, QWidget *parent) :
	QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
{
	ui = new Ui::CreateLobbyDialog() ;
	ui->setupUi(this);

	ui->headerFrame->setHeaderImage(QPixmap(":/images/chat_64.png"));
	ui->headerFrame->setHeaderText(tr("Create Chat Lobby"));

	std::string default_nick ;
	rsMsgs->getDefaultNickNameForChatLobby(default_nick) ;

#if QT_VERSION >= 0x040700
	ui->lobbyName_LE->setPlaceholderText(tr("Put a sensible lobby name here")) ;
	ui->nickName_LE->setPlaceholderText(tr("Your nickname for this lobby (Change default name in options->chat)")) ;
#endif
	ui->nickName_LE->setText(QString::fromUtf8(default_nick.c_str())) ;

	connect( ui->buttonBox, SIGNAL(accepted()), this, SLOT(createLobby()));
	connect( ui->buttonBox, SIGNAL(rejected()), this, SLOT(close()));
	connect( ui->lobbyName_LE, SIGNAL( textChanged ( QString ) ), this, SLOT( checkTextFields( ) ) );
	connect( ui->lobbyTopic_LE, SIGNAL( textChanged ( QString ) ), this, SLOT( checkTextFields( ) ) );
	connect( ui->nickName_LE, SIGNAL( textChanged ( QString ) ), this, SLOT( checkTextFields( ) ) );

	/* initialize key share list */
	ui->keyShareList->setHeaderText(tr("Contacts:"));
	ui->keyShareList->setModus(FriendSelectionWidget::MODUS_CHECK);
	ui->keyShareList->setShowType(FriendSelectionWidget::SHOW_GROUP | FriendSelectionWidget::SHOW_SSL);
	ui->keyShareList->start();
	ui->keyShareList->setSelectedSslIds(peer_list, false);

	if (privacyLevel) {
		ui->security_CB->setCurrentIndex((privacyLevel == RS_CHAT_LOBBY_PRIVACY_LEVEL_PUBLIC) ? 0 : 1);
	}

	checkTextFields();

	ui->lobbyName_LE->setFocus();
}

CreateLobbyDialog::~CreateLobbyDialog()
{
	delete ui;
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
		ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false) ;
	else
		ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true) ;
}

void CreateLobbyDialog::createLobby()
{
	std::list<std::string> shareList;
	ui->keyShareList->selectedSslIds(shareList, false);

//	if (shareList.empty()) {
//		QMessageBox::warning(this, "RetroShare", tr("Please select at least one friend"), QMessageBox::Ok, QMessageBox::Ok);
//		return;
//	}

	// create chat lobby !!
	std::string lobby_name = ui->lobbyName_LE->text().toUtf8().constData() ;
	std::string lobby_topic = ui->lobbyTopic_LE->text().toUtf8().constData() ;

	// add to group

	int lobby_privacy_type = (ui->security_CB->currentIndex() == 0)?RS_CHAT_LOBBY_PRIVACY_LEVEL_PUBLIC:RS_CHAT_LOBBY_PRIVACY_LEVEL_PRIVATE ;

	ChatLobbyId id = rsMsgs->createChatLobby(lobby_name, lobby_topic, shareList, lobby_privacy_type);

	std::cerr << "gui: Created chat lobby " << std::hex << id << std::endl ;

	// set nick name !

	rsMsgs->setNickNameForChatLobby(id,ui->nickName_LE->text().toUtf8().constData()) ;

	// open chat window !!
	std::string vpid ;
	
	if(rsMsgs->getVirtualPeerId(id,vpid))
		ChatDialog::chatFriend(vpid) ; 

	close();
}
