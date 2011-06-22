#ifndef DHTWINDOW_H
#define DHTWINDOW_H

#include <QMainWindow>
#include "peernet.h"

#include "dhtquery.h"

namespace Ui {
    class DhtWindow;
}

class DhtWindow : public QMainWindow {
    Q_OBJECT
public:
    DhtWindow(QWidget *parent = 0);
    ~DhtWindow();

	void setPeerNet(PeerNet *pnet);
	void setDhtQuery(DhtQuery *qw);
	
	void updateDhtPeers();
	void updateDhtQueries();

public slots:
	void update();
	void setQueryId();
	void showQuery();

	
protected:
    void changeEvent(QEvent *e);

private:
    Ui::DhtWindow *ui;
    PeerNet *mPeerNet;
    DhtQuery *mQueryWindow;
};

#endif // DHTWINDOW_H
