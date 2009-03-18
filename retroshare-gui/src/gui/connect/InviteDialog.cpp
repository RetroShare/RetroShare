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
 
#include "InviteDialog.h"

#include "rsiface/rsiface.h"
#include "rsiface/rspeers.h"
//#include <util/WidgetBackgroundImage.h>

#include <QMessageBox>

/** Default constructor */
InviteDialog::InviteDialog(QWidget *parent, Qt::WFlags flags)
  : QDialog(parent, flags)
{
  /* Invoke Qt Designer generated QObject setup routine */
  ui.setupUi(this);
  
  /* add a Background image for Invite a Friend Label */
  //WidgetBackgroundImage::setBackgroundImage(ui.invitefriendLabel, ":images/new-contact.png", WidgetBackgroundImage::AdjustHeight);

  connect(ui.sCertButton, SIGNAL(clicked()), this, SLOT(savecertbutton()));
 
  //setFixedSize(QSize(434, 462));
}

void InviteDialog::setInfo(std::string invite)
{
//	ui.emailText->setCurrentFont(QFont("TypeWriter",10)) ;
//	ui.emailText->currentFont().setLetterSpacing(QFont::AbsoluteSpacing,1) ;
//	ui.emailText->currentFont().setStyleHint(QFont::TypeWriter,QFont::Courier) ;
	ui.emailText->setText(QString::fromStdString(invite));
}

void InviteDialog::savecertbutton(void)
{
    QString qdir = QFileDialog::getSaveFileName(this,
                                                "Please choose a filename",
                                                QDir::homePath(),
                                                "RetroShare Certificate (*.pqi)");
                                                
    if ( rsPeers->SaveCertificateToFile(rsPeers->getOwnId(), qdir.toStdString()) )
    {
        QMessageBox::information(this, tr("RetroShare"),
                         tr("Certificate file successfully created"),
                         QMessageBox::Ok, QMessageBox::Ok);
         //close the window after messagebox finished
        close();
    }
    else
    {
        QMessageBox::information(this, tr("RetroShare"),
                         tr("Sorry, certificate file creation failed"),
                         QMessageBox::Ok, QMessageBox::Ok);
    }
}
		     
