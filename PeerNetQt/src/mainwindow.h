#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "peernet.h"

#include "DhtWindow.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

	void setPeerNet(PeerNet *pnet);
	void setDhtWindow(DhtWindow *dw);
	
	void updateNetStatus();
	void updateNetPeers();
	void updateRelays();
	void updateChat();
	void addChatMsg(std::string id, std::string msg);

public slots:
	void update();
	void addPeer();
	void sendChat();
	void showDhtWindow();
	
protected:
    void changeEvent(QEvent *e);

private:
    Ui::MainWindow *ui;
	PeerNet *mPeerNet;
	DhtWindow *mDhtWindow;
};

#endif // MAINWINDOW_H
