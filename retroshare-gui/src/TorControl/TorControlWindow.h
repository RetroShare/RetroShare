#include "ui_TorControlWindow.h"

class QTcpServer ;

namespace Tor {
	class HiddenService ;
	class TorManager ;
}

class TorControlDialog: public QDialog, public Ui::TorControlDialog
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
		HIDDEN_SERVICE_STATUS_UNKNOWN = 0x00,
		HIDDEN_SERVICE_STATUS_OK      = 0x01,
		HIDDEN_SERVICE_STATUS_FAIL    = 0x02
	};

	// Should be called multiple times in a loop until it returns something else than *_UNKNOWN

	TorStatus checkForTor() ;
	HiddenServiceStatus checkForHiddenService() ;

protected slots:
	void showLog();
	void statusChanged();
//	void checkForHiddenService();
	void onIncomingConnection();

private:
	void setupHiddenService();

	Tor::TorManager *mTorManager ;
	Tor::HiddenService *mHiddenService ;

	QTcpServer *mIncomingServer ;
};
