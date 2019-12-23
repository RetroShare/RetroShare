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
#include <QMessageBox>
#include <QClipboard>
#include <QFile>
#include <QTextStream>
#include <QTextCodec>

#include "rshare.h"
#include "CryptoPage.h"
#include "util/misc.h"
#include "util/DateTime.h"
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

	connect(ui.exportprofile,SIGNAL(clicked()), this, SLOT(profilemanager()));


	ui.onlinesince->setText(DateTime::formatLongDateTime(Rshare::startupTime()));
}

void CryptoPage::profilemanager()
{
    ProfileManager().exec();
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
        rsPeers->getShortInvite(invite,rsPeers->getOwnId(),true,false);
        ui.retroshareid->setText(QString::fromUtf8(invite.c_str()));
		
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

    if(ui._shortFormat_CB->isChecked())
        rsPeers->getShortInvite(cert,rsPeers->getOwnId(), true, !ui._includeAllIPs_CB->isChecked());
	else
		cert = rsPeers->GetRetroshareInvite( rsPeers->getOwnId(), ui._includeSignatures_CB->isChecked(), ui._includeAllIPs_CB->isChecked() );

	ui.certplainTextEdit->setPlainText( QString::fromUtf8( cert.c_str() ) );

    RsPeerDetails detail;
    rsPeers->getPeerDetails(rsPeers->getOwnId(),detail);

    ui.certplainTextEdit->setToolTip(ConfCertDialog::getCertificateDescription(detail, ui._includeSignatures_CB->isChecked(), ui._shortFormat_CB->isChecked(), ui._includeAllIPs_CB->isChecked() ));
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
