/*******************************************************************************
 * gui/settings/CryptoPage.cpp                                                 *
 *                                                                             *
 * Copyright 2006, Retroshare Team <retroshare.project@gmail.com>              *
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

#include <QDate>
#include <QDomDocument>
#include <QMessageBox>
#include <QClipboard>
#include <QFile>
#include <QTextStream>
#include <QTextCodec>

#include "rshare.h"
#include "CryptoPage.h"
#include "util/misc.h"
#include "util/DateTime.h"
#include "retroshare/rsinit.h"
#include <gui/RetroShareLink.h>
#include <gui/connect/ConfCertDialog.h>
#include <gui/profile/ProfileManager.h>
#include <gui/statistics/StatisticsWindow.h>

#include <retroshare/rspeers.h> //for rsPeers variable
#include <retroshare/rsdisc.h> //for rsPeers variable

/** Constructor */
CryptoPage::CryptoPage(QWidget * parent, Qt::WindowFlags flags)
    : ConfigPage(parent, flags)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);

	ui._shortFormat_CB->setChecked(true);

	//  connect(ui.copykeyButton, SIGNAL(clicked()), this, SLOT(copyPublicKey()));
	connect(ui.saveButton, SIGNAL(clicked()), this, SLOT(fileSaveAs()));
	connect(ui._includeSignatures_CB, SIGNAL(toggled(bool)), this, SLOT(load()));
	connect(ui._shortFormat_CB, SIGNAL(toggled(bool)), this, SLOT(load()));
	connect(ui._includeAllIPs_CB, SIGNAL(toggled(bool)), this, SLOT(load()));
	connect(ui._copyLink_PB, SIGNAL(clicked()), this, SLOT(copyRSLink()));
	connect(ui.showStats_PB, SIGNAL(clicked()), this, SLOT(showStats()));

	// hide profile manager as it causes bugs when generating a new profile.
	//ui.profile_Button->hide() ;

    //connect(ui.exportprofile,SIGNAL(clicked()), this, SLOT(profilemanager()));
    connect(ui.exportprofile,SIGNAL(clicked()), this, SLOT(exportProfile()));
    connect(ui.exportfriendslist,SIGNAL(clicked()), this, SLOT(exportFriendlistClicked()));

    // Remove this because it duplicates functionality of the HomePage.
    ui.retroshareId_LB->hide();
    ui.retroshareId_content_LB->hide();
    ui.stackPageCertificate->hide();

	ui.onlinesince->setText(DateTime::formatLongDateTime(Rshare::startupTime()));
}

#ifdef UNUSED_CODE
void CryptoPage::profilemanager()
{
    ProfileManager().exec();
}
#endif

void CryptoPage::exportProfile()
{
    RsPgpId gpgId(rsPeers->getGPGOwnId());

    QString fname = QFileDialog::getSaveFileName(this, tr("Export Identity"), "", tr("RetroShare Identity files (*.asc)"));

    if (fname.isNull())
        return;

    if (fname.right(4).toUpper() != ".ASC") fname += ".asc";

    if (RsAccounts::ExportIdentity(fname.toUtf8().constData(), gpgId))
        QMessageBox::information(this, tr("Identity saved"), tr("Your identity was successfully saved\nIt is encrypted\n\nYou can now copy it to another computer\nand use the import button to load it"));
    else
        QMessageBox::information(this, tr("Identity not saved"), tr("Your identity was not saved. An error occurred."));
}


void CryptoPage::showEvent ( QShowEvent * /*event*/ )
{
    RsPeerDetails detail;
    if (rsPeers->getPeerDetails(rsPeers->getOwnId(),detail))
    {
        ui.name->setText(QString::fromUtf8(detail.name.c_str()));
        ui.country->setText(QString::fromUtf8(detail.location.c_str()));

        ui.peerid->setText(QString::fromStdString(detail.id.toStdString()));
        ui.pgpid->setText(QString::fromStdString(detail.gpg_id.toStdString()));
        ui.pgpfingerprint->setText(misc::fingerPrintStyleSplit(QString::fromStdString(detail.fpr.toStdString())));

        std::string invite ;
        rsPeers->getShortInvite(invite,rsPeers->getOwnId(),RetroshareInviteFlags::RADIX_FORMAT | RsPeers::defaultCertificateFlags);
        ui.retroshareId_content_LB->setText(QString::fromUtf8(invite.c_str()));
		
        /* set retroshare version */
        ui.version->setText(Rshare::retroshareVersion(true));

        std::list<RsPgpId> ids;
        ids.clear();
        rsPeers->getGPGAcceptedList(ids);
        int friends = ids.size();

        ui.friendsEdit->setText(QString::number(friends));
		
		
		QString string ;
		string = rsFiles->getPartialsDirectory().c_str();
		QString datadir = string;
			if(datadir.contains("Partials"))
			{
				datadir.replace("Partials","");
			}
				ui.labelpath->setText(datadir);
    }
	 load() ;
}


