/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006,  crypton
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



#include "rshare.h"
#include "AuthorizationDialog.h"

#include "rsiface/rsiface.h"

#include <iostream>


/** Default constructor */
AuthorizationDialog::AuthorizationDialog(QWidget *parent, Qt::WFlags flags)
  : QDialog(parent, flags)
{
  /* Invoke Qt Designer generated QObject setup routine */
  ui.setupUi(this);

// GConfig config;
// config.loadWidgetInformation(this);

  // Create the status bar
  //statusBar()->showMessage("Please enter the correct AUTH CODE !");

  setFixedSize(QSize(267, 103));
  
  connect(ui.cancelButton, SIGNAL(clicked()), this, SLOT(closedlg()));
  connect(ui.okButton, SIGNAL(clicked()), this, SLOT(authAttempt()));
  //connect(ui.Ledit_name, SIGNAL(textChanged()), this, SLOT(checkAuthCode()));
  connect(ui.Ledit_name, SIGNAL(textChanged ( const QString & ) ), this, SLOT(checkAuthCode( const QString & )));
 
}



/** 
 Overloads the default show() slot so we can set opacity*/

void
AuthorizationDialog::show()
{
  //loadSettings();
  if(!this->isVisible()) {
    QDialog::show();

  }
}

void AuthorizationDialog::closeEvent (QCloseEvent * event)
{
// GConfig config;
// config.saveWidgetInformation(this);

 QWidget::closeEvent(event);
}

void AuthorizationDialog::closedlg()
{
	close();
}


void AuthorizationDialog::setAuthCode(std::string id, std::string code)
{
	authId = id;
	authCode = code;
	ui.Ledit_name->setText(QString::fromStdString(code));
	//ui.okButton ->setEnabled(true);
}

void AuthorizationDialog::checkAuthCode(const QString &txt)
{
	
	//std::cerr << "AuthCode:" << authCode << std::endl;
	//std::cerr << "Entered:" << ui.Ledit_name -> text().toStdString() << std::endl;
	//std::cerr << "Entered:" << txt.toStdString() << std::endl;

	if (authCode == txt.toStdString())
	{
		/* enable ok button */
		ui.okButton ->setEnabled(true);
	}
	else
	{
		/* disable ok button */
		ui.okButton ->setEnabled(false);
	}
}

void AuthorizationDialog::authAttempt()
{

	/* well lets do it ! */
	std::cerr << "Attempting AuthCode:" << authCode << std::endl;
	rsicontrol -> NeighAuthFriend(authId, authCode);
	rsicontrol -> NeighAddFriend(authId);

	/* close it up! */
	closedlg();
}


