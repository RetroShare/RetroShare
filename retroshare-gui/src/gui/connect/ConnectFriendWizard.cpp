/*******************************************************************************
 * gui/connect/ConnectFriendWizard.cpp                                         *
 *                                                                             *
 * Copyright (C) 2009 retroshare team <retroshare.project@gmail.com>           *
 * Copyright (C) 2009 Oleksiy Bilyanskyy                                       *
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

#include <QCheckBox>
#include <QClipboard>
#include <QDesktopServices>
#include <QFileDialog>
#include <QLayout>
#include <QMessageBox>
#include <QTextCodec>
#include <QTextStream>
#include <QUrl>

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#include <QUrlQuery>
#endif

#include "gui/settings/rsharesettings.h"
#include "util/misc.h"
#include "ConnectFriendWizard.h"
#include "ui_ConnectFriendWizard.h"
#include "gui/common/PeerDefs.h"
#include "gui/notifyqt.h"
#include "gui/common/GroupDefs.h"
#include "gui/msgs/MessageComposer.h"

#include <retroshare/rsiface.h>
#include <retroshare/rsbanlist.h>
#include <retroshare/rsconfig.h>

#include "ConnectProgressDialog.h"
#include "gui/GetStartedDialog.h"

//#define FRIEND_WIZARD_DEBUG

ConnectFriendPage::ConnectFriendPage(QWidget *parent) : QWizardPage(parent)
{
	useComplete = false;
	complete = true;
}

void ConnectFriendPage::setComplete(bool isComplete)
{
	useComplete = true;
	complete = isComplete;
	emit completeChanged();
}

bool ConnectFriendPage::isComplete() const
{
	if (useComplete) {
		return complete;
	}

	return QWizardPage::isComplete();
}

ConnectFriendWizard::ConnectFriendWizard(QWidget *parent) :
	QWizard(parent), ui(new Ui::ConnectFriendWizard)
{
	ui->setupUi(this);

	mTitleFontSize = 0; // Standard
	mTitleFontWeight = 0; // Standard

    // (csoler) I'm hiding this, since it is not needed anymore with the new Home page.
    ui->userFrame->hide();
	
	ui->userFileFrame->hide(); // in homepage dropmenu now

// this define comes from Qt example. I don't have mac, so it wasn't tested
#ifndef Q_OS_MAC
	setWizardStyle(ModernStyle);
#endif

	setStartId(Page_Cert);

// at this moment I don't know, what information should be in help
//	setOption(HaveHelpButton, true);
//	connect(this, SIGNAL(helpRequested()), this, SLOT(showHelp()));

	setPixmap(QWizard::LogoPixmap, QPixmap(":/icons/invite64.png"));

// we have no good pictures for watermarks
//	setPixmap(QWizard::WatermarkPixmap, QPixmap(":/images/connectFriendWatermark.png"));

	/* register global fields */
	ui->ErrorMessagePage->registerField("errorMessage", ui->messageLabel, "text");

	/* disable not used pages */
	//ui->foffRadioButton->hide();
	//ui->rsidRadioButton->hide();
	
	ui->cp_Label->hide();
	ui->requestinfolabel->hide();
	
    connect(ui->acceptNoSignGPGCheckBox,SIGNAL(toggled(bool)), ui->_options_GB,SLOT(setEnabled(bool))) ;
    connect(ui->addKeyToKeyring_CB,SIGNAL(toggled(bool)), ui->acceptNoSignGPGCheckBox,SLOT(setChecked(bool))) ;
	
    connect(ui->gmailButton, SIGNAL(clicked()), this, SLOT(inviteGmail()));
    connect(ui->yahooButton, SIGNAL(clicked()), this, SLOT(inviteYahoo()));
    connect(ui->outlookButton, SIGNAL(clicked()), this, SLOT(inviteOutlook()));
    connect(ui->aolButton, SIGNAL(clicked()), this, SLOT(inviteAol()));
    connect(ui->yandexButton, SIGNAL(clicked()), this, SLOT(inviteYandex()));
    connect(ui->emailButton, SIGNAL(clicked()), this, SLOT(runEmailClient2()));
	connect(ui->toggleadvancedButton, SIGNAL(clicked()), this, SLOT(toggleAdvanced()));
    
    subject = tr("RetroShare Invitation");
    body = GetStartedDialog::GetInviteText();
	
    body += "\n" + GetStartedDialog::GetCutBelowText();
	body += "\n\n" + QString::fromUtf8(rsPeers->GetRetroshareInvite().c_str());
	
	std::string advsetting;
	if(rsConfig->getConfigurationOption(RS_CONFIG_ADVANCED, advsetting) && (advsetting == "YES"))
	{
		ui->toggleadvancedButton->setVisible(false);
	}
	else
	{
		ui->userFrame->hide(); // certificates page - top half with own cert and it's functions
		ui->cp_Frame->hide(); // Advanced options - key sign, whitelist, direct source ...
		AdvancedVisible=false;
		ui->trustLabel->hide();
		ui->trustEdit->hide();
	}
	
	//Add warning to direct source checkbox depends general setting.
	switch (rsFiles->filePermDirectDL())
	{
		case RS_FILE_PERM_DIRECT_DL_YES:
//			ui->_direct_transfer_CB->setIcon(QIcon(":/icons/warning_yellow_128.png"));
//			ui->_direct_transfer_CB->setToolTip(ui->_direct_transfer_CB->toolTip().append(tr("\nWarning: In your File-Transfer option, you select allow direct download to Yes.")));
			ui->_direct_transfer_CB_2->setIcon(QIcon(":/icons/warning_yellow_128.png"));
			ui->_direct_transfer_CB_2->setToolTip(ui->_direct_transfer_CB_2->toolTip().append(tr("\nWarning: In your File-Transfer option, you select allow direct download to Yes.")));
			break ;
		case RS_FILE_PERM_DIRECT_DL_NO:
//			ui->_direct_transfer_CB->setIcon(QIcon(":/icons/warning_yellow_128.png"));
//			ui->_direct_transfer_CB->setToolTip(ui->_direct_transfer_CB->toolTip().append(tr("\nWarning: In your File-Transfer option, you select allow direct download to No.")));
			ui->_direct_transfer_CB_2->setIcon(QIcon(":/icons/warning_yellow_128.png"));
			ui->_direct_transfer_CB_2->setToolTip(ui->_direct_transfer_CB_2->toolTip().append(tr("\nWarning: In your File-Transfer option, you select allow direct download to No.")));
			break ;

		default: break ;
	}
	updateStylesheet();
}