CryptoPage::~CryptoPage()
{
}

/** Loads the settings for this page */
void
CryptoPage::load()
{
    std::string cert ;
    RetroshareInviteFlags flags = RetroshareInviteFlags::DNS | RetroshareInviteFlags::CURRENT_LOCAL_IP | RetroshareInviteFlags::CURRENT_EXTERNAL_IP;

    if(ui._shortFormat_CB->isChecked())
    {
        if(ui._includeAllIPs_CB->isChecked())
            flags |= RetroshareInviteFlags::FULL_IP_HISTORY;

        rsPeers->getShortInvite(cert,rsPeers->getOwnId(), RetroshareInviteFlags::RADIX_FORMAT | flags);
    }
	else
    {
        if(ui._includeSignatures_CB->isChecked())
            flags |= RetroshareInviteFlags::PGP_SIGNATURES;

        cert = rsPeers->GetRetroshareInvite( rsPeers->getOwnId(), flags);
    }

	ui.certplainTextEdit->setPlainText( QString::fromUtf8( cert.c_str() ) );

    RsPeerDetails detail;
    rsPeers->getPeerDetails(rsPeers->getOwnId(),detail);

    ui.certplainTextEdit->setToolTip(ConfCertDialog::getCertificateDescription(detail, ui._includeSignatures_CB->isChecked(), ui._shortFormat_CB->isChecked(), flags));
}

void
CryptoPage::copyRSLink()
{
	RsPeerId ownId = rsPeers->getOwnId() ;
	RetroShareLink link = RetroShareLink::createCertificate(ownId);

	if( link.valid() )
	{
		QList<RetroShareLink> urls ;

		urls.push_back(link) ;
		RSLinkClipboard::copyLinks(urls) ;
		QMessageBox::information(this,
				"RetroShare",
				tr("A RetroShare link with your Public Key is copied to Clipboard, paste and send it to your"
					" friend via email or some other way"));
	}
	else
		QMessageBox::warning(this,
				tr("Error"),
				tr("Your certificate could not be parsed correctly. Please contact the developers."));
}
void
CryptoPage::copyPublicKey()
{
    QMessageBox::information(this,
                             tr("RetroShare"),
                             tr("Your Public Key is copied to Clipboard, paste and send it to your"
                                " friend via email or some other way"));
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(ui.certplainTextEdit->toPlainText());
}

bool CryptoPage::fileSave()
{
    if (fileName.isEmpty())
        return fileSaveAs();

    QFile file(fileName);
    if (!file.open(QFile::WriteOnly))
        return false;
    QTextStream ts(&file);
    ts.setCodec(QTextCodec::codecForName("UTF-8"));
    ts << ui.certplainTextEdit->document()->toPlainText();
    return true;
}

bool CryptoPage::fileSaveAs()
{
    QString fn;
    if (misc::getSaveFileName(this, RshareSettings::LASTDIR_CERT, tr("Save as..."), tr("RetroShare Certificate (*.rsc );;All Files (*)"), fn)) {
        fileName = fn;
        return fileSave();
    }
    return false;
}

void CryptoPage::showStats()
{
    StatisticsWindow::showYourself();
}

void CryptoPage::exportFriendlistClicked()
{
   QString fileName = QFileDialog::getSaveFileName(this, tr("Export Friendslist"), "Friendslist", tr("RetroShare Friendslist (*.xml)"));

   if(!exportFriendlist(fileName))
        // error was already shown - just return
        return;

    QMessageBox mbox;
    mbox.setIcon(QMessageBox::Information);
    mbox.setText(tr("Done!"));
    mbox.setInformativeText(tr("Your friendlist is stored at:\n") + fileName +
                            tr("\n(keep in mind that the file is unencrypted!)"));
    mbox.setStandardButtons(QMessageBox::Ok);
    mbox.exec();
}

