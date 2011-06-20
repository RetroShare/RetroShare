#ifndef DHTWINDOW_H
#define DHTWINDOW_H

#include <QMainWindow>
#include "peernet.h"

namespace Ui {
    class DhtWindow;
}

class DhtWindow : public QMainWindow {
    Q_OBJECT
public:
    DhtWindow(QWidget *parent = 0);
    ~DhtWindow();

	void setPeerNet(PeerNet *pnet);
	
	void updateDhtPeers();
	void updateDhtQueries();

public slots:
	void update();
	
protected:
    void changeEvent(QEvent *e);

private:
    Ui::DhtWindow *ui;
    PeerNet *mPeerNet;
};

#endif // DHTWINDOW_H
