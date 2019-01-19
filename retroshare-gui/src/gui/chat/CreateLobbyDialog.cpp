/*******************************************************************************
 * gui/chat/CreateLobbyDialog.cpp                                              *
 *                                                                             *
 * LibResAPI: API for local socket server                                      *
 *                                                                             *
 * Copyright (C) 2010, Christopher Evi-Parker <retroshare.project@gmail.com>   *
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

#include "CreateLobbyDialog.h"

#include <QMessageBox>
#include <QPushButton>
#include <algorithm>

#include <retroshare/rsmsgs.h>
#include <retroshare/rspeers.h>
#include <retroshare/rstypes.h>

#include "gui/common/PeerDefs.h"
#include "ChatDialog.h"
#include "gui/ChatLobbyWidget.h"

CreateLobbyDialog::CreateLobbyDialog(const std::set<RsPeerId>& peer_list, int privacyLevel, QWidget *parent) :
	QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
{
	ui = new Ui::CreateLobbyDialog() ;
	ui->setupUi(this);

    RsGxsId default_identity ;
    rsMsgs->getDefaultIdentityForChatLobby(default_identity) ;

    ui->idChooser_CB->loadIds(IDCHOOSER_ID_REQUIRED, default_identity);

#if QT_VERSION >= 0x040700
	ui->lobbyName_LE->setPlaceholderText(tr("Put a sensible chat room name here"));
	ui->lobbyTopic_LE->setPlaceholderText(tr("Set a descriptive topic here"));
#endif

	connect( ui->buttonBox, SIGNAL(accepted()), this, SLOT(createLobby()));
	connect( ui->buttonBox, SIGNAL(rejected()), this, SLOT(close()));
	connect( ui->lobbyName_LE, SIGNAL( textChanged ( QString ) ), this, SLOT( checkTextFields( ) ) );
	connect( ui->lobbyTopic_LE, SIGNAL( textChanged ( QString ) ), this, SLOT( checkTextFields( ) ) );
    connect( ui->idChooser_CB, SIGNAL( currentIndexChanged ( int ) ), this, SLOT( checkTextFields( ) ) );
    connect( ui->pgp_signed_CB, SIGNAL( toggled ( bool ) ), this, SLOT( checkTextFields( ) ) );

	/* initialize key share list */
	ui->keyShareList->setHeaderText(tr("Contacts:"));
	ui->keyShareList->setModus(FriendSelectionWidget::MODUS_CHECK);
	ui->keyShareList->setShowType(FriendSelectionWidget::SHOW_GROUP | FriendSelectionWidget::SHOW_SSL);
	ui->keyShareList->start();
    ui->keyShareList->setSelectedIds<RsPeerId,FriendSelectionWidget::IDTYPE_SSL>(peer_list, false);

	if (privacyLevel) {
        ui->security_CB->setCurrentIndex((privacyLevel == CHAT_LOBBY_PRIVACY_LEVEL_PUBLIC) ? 0 : 1);
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
    RsGxsId id ;

    switch(ui->idChooser_CB->getChosenId(id))
    {
        case GxsIdChooser::NoId:
        case GxsIdChooser::None: ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false) ;
                    break ;
        default:
                    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true) ;
                    break ;
    }
    
    RsIdentityDetails idd;
    
    rsIdentity->getIdDetails(id,idd) ;
    
    if( (!(idd.mFlags & RS_IDENTITY_FLAGS_PGP_KNOWN)) && ui->pgp_signed_CB->isChecked())
                    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false) ;
}

void CreateLobbyDialog::createLobby()
{
    std::set<RsPeerId> shareList;
    ui->keyShareList->selectedIds<RsPeerId,FriendSelectionWidget::IDTYPE_SSL>(shareList, false);

    //	if (shareList.empty()) {
    //		QMessageBox::warning(this, "RetroShare", tr("Please select at least one friend"), QMessageBox::Ok, QMessageBox::Ok);
    //		return;
    //	}

    // create chat lobby !!
    std::string lobby_name = ui->lobbyName_LE->text().toUtf8().constData() ;
    std::string lobby_topic = ui->lobbyTopic_LE->text().toUtf8().constData() ;

    // set nick name !
    RsGxsId gxs_id ;
    switch(ui->idChooser_CB->getChosenId(gxs_id))
    {
    case GxsIdChooser::NoId:
    case GxsIdChooser::None:
        return ;
    default: break ;
    }
    // add to group

    ChatLobbyFlags lobby_flags ;

    if(ui->security_CB->currentIndex() == 0)
        lobby_flags |= RS_CHAT_LOBBY_FLAGS_PUBLIC ;

    if(ui->pgp_signed_CB->isChecked())
        lobby_flags |= RS_CHAT_LOBBY_FLAGS_PGP_SIGNED ;
    
    ChatLobbyId id = rsMsgs->createChatLobby(lobby_name,gxs_id, lobby_topic, shareList, lobby_flags);

    std::cerr << "gui: Created chat room " << std::hex << id << std::dec << std::endl ;

    // open chat window !!
    ChatDialog::chatFriend(ChatId(id)) ;

    close();
}
