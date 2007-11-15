/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2007, RetroShare Team
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#include <QtGui>

#include "bgwindow.h"
#include "bgwidget.h"

BgWindow::BgWindow(QWidget* parent, Qt::WFlags flags)
    : QMainWindow(parent, flags)
{
	bgWidget = new BgWidget;
	setCentralWidget( bgWidget );
	
	createActions();
	createMenus();
	statusBar()->showMessage(tr("Ready"));
	
	setWindowTitle(tr("QBackgammon"));
	resize(600,600);
}

/** Destructor. */
BgWindow::~BgWindow()
{

}

void BgWindow::createActions()
{
	newAct = new QAction( tr( "&New Game"), this );
	newAct->setShortcut( tr( "Ctrl+O") );
	undoAct = new QAction( tr( "&Undo" ), this );
	undoAct->setShortcut( tr( "Ctrl+Z") );
	resignAct = new QAction( tr( "Resign" ), this );
	optionsAct = new QAction( tr( "Options" ), this );
	hintAct = new QAction( tr( "Hint" ), this );
	quitAct = new QAction( tr( "Quit" ), this );
	aboutAct = new QAction( tr( "About" ), this );
	
	connect( newAct, SIGNAL( triggered() ), this, SLOT(newGame() ) );
	connect( undoAct, SIGNAL( triggered() ), bgWidget, SLOT( undo() ));
	connect( resignAct, SIGNAL( triggered() ), this, SLOT( resign() ) );
	connect( optionsAct, SIGNAL( triggered() ), bgWidget, SLOT( options() ) ); 
	connect( hintAct, SIGNAL( triggered() ), bgWidget, SLOT( hint() ) );
	connect( quitAct, SIGNAL( triggered() ), this, SLOT( quit() ) );
	connect( aboutAct, SIGNAL( triggered() ), this, SLOT( about() ) );
	connect( this, SIGNAL( newGameCalled() ), bgWidget, SLOT( newGame() ) );
}

void BgWindow::createMenus()
{
	fileMenu = new QMenu(tr("&File"), this );
	fileMenu->addAction( newAct);
	fileMenu->addAction( undoAct );
	fileMenu->addAction( resignAct );
	fileMenu->addAction( optionsAct );
	fileMenu->addAction( hintAct );
	fileMenu->addAction( quitAct );
	fileMenu->addAction( aboutAct );
	
	menuBar()->addMenu(fileMenu);
}

void BgWindow::newGame()
{
	//FIXME check to see if this is a brand new game first
	QMessageBox::StandardButton reply;
	reply = QMessageBox::question(this, tr("QBackgammon"),
					tr("Are you sure you want to start a new game?\n"
							"The current one will be lost."),
					QMessageBox::Yes | QMessageBox::No );
	if (reply == QMessageBox::Yes) {
		bgWidget->newGame();
	}

}

void BgWindow::undo()
{
}

void BgWindow::resign()
{
	QMessageBox::StandardButton reply;	
	reply = QMessageBox::question(this, tr("QBackgammon"),
					tr("Are you sure you want to resign?"),
			QMessageBox::Yes | QMessageBox::No );
	if ( reply == QMessageBox::Yes ) {
		bgWidget->changeScore( -1 );
		bgWidget->newGame();
	}
}

void BgWindow::quit()
{
	QMessageBox::StandardButton reply;	
	reply = QMessageBox::question(this, tr("QBackgammon"),
					tr("Are you sure you want to quit?"),
					QMessageBox::Yes | QMessageBox::No );
	if ( reply == QMessageBox::Yes ) {
		emit callQuit();
	}
	

	
}

void BgWindow::about()
{
	QMessageBox::StandardButton reply;
	reply = QMessageBox::information( this, tr( "QBackgammon" ),
					    tr("QBackgammon\n(C) Daren Sawkey 2006\ndaren@sawkey.net\nReleased under GPL"),
					    QMessageBox::Ok );
	return;
}
							
void BgWindow::show()
{

  if(!this->isVisible()) {
    QMainWindow::show();
  } else {
    QMainWindow::activateWindow();
    setWindowState(windowState() & ~Qt::WindowMinimized | Qt::WindowActive);
    QMainWindow::raise();
  }
  
}

void BgWindow::closeEvent (QCloseEvent * event)
{
    hide();
    event->ignore();
}