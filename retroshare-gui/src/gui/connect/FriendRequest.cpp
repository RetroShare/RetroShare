/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2012,  RetroShare Team
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

#include "FriendRequest.h"

#include <QMessageBox>
#include <QDateTime>
#include <QMenu>
#include <QClipboard>
#include <QMap>

#include <iostream>

#include <retroshare/rspeers.h>
#include <retroshare/rsdisc.h>
#include <retroshare/rsmsgs.h>

#include "gui/help/browser/helpbrowser.h"
#include "gui/common/PeerDefs.h"
#include "gui/common/StatusDefs.h"
#include "gui/RetroShareLink.h"
#include "gui/notifyqt.h"
#include "gui/common/AvatarDefs.h"



/* Define the format used for displaying the date and time */
#define DATETIME_FMT  "MMM dd hh:mm:ss"

/** Default constructor */
FriendRequest::FriendRequest(const std::string& id, QWidget *parent, Qt::WFlags flags)
  : QDialog(parent, flags), mId(id)
{
    /* Invoke Qt Designer generated QObject setup routine */
    ui.setupUi(this);

	ui.headerFrame->setHeaderImage(QPixmap(":/images/user/user_request48.png"));
	ui.headerFrame->setHeaderText(tr("Friend Request"));

	setAttribute(Qt::WA_DeleteOnClose, true);

    connect(ui.applyButton, SIGNAL(clicked()), this, SLOT(applyDialog()));
    connect(ui.cancelButton, SIGNAL(clicked()), this, SLOT(close()));

    load();
}

/** Destructor. */
FriendRequest::~FriendRequest()
{
}

void FriendRequest::load()
{
    RsPeerDetails detail;
    if (!rsPeers->getPeerDetails(mId, detail))
    {
        QMessageBox::information(this,
                                 tr("RetroShare"),
                                 tr("Error : cannot get peer details."));
        close();
        return;
    }

    ui.name->setText(QString::fromUtf8(detail.name.c_str()));
    ui.peerid->setText(QString::fromStdString(detail.id));

    ui.avatar->setId(mId, false);

}

void FriendRequest::applyDialog()
{
    std::cerr << "FriendRequest::applyDialog() called" << std::endl ;
    RsPeerDetails detail;
    if (!rsPeers->getPeerDetails(mId, detail))
    {
        if (!rsPeers->getGPGDetails(mId, detail)) {
            QMessageBox::information(this,
                                     tr("RetroShare"),
                                     tr("Error : cannot get peer details."));
            close();
            return;
        }
    }

    makeFriend();
    close();
}

void FriendRequest::makeFriend()
{
    std::string gpg_id = rsPeers->getGPGId(mId);
    if (ui.signGPGKeyCheckBox->isChecked()) {
        rsPeers->signGPGCertificate(gpg_id);
    } 
	
    rsPeers->addFriend(mId, gpg_id);

    emit configChanged();
}

void FriendRequest::denyFriend()
{
    std::string gpg_id = rsPeers->getGPGId(mId);
    rsPeers->removeFriend(gpg_id);

    emit configChanged();
}

void FriendRequest::signGPGKey()
{
    std::string gpg_id = rsPeers->getGPGId(mId);
    if (!rsPeers->signGPGCertificate(gpg_id)) {
                 QMessageBox::warning ( NULL,
                                tr("Signature Failure"),
                                tr("Maybe password is wrong"),
                                QMessageBox::Ok);
    }

    emit configChanged();
}