bool CryptoPage::exportFriendlist(QString &fileName)
{
    QDomDocument doc("FriendListWithGroups");
    QDomElement root = doc.createElement("root");
    doc.appendChild(root);

    QFile file(fileName);

    if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        // show error to user
        QMessageBox mbox;
        mbox.setIcon(QMessageBox::Warning);
        mbox.setText(tr("Error"));
        mbox.setInformativeText(tr("File is not writeable!\n") + fileName);
        mbox.setStandardButtons(QMessageBox::Ok);
        mbox.exec();
        return false;
    }

    std::list<RsPgpId> gpg_ids;
    rsPeers->getGPGAcceptedList(gpg_ids);

    std::list<RsGroupInfo> group_info_list;
    rsPeers->getGroupInfoList(group_info_list);

    QDomElement pgpIDs = doc.createElement("pgpIDs");
    RsPeerDetails detailPGP;
    for(std::list<RsPgpId>::iterator list_iter = gpg_ids.begin(); list_iter !=  gpg_ids.end(); ++list_iter)	{
        rsPeers->getGPGDetails(*list_iter, detailPGP);
        QDomElement pgpID = doc.createElement("pgpID");
        // these values aren't used and just stored for better human readability
        pgpID.setAttribute("id", QString::fromStdString(detailPGP.gpg_id.toStdString()));
        pgpID.setAttribute("name", QString::fromUtf8(detailPGP.name.c_str()));

        std::list<RsPeerId> ssl_ids;
        rsPeers->getAssociatedSSLIds(*list_iter, ssl_ids);
        for(std::list<RsPeerId>::iterator list_iter2 = ssl_ids.begin(); list_iter2 !=  ssl_ids.end(); ++list_iter2) {
            RsPeerDetails detailSSL;
            if (!rsPeers->getPeerDetails(*list_iter2, detailSSL))
                continue;

            std::string certificate = rsPeers->GetRetroshareInvite(detailSSL.id, RsPeers::defaultCertificateFlags | RetroshareInviteFlags::RADIX_FORMAT);

            // remove \n from certificate
            certificate.erase(std::remove(certificate.begin(), certificate.end(), '\n'), certificate.end());

            QDomElement sslID = doc.createElement("sslID");
            // these values aren't used and just stored for better human readability
            sslID.setAttribute("sslID", QString::fromStdString(detailSSL.id.toStdString()));
            if(!detailSSL.location.empty())
                sslID.setAttribute("location", QString::fromUtf8(detailSSL.location.c_str()));

            // required values
            sslID.setAttribute("certificate", QString::fromStdString(certificate));
            sslID.setAttribute("service_perm_flags", detailSSL.service_perm_flags.toUInt32());

            pgpID.appendChild(sslID);
        }
        pgpIDs.appendChild(pgpID);
    }
    root.appendChild(pgpIDs);

    QDomElement groups = doc.createElement("groups");
    for(std::list<RsGroupInfo>::iterator list_iter = group_info_list.begin(); list_iter !=  group_info_list.end(); ++list_iter)	{
        RsGroupInfo group_info = *list_iter;

        //skip groups without peers
        if(group_info.peerIds.empty())
            continue;

        QDomElement group = doc.createElement("group");
        // id is not needed since it may differ between locatiosn / pgp ids (groups are identified by name)
        group.setAttribute("name", QString::fromUtf8(group_info.name.c_str()));
        group.setAttribute("flag", group_info.flag);

        for(std::set<RsPgpId>::iterator i = group_info.peerIds.begin(); i !=  group_info.peerIds.end(); ++i) {
            QDomElement pgpID = doc.createElement("pgpID");
            std::string pid = i->toStdString();
            pgpID.setAttribute("id", QString::fromStdString(pid));
            group.appendChild(pgpID);
        }
        groups.appendChild(group);
    }
    root.appendChild(groups);

    QTextStream ts(&file);
    ts.setCodec("UTF-8");
    ts << doc.toString();
    file.close();

    return true;
}
