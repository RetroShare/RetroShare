/*******************************************************************************
 * gui/connect/PGPKeyDialog.cpp                                                *
 *                                                                             *
 * Copyright 2006 by Crypton              <retroshare.project@gmail.com>       *
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

#include "gui/common/FilesDefs.h"
#include "PGPKeyDialog.h"

#include <QMessageBox>
#include <QDateTime>
#include <QMenu>
#include <QClipboard>
#include <QMap>

#include <iostream>

#include <retroshare/rspeers.h>
#include <retroshare/rsdisc.h>
#include <retroshare/rsmsgs.h>

#include <retroshare-gui/mainpage.h>

#include "gui/help/browser/helpbrowser.h"
#include "gui/common/PeerDefs.h"
#include "gui/common/StatusDefs.h"
#include "gui/RetroShareLink.h"
#include "gui/notifyqt.h"
#include "gui/common/AvatarDefs.h"
#include "gui/MainWindow.h"
#include "util/DateTime.h"
#include "util/misc.h"

#define FRIEND_OPTIONS_TAB_INDEX 2

QMap<RsPgpId, PGPKeyDialog*> PGPKeyDialog::instances_pgp;

PGPKeyDialog *PGPKeyDialog::instance(const RsPgpId& pgp_id)
{

    PGPKeyDialog *d = instances_pgp[pgp_id];
    if (d) {
        return d;
    }

    d = new PGPKeyDialog(RsPeerId(),pgp_id);
    instances_pgp[pgp_id] = d;

    return d;
}
/** Default constructor */
PGPKeyDialog::PGPKeyDialog(const RsPeerId& id, const RsPgpId &pgp_id, QWidget *parent, Qt::WindowFlags flags)
  : QDialog(parent, flags), peerId(id), pgpId(pgp_id)
{
    /* Invoke Qt Designer generated QObject setup routine */
    ui.setupUi(this);
	Settings->loadWidgetInformation(this);
//	 if(id.isNull())
//		 ui._useOldFormat_CB->setChecked(true) ;
//	 else
//	 {
//		 ui._useOldFormat_CB->setChecked(false) ;
//		 ui._useOldFormat_CB->setEnabled(false) ;
//	 }

    ui.headerFrame->setHeaderImage(FilesDefs::getPixmapFromQtResourcePath(":/images/user/identityinfo64.png"));
    ui.headerFrame->setHeaderText(tr("Retroshare profile"));

    //ui._chat_CB->hide() ;

	setAttribute(Qt::WA_DeleteOnClose, true);

    connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(applyDialog()));
    connect(ui.buttonBox, SIGNAL(rejected()), this, SLOT(close()));
    connect(ui.make_friend_button, SIGNAL(clicked()), this, SLOT(makeFriend()));
    connect(ui.denyFriendButton, SIGNAL(clicked()), this, SLOT(denyFriend()));
    connect(ui.signKeyButton, SIGNAL(clicked()), this, SLOT(signGPGKey()));
    connect(ui._shouldAddSignatures_CB_2, SIGNAL(toggled(bool)), this, SLOT(loadKeyPage()));

    //ui.avatar->setFrameType(AvatarWidget::NORMAL_FRAME);

    MainWindow *w = MainWindow::getInstance();
    if (w) {
        connect(this, SIGNAL(configChanged()), w->getPage(MainWindow::Network), SLOT(insertConnect()));
    }
}

PGPKeyDialog::~PGPKeyDialog()
{
	Settings->saveWidgetInformation(this);
        QMap<RsPgpId, PGPKeyDialog*>::iterator it = instances_pgp.find(pgpId);
        if (it != instances_pgp.end())
            instances_pgp.erase(it);
}


//void PGPKeyDialog::setServiceFlags()
//{
//    ServicePermissionFlags flags(0) ;
//
//    if(  ui._direct_transfer_CB->isChecked()) flags = flags | RS_NODE_PERM_DIRECT_DL ;
//    if(  ui._allow_push_CB->isChecked()) flags = flags | RS_NODE_PERM_ALLOW_PUSH ;
//
//    rsPeers->setServicePermissionFlags(pgpId,flags) ;
//}

void PGPKeyDialog::loadAll()
{
    for(QMap<RsPgpId , PGPKeyDialog*>::iterator it = instances_pgp.begin(); it != instances_pgp.end(); ++it)  it.value()->load();
}