void ConnectFriendWizard::setBannerPixmap(const QString &pixmap)
{
	mBannerPixmap = pixmap;
	setPixmap(QWizard::BannerPixmap, mBannerPixmap);
}

QString ConnectFriendWizard::bannerPixmap()
{
	return mBannerPixmap;
}

void ConnectFriendWizard::setTitleFontSize(int size)
{
	mTitleFontSize = size;
	updateStylesheet();
}

int ConnectFriendWizard::titleFontSize()
{
	return mTitleFontSize;
}

void ConnectFriendWizard::setTitleFontWeight(int weight)
{
	mTitleFontWeight = weight;
	updateStylesheet();
}

int ConnectFriendWizard::titleFontWeight()
{
	return mTitleFontWeight;
}

void ConnectFriendWizard::setTitleColor(const QString &color)
{
	mTitleColor = color;
	updateStylesheet();
}

QString ConnectFriendWizard::titleColor()
{
	return mTitleColor;
}

void ConnectFriendWizard::setTitleText(QWizardPage *page, const QString &title)
{
	if (!page) {
		return;
	}

	page->setTitle(title);

	mTitleString.remove(page);
	updateStylesheet();
}

void ConnectFriendWizard::updateStylesheet()
{
	/* add stylesheet to title */
	QList<int> ids = pageIds();
	for (QList<int>::iterator pageIt = ids.begin(); pageIt != ids.end(); ++pageIt) {
		QWizardPage *p = page(*pageIt);

		QString title;
		QMap<QWizardPage*, QString>::iterator it = mTitleString.find(p);
		if (it == mTitleString.end()) {
			/* Save title string */
			title = p->title();
			mTitleString[p] = title;
		} else {
			title = it.value();
		}

		QString stylesheet = "<span style=\"";

		if (mTitleFontSize) {
			stylesheet += QString("font-size:%1pt; ").arg(mTitleFontSize);
		}
		if (mTitleFontWeight) {
			stylesheet += QString("font-weight:%1; ").arg(mTitleFontWeight);
		}
		if (!mTitleColor.isEmpty()) {
			stylesheet += QString("color:%1; ").arg(mTitleColor);
		}

		stylesheet += QString("\">%1</span>").arg(title);

		p->setTitle(stylesheet);
	}
}

QString ConnectFriendWizard::getErrorString(uint32_t error_code)
{
	switch(error_code)
	{
		case CERTIFICATE_PARSING_ERROR_SIZE_ERROR: 					return tr("Abnormal size read is bigger than memory block.") ;
		case CERTIFICATE_PARSING_ERROR_INVALID_LOCATION_ID: 		return tr("Invalid node id.") ;
		case CERTIFICATE_PARSING_ERROR_INVALID_EXTERNAL_IP: 		return tr("Invalid external IP.") ;
		case CERTIFICATE_PARSING_ERROR_INVALID_LOCAL_IP: 			return tr("Invalid local IP.") ;
		case CERTIFICATE_PARSING_ERROR_INVALID_CHECKSUM_SECTION: return tr("Invalid checksum section.") ;
		case CERTIFICATE_PARSING_ERROR_CHECKSUM_ERROR: 				return tr("Checksum mismatch. Certificate is corrupted.") ;
		case CERTIFICATE_PARSING_ERROR_WRONG_VERSION: 				return tr("Certificate has wrong version number. Remember that v0.6 and v0.5 networks are incompatible.") ;
		case CERTIFICATE_PARSING_ERROR_UNKNOWN_SECTION_PTAG:		return tr("Unknown section type found (Certificate might be corrupted).") ;
		case CERTIFICATE_PARSING_ERROR_MISSING_CHECKSUM:			return tr("Missing checksum.") ;

		default:
			return tr("Unknown certificate error") ;
	}
}

void ConnectFriendWizard::setCertificate(const QString &certificate, bool friendRequest)
{
	if (certificate.isEmpty()) {
		return;
	}

	uint32_t cert_load_error_code;

	if (rsPeers->loadDetailsFromStringCert(certificate.toUtf8().constData(), peerDetails, cert_load_error_code))
	{
#ifdef FRIEND_WIZARD_DEBUG
		std::cerr << "ConnectFriendWizard got id : " << peerDetails.id << "; gpg_id : " << peerDetails.gpg_id << std::endl;
#endif

		if(peerDetails.id == rsPeers->getOwnId())
		{
			setField("errorMessage", tr("This is your own certificate! You would not want to make friend with yourself. Wouldn't you?") ) ;
			error = false;
			setStartId(Page_ErrorMessage);
		}
		else
		{
			mCertificate = certificate.toUtf8().constData();

			setStartId(Page_Conclusion);
			if (friendRequest){
				ui->cp_Label->show();
				ui->requestinfolabel->show();
				setTitleText(ui->ConclusionPage, tr("Friend request"));
				ui->ConclusionPage->setSubTitle(tr("Details about the request"));
			}
		}
    }
    else if(rsPeers->parseShortInvite(certificate.toUtf8().constData(),peerDetails))
    {
		if(peerDetails.id == rsPeers->getOwnId())
		{
			setField("errorMessage", tr("This is your own certificate! You would not want to make friend with yourself. Wouldn't you?") ) ;
			error = false;
			setStartId(Page_ErrorMessage);
		}
		else
		{
			mCertificate = certificate.toUtf8().constData();

			setStartId(Page_Conclusion);

			if (friendRequest){
				ui->cp_Label->show();
				ui->requestinfolabel->show();
				setTitleText(ui->ConclusionPage, tr("Friend request"));
				ui->ConclusionPage->setSubTitle(tr("Details about the request"));
			}
		}
	}
    else
    {
		// error message
		setField("errorMessage", tr("Certificate Load Failed") + ": \n\n" + getErrorString(cert_load_error_code)) ;
		setStartId(Page_ErrorMessage);
	}
}

