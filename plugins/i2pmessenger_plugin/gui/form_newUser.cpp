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

	if(Name.isEmpty())
	{
		QMessageBox* msgBox= new QMessageBox(this);
			msgBox->setIcon(QMessageBox::Warning);
			msgBox->setText("Adding User");
			msgBox->setInformativeText("You must add a nick for the User\nadding abborted");
			msgBox->setStandardButtons(QMessageBox::Ok);
			msgBox->setDefaultButton(QMessageBox::Ok);
			msgBox->setWindowModality(Qt::NonModal);
			msgBox->show();
		return;
	}

	if(I2PDestination.length()!=516)
	{
		QMessageBox* msgBox= new QMessageBox(this);
			msgBox->setIcon(QMessageBox::Warning);
			msgBox->setText("Adding User");
			msgBox->setInformativeText("The Destination is to short (must be 516)\nadding abborted");
			msgBox->setStandardButtons(QMessageBox::Ok);
			msgBox->setDefaultButton(QMessageBox::Ok);
			msgBox->setWindowModality(Qt::NonModal);
			msgBox->show();

		return;
	}



	if(!I2PDestination.right(4).contains("AAAA",Qt::CaseInsensitive)){
		//the last 4 char must be "AAAA"
		QMessageBox* msgBox= new QMessageBox(this);
			msgBox->setIcon(QMessageBox::Warning);
			msgBox->setText("Adding User");
			msgBox->setInformativeText("The Destination must end with AAAA\nadding abborted");
			msgBox->setStandardButtons(QMessageBox::Ok);
			msgBox->setDefaultButton(QMessageBox::Ok);
			msgBox->setWindowModality(Qt::NonModal);
			msgBox->show();
		return;
	}


	if(I2PDestination==Core->getMyDestination())
	{
		QMessageBox* msgBox= new QMessageBox(this);
			msgBox->setIcon(QMessageBox::Warning);
			msgBox->setText("Adding User");
			msgBox->setInformativeText("This Destination is yours, adding aborted !");
			msgBox->setStandardButtons(QMessageBox::Ok);
			msgBox->setDefaultButton(QMessageBox::Ok);
			msgBox->setWindowModality(Qt::NonModal);
			msgBox->show();
		return;

	}

	if(Core->addNewUser(Name,I2PDestination,0)==false){

		QMessageBox* msgBox= new QMessageBox(NULL);
		msgBox->setIcon(QMessageBox::Warning);
                msgBox->setInformativeText("There allready exits one user with the same I2P,- or TorDestination");
		msgBox->setStandardButtons(QMessageBox::Ok);
		msgBox->setDefaultButton(QMessageBox::Ok);
		msgBox->setWindowModality(Qt::NonModal);
		msgBox->show();

		this->close();
	}
}
