#ifndef DHTQUERYWINDOW_H
#define DHTQUERYWINDOW_H

#include <QMainWindow>
#include "peernet.h"

namespace Ui {
    class DhtQuery;
}

class DhtQuery : public QMainWindow {
    Q_OBJECT
public:
    DhtQuery(QWidget *parent = 0);
    ~DhtQuery();

	void setPeerNet(PeerNet *pnet);
	void setQueryId(std::string id);
	
	void updateDhtQuery();

public slots:
	void update();
	
protected:
    void changeEvent(QEvent *e);

private:
    Ui::DhtQuery *ui;
    PeerNet *mPeerNet;
    std::string mQueryId;
};

#endif // DHTQUERYWINDOW_H