void ConnectFriendWizard::setGpgId(const RsPgpId &gpgId, const RsPeerId &sslId, bool friendRequest)
{
	if (!rsPeers->getGPGDetails(gpgId, peerDetails)) {
		setField("errorMessage", tr("Cannot get peer details of PGP key %1").arg(QString::fromStdString(gpgId.toStdString())));
		setStartId(Page_ErrorMessage);
		return;
	}

	/* Set ssl id when available */
	peerDetails.id = sslId;

    //setStartId(friendRequest ? Page_FriendRequest : Page_Conclusion);
    setStartId(Page_Conclusion);
    if (friendRequest){
    ui->cp_Label->show();
    ui->requestinfolabel->show();
    setTitleText(ui->ConclusionPage,tr("Friend request"));
    ui->ConclusionPage->setSubTitle(tr("Details about the request"));
    }
}

ConnectFriendWizard::~ConnectFriendWizard()
{
	delete ui;
}

static void fillGroups(ConnectFriendWizard *wizard, QComboBox *comboBox, const QString &groupId)
{
	std::list<RsGroupInfo> groupInfoList;
	rsPeers->getGroupInfoList(groupInfoList);
	GroupDefs::sortByName(groupInfoList);
	comboBox->addItem("", ""); // empty value
	for (std::list<RsGroupInfo>::iterator groupIt = groupInfoList.begin(); groupIt != groupInfoList.end(); ++groupIt) {
        comboBox->addItem(GroupDefs::name(*groupIt), QString::fromStdString(groupIt->id.toStdString()));
	}

	if (groupId.isEmpty() == false) {
		comboBox->setCurrentIndex(comboBox->findData(groupId));
	}
	QObject::connect(comboBox, SIGNAL(currentIndexChanged(int)), wizard, SLOT(groupCurrentIndexChanged(int)));
}

