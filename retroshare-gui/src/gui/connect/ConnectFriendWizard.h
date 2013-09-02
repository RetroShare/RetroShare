#ifndef CONNECTFRIENDWIZARD_H
#define CONNECTFRIENDWIZARD_H

#include <QWizard>
#include <retroshare/rspeers.h>
#include <map>

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

public:
	enum Page { Page_Intro, Page_Text, Page_Cert, Page_ErrorMessage, Page_Conclusion, Page_Foff, Page_Rsid, Page_Email, Page_FriendRequest, Page_FriendRecommendations };

	ConnectFriendWizard(QWidget *parent = 0);
	~ConnectFriendWizard();

	void setCertificate(const QString &certificate, bool friendRequest);
	void setGpgId(const std::string &gpgId, const std::string &sslId, bool friendRequest);

	virtual bool validateCurrentPage();
	virtual int nextId() const;
	virtual void accept();

	void setGroup(const std::string &id);

protected:
	void initializePage(int id);

private slots:
	/* TextPage */
	void updateOwnCert();
	void toggleSignatureState(bool doUpdate = true);
	void toggleFormatState(bool doUpdate = true);
	void runEmailClient();
	void showHelpUserCert();
	void copyCert();
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

private:
	// returns the translated error string for the error code (to be found in rspeers.h)
	QString getErrorString(uint32_t) ;

	bool error;
	RsPeerDetails peerDetails;
	std::string mCertificate;

	/* TextPage */
	QTimer *cleanfriendCertTimer;

	/* FofPage */
	std::map<QCheckBox*, std::string> _id_boxes;
	std::map<QCheckBox*, std::string> _gpg_id_boxes;

	/* ConclusionPage */
	QString groupId;

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
