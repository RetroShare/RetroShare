/***************************************************************************
 *   Copyright (C) 2008 by normal   *
 *   normal@Desktop2   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "form_rename.h"

form_RenameWindow::form_RenameWindow(cCore* Core,QString OldNickname,QString Destination){
	setupUi(this);

	this->setAttribute(Qt::WA_DeleteOnClose,true);
	this->Core=Core;
	this->Destination=Destination;
	
	QLineEdit* lineEdit = this->lineEdit;
	lineEdit->setText(OldNickname);

	connect(okButton,SIGNAL(clicked()),this,
		SLOT(OK()));
}

void form_RenameWindow::OK(){
	QLineEdit* lineEdit_2 = this->lineEdit_2;
	Core->renameuserByI2PDestination(Destination,lineEdit_2->text());
	this->close();
}