void ConnectFriendWizard::initializePage(int id)
{
	switch ((Page) id) {
	case Page_Text:
		connect(ui->userCertHelpButton, SIGNAL( clicked()), this, SLOT(showHelpUserCert()));
		connect(ui->userCertIncludeSignaturesButton, SIGNAL(clicked()), this, SLOT(toggleSignatureState()));
		connect(ui->userCertOldFormatButton, SIGNAL(clicked()), this, SLOT(toggleFormatState()));
		connect(ui->userCertCopyButton, SIGNAL(clicked()), this, SLOT(copyCert()));
		connect(ui->userCertPasteButton, SIGNAL(clicked()), this, SLOT(pasteCert()));
		connect(ui->userCertOpenButton, SIGNAL(clicked()), this, SLOT(openCert()));
		connect(ui->userCertSaveButton, SIGNAL(clicked()), this, SLOT(saveCert()));
		connect(ui->userCertMailButton, SIGNAL(clicked()), this, SLOT(runEmailClient()));
		connect(ui->friendCertEdit, SIGNAL(textChanged()), this, SLOT(friendCertChanged()));

		cleanfriendCertTimer = new QTimer(this);
		cleanfriendCertTimer->setSingleShot(true);
		cleanfriendCertTimer->setInterval(1000); // 1 second
		connect(cleanfriendCertTimer, SIGNAL(timeout()), this, SLOT(cleanFriendCert()));

		ui->userCertOldFormatButton->setChecked(false); 
		ui->userCertOldFormatButton->hide() ;

		toggleFormatState(true);
		toggleSignatureState(false);
		updateOwnCert();

		cleanFriendCert();

		break;
	case Page_Cert:
		connect(ui->userFileCreateButton, SIGNAL(clicked()), this, SLOT(generateCertificateCalled()));
		connect(ui->friendFileNameOpenButton, SIGNAL(clicked()), this, SLOT(loadFriendCert()));

		ui->friendFileNameEdit->setAcceptFile(true);

		ui->CertificatePage->registerField("friendCertificateFile*", ui->friendFileNameEdit);
		break;
	case Page_WebMail:

	case Page_ErrorMessage:
		break;
	case Page_Conclusion:
		{
			bool peerIsHiddenNode = peerDetails.isHiddenNode ;
			bool amIHiddenNode = rsPeers->isHiddenNode(rsPeers->getOwnId()) ;

			std::cerr << "Conclusion page id : " << peerDetails.id << "; gpg_id : " << peerDetails.gpg_id << std::endl;

            ui->_direct_transfer_CB_2  ->setChecked(false) ; //peerDetails.service_perm_flags & RS_NODE_PERM_DIRECT_DL) ;
            ui->_allow_push_CB_2  ->setChecked(peerDetails.service_perm_flags & RS_NODE_PERM_ALLOW_PUSH) ;

			if(!peerIsHiddenNode && !amIHiddenNode)
				ui->_require_WL_CB_2  ->setChecked(peerDetails.service_perm_flags & RS_NODE_PERM_REQUIRE_WL) ;
			else
			{
				ui->_require_WL_CB_2  ->setChecked(false) ;
				ui->_require_WL_CB_2  ->hide() ;

				ui->_addIPToWhiteList_CB_2->hide();
				ui->_addIPToWhiteList_ComboBox_2->hide();
			}

        sockaddr_storage addr ;

		std::cerr << "Cert IP = " << peerDetails.extAddr << std::endl;

        if(sockaddr_storage_ipv4_aton(addr,peerDetails.extAddr.c_str()) && sockaddr_storage_isValidNet(addr))
        {
            QString ipstring0 = QString::fromStdString(sockaddr_storage_iptostring(addr));

            ui->_addIPToWhiteList_CB_2->setChecked(ui->_require_WL_CB_2->isChecked());
            ui->_addIPToWhiteList_ComboBox_2->addItem(ipstring0) ;
            ui->_addIPToWhiteList_ComboBox_2->addItem(ipstring0+"/24") ;
            ui->_addIPToWhiteList_ComboBox_2->addItem(ipstring0+"/16") ;
            ui->_addIPToWhiteList_ComboBox_2->setEnabled(true) ;
            ui->_addIPToWhiteList_CB_2->setEnabled(true) ;
        }
        else if(ui->_require_WL_CB_2->isChecked())
        {
        ui->_addIPToWhiteList_ComboBox_2->addItem(tr("No IP in this certificate!")) ;
            ui->_addIPToWhiteList_ComboBox_2->setToolTip(tr("<p>This certificate has no IP. You will rely on discovery and DHT to find it. Because you require whitelist clearance, the peer will raise a security warning in the NewsFeed tab. From there, you can whitelist his IP.</p>")) ;
            ui->_addIPToWhiteList_ComboBox_2->setEnabled(false) ;
            ui->_addIPToWhiteList_CB_2->setChecked(false) ;
            ui->_addIPToWhiteList_CB_2->setEnabled(false) ;
        }

			RsPeerDetails tmp_det ;
			bool already_in_keyring = rsPeers->getGPGDetails(peerDetails.gpg_id, tmp_det) ;

			ui->addKeyToKeyring_CB->setChecked(true) ;
			ui->addKeyToKeyring_CB->setEnabled(!already_in_keyring) ;

			if(already_in_keyring)
				ui->addKeyToKeyring_CB->setToolTip(tr("This key is already in your keyring")) ;
			else
				ui->addKeyToKeyring_CB->setToolTip(tr("Check this to add the key to your keyring\nThis might be useful for sending\ndistant messages to this peer\neven if you don't make friends.")) ;

			if(tmp_det.accept_connection) {
				ui->acceptNoSignGPGCheckBox->setChecked(true);
				ui->acceptNoSignGPGCheckBox->setEnabled(false);
				ui->acceptNoSignGPGCheckBox->setToolTip(tr("This key is already on your trusted list"));
			}
			else
				ui->alreadyRegisteredLabel->hide();
			if(tmp_det.ownsign) {
				ui->signGPGCheckBox->setChecked(false);	// if already signed, we dont allow to sign it again, and dont show the box.
				ui->signGPGCheckBox->setVisible(false);
				ui->signGPGCheckBox->setToolTip(tr("You have already signed this key"));
			}

			QString trustString;
			switch (peerDetails.validLvl) {
			case RS_TRUST_LVL_ULTIMATE:
				trustString = tr("Ultimate");
				break;
			case RS_TRUST_LVL_FULL:
				trustString = tr("Full");
				break;
			case RS_TRUST_LVL_MARGINAL:
				trustString = tr("Marginal");
				break;
			case RS_TRUST_LVL_NEVER:
				trustString = tr("None");
				break;
			default:
				trustString = tr("No Trust");
				break;
			}

			QString ts;
			std::list<RsPgpId>::iterator it;
			for (it = peerDetails.gpgSigners.begin(); it != peerDetails.gpgSigners.end(); ++it) {
				{
					std::string peer_name = rsPeers->getGPGName(*it) ;

					// This is baaaad code. We should handle this kind of errors with proper exceptions.
					// This happens because signers from a unknown key cannt be found in the keyring, including
					// self-signatures.
					//
					if(peer_name == "[Unknown PGP Cert name]" && *it == peerDetails.gpg_id)
						peer_name = peerDetails.name ;

				ts += QString("%1<%2>\n").arg(QString::fromUtf8(peer_name.c_str()), QString::fromStdString( (*it).toStdString()));
				}
			}

			ui->cp_Label->setText(tr("You have a friend request from") + " " + QString::fromUtf8(peerDetails.name.c_str()));
			ui->nameEdit->setText(QString::fromUtf8(peerDetails.name.c_str()));
			ui->trustEdit->setText(trustString);
			ui->emailEdit->setText(QString::fromUtf8(peerDetails.email.c_str()));
			QString loc = QString::fromUtf8(peerDetails.location.c_str());
			if (!loc.isEmpty())
			{
				loc += " (";
				loc += QString::fromStdString(peerDetails.id.toStdString());
				loc += ")";
			}
			else
			{
				if (!peerDetails.id.isNull())
				{
				    loc += QString::fromStdString(peerDetails.id.toStdString());
				}
			}

			ui->nodeEdit->setText(loc);
			ui->ipEdit->setText(QString::fromStdString(peerDetails.isHiddenNode ? peerDetails.hiddenNodeAddress : peerDetails.extAddr));
			ui->signersEdit->setPlainText(ts);

			fillGroups(this, ui->groupComboBox, groupId);
			
			if(peerDetails.isHiddenNode)
			{
				ui->_addIPToWhiteList_CB_2->setEnabled(false) ;
				ui->_require_WL_CB_2->setEnabled(false) ;
				ui->_addIPToWhiteList_ComboBox_2->setEnabled(false) ;
				ui->_addIPToWhiteList_ComboBox_2->addItem("(Hidden node)") ;
				int S = QFontMetricsF(ui->ipEdit->font()).height() ;
				ui->ipEdit->setToolTip("This is a Hidden node - you need tor/i2p proxy to connect");
				ui->ipLabel->setPixmap(QPixmap(":/images/anonymous_128_blue.png").scaledToHeight(S*2,Qt::SmoothTransformation));
				ui->ipLabel->setToolTip("This is a Hidden node - you need tor/i2p proxy to connect");
			}

			if(peerDetails.email.empty())
			{
				ui->emailLabel->hide(); // is it ever used?
				ui->emailEdit->hide();
			}
			ui->ipEdit->setTextInteractionFlags(Qt::TextSelectableByMouse);

		}
		break;
	case Page_FriendRecommendations:
		ui->frec_recommendList->setHeaderText(tr("Recommend friends"));
		ui->frec_recommendList->setModus(FriendSelectionWidget::MODUS_CHECK);
		ui->frec_recommendList->setShowType(FriendSelectionWidget::SHOW_GROUP | FriendSelectionWidget::SHOW_SSL);
		ui->frec_recommendList->start();

		ui->frec_toList->setHeaderText(tr("To"));
		ui->frec_toList->setModus(FriendSelectionWidget::MODUS_CHECK);
		ui->frec_toList->start();

		ui->frec_messageEdit->setText(MessageComposer::recommendMessage());
		break;
	}
}

static void sendMail(QString sAddress, QString sSubject, QString sBody)
{
#ifdef Q_OS_WIN
	/* search and replace the end of lines with: "%0D%0A" */
	sBody.replace("\n", "%0D%0A");
#endif

	QUrl url = QUrl("mailto:" + sAddress);

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	QUrlQuery urlQuery;
#else
	QUrl &urlQuery(url);
#endif

	urlQuery.addQueryItem("subject", sSubject);
	urlQuery.addQueryItem("body", sBody);

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	url.setQuery(urlQuery);
#endif

	std::cerr << "MAIL STRING:" << (std::string)url.toEncoded().constData() << std::endl;

	/* pass the url directly to QDesktopServices::openUrl */
	QDesktopServices::openUrl (url);
}

