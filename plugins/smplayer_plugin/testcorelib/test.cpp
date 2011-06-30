/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2008 Ricardo Villalba <rvm@escomposlinux.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "test.h"
#include "smplayercorelib.h"
#include "helper.h"
#include "global.h"
#include <QAction>
#include <QMenuBar>
#include <QToolBar>
#include "timeslider.h"
#include <QFileDialog>

#include <QApplication>

Gui::Gui( QWidget * parent, Qt::WindowFlags flags ) 
	: QMainWindow(parent, flags)
{
	smplayerlib = new SmplayerCoreLib(this);
	core = smplayerlib->core();
	setCentralWidget(smplayerlib->mplayerWindow());

	QAction * openAct = new QAction( tr("&Open..."), this);
	connect( openAct, SIGNAL(triggered()), this, SLOT(open()) );

	QAction * closeAct = new QAction( tr("&Close"), this);
	connect( closeAct, SIGNAL(triggered()), this, SLOT(close()) );

	QMenu * open_menu = menuBar()->addMenu( tr("&Open") );
	open_menu->addAction(openAct);
	open_menu->addAction(closeAct);

	QAction * playAct = new QAction( tr("&Play/Pause"), this);
	playAct->setShortcut( Qt::Key_Space );
	connect( playAct, SIGNAL(triggered()), 
             core, SLOT(play_or_pause()) );

	QAction * stopAct = new QAction( tr("&Stop"), this);
	connect( stopAct, SIGNAL(triggered()), 
             core, SLOT(stop()) );

	QMenu * play_menu = menuBar()->addMenu( tr("&Play") );
	play_menu->addAction(playAct);
	play_menu->addAction(stopAct);


	TimeSlider * time_slider = new TimeSlider(this);
	connect( time_slider, SIGNAL(posChanged(int)), 
             core, SLOT(goToPos(int)) );
	connect( core, SIGNAL(posChanged(int)),
             time_slider, SLOT(setPos(int)) );

	QToolBar * control = new QToolBar( tr("Control"), this);
	control->addAction(playAct);
	control->addAction(stopAct);
	control->addSeparator();
	control->addWidget(time_slider);

	addToolBar(Qt::BottomToolBarArea, control);
}

Gui::~Gui() {
}

void Gui::closeEvent( QCloseEvent * event ) {
	core->stop();
	event->accept();
}

void Gui::open() {
	QString f = QFileDialog::getOpenFileName( this, tr("Open file") );

	if (!f.isEmpty()) {
		core->open(f);
	}
}

int main( int argc, char ** argv ) {
	QApplication a( argc, argv );
	a.connect( &a, SIGNAL( lastWindowClosed() ), &a, SLOT( quit() ) );

    Helper::setAppPath( qApp->applicationDirPath() );
    Global::global_init();

	Gui * w = new Gui();
	w->show();

	int r = a.exec();
	Global::global_end();

	return r;
}

#include "moc_test.cpp"
