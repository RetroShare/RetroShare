/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008 Robert Fernie
 *
 *  This program is free software; you can redistribute it and/or
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

#include <iostream>
#include <QMessageBox>
#include <retroshare/rsmsgs.h>
#include "CreateMsgLinkDialog.h"
#include <gui/common/FriendSelectionWidget.h>
#include <gui/RetroShareLink.h>

CreateMsgLinkDialog::CreateMsgLinkDialog(QWidget *parent)
	:QDialog(NULL, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint)
{
	/* Invoke the Qt Designer generated object setup routine */
	setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose, false);

	_info_GB->layout()->addWidget( _gpg_selection = new FriendSelectionWidget(this) ) ;

	QObject::connect(_link_type_CB,SIGNAL(currentIndexChanged(int)),this,SLOT(update())) ;
	QObject::connect(_create_link_PB,SIGNAL(clicked()),this,SLOT(createLink())) ;
	QObject::connect(_create_new_PB,SIGNAL(toggled(bool)),this,SLOT(toggleCreateLink(bool))) ;

	_gpg_selection->setModus(FriendSelectionWidget::MODUS_SINGLE) ;
	_gpg_selection->setShowType(FriendSelectionWidget::SHOW_NON_FRIEND_GPG | FriendSelectionWidget::SHOW_GPG) ;
	_gpg_selection->setHeaderText(QObject::tr("Select who can contact you:")) ;
	_gpg_selection->start() ;

	toggleCreateLink(false) ;
	update() ;
}

void CreateMsgLinkDialog::toggleCreateLink(bool b)
{
	_new_link_F->setHidden(!b) ;
}
void CreateMsgLinkDialog::update()
{
	if(_link_type_CB->currentIndex() == 0)
	{
		QString s ;

		s += "A private chat invite allows a specific peer to contact you using encrypted private chat. You need to select a destination peer from your PGP keyring before creating the link. The link contains the encryption code and your PGP signature, so that the peer can authenticate you." ;

		_info_TB->setHtml(s) ;
		_gpg_selection->setHidden(false) ;
	}
	else
	{
		QString s ;

		s += "A public message link allows any peer in the nearby network to send a private message to you. The message is encrypted and only you can read it." ;

		_info_TB->setHtml(s) ;
		_gpg_selection->setHidden(true) ;
	}
}

time_t CreateMsgLinkDialog::computeValidityDuration() const
{
	time_t unit ;

	switch(_validity_time_CB->currentIndex())
	{
		default:
		case 0: unit = 3600 ; 
				  break ;
		case 1: unit = 3600*24 ; 
				  break ;
		case 2: unit = 3600*24*7 ; 
				  break ;
		case 3: unit = 3600*24*30 ; 
				  break ;
		case 4: unit = 3600*24*365 ; 
				  break ;
	}

	return unit * _validity_time_SB->value() ;
}

void CreateMsgLinkDialog::createLink()
{
	std::cerr << "Creating link!" << std::endl;

	if(_link_type_CB->currentIndex() == 0)
	{
		time_t validity_duration = computeValidityDuration() ;
		FriendSelectionWidget::IdType type ;
		std::string current_pgp_id = _gpg_selection->selectedId(type) ;

		std::string encrypted_string ;

		bool res = rsMsgs->createDistantChatInvite(current_pgp_id,validity_duration,encrypted_string) ;

		RetroShareLink link ;

		if(!link.createPrivateChatInvite(validity_duration + time(NULL),QString::fromStdString(current_pgp_id),QString::fromStdString(encrypted_string)) )
			std::cerr << "Cannot create link." << std::endl;

		QList<RetroShareLink> links ;
		links.push_back(link) ;

		RSLinkClipboard::copyLinks(links) ;

		if(!res)
			QMessageBox::critical(NULL,tr("Private chat invite creation failed"),tr("The creation of the chat invite failed")) ;
		else
			QMessageBox::information(NULL,tr("Private chat invite created"),tr("Your new chat invite has been copied to clipboard. You can now paste it as a Retroshare link.")) ;

	}
	else
	{
		std::cerr << "Private msg links not yet implemented." << std::endl;
	}
}

