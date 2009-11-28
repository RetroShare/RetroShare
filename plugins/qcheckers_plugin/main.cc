#include <QApplication>
#include <QTranslator>
#include <QLocale>
#include <QDebug>
//#include <QPlastiqueStyle>


#include "toplevel.h"
#include "common.h"


int main(int argc, char *argv[])
{
    QApplication app(argc,argv);
//    app.setStyle(new QPlastiqueStyle);

    qDebug()
	<< "Your Locale:" << QLocale::system().name() << endl
	<< "Prefix path:" << PREFIX;

    // Qt translations
    QTranslator qt_tr;
    if(qt_tr.load("qt_" + QLocale::system().name()))
	app.installTranslator(&qt_tr);
    else
	qDebug() << "Loading Qt translations failed.";

    // App translations
    QTranslator app_tr;
    if(app_tr.load("kcheckers_" + QLocale::system().name(),
		PREFIX"/share/kcheckers"))
	app.installTranslator(&app_tr);
    else
	qDebug() << "Loading KCheckers translations failed.";

    myTopLevel* top = new myTopLevel();
    top->show();

    // command line
    if(app.argc()==2)
	top->open(app.argv()[1]);

    int exit = app.exec();

    delete top;
    return exit;
}


