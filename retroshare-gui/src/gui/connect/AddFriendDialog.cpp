/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006, 2007 crypton
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
#include "AddFriendDialog.h"

#include "rsiface/rsiface.h"
#include "rsiface/rspeers.h"

#include "gui/NetworkDialog.h"
#include <util/WidgetBackgroundImage.h>

#include <QMessageBox>

/** Default constructor */
AddFriendDialog::AddFriendDialog(NetworkDialog *cd, QWidget *parent, Qt::WFlags flags)
  : QDialog(parent, flags), cDialog(cd)
{
  /* Invoke Qt Designer generated QObject setup routine */
  ui.setupUi(this);
  
  /* add a Background image for Add Friend Label */
  WidgetBackgroundImage::setBackgroundImage(ui.addfriendLabel, ":images/new-contact.png", WidgetBackgroundImage::AdjustHeight);

  connect(ui.afcancelButton, SIGNAL(clicked()), this, SLOT(afcancelbutton()));
  connect(ui.fileButton, SIGNAL(clicked()), this, SLOT(filebutton()));
  connect(ui.doneButton, SIGNAL(clicked()), this, SLOT(donebutton()));

  //setFixedSize(QSize(434, 462));
}

void AddFriendDialog::donebutton()
{
	/* something complicated ;) */
        std::string id;

	/* get the text from the window */
	/* load into string */
	std::string certstr  = ui.emailText->toPlainText().toStdString();

	/* ask retroshare to load */
	if ((cDialog) && (rsPeers->LoadCertificateFromString(certstr, id)))
	{
		close();
		cDialog->showpeerdetails(id);
	}
	else
	{
		/* error message */
		int ret = QMessageBox::warning(this, tr("RetroShare"),
                   tr("Certificate Load Failed"),
                   QMessageBox::Ok, QMessageBox::Ok);
	}
}


void AddFriendDialog::afcancelbutton()
{
	close();
}


void AddFriendDialog::filebutton()
{

	/* show file dialog, 
	 * load file into screen, 
	 * push done button!
	 */
        std::string id;
	if (cDialog)
	{
		id = cDialog->loadneighbour();
	}
					  
	/* call make Friend */
	if (id != "")
	{
		close();
		cDialog->showpeerdetails(id);
	}
	else
	{
		/* error message */
		int ret = QMessageBox::warning(this, tr("RetroShare"),
                   tr("Certificate Load Failed"),
                   QMessageBox::Ok, QMessageBox::Ok);
	}
}


void AddFriendDialog::setInfo(std::string invite)
{
	ui.emailText->setText(QString::fromStdString(invite));
}

		     