void PGPKeyDialog::load()
{
    RsPeerDetails detail;

    if(!rsPeers->getGPGDetails(pgpId, detail))
    {
        QMessageBox::information(this,
                                 tr("RetroShare"),
                                 tr("Error : cannot get peer details."));
        close();
        return;
    }

    if(!rsPeers->isKeySupported(pgpId))
    {
        ui.make_friend_button->setEnabled(false) ;
        ui.make_friend_button->setToolTip(tr("The supplied key algorithm is not supported by RetroShare\n(Only RSA keys are supported at the moment)")) ;
    }
    else
    {
        ui.make_friend_button->setEnabled(true) ;
        ui.make_friend_button->setToolTip("") ;
    }

    ui.name->setText(QString::fromUtf8(detail.name.c_str()));
    ui.pgpfingerprint->setText(misc::fingerPrintStyleSplit(QString::fromStdString(detail.fpr.toStdString())));

    ui._direct_transfer_CB->setChecked(  detail.service_perm_flags & RS_NODE_PERM_DIRECT_DL ) ;
	//Add warning to direct source checkbox depends general setting.
	switch (rsFiles->filePermDirectDL())
	{
		case RS_FILE_PERM_DIRECT_DL_YES:
            ui._direct_transfer_CB->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/warning_yellow_128.png"));
			ui._direct_transfer_CB->setToolTip(ui._direct_transfer_CB->toolTip().append(tr("\nWarning: In your File-Transfer option, you select allow direct download to Yes.")));
			break ;
		case RS_FILE_PERM_DIRECT_DL_NO:
            ui._direct_transfer_CB->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/warning_yellow_128.png"));
			ui._direct_transfer_CB->setToolTip(ui._direct_transfer_CB->toolTip().append(tr("\nWarning: In your File-Transfer option, you select allow direct download to No.")));
			break ;

		default: break ;
	}
    ui._allow_push_CB->setChecked(  detail.service_perm_flags & RS_NODE_PERM_ALLOW_PUSH) ;
    ui._require_WL_CB->setChecked(  detail.service_perm_flags & RS_NODE_PERM_REQUIRE_WL) ;

    uint32_t max_upload_speed = 0 ;
    uint32_t max_download_speed = 0 ;
    
    rsPeers->getPeerMaximumRates(pgpId,max_upload_speed,max_download_speed);
    
    ui.maxUploadSpeed_SB->setValue(max_upload_speed) ;
    ui.maxDownloadSpeed_SB->setValue(max_download_speed) ;
    
    if (detail.gpg_id == rsPeers->getGPGOwnId())
    {
        ui.make_friend_button->hide();
        ui.signKeyButton->hide();
        ui.label_signKey->hide();
        ui.denyFriendButton->hide();

        ui.label_trustlevel->hide();
        ui.trustlevel_CB->hide();

        ui.is_signing_me->hide();
        ui.signersLabel->setText(tr("This is your own PGP key, and it is signed by :")+" ");

    }
    else
    {
        ui.label_trustlevel->show();
        ui.trustlevel_CB->show();
        ui.is_signing_me->show();
        ui.signersLabel->setText(tr("This key is signed by :")+" ");
#ifdef USE_OPENPGPSDK
        ui.signKeyButton->setEnabled(!detail.ownsign);
#else
        ui.signKeyButton->setEnabled(false);
        ui.signKeyButton->setToolTip("Disabled because key signing is not yet implemented in RNP lib");
#endif

        if (detail.accept_connection)
        {
            ui.make_friend_button->hide();
            ui.denyFriendButton->show();
            ui.stabWidget->setTabEnabled(FRIEND_OPTIONS_TAB_INDEX,true);
        }
        else
        {
            ui.make_friend_button->show();
            ui.denyFriendButton->hide();
            ui.stabWidget->setTabEnabled(FRIEND_OPTIONS_TAB_INDEX,false);
        }

        //web of trust

        ui.trustlevel_CB->setCurrentIndex(detail.trustLvl) ;


        QString truststring = "<p>" ;
        truststring += tr("The trust level is a way to express your own trust in this key. It is not used by the software nor shared, but can be useful to you in order to remember good/bad keys.") ;
        truststring += "</p>" ;
        truststring += "<p>" ;
        switch(detail.trustLvl)
        {
        case  RS_TRUST_LVL_ULTIMATE:
            //trust is ultimate, it means it's one of our own keys
            truststring += tr("Your trust in this peer is ultimate");
            break ;
        case RS_TRUST_LVL_FULL:
            truststring += tr("Your trust in this peer is full.");
            break ;
        case RS_TRUST_LVL_MARGINAL:
            truststring += tr("Your trust in this peer is marginal.");
            break ;
        case RS_TRUST_LVL_NEVER:
            truststring += tr("Your trust in this peer is none.");
            break ;

        default:
            truststring += tr("You haven't set a trust level for this key.");
            break ;
        }
        truststring += "</p>" ;
        ui.trustlevel_CB->setToolTip(truststring) ;

        if (detail.hasSignedMe) {
            ui.is_signing_me->setText(tr("This key has signed your own PGP key"));
            ui.is_signing_me->show() ;
        } else {
            ui.is_signing_me->hide() ;
        }
    }

    QString text;
    RetroShareLink link ;

    for(std::list<RsPgpId>::const_iterator it(detail.gpgSigners.begin());it!=detail.gpgSigners.end();++it) {
        link = RetroShareLink::createPerson(*it);
        if (link.valid()) {
            text += link.toHtml() + "<BR>";
        }
    }
    ui.signers->setHtml(text);

    loadKeyPage() ;
}