bool ConnectFriendWizard::validateCurrentPage()
{
	error = true;

	switch ((Page) currentId()) {
	case Page_WebMail:
		break;
	case Page_Text:
		{
			std::string certstr = ui->friendCertEdit->toPlainText().toUtf8().constData();
			uint32_t cert_load_error_code;

			if (rsPeers->loadDetailsFromStringCert(certstr, peerDetails, cert_load_error_code) || rsPeers->parseShortInvite(certstr,peerDetails))
            {
				mCertificate = certstr;
#ifdef FRIEND_WIZARD_DEBUG
				std::cerr << "ConnectFriendWizard got id : " << peerDetails.id << "; gpg_id : " << peerDetails.gpg_id << std::endl;
#endif

				if(peerDetails.id == rsPeers->getOwnId())
				{
					setField("errorMessage", tr("This is your own certificate! You would not want to make friend with yourself. Wouldn't you?") ) ;
					error = false;
				}

				break;
			}
			// error message
			setField("errorMessage", tr("Certificate Load Failed") + ": \n\n" + getErrorString(cert_load_error_code)) ;
			error = false;
			break;
		}
	case Page_Cert:
		{
			QString fn = ui->friendFileNameEdit->text();
			if (QFile::exists(fn)) {
				//Todo: move read from file to p3Peers::loadCertificateFromFile

				// read from file
				std::string certstr;
				QFile CertFile(fn);
				if (CertFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
					certstr = QString(CertFile.readAll()).toStdString();
					CertFile.close();
				}

				if (certstr.empty()) {
					setField("errorMessage", QString(tr("Certificate Load Failed:can't read from file %1")).arg(fn+" ") );
					error = false;
					break;
				}

				uint32_t cert_error_code;
				if (rsPeers->loadDetailsFromStringCert(certstr, peerDetails, cert_error_code) || rsPeers->parseShortInvite(certstr,peerDetails))
                {
					mCertificate = certstr;
#ifdef FRIEND_WIZARD_DEBUG
					std::cerr << "ConnectFriendWizard got id : " << peerDetails.id << "; gpg_id : " << peerDetails.gpg_id << std::endl;
#endif

					if(peerDetails.id == rsPeers->getOwnId())
					{
						setField("errorMessage", tr("This is your own certificate! You would not want to make friend with yourself. Wouldn't you?") ) ;
						error = false;
					}
				} else {
					setField("errorMessage", QString(tr("Certificate Load Failed:something is wrong with %1")).arg(fn) + " : " + getErrorString(cert_error_code));
					error = false;
				}
			} else {
				setField("errorMessage", QString(tr("Certificate Load Failed:file %1 not found")).arg(fn));
				error = false;
			}
			break;
		}
	case Page_ErrorMessage:
		break;
	case Page_Conclusion:
		break;
	case Page_FriendRecommendations:
		{
            std::set<RsPeerId> recommendIds;
            ui->frec_recommendList->selectedIds<RsPeerId,FriendSelectionWidget::IDTYPE_SSL>(recommendIds, false);

			if (recommendIds.empty()) {
				QMessageBox::warning(this, "RetroShare", tr("Please select at least one friend for recommendation."), QMessageBox::Ok, QMessageBox::Ok);
				return false;
			}

            std::set<RsPeerId> toIds;
            ui->frec_toList->selectedIds<RsPeerId,FriendSelectionWidget::IDTYPE_SSL>(toIds, false);

			if (toIds.empty()) {
				QMessageBox::warning(this, "RetroShare", tr("Please select at least one friend as recipient."), QMessageBox::Ok, QMessageBox::Ok);
				return false;
			}

            std::set<RsPeerId>::iterator toId;
			for (toId = toIds.begin(); toId != toIds.end(); ++toId) {
				MessageComposer::recommendFriend(recommendIds, *toId, ui->frec_messageEdit->toHtml(), true);
			}
		}
	}

	return true;
}

int ConnectFriendWizard::nextId() const
{
	switch ((Page) currentId()) {
	case Page_Text:
	case Page_Cert:
	case Page_WebMail:
	case Page_ErrorMessage:
	case Page_Conclusion:
	case Page_FriendRecommendations:
		return -1;
	}

	return -1;
}

