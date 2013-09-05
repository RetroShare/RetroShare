/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2013 Cyril Soler
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
#include <time.h>
#include <QMessageBox>
#include <QTimer>
#include <retroshare/rsmsgs.h>
#include <retroshare/rspeers.h>
#include "CreateMsgLinkDialog.h"
#include <gui/common/FriendSelectionWidget.h>
#include <gui/RetroShareLink.h>

CreateMsgLinkDialog::CreateMsgLinkDialog()
	:QDialog(NULL, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint)
{
	/* Invoke the Qt Designer generated object setup routine */
	setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose, false);

	QObject::connect(_create_link_PB,SIGNAL(clicked()),this,SLOT(createLink())) ;
	
	headerFrame->setHeaderImage(QPixmap(":/images/d-chat64.png"));
	headerFrame->setHeaderText(tr("Create distant chat"));

	friendSelectionWidget->setModus(FriendSelectionWidget::MODUS_SINGLE) ;
	friendSelectionWidget->setShowType(FriendSelectionWidget::SHOW_NON_FRIEND_GPG | FriendSelectionWidget::SHOW_GPG) ;
	friendSelectionWidget->setHeaderText(QObject::tr("Select who can contact you:")) ;
	friendSelectionWidget->start() ;
}

void CreateMsgLinkDialog::createNewChatLink()
{
	std::cerr << "In static method..." << std::endl;
	CreateMsgLinkDialog dialog ;
	dialog.exec() ;
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

		time_t validity_duration = computeValidityDuration() ;
		FriendSelectionWidget::IdType type ;
		std::string current_pgp_id = friendSelectionWidget->selectedId(type) ;

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
			QMessageBox::information(NULL,tr("Private chat invite created"),tr("Your new chat invite has been created. You can now copy/paste it as a Retroshare link.")) ;

#ifdef TO_REMOVE
		/*	 OLD  CODE TO CREATE A MSG LINK  */

		time_t validity_duration = computeValidityDuration() ;
		std::string hash; 
		std::string issuer_pgp_id = rsPeers->getGPGOwnId() ;

		bool res = rsMsgs->createDistantOfflineMessengingInvite(validity_duration,hash) ;

		RetroShareLink link ;

		if(!link.createPublicMsgInvite(validity_duration + time(NULL),QString::fromStdString(issuer_pgp_id),QString::fromStdString(hash)) )
		{
			std::cerr << "Cannot create link." << std::endl;
			return ;
		}

		QList<RetroShareLink> links ;
		links.push_back(link) ;

		RSLinkClipboard::copyLinks(links) ;

		if(!res)
			QMessageBox::critical(NULL,tr("Messaging invite creation failed"),tr("The creation of the messaging invite failed")) ;
		else
			QMessageBox::information(NULL,tr("Messaging invite created"),tr("Your new messaging chat invite has been copied to clipboard. You can now paste it as a Retroshare link.")) ;
#endif
}
