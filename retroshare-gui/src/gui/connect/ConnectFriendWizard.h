#ifndef __ConnectFriendWizard__
#define __ConnectFriendWizard__

#include <QWizard>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QLabel;
class QTextEdit;
class QLineEdit;
class QRadioButton;
class QVBoxLayout;
class QHBoxLayout;
class QGroupBox;
class QGridLayout;
QT_END_NAMESPACE

//============================================================================
// 
class ConnectFriendWizard : public QWizard
{
// 
    Q_OBJECT

public:

    enum { Page_Intro, Page_Text, Page_Cert, Page_ErrorMessage,
           Page_Conclusion };

    ConnectFriendWizard(QWidget *parent = 0);

    void accept();

private slots:
    void showHelp();
//! 
};

//============================================================================
//! 
class IntroPage : public QWizardPage
{
    Q_OBJECT

public:
    IntroPage(QWidget *parent = 0);

    int nextId() const;

private:
    QLabel *topLabel;
    QRadioButton *textRadioButton;
    QRadioButton *certRadioButton;
};

//============================================================================
//! 
class TextPage : public QWizardPage
{
    Q_OBJECT

public:
    TextPage(QWidget *parent = 0);

    int nextId() const;

private:
    QLabel*      userCertLabel;
    QTextEdit*   userCertEdit;
    QHBoxLayout* userCertLayout;
    QVBoxLayout* userCertButtonsLayout;
    QPushButton* userCertHelpButton;
    #if defined(Q_OS_WIN)
    QPushButton* userCertMailButton;//! on Windows, click on this button
                                   //! launches default email client
    #endif
    QLabel*      friendCertLabel;
    QTextEdit*   friendCertEdit;
    
    QVBoxLayout* textPageLayout;

private slots:
    void showHelpUserCert();
    
    #if defined(Q_OS_WIN)
    //! launches default email client (on windows)
    
    //! Tested on Vista, it work normally... But a bit slowly.
    void runEmailClient();
    #endif
};

//============================================================================
//! 
class CertificatePage : public QWizardPage
{
    Q_OBJECT

public:
    CertificatePage(QWidget *parent = 0);

    int nextId() const;
    bool isComplete() const ;

private:
    QGroupBox* userFileFrame;
    QLabel *userFileLabel;
    QPushButton* userFileCreateButton;
    QHBoxLayout* userFileLayout;
    
    QLabel* friendFileLabel;
    QLineEdit *friendFileNameEdit;
    QPushButton* friendFileNameOpenButton;
    QHBoxLayout* friendFileLayout;

    QVBoxLayout* certPageLayout;    

private slots:
    void generateCertificateCalled();
    void loadFriendCert();
};

//============================================================================

class ErrorMessagePage : public QWizardPage
{
    Q_OBJECT

public:
    ErrorMessagePage(QWidget *parent = 0);

    int nextId() const;

private:
    QLabel *messageLabel;
    QVBoxLayout* errMessLayout;
};

//============================================================================
//! 
class ConclusionPage : public QWizardPage
{
    Q_OBJECT

public:
    ConclusionPage(QWidget *parent = 0);

    void initializePage();
    int nextId() const;
 //   void setVisible(bool visible);

private slots:
//    void printButtonClicked();

private:
    QGroupBox* peerDetailsFrame;
    QLabel* trustLabel;
    QLineEdit* trustEdit;
    QLabel* nameLabel;
    QLineEdit* nameEdit;
    QLabel* orgLabel;
    QLineEdit* orgEdit;
    QLabel* locLabel;
    QLineEdit* locEdit;
    QLabel* countryLabel;
    QLineEdit* countryEdit;
    QLabel* signersLabel;
    QTextEdit* signersEdit;
    QGridLayout* peerDetailsLayout;
    
    QLabel* authCodeLabel;
    QLineEdit* authCodeEdit;
    QHBoxLayout* authCodeLayout;

    QVBoxLayout* conclusionPageLayout;

    //! peer id

    //! It's a hack; This widget is used only to register "id" field in the
    //! wizard. Really the widget isn't displayed.
    QLineEdit* peerIdEdit;
};

//============================================================================

#endif
