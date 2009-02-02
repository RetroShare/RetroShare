
#include "smplayercorelib.h"
#include "global.h"
#include "helper.h"
#include "preferences.h"

#include <QApplication>

int main( int argc, char ** argv ) {
	QApplication a( argc, argv );
	a.connect( &a, SIGNAL( lastWindowClosed() ), &a, SLOT( quit() ) );

    Helper::setAppPath( qApp->applicationDirPath() );
    Global::global_init();

	Global::pref->vo = "x11";

	SmplayerCoreLib * player1 = new SmplayerCoreLib;
	player1->mplayerWindow()->show();
	player1->mplayerWindow()->resize(624,352);
	player1->core()->openFile("video1.avi");

	SmplayerCoreLib * player2 = new SmplayerCoreLib;

	player2->mplayerWindow()->show();
	player2->mplayerWindow()->resize(624,352);
	player2->core()->openFile("video2.avi");

	int r = a.exec();
	Global::global_end();

	return r;
}
