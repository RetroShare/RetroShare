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
#include "CreateMsgLinkDialog.h"
#include <gui/common/FriendSelectionWidget.h>

CreateMsgLinkDialog::CreateMsgLinkDialog(QWidget *parent)
	:QDialog(NULL, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint)
{
	/* Invoke the Qt Designer generated object setup routine */
	setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose, true);

	_info_GB->layout()->addWidget( _gpg_selection = new FriendSelectionWidget(this) ) ;

	QObject::connect(_link_type_CB,SIGNAL(currentIndexChanged(int)),this,SLOT(update())) ;
	QObject::connect(_create_link_PB,SIGNAL(clicked()),this,SLOT(create())) ;

	update() ;
}

void CreateMsgLinkDialog::update()
{
	if(_link_type_CB->currentIndex() == 0)
	{
		QString s ;

		s += "A private chat invite allows a specific peer to contact you\nusing encrypted private chat. You need to select a destination peer from your PGP keyring\nbefore creating the link. The link contains the encryption code and your PGP signature, so that the peer can authenticate you." ;

		_info_LB->setText(s) ;
		_gpg_selection->setHidden(false) ;
	}
	else
	{
		QString s ;

		s += "A private chat invite allows a specific peer to contact you\nusing encrypted private chat. You need to select a destination peer from your PGP keyring\nbefore creating the link. The link contains the encryption code and your PGP signature, so that the peer can authenticate you." ;

		_info_LB->setText(s) ;
		_gpg_selection->setHidden(true) ;
	}
}

void CreateMsgLinkDialog::createLink()
{
	std::cerr << "Creating link!" << std::endl;
}