void PGPKeyDialog::loadKeyPage()
{
    RsPeerDetails detail;

    if (!rsPeers->getGPGDetails(pgpId,detail))
    {
        QMessageBox::information(this,
                                 tr("RetroShare"),
                                 tr("Error : cannot get peer details."));
        close();
        return;
    }
    ui._shouldAddSignatures_CB_2->setEnabled(detail.gpgSigners.size() > 1) ;

     std::string pgp_key = rsPeers->getPGPKey(detail.gpg_id,ui._shouldAddSignatures_CB_2->isChecked()) ; // this needs to be a SSL id

    ui.userCertificateText->setReadOnly(true);
    ui.userCertificateText->setMinimumHeight(200);
    ui.userCertificateText->setMinimumWidth(530);
    QFont font("Courier New",10,50,false);
    font.setStyleHint(QFont::TypeWriter,QFont::PreferMatch);
    font.setStyle(QFont::StyleNormal);
    ui.userCertificateText->setFont(font);
    ui.userCertificateText->setText(QString::fromUtf8(pgp_key.c_str()));

    QString helptext ;
    helptext += tr("<p>This PGP key (ID=")+detail.gpg_id.toStdString().c_str()+")" + " authenticates one or more retroshare nodes.</p> ";

    helptext += "<p>" ;
    if(detail.accept_connection)
        helptext += tr("You have chosen to accept connections from Retroshare nodes signed by this key.") ;
    else
        helptext += tr("You are currently not allowing connections from Retroshare nodes signed by this key.") ;

    helptext += "</p>" ;

    ui.userCertificateText->setToolTip(helptext) ;
}

void PGPKeyDialog::applyDialog()
{
    std::cerr << "PGPKeyDialog::applyDialog() called" << std::endl ;
    RsPeerDetails detail;
    if (!rsPeers->getPeerDetails(peerId, detail))
    {
        if (!rsPeers->getGPGDetails(pgpId, detail)) {
            QMessageBox::information(this,
                                     tr("RetroShare"),
                                     tr("Error : cannot get peer details."));
            close();
            return;
        }
    }

    //check the GPG trustlvl
    if(ui.trustlevel_CB->currentIndex() != (int)detail.trustLvl)
        rsPeers->trustGPGCertificate(pgpId, ui.trustlevel_CB->currentIndex());

    uint32_t max_upload_speed = ui.maxUploadSpeed_SB->value() ;
    uint32_t max_download_speed = ui.maxDownloadSpeed_SB->value();

    rsPeers->setPeerMaximumRates(pgpId,max_upload_speed,max_download_speed);

    ServicePermissionFlags flags(0) ;

    if(  ui._direct_transfer_CB->isChecked()) flags = flags | RS_NODE_PERM_DIRECT_DL ;
    if(  ui._allow_push_CB->isChecked()) flags = flags | RS_NODE_PERM_ALLOW_PUSH ;
    if(  ui._require_WL_CB->isChecked()) flags = flags | RS_NODE_PERM_REQUIRE_WL ;

    rsPeers->setServicePermissionFlags(pgpId,flags) ;
    
     //setServiceFlags() ;

    loadAll();
    close();
}

void PGPKeyDialog::makeFriend()
{
    rsPeers->addFriend(peerId, pgpId);
    loadAll();

    emit configChanged();
}

void PGPKeyDialog::denyFriend()
{
    rsPeers->removeFriend(pgpId);
    loadAll();

    emit configChanged();
}

void PGPKeyDialog::signGPGKey()
{
    std::string gpg_name = rsPeers->getGPGName(rsPeers->getGPGOwnId());
    bool cancelled;
    std::string gpg_password;

    if(!NotifyQt::getInstance()->askForPassword(tr("Profile password needed.").toStdString(), gpg_name + " (" + rsPeers->getOwnId().toStdString() + ")", false, gpg_password,cancelled))
    {
        QMessageBox::critical(NULL,tr("Identity creation failed"),tr("Cannot create an identity linked to your profile without your profile password."));
        return;
    }

    rsNotify->clearPgpPassphrase(); // just in case

    if(!rsPeers->signGPGCertificate(pgpId,gpg_password))
        QMessageBox::warning ( NULL, tr("Signature Failure"), tr("Check the password!"), QMessageBox::Ok);

    loadAll();

    emit configChanged();
}

/** Displays the help browser and displays the most recently viewed help
 * topic. */
void PGPKeyDialog::showHelpDialog()
{
    showHelpDialog("trust");
}

/**< Shows the help browser and displays the given help <b>topic</b>. */
void PGPKeyDialog::showHelpDialog(const QString &topic)
{
    HelpBrowser::showWindow(topic);
}
