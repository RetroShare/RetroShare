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
	void sendChat();
	void updateChat();

public slots:
	void update();
	void addPeer();
	
protected:
    void changeEvent(QEvent *e);

private:
    Ui::MainWindow *ui;
	PeerNet *mPeerNet;
};

#endif // MAINWINDOW_H