ServicePermissionFlags ConnectFriendWizard::serviceFlags() const
{
    ServicePermissionFlags flags(0) ;

if (hasVisitedPage(Page_Conclusion)) {
        if(  ui->_direct_transfer_CB_2->isChecked()) flags |= RS_NODE_PERM_DIRECT_DL ;
        if(  ui->_allow_push_CB_2->isChecked()) flags |= RS_NODE_PERM_ALLOW_PUSH ;
        if(  ui->_require_WL_CB_2->isChecked()) flags |= RS_NODE_PERM_REQUIRE_WL ;
    }
    return flags ;
}
void ConnectFriendWizard::accept()
{
	bool sign = false;
	bool accept_connection = false;
	bool add_key_to_keyring = false;

	if (hasVisitedPage(Page_Conclusion)) {
		std::cerr << "ConnectFriendWizard::accept() called with page conclusion visited" << std::endl;

		sign = ui->signGPGCheckBox->isChecked();
		accept_connection = ui->acceptNoSignGPGCheckBox->isChecked();
		add_key_to_keyring = ui->addKeyToKeyring_CB->isChecked() ;
	} else  {
		QDialog::accept();
		return;
	}

    if(!peerDetails.skip_signature_validation && !mCertificate.empty() && add_key_to_keyring)
	{
		RsPgpId pgp_id ;
		RsPeerId ssl_id ;
		std::string error_string ;

		if(!rsPeers->loadCertificateFromString(mCertificate,ssl_id,pgp_id,error_string))
		{
			std::cerr << "ConnectFriendWizard::accept(): cannot load that certificate." << std::endl;
			return ;
		}
	}

	bool runProgressDialog = false;

	if(accept_connection && !peerDetails.gpg_id.isNull()) 
	{
		std::cerr << "ConclusionPage::validatePage() accepting GPG key for connection." << std::endl;

        if(peerDetails.skip_signature_validation)
			rsPeers->addSslOnlyFriend(peerDetails.id, peerDetails.gpg_id,peerDetails);
		else
			rsPeers->addFriend(peerDetails.id, peerDetails.gpg_id,serviceFlags()) ;

		rsPeers->setServicePermissionFlags(peerDetails.gpg_id,serviceFlags()) ;

		if(ui->_addIPToWhiteList_CB_2->isChecked())
		{
			sockaddr_storage addr ;
			if(sockaddr_storage_ipv4_aton(addr,peerDetails.extAddr.c_str()) && sockaddr_storage_isValidNet(addr))
			{
				std::cerr << "ConclusionPage::adding IP " << sockaddr_storage_tostring(addr) << " to whitelist." << std::endl;
				rsBanList->addIpRange(addr,ui->_addIPToWhiteList_ComboBox_2->currentIndex(),RSBANLIST_TYPE_WHITELIST,std::string(tr("Added with certificate from %1").arg(ui->nameEdit->text()).toUtf8().constData()));
			}
		}

		if(sign)
		{
			std::cerr << "ConclusionPage::validatePage() signing GPG key." << std::endl;
			rsPeers->signGPGCertificate(peerDetails.gpg_id); //bye default sign set accept_connection to true;
			rsPeers->setServicePermissionFlags(peerDetails.gpg_id,serviceFlags()) ;
		}

		if (!groupId.isEmpty())
			rsPeers->assignPeerToGroup(RsNodeGroupId(groupId.toStdString()), peerDetails.gpg_id, true);
	}

	if ((accept_connection) && (!peerDetails.id.isNull()))
	{
		runProgressDialog = true;

		if (!peerDetails.location.empty()) {
			std::cerr << "ConnectFriendWizard::accept() : setting peer node." << std::endl;
			rsPeers->setLocation(peerDetails.id, peerDetails.location);
		}

		if (peerDetails.isHiddenNode)
		{
			std::cerr << "ConnectFriendWizard::accept() : setting hidden node." << std::endl;
			rsPeers->setHiddenNode(peerDetails.id, peerDetails.hiddenNodeAddress, peerDetails.hiddenNodePort);
		}
		else
		{
			//let's check if there is ip adresses in the wizard.
			if (!peerDetails.extAddr.empty() && peerDetails.extPort) {
				std::cerr << "ConnectFriendWizard::accept() : setting ip ext address." << std::endl;
				rsPeers->setExtAddress(peerDetails.id, peerDetails.extAddr, peerDetails.extPort);
			}
			if (!peerDetails.localAddr.empty() && peerDetails.localPort) {
				std::cerr << "ConnectFriendWizard::accept() : setting ip local address." << std::endl;
				rsPeers->setLocalAddress(peerDetails.id, peerDetails.localAddr, peerDetails.localPort);
			}
			if (!peerDetails.dyndns.empty()) {
				std::cerr << "ConnectFriendWizard::accept() : setting DynDNS." << std::endl;
				rsPeers->setDynDNS(peerDetails.id, peerDetails.dyndns);
			}
			for(auto&& ipr : peerDetails.ipAddressList)
				rsPeers->addPeerLocator(
				            peerDetails.id,
				            RsUrl(ipr.substr(0, ipr.find(' '))) );
		}

	}
		
	if (runProgressDialog)
	{
		RsPeerId ssl_id = peerDetails.id;
		// its okay if ssl_id is invalid - dialog will show error.
		ConnectProgressDialog::showProgress(ssl_id);
	}

	NotifyQt::getInstance()->notifyListChange(NOTIFY_LIST_NEIGHBOURS,1) ;

	QDialog::accept();
}

//============================= TextPage =====================================

void ConnectFriendWizard::updateOwnCert()
{
	std::string invite = rsPeers->GetRetroshareInvite( rsPeers->getOwnId(),
	            ui->userCertIncludeSignaturesButton->isChecked() );

	std::cerr << "TextPage() getting Invite: " << invite << std::endl;

	ui->userCertEdit->setPlainText(QString::fromUtf8(invite.c_str()));
}

void ConnectFriendWizard::toggleFormatState(bool doUpdate)
{
	if (ui->userCertOldFormatButton->isChecked()) 
	{
		ui->userCertOldFormatButton->setToolTip(tr("Use new certificate format (safer, more robust)"));
		ui->userCertOldFormatButton->setIcon(QIcon(":/images/ledoff1.png")) ;
	}
	else 
	{
		ui->userCertOldFormatButton->setToolTip(tr("Use old (backward compatible) certificate format"));
		ui->userCertOldFormatButton->setIcon(QIcon(":/images/ledon1.png")) ;
	}

	if (doUpdate) {
		updateOwnCert();
	}
}

void ConnectFriendWizard::toggleSignatureState(bool doUpdate)
{
	if (ui->userCertIncludeSignaturesButton->isChecked()) {
		ui->userCertIncludeSignaturesButton->setToolTip(tr("Remove signatures"));
	} else {
		ui->userCertIncludeSignaturesButton->setToolTip(tr("Include signatures"));
	}

	if (doUpdate) {
		updateOwnCert();
	}
}

void ConnectFriendWizard::runEmailClient()
{
	sendMail("", tr("RetroShare Invite"), ui->userCertEdit->toPlainText());
}

void ConnectFriendWizard::friendCertChanged()
{
	ui->TextPage->setComplete(false);
	cleanfriendCertTimer->start();
}

