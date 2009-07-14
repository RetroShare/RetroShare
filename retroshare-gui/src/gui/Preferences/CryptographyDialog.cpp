/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006 - 2009 RetroShare Team
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

#include "rsiface/rspeers.h" //for rsPeers variable
#include "rsiface/rsiface.h"

#include <QtGui>
#include <QClipboard>

#include <rshare.h>
#include "CryptographyDialog.h"

#include <sstream>
#include <iostream>
#include <set>

/** Constructor */
CryptographyDialog::CryptographyDialog(QWidget *parent)
: ConfigPage(parent)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

  /* Create RshareSettings object */
  _settings = new RshareSettings();

  connect(ui.copykeyButton, SIGNAL(clicked()), this, SLOT(copyPublicKey()));
  connect(ui.exportkeyButton, SIGNAL(clicked()), this, SLOT(exportPublicKey()));


  loadPublicKey();


  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

/** Saves the changes on this page */
bool
CryptographyDialog::save(QString &errmsg)
{
 	return true;
}

/** Loads the settings for this page */
void
CryptographyDialog::load()
{

}

/** Loads ouer default Puplickey  */
void
CryptographyDialog::loadPublicKey()
{
    //std::cerr << "CryptoPage() getting Invite" << std::endl;

    std::string invite = rsPeers->GetRetroshareInvite();

    RsPeerDetails ownDetail;
    rsPeers->getPeerDetails(rsPeers->getOwnId(), ownDetail);
    invite += LOCAL_IP;
    invite += ownDetail.localAddr + ":";
    std::ostringstream out;
    out << ownDetail.localPort;
    invite += out.str() + ";";
    invite += "\n";
    invite += EXT_IP;
    invite += ownDetail.extAddr + ":";
    std::ostringstream out2;
    out2 << ownDetail.extPort;
    invite += out2.str() + ";";

    ui.certtextEdit->setText(QString::fromStdString(invite));
    ui.certtextEdit->setReadOnly(true);
    ui.certtextEdit->setMinimumHeight(200);

    //std::cerr << "CryptoPage() getting Invite: " << invite << std::endl;

}

void
CryptographyDialog::copyPublicKey()
{
    QMessageBox::information(this,
                             tr("RetroShare"),
                             tr("Your Public Key is copied to Clipbard, paste and send it to your"
                                "friend via email or some other way"));
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(ui.certtextEdit->toPlainText());                            

}

void
CryptographyDialog::exportPublicKey()
{
    qDebug() << "  exportPulicKey";

    QString qdir = QFileDialog::getSaveFileName(this,
                                                "Please choose a filename",
                                                QDir::homePath(),
                                                "RetroShare Certificate (*.pqi)");

    if ( rsPeers->SaveCertificateToFile(rsPeers->getOwnId(), qdir.toStdString()) )
    {
        QMessageBox::information(this, tr("RetroShare"),
                         tr("Certificate file successfully created"),
                         QMessageBox::Ok, QMessageBox::Ok);
    }
    else
    {
        QMessageBox::information(this, tr("RetroShare"),
                         tr("Sorry, certificate file creation failed"),
                         QMessageBox::Ok, QMessageBox::Ok);
    }
}
