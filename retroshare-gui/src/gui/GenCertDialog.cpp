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


#include <rshare.h>
#include <rsiface/rsinit.h>
#include "GenCertDialog.h"
#include "gui/Preferences/rsharesettings.h"
#include <QFileDialog>
#include <QMessageBox>
#include <util/WidgetBackgroundImage.h>

/* Define the format used for displaying the date and time */
#define DATETIME_FMT  "MMM dd hh:mm:ss"


/** Default constructor */
GenCertDialog::GenCertDialog(QWidget *parent, Qt::WFlags flags)
  : QDialog(parent, flags)
{
  /* Invoke Qt Designer generated QObject setup routine */
  ui.setupUi(this);

  WidgetBackgroundImage::setBackgroundImage(ui.loginLabel, ":images/new-account.png", WidgetBackgroundImage::AdjustHeight);

  
  connect(ui.genButton, SIGNAL(clicked()), this, SLOT(genPerson()));
  //connect(ui.selectButton, SIGNAL(clicked()), this, SLOT(selectFriend()));
  //connect(ui.friendBox, SIGNAL(stateChanged(int)), this, SLOT(checkChanged(int)));

  ui.genName->setFocus(Qt::OtherFocusReason);

#ifdef RS_USE_PGPSSL
        /* get all available pgp private certificates....
         * mark last one as default.
         */
	std::cerr << "Finding PGPUsers" << std::endl;

        std::list<std::string> pgpIds;
        std::list<std::string>::iterator it;
        if (RsInit::GetPGPLogins(pgpIds))
        {
                for(it = pgpIds.begin(); it != pgpIds.end(); it++)
                {
                        const QVariant & userData = QVariant(QString::fromStdString(*it));
                        std::string name, email;
                        RsInit::GetPGPLoginDetails(*it, name, email);
			std::cerr << "Adding PGPUser: " << name << " id: " << *it << std::endl;
                        ui.genPGPuser->addItem(QString::fromStdString(name), userData);
                }
        }
#endif


}

/** Destructor. */
//GenCertDialog::~GenCertDialog()
//{
//}


/** 
 Overloads the default show() slot so we can set opacity*/

void GenCertDialog::show()
{
  //loadSettings();
  if(!this->isVisible()) {
    QWidget::show();

  }
}

void GenCertDialog::closeEvent (QCloseEvent * event)
{


 QDialog::closeEvent(event);
}

void GenCertDialog::closeinfodlg()
{
	close();
}

void GenCertDialog::genPerson()
{

	/* Check the data from the GUI. */
	std::string genName = ui.genName->text().toStdString();
	std::string genOrg  = ui.genOrg->text().toStdString();
	std::string genLoc  = ui.genLoc->text().toStdString();
	std::string genCountry = ui.genCountry->text().toStdString();
	std::string err;


#ifdef RS_USE_PGPSSL

	std::string PGPpasswd  = ui.genPGPpassword->text().toStdString();
	int pgpidx = ui.genPGPuser->currentIndex();
	if (pgpidx < 0)
	{
		/* Message Dialog */
		QMessageBox::StandardButton sb = QMessageBox::warning ( NULL,
	                        "Generate ID Failure",
			        "Missing PGP Certificate",
			          QMessageBox::Ok);
		return;
	}

	QVariant data = ui.genPGPuser->itemData(pgpidx);
	std::string PGPId = (data.toString()).toStdString();

#endif


	if (genName.length() >= 3)
	{
		/* name passes basic test */
	}
	else
	{
		/* Message Dialog */
		QMessageBox::StandardButton sb = QMessageBox::warning ( NULL,
	                        "Generate ID Failure",
			        "Your Name is too short (3+ characters)",
			          QMessageBox::Ok);
		return;
	}

	//generate a random ssl password
	std::cerr << " generating sslPasswd." << std::endl;
	qsrand(time(NULL));
	std::string sslPasswd = "";
	for( int i = 0 ; i < 6 ; ++i )
	{
	    int iNumber;
	    iNumber = qrand()%25 + 65;
	    sslPasswd += (char)iNumber;
	}

	/* Initialise the PGP user first */
	RsInit::SelectGPGAccount(PGPId);
	RsInit::LoadGPGPassword(PGPpasswd);

	std::string sslId;
	bool okGen = RsInit::GenerateSSLCertificate(genName, genOrg, genLoc, genCountry, sslPasswd, sslId, err);

	if (okGen)
	{
		/* complete the process */
		RsInit::LoadPassword(sslId, sslPasswd);
		loadCertificates();
	}
	else
	{
		/* Message Dialog */
		QMessageBox::StandardButton sb = QMessageBox::warning ( NULL,
	                        "Generate ID Failure",
			        "Failed to Generate your new Certificate!",
			          QMessageBox::Ok);
	}
}





void GenCertDialog::selectFriend()
{

#if 0
	/* still need to find home (first) */

	QString fileName = QFileDialog::getOpenFileName(this, tr("Select Trusted Friend"), "",
                                             tr("Certificates (*.pqi *.pem)"));

	std::string fname, userName;
	fname = fileName.toStdString();
	if (RsInit::ValidateTrustedUser(fname, userName))
	{
		ui.genFriend -> setText(QString::fromStdString(userName));
	}
	else
	{
		ui.genFriend -> setText("<Invalid Selected>");
	}
#endif

}


void GenCertDialog::checkChanged(int i)
{

#if 0
	if (i)
	{
		selectFriend();
	}
	else
	{
		/* invalidate selection */
		std::string fname = "";
		std::string userName = "";
		RsInit::ValidateTrustedUser(fname, userName);
		ui.genFriend -> setText("<None Selected>");
	}
#endif

}


void GenCertDialog::loadCertificates()
{
	bool autoSave = false; 
	/* Final stage of loading */
	if (RsInit::LoadCertificates(autoSave))
	{
		close();
	}
	else
	{
		/* some error msg */
		QMessageBox::StandardButton sb = QMessageBox::warning ( NULL,
	                        "Generate ID Failure",
			        "Failed to Load your new Certificate!",
			          QMessageBox::Ok);
	}
}

