/*******************************************************************************
 * gui/connect/ConnectFriendWizard.h                                           *
 *                                                                             *
 * Copyright (C) 2018 retroshare team <retroshare.project@gmail.com>           *
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

#ifndef CONNECTFRIENDWIZARD_H
#define CONNECTFRIENDWIZARD_H

#include <QWizard>
#include <retroshare/rspeers.h>
#include <QMap>

class QCheckBox;

namespace Ui {
class ConnectFriendWizard;
}

//============================================================================
//! A wizard for adding friends. Based on standard QWizard component

//! The process of adding friends follows this scheme:
//!         /-> Use text certificates \       /-> errorpage(if  went wrong)
//! intro -|                           |-> ->
//!         \-> Use *.pqi files      /        \-> fill peer details
//!

class ConnectFriendWizard : public QWizard
{
	Q_OBJECT

	Q_PROPERTY(QString bannerPixmap READ bannerPixmap WRITE setBannerPixmap)
	Q_PROPERTY(int titleFontSize READ titleFontSize WRITE setTitleFontSize)
	Q_PROPERTY(int titleFontWeight READ titleFontWeight WRITE setTitleFontWeight)
	Q_PROPERTY(QString titleColor READ titleColor WRITE setTitleColor)

public:
	enum Page { Page_Text, Page_Cert, Page_ErrorMessage, Page_Conclusion, Page_WebMail, Page_FriendRecommendations };

	ConnectFriendWizard(QWidget *parent = 0);
	~ConnectFriendWizard();

	void setCertificate(const QString &certificate, bool friendRequest);
	void setGpgId(const RsPgpId &gpgId, const RsPeerId &sslId, bool friendRequest);

	virtual bool validateCurrentPage();
	virtual int nextId() const;
	virtual void accept();

	void setGroup(const std::string &id);

	void setBannerPixmap(const QString &pixmap);
	QString bannerPixmap();
	void setTitleFontSize(int size);
	int titleFontSize();
	void setTitleFontWeight(int weight);
	int titleFontWeight();
	void setTitleColor(const QString &color);
	QString titleColor();

protected:
	void initializePage(int id);

private slots:
	/* TextPage */
	void updateOwnCert();
	void toggleSignatureState(bool doUpdate = true);
	void toggleFormatState(bool doUpdate = true);
	void runEmailClient();
	void runEmailClient2();
	void showHelpUserCert();
	void copyCert();
	void pasteCert();
	void openCert();
	void saveCert();
	void friendCertChanged();
	void cleanFriendCert();

	ServicePermissionFlags serviceFlags() const ;

	/* CertificatePage */
	void loadFriendCert();
	void generateCertificateCalled();

	/* ConclusionPage */
	void groupCurrentIndexChanged(int index);
	
	/* WebMailPage */
    void inviteGmail();
    void inviteYahoo();
    void inviteOutlook();
    void inviteAol();
    void inviteYandex();

	void toggleAdvanced();

private:
	// returns the translated error string for the error code (to be found in rspeers.h)
	QString getErrorString(uint32_t) ;
	void updateStylesheet();
	void setTitleText(QWizardPage *page, const QString &title);
	bool AdvancedVisible;
	
private:
	bool error;
	RsPeerDetails peerDetails;
	std::string mCertificate;

	/* Stylesheet */
	QString mBannerPixmap;
	int mTitleFontSize;
	int mTitleFontWeight;
	QString mTitleColor;
	QMap<QWizardPage*, QString> mTitleString;

	/* TextPage */
	QTimer *cleanfriendCertTimer;

	/* FofPage */
	std::map<QCheckBox*, RsPeerId> _id_boxes;
	std::map<QCheckBox*, RsPgpId> _gpg_id_boxes;

	/* ConclusionPage */
	QString groupId;
	
	/* WebMailPage */
  QString subject;
	QString body;

	Ui::ConnectFriendWizard *ui;
};

class ConnectFriendPage : public QWizardPage
{
friend class ConnectFriendWizard; // for access to registerField

	Q_OBJECT

public:
	ConnectFriendPage(QWidget *parent = 0);

	void setComplete(bool isComplete);
	virtual bool isComplete() const;

private:
	bool useComplete;
	bool complete;
};

#endif // CONNECTFRIENDWIZARD_H
