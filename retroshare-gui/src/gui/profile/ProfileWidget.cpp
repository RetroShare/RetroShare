/*******************************************************************************
 * retroshare-gui/src/gui/profile/ProfileManager.cpp                           *
 *                                                                             *
 * Copyright (C) 2009 by Retroshare Team     <retroshare.project@gmail.com>    *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include "ProfileWidget.h"

#include <retroshare/rsiface.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsdisc.h>

#include "StatusMessage.h"
#include "ProfileManager.h"
#include "util/DateTime.h"
#include "rshare.h"

#include <QApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QDateTime>

/** Default constructor */
ProfileWidget::ProfileWidget(QWidget *parent, Qt::WindowFlags flags)
  : QWidget(parent, flags)
{
    /* Invoke Qt Designer generated QObject setup routine */
    ui.setupUi(this);

    connect(ui.editstatustoolButton,SIGNAL(clicked()), this, SLOT(statusmessagedlg()));
    connect(ui.CopyCertButton,SIGNAL(clicked()), this, SLOT(copyCert()));
    connect(ui.profile_Button,SIGNAL(clicked()), this, SLOT(profilemanager()));

    ui.onLineSince->setText(DateTime::formatLongDateTime(Rshare::startupTime()));
}

void ProfileWidget::showEvent ( QShowEvent * /*event*/ )
{
    /* set retroshare version */
    ui.version->setText(Rshare::retroshareVersion(true));

    RsPeerDetails detail;
    if (rsPeers->getPeerDetails(rsPeers->getOwnId(),detail))
    {
        ui.name->setText(QString::fromUtf8(detail.name.c_str()));
        ui.country->setText(QString::fromUtf8(detail.location.c_str()));

        ui.peerId->setText(QString::fromStdString(detail.id.toStdString()));

        ui.ipAddressList->clear();
        for(std::list<std::string>::const_iterator it(detail.ipAddressList.begin());it!=detail.ipAddressList.end();++it)
            ui.ipAddressList->addItem(QString::fromStdString(*it));

        /* set local address */
        ui.localAddress->setText(QString::fromStdString(detail.localAddr));
        ui.localPort -> setText(QString::number(detail.localPort));
        /* set the server address */
        ui.extAddress->setText(QString::fromStdString(detail.extAddr));
        ui.extPort -> setText(QString::number(detail.extPort));
        /* set DynDNS */
        ui.dyndns->setText(QString::fromStdString(detail.dyndns));
        ui.dyndns->setCursorPosition (0);

        std::list<RsPgpId> ids;
        ids.clear();
        rsPeers->getGPGAcceptedList(ids);
        int friends = ids.size();

        ui.friendsEdit->setText(QString::number(friends));
    }
}

void ProfileWidget::statusmessagedlg()
{
    StatusMessage statusMsgDialog;
    statusMsgDialog.exec();
}

void ProfileWidget::copyCert()
{
    std::string cert = rsPeers->GetRetroshareInvite(RsPeerId());

    if (cert.empty()) {
        QMessageBox::information(this, tr("RetroShare"),
                         tr("Sorry, create certificate failed"),
                         QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(QString::fromStdString(cert));
                                
    QMessageBox::information(this,
                             tr("RetroShare"),
                             tr("Your Cert is copied to Clipboard, paste and send it to your "
                                "friend via email or some other way"));                               
}

void ProfileWidget::profilemanager()
{
    ProfileManager profilemanager;
    profilemanager.exec();
}
