#include <QtGui/QApplication>
#include "mainwindow.h"
#include "peernet.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
	
	
 #ifdef __APPLE__
	PeerNet pnet("", "../..", 0);  // This is because of the apple APP structure.
 #else
	PeerNet pnet("", ".", 0);
 #endif
	
	
    MainWindow w;
    w.show();
	
	w.setPeerNet(&pnet);
	
	
    return a.exec();
}
