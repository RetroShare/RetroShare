/****************************************************************
 *  RShare is distributed under the following license:
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



#include <rshare.h>
#include "ConnectDialog.h"

#include "rsiface/rsiface.h"
#include <iostream>

/** Default constructor */
ConnectDialog::ConnectDialog(QWidget *parent, Qt::WFlags flags)
  : QMainWindow(parent, flags)
{
  /* Invoke Qt Designer generated QObject setup routine */
  ui.setupUi(this);

  /* Create Bandwidth Graph related QObjects */
  _settings = new RshareSettings();
  
  // Create the status bar
  statusBar()->showMessage("Peer Informations");

  setFixedSize(QSize(330, 412));
  
  connect(ui.cancelButton, SIGNAL(clicked()), this, SLOT(closeinfodlg()));
  connect(ui.okButton, SIGNAL(clicked()), this, SLOT(authAttempt()));
  connect(ui.Ledit_name, SIGNAL(textChanged ( const QString & ) ), this, SLOT(checkAuthCode( const QString & )));
 
}



/** 
 Overloads the default show() slot so we can set opacity*/

void
ConnectDialog::show()
{
  //loadSettings();
  if(!this->isVisible()) {
    QMainWindow::show();

  }
}

void ConnectDialog::closeEvent (QCloseEvent * event)
{


 QWidget::closeEvent(event);
}

void ConnectDialog::closeinfodlg()
{
	close();
}


void ConnectDialog::setInfo(std::string name, 
				std::string trust,
				std::string org,
				std::string loc,
				std::string country,
				std::string signers)
{
	ui.name->setText(QString::fromStdString(name));
	ui.trust->setText(QString::fromStdString(trust));
	ui.org->setText(QString::fromStdString(org));
	ui.loc->setText(QString::fromStdString(loc));
	ui.country->setText(QString::fromStdString(country));
	ui.signers->setText(QString::fromStdString(signers));
}

		    
void ConnectDialog::setAuthCode(std::string id, std::string code)
{
	authId = id;
	authCode = code;
	ui.Ledit_name->setText(QString::fromStdString(code));
	//ui.okButton ->setEnabled(true);
}

void ConnectDialog::checkAuthCode(const QString &txt)
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

void ConnectDialog::authAttempt()
{

	/* well lets do it ! */
	std::cerr << "Attempting AuthCode:" << authCode << std::endl;
	rsicontrol -> NeighAuthFriend(authId, authCode);
	rsicontrol -> NeighAddFriend(authId);

	/* close it up! */
	closeinfodlg();
}

bool ConnectDialog::loadPeer(std::string id)
{ 
	std::map<RsCertId,NeighbourInfo>::const_iterator it;
	/* first grab the list of signers (outside lock) */

	char signerstr[1024];
	rsicontrol->NeighGetSigners(id, signerstr, 1024);

	/* XXX NOTE We must copy the char * to a string, 
         * otherwise it get trampled on by the it = find(id).
         * ( DONT KNOW WHY???? ) under mingw.
         */

	std::string signersString; 
	signersString = signerstr;

	/* grab the interface and check person */
	rsiface->lockData(); /* Lock Interface */
	
	const std::map<RsCertId,NeighbourInfo> &neighs = 
			rsiface->getNeighbourMap();

	it = neighs.find(id);

	if (it == neighs.end())
	{
		rsiface->unlockData(); /* UnLock Interface */
		return false;
	}

	/* setup the gui */
	setInfo(it->second.name, it->second.trustString, it->second.org,
	it->second.loc, it->second.country, signersString);

	setAuthCode(id, it->second.authCode);
	
	rsiface->unlockData(); /* UnLock Interface */

	return true;
}
	
