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
	enum Page { Page_Intro, Page_Text, Page_Cert, Page_ErrorMessage, Page_Conclusion, Page_Foff, Page_Rsid, Page_WebMail, Page_Email, Page_FriendRequest, Page_FriendRecommendations };

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
	void saveCert();
	void friendCertChanged();
	void cleanFriendCert();

	ServicePermissionFlags serviceFlags() const ;

	/* CertificatePage */
	void loadFriendCert();
	void generateCertificateCalled();

	/* FofPage */
	void updatePeersList(int index);
	void signAllSelectedUsers();

	/* ConclusionPage */
	void groupCurrentIndexChanged(int index);
	
	/* WebMailPage */
    void inviteGmail();
    void inviteYahoo();
    void inviteOutlook();
    void inviteAol();
    void inviteYandex();


private:
	// returns the translated error string for the error code (to be found in rspeers.h)
	QString getErrorString(uint32_t) ;
	void updateStylesheet();
	void setTitleText(QWizardPage *page, const QString &title);

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