void ConnectFriendWizard::cleanFriendCert()
{
	bool certValid = false;
	QString errorMsg ;
	std::string cert = ui->friendCertEdit->toPlainText().toUtf8().constData();

	if (cert.empty()) {
		ui->friendCertCleanLabel->setPixmap(QPixmap(":/images/delete.png"));
		ui->friendCertCleanLabel->setToolTip("");
		ui->friendCertCleanLabel->setStyleSheet("");
		errorMsg = tr("");

	} else {
		std::string cleanCert;
		int error_code;
        bool is_short_format;

		if (rsPeers->cleanCertificate(cert, cleanCert, is_short_format, error_code))
        {
			certValid = true;

			if (cert != cleanCert)
            {
				QTextCursor textCursor = ui->friendCertEdit->textCursor();

				whileBlocking(ui->friendCertEdit)->setPlainText(QString::fromUtf8(cleanCert.c_str()));
				whileBlocking(ui->friendCertEdit)->setTextCursor(textCursor);

				ui->friendCertCleanLabel->setStyleSheet("");
			}
			errorMsg = tr("Valid certificate") + (is_short_format?" (Short format)":" (plain format with profile key)");

			ui->friendCertCleanLabel->setPixmap(QPixmap(":/images/accepted16.png"));
		} else {
			if (error_code > 0) {
				switch (error_code) {
				case CERTIFICATE_PARSING_ERROR_CHECKSUM_ERROR            :
				case CERTIFICATE_PARSING_ERROR_WRONG_VERSION             :
				case CERTIFICATE_PARSING_ERROR_SIZE_ERROR                :
				case CERTIFICATE_PARSING_ERROR_INVALID_LOCATION_ID       :
				case CERTIFICATE_PARSING_ERROR_INVALID_EXTERNAL_IP       :
				case CERTIFICATE_PARSING_ERROR_INVALID_LOCAL_IP          :
				case CERTIFICATE_PARSING_ERROR_INVALID_CHECKSUM_SECTION  :
				case CERTIFICATE_PARSING_ERROR_UNKNOWN_SECTION_PTAG      :
				case CERTIFICATE_PARSING_ERROR_MISSING_CHECKSUM          :

				default:
					errorMsg = tr("Not a valid Retroshare certificate!") ;
					ui->friendCertCleanLabel->setStyleSheet("QLabel#friendCertCleanLabel {border: 2px solid red; border-radius: 6px;}");
				}
			}
			ui->friendCertCleanLabel->setPixmap(QPixmap(":/images/delete.png"));
		}
	}

	ui->friendCertCleanLabel->setPixmap(certValid ? QPixmap(":/images/accepted16.png") : QPixmap(":/images/delete.png"));
	ui->friendCertCleanLabel->setToolTip(errorMsg);
	ui->friendCertCleanLabel->setText(errorMsg);

	ui->TextPage->setComplete(certValid);
}

void ConnectFriendWizard::showHelpUserCert()
{
	QMessageBox::information(this, tr("Connect Friend Help"), tr("You can copy this text and send it to your friend via email or some other way"));
}

void ConnectFriendWizard::copyCert()
{
	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setText(ui->userCertEdit->toPlainText());
	QMessageBox::information(this, "RetroShare", tr("Your Cert is copied to Clipboard, paste and send it to your friend via email or some other way"));
}

void ConnectFriendWizard::pasteCert()
{
	QClipboard *clipboard = QApplication::clipboard();
	ui->friendCertEdit->setPlainText(clipboard->text());
}

void ConnectFriendWizard::openCert()
{
	QString fileName ;
	if(!misc::getOpenFileName(this, RshareSettings::LASTDIR_CERT, tr("Select Certificate"), tr("RetroShare Certificate (*.rsc );;All Files (*)"),fileName))
		return ;

	if (!fileName.isNull()) {
		QFile fileCert(fileName);
		if (fileCert.open(QIODevice::ReadOnly )) {
			QByteArray arrayCert(fileCert.readAll());
			ui->friendCertEdit->setPlainText(QString::fromUtf8(arrayCert));
			fileCert.close();
		}
	}
}

void ConnectFriendWizard::saveCert()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save as..."), "", tr("RetroShare Certificate (*.rsc );;All Files (*)"));
	if (fileName.isEmpty())
		return;

	QFile file(fileName);
	if (!file.open(QFile::WriteOnly))
		return;

	//Todo: move save to file to p3Peers::SaveCertificateToFile

	QTextStream ts(&file);
	ts.setCodec(QTextCodec::codecForName("UTF-8"));
	ts << ui->userCertEdit->document()->toPlainText();
}

//========================== CertificatePage =================================

void ConnectFriendWizard::loadFriendCert()
{
    QString fileName ;
    if(!misc::getOpenFileName(this, RshareSettings::LASTDIR_CERT, tr("Select Certificate"), tr("RetroShare Certificate (*.rsc );;All Files (*)"),fileName))
            return ;

	if (!fileName.isNull()) {
		ui->friendFileNameEdit->setText(fileName);
	}
}

void ConnectFriendWizard::generateCertificateCalled()
{
#ifdef FRIEND_WIZARD_DEBUG
	std::cerr << "  generateCertificateCalled" << std::endl;
#endif

	std::string cert = rsPeers->GetRetroshareInvite();
	if (cert.empty()) {
		QMessageBox::information(this, "RetroShare", tr("Sorry, create certificate failed"), QMessageBox::Ok, QMessageBox::Ok);
		return;
	}

	QString qdir = QFileDialog::getSaveFileName(this, tr("Please choose a filename"), QDir::homePath(), tr("RetroShare Certificate (*.rsc );;All Files (*)"));

	//Todo: move save to file to p3Peers::SaveCertificateToFile

	if (qdir.isEmpty() == false) {
		QFile CertFile(qdir);
		if (CertFile.open(QIODevice::WriteOnly/* | QIODevice::Text*/)) {
			if (CertFile.write(QByteArray(cert.c_str())) > 0) {
				QMessageBox::information(this, "RetroShare", tr("Certificate file successfully created"), QMessageBox::Ok, QMessageBox::Ok);
			} else {
				QMessageBox::information(this, "RetroShare", tr("Sorry, certificate file creation failed"), QMessageBox::Ok, QMessageBox::Ok);
			}
			CertFile.close();
		} else {
			QMessageBox::information(this, "RetroShare", tr("Sorry, certificate file creation failed"), QMessageBox::Ok, QMessageBox::Ok);
		}
	}
}

#ifdef TO_BE_REMOVED
//============================= FofPage ======================================

