#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "peernet.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

	void setPeerNet(PeerNet *pnet);
	
	void updateNetStatus();
	void updateDhtPeers();
	void updateNetPeers();
	void updateChat();
	void addChatMsg(std::string id, std::string msg);

public slots:
	void update();
	void addPeer();
	void sendChat();
	
protected:
    void changeEvent(QEvent *e);

private:
    Ui::MainWindow *ui;
	PeerNet *mPeerNet;
};

#endif // MAINWINDOW_H
