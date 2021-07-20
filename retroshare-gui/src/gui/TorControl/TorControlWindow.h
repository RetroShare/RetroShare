#include "ui_TorControlWindow.h"

class QTcpServer ;

namespace Tor {
	class HiddenService ;
	class TorManager ;
}

class TorControlDialog: public QWidget, public Ui::TorControlDialog
{
	Q_OBJECT

public:
	TorControlDialog(Tor::TorManager *tm,QWidget *parent =NULL);

	enum TorStatus {
		TOR_STATUS_UNKNOWN = 0x00,
		TOR_STATUS_OK      = 0x01,
		TOR_STATUS_FAIL    = 0x02
	};

	enum HiddenServiceStatus {
		HIDDEN_SERVICE_STATUS_UNKNOWN   = 0x00,	// no information known.
		HIDDEN_SERVICE_STATUS_FAIL      = 0x01, // some error occurred
		HIDDEN_SERVICE_STATUS_REQUESTED = 0x02, // one service at least has been requested. Still being tested.
		HIDDEN_SERVICE_STATUS_OK        = 0x03	// one service responds and has been tested
	};

	// Should be called multiple times in a loop until it returns something else than *_UNKNOWN

	TorStatus checkForTor(QString& error_msg) ;
	HiddenServiceStatus checkForHiddenService() ;

protected slots:
	void showLog();
	void statusChanged();
	void onIncomingConnection();

private:
	QString mErrorMsg ;
	HiddenServiceStatus mHiddenServiceStatus ;

	Tor::TorManager *mTorManager ;
	Tor::HiddenService *mHiddenService ;

	QTcpServer *mIncomingServer ;
};
