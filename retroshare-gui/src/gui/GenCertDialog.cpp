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
#include "GenCertDialog.h"
#include "gui/Preferences/rsharesettings.h"
#include <QFileDialog>
#include <QMessageBox>
#include <util/WidgetBackgroundImage.h>

/* Define the format used for displaying the date and time */
#define DATETIME_FMT  "MMM dd hh:mm:ss"



/** Default constructor */
GenCertDialog::GenCertDialog(RsInit *conf, QWidget *parent, Qt::WFlags flags)
  : QDialog(parent, flags), rsConfig(conf)
{
  /* Invoke Qt Designer generated QObject setup routine */
  ui.setupUi(this);

  WidgetBackgroundImage::setBackgroundImage(ui.loginLabel, ":images/new-account.png", WidgetBackgroundImage::AdjustHeight);

  
  connect(ui.genButton, SIGNAL(clicked()), this, SLOT(genPerson()));
  connect(ui.selectButton, SIGNAL(clicked()), this, SLOT(selectFriend()));
  connect(ui.friendBox, SIGNAL(stateChanged(int)), this, SLOT(checkChanged(int)));

  ui.genName->setFocus(Qt::OtherFocusReason);

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
	std::string passwd  = ui.genPasswd->text().toStdString();
	std::string passwd2 = ui.genPasswd2->text().toStdString();
	std::string err;

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

	if ((passwd.length() >= 4) && (passwd == passwd2))
	{
		/* passwd passes basic test */
	}
	else
	{
		/* Message Dialog */
		QMessageBox::StandardButton sb = QMessageBox::warning ( NULL,
	                        "Generate ID Failure",
		        "Your password is too short, or don't match",
			          QMessageBox::Ok);

	        ui.genPasswd->setText("");
	        ui.genPasswd2->setText("");

		return;
	}

	bool okGen = RsGenerateCertificate(rsConfig, genName, genOrg, genLoc, genCountry, passwd, err);

	if (okGen)
	{
		/* complete the process */
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

	/* still need to find home (first) */

	QString fileName = QFileDialog::getOpenFileName(this, tr("Select Trusted Friend"), "",
                                             tr("Certificates (*.pqi *.pem)"));

	std::string fname, userName;
	fname = fileName.toStdString();
	if (ValidateTrustedUser(rsConfig, fname, userName))
	{
		ui.genFriend -> setText(QString::fromStdString(userName));
	}
	else
	{
		ui.genFriend -> setText("<Invalid Selected>");
	}
}


void GenCertDialog::checkChanged(int i)
{
	if (i)
	{
		selectFriend();
	}
	else
	{
		/* invalidate selection */
		std::string fname = "";
		std::string userName = "";
		ValidateTrustedUser(rsConfig, fname, userName);
		ui.genFriend -> setText("<None Selected>");
	}
}


void GenCertDialog::loadCertificates()
{
	bool autoSave = false; 
	/* Final stage of loading */
	if (LoadCertificates(rsConfig, autoSave))
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

