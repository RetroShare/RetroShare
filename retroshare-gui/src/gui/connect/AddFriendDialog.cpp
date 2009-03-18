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

//============================================================================
/** Default constructor */
AddFriendDialog::AddFriendDialog(NetworkDialog *cd, QWidget *parent, Qt::WFlags flags)
  : QDialog(parent, flags), cDialog(cd)
{
  /* Invoke Qt Designer generated QObject setup routine */
  ui.setupUi(this);
  
  /* add a Background image for Add Friend Label */
  //WidgetBackgroundImage::setBackgroundImage(ui.addfriendLabel, ":images/new-contact.png", WidgetBackgroundImage::AdjustHeight);

  connect(ui.afcancelButton, SIGNAL(clicked()), this, SLOT(afcancelbutton()));
  connect(ui.fileButton, SIGNAL(clicked()), this, SLOT(filebutton()));
  connect(ui.doneButton, SIGNAL(clicked()), this, SLOT(donebutton()));

  //maybe, it was already set somewere, but just in case. This settings should
  //prevent some bugs...
  ui.emailText->setLineWrapMode(QTextEdit::NoWrap);
  ui.emailText->setAcceptRichText(false);
  
  //setFixedSize(QSize(434, 462));
}

//============================================================================

void AddFriendDialog::donebutton()
{
    std::string id;
    std::string certstr;

    QString fn = ui.fileSelectEdit->text() ;
    if (fn.isEmpty())
    {
        //load certificate from text box
        certstr = ui.emailText->toPlainText().toStdString();

        if ((cDialog) && (rsPeers->LoadCertificateFromString(certstr, id)))
        {
            close();
            cDialog->showpeerdetails(id);
        }
        else
        {
            /* error message */
            QMessageBox::warning(this, tr("RetroShare"),
                    tr("Certificate Load Failed"),
                    QMessageBox::Ok, QMessageBox::Ok);

            close();
            return;
        }
    }
    else
    {
        //=== try to load selected certificate file
        if (QFile::exists(fn))
        {
            std::string fnstr = fn.toStdString();
            if ( (cDialog) && (rsPeers->LoadCertificateFromFile(fnstr, id)) )
            {
                close();
                cDialog->showpeerdetails(id);
            }
            else
            {
                QString mbxmess =
                    QString(tr("Certificate Load Failed:something is wrong with %1 "))
                           .arg(fn);

                QMessageBox::warning(this, tr("RetroShare"),
                                mbxmess, QMessageBox::Ok, QMessageBox::Ok);
                close();
                return;
            }
        }
        else
        {
           QString mbxmess =
               QString(tr("Certificate Load Failed:file %1 not found"))
                      .arg(fn);
                      
           QMessageBox::warning(this, tr("RetroShare"),
                                mbxmess, QMessageBox::Ok, QMessageBox::Ok);

           close();
           return;
        }
    }
}

//============================================================================

void AddFriendDialog::afcancelbutton()
{
	close();
}

//============================================================================

void AddFriendDialog::filebutton()
{
    QString fileName =
        QFileDialog::getOpenFileName(this, tr("Select Certificate"),
                                     "", tr("Certificates (*.pqi *.pem)"));

    if (!fileName.isNull())
        ui.fileSelectEdit->setText(fileName);
}

//============================================================================

void AddFriendDialog::setInfo(std::string invite)
{
	ui.emailText->setText(QString::fromStdString(invite));
}

		     