void ConnectFriendWizard::updatePeersList(int index)
{

	ui->selectedPeersTW->clearContents();
	ui->selectedPeersTW->setRowCount(0);

	RsPgpId ownId = rsPeers->getGPGOwnId();

	int row = 0;

	_id_boxes.clear();

	// We have to use this trick because signers are given by their names instead of their ids. That's a cause
	// for some confusion when two peers have the same name.
	//
	std::list<RsPgpId> gpg_ids;
	rsPeers->getGPGAllList(gpg_ids);
	for (std::list<RsPgpId>::const_iterator it(gpg_ids.begin()); it != gpg_ids.end(); ++it) {
		if (*it == ownId) {
			// its me
			continue;
		}

#ifdef FRIEND_WIZARD_DEBUG
		std::cerr << "examining peer " << *it << " (name=" << rsPeers->getPeerName(*it);
#endif

		RsPeerDetails details ;
		if (!rsPeers->getGPGDetails(*it,details)) {
#ifdef FRIEND_WIZARD_DEBUG
			std::cerr << " no details." << std::endl ;
#endif
			continue;
		}

		// determine common friends

		std::list<RsPgpId> common_friends;

		for (std::list<RsPgpId>::const_iterator it2(details.gpgSigners.begin()); it2 != details.gpgSigners.end(); ++it2) {
			if(rsPeers->isGPGAccepted(*it2)) {
				common_friends.push_back(*it2);
			}
		}
		bool show = false;

		switch(index) {
		case 0: // "All unsigned friends of my friends"
			show = !details.ownsign;
#ifdef FRIEND_WIZARD_DEBUG
			std::cerr << "case 0: ownsign=" << details.ownsign << ", show=" << show << std::endl;
#endif
			break ;
		case 1: // "Unsigned peers who already signed my certificate"
			show = details.hasSignedMe && !(details.state & RS_PEER_STATE_FRIEND);
#ifdef FRIEND_WIZARD_DEBUG
			std::cerr << "case 1, ownsign=" << details.ownsign << ", is_authed_me=" << details.hasSignedMe << ", show=" << show << std::endl;
#endif
			break ;
		case 2: // "Peers shown as denied"
			show = details.ownsign && !(details.state & RS_PEER_STATE_FRIEND);
#ifdef FRIEND_WIZARD_DEBUG
			std::cerr << "case 2, ownsign=" << details.ownsign << ", state_friend=" << (details.state & RS_PEER_STATE_FRIEND) << ", show=" << show << std::endl;
#endif
			break ;
		}

		if (show) {
			ui->selectedPeersTW->insertRow(row);

			QCheckBox *cb = new QCheckBox;
			cb->setChecked(true);
			_id_boxes[cb] = details.id;
			_gpg_id_boxes[cb] = details.gpg_id;

			ui->selectedPeersTW->setCellWidget(row, 0, cb);
			ui->selectedPeersTW->setItem(row, 1, new QTableWidgetItem(QString::fromUtf8(details.name.c_str())));

			QComboBox *qcb = new QComboBox;

			if (common_friends.empty()) {
				qcb->addItem(tr("*** None ***"));
			} else {
				for (std::list<RsPgpId>::const_iterator it2(common_friends.begin()); it2 != common_friends.end(); ++it2) {
					qcb->addItem(QString::fromStdString( (*it2).toStdString()));
				}
			}

			ui->selectedPeersTW->setCellWidget(row, 2, qcb);
			ui->selectedPeersTW->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(details.id.toStdString())));
			++row;
		}
	}
#ifdef FRIEND_WIZARD_DEBUG
	std::cerr << "FofPage::updatePeersList() finished iterating over peers" << std::endl;
#endif

	if (row>0) {
		ui->selectedPeersTW->resizeColumnsToContents();
		ui->makeFriendButton->setEnabled(true);
	} else {
		ui->makeFriendButton->setEnabled(false);
	}
}

void ConnectFriendWizard::signAllSelectedUsers()
{
#ifdef FRIEND_WIZARD_DEBUG
	std::cerr << "making lots of friends !!" << std::endl;
#endif

	for (std::map<QCheckBox*, RsPeerId>::const_iterator it(_id_boxes.begin()); it != _id_boxes.end(); ++it) {
		if (it->first->isChecked()) {
#ifdef FRIEND_WIZARD_DEBUG
			std::cerr << "Making friend with " << it->second << std::endl ;
#endif
			//rsPeers->AuthCertificate(it->second, "");
			rsPeers->addFriend(it->second, _gpg_id_boxes[it->first]);
		}
	}

	ui->FofPage->setComplete(true);

	ui->userSelectionCB->setEnabled(false);
	ui->selectedPeersTW->setEnabled(false);
	ui->makeFriendButton->setEnabled(false);

	NotifyQt::getInstance()->notifyListChange(NOTIFY_LIST_NEIGHBOURS,0);
	NotifyQt::getInstance()->notifyListChange(NOTIFY_LIST_FRIENDS,0);
}

//============================= RsidPage =====================================


//============================ Emailpage =====================================
#endif

//========================= ErrorMessagePage =================================

//========================== ConclusionPage ==================================

void ConnectFriendWizard::setGroup(const std::string &id)
{
	groupId = QString::fromStdString(id);
}

void ConnectFriendWizard::groupCurrentIndexChanged(int index)
{
	QComboBox *comboBox = dynamic_cast<QComboBox*>(sender());
	if (comboBox) {
		groupId = comboBox->itemData(index, Qt::UserRole).toString();
	}
}

//========================== WebMailPage ==================================

void ConnectFriendWizard::inviteGmail()
{
    QDesktopServices::openUrl(QUrl("https://mail.google.com/mail/?view=cm&fs=1&su=" + subject + "&body=" + body , QUrl::TolerantMode));
}

void ConnectFriendWizard::inviteYahoo()
{
    QDesktopServices::openUrl(QUrl("http://compose.mail.yahoo.com/?&subject=" + subject + "&body=" + body, QUrl::TolerantMode));
}

void ConnectFriendWizard::inviteOutlook()
{
    QDesktopServices::openUrl(QUrl("http://mail.live.com/mail/EditMessageLight.aspx?n=&subject=" + subject + "&body=" + body, QUrl::TolerantMode));
}

void ConnectFriendWizard::inviteAol()
{
    QDesktopServices::openUrl(QUrl("http://webmail.aol.com/Mail/ComposeMessage.aspx?&subject=" + subject + "&body=" + body, QUrl::TolerantMode));
}

void ConnectFriendWizard::inviteYandex()
{
    QDesktopServices::openUrl(QUrl("https://mail.yandex.com/neo2/#compose/subject=" + subject + "&body=" + body, QUrl::TolerantMode));
}

void ConnectFriendWizard::runEmailClient2()
{
	sendMail("", subject, body );
}

void ConnectFriendWizard::toggleAdvanced()
{
	if(AdvancedVisible)
	{
		ui->cp_Frame->hide();
		ui->toggleadvancedButton->setText("Show advanced options");
		AdvancedVisible=false;
	}
	else
	{
		ui->cp_Frame->show();
		ui->toggleadvancedButton->setText("Hide advanced options");
		AdvancedVisible=true;
	}
}
