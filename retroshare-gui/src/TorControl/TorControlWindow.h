#include "ui_TorControlWindow.h"
#include "TorManager.h"

class TorControlDialog: public QDialog, public Ui::TorControlDialog
{
	Q_OBJECT

public:
	TorControlDialog(Tor::TorManager *tm,QWidget *parent =NULL);

protected slots:
	void showLog();
	void statusChanged();
	void checkForHiddenService();

private:
	void setupHiddenService();

	Tor::TorManager *mTorManager ;
};
