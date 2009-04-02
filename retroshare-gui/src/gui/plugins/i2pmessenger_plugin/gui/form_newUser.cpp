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
#include "form_newUser.h"

form_newUserWindow::form_newUserWindow(cCore* Core,QDialog *parent){
	setupUi(this);	
	this->setAttribute(Qt::WA_DeleteOnClose,true);
	this->Core=Core;
	connect(buttonBox,SIGNAL(accepted()),this,SLOT(addnewUser()));
}
void form_newUserWindow::addnewUser()
{
	QString Name=lineEdit->text();
	QString I2PDestination=textEdit->toPlainText();


	if(I2PDestination==Core->getMyDestination())
	{
		QMessageBox msgBox;
			msgBox.setIcon(QMessageBox::Warning);
			msgBox.setText("Adding User");
			msgBox.setInformativeText("This Destination is yours, adding aborted !");
			msgBox.setStandardButtons(QMessageBox::Ok);
			msgBox.setDefaultButton(QMessageBox::Ok);
			msgBox.exec();
		return;

	}

	if(Core->addNewUser(Name,I2PDestination,"","")==false)
		QMessageBox::warning(this, tr("I2PChat"),
                ("There allready exits one user with the same I2P,- or TorDestination"),
                QMessageBox::Ok);

	this->close();
}
