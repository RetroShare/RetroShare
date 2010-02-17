/*
    QSoloCards is a collection of Solitaire card games written using Qt
    Copyright (C) 2009  Steve Moore

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "mainwindow.h"

#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>
#include <QtGui/QMenu>
#include <QtGui/QAction>
#include <QtGui/QMessageBox>
#include <QtCore/QTimer>

#include "CardAnimationLock.h"

const unsigned int MainWindow::MaxWidth=780;
const unsigned int MainWindow::MaxHeight=900;
const QString MainWindow::SizeStr("size");
const QString MainWindow::PtStr("point");
const QString MainWindow::HelpStr("help");
const QString MainWindow::GameIdStr("LastGame");
const QString MainWindow::AnimationStr("EnableAnimations");

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_pGameBoard(NULL),
      m_pMenuBar(NULL),
      m_pGameOptionsMenu(NULL),
      m_pHelpMenu(NULL),
      m_settings("QSoloCards","QSoloCards"),
      m_pNewGameAction(NULL),
      m_pRestartAction(NULL),
      m_pUndoAction(NULL),
      m_pRedoAction(NULL),
      m_pHintAction(NULL),
      m_pAnimationAction(NULL),
      m_pDemoAction(NULL),
      m_pCheatAction(NULL),
      m_pStatusBar(NULL),
      m_pStatusBarLabel(NULL),
      m_helpWindow(NULL),
      m_aboutWindow(NULL),
      m_gameMgr(),
      m_firstShow(false)
{
    // make the window a nice size and make sure it is on the screen
    QSize deskSize(QApplication::desktop()->size());

    unsigned int width=MaxWidth<(unsigned int)deskSize.width()?MaxWidth:deskSize.width()-100;
    unsigned int height=MaxHeight<(unsigned int)deskSize.height()?MaxHeight:deskSize.height()-100;

    int locX=(deskSize.width()-width)/2;
    int locY=(deskSize.height()-height)/2;

    this->resize(m_settings.value(SizeStr, QSize(width, height)).toSize());
    this->move(m_settings.value(PtStr, QPoint(locX, locY)).toPoint());

    // create the status bar we will use to show the score and game info.
    this->m_pStatusBar=new QStatusBar;
    this->m_pStatusBarLabel=new QLabel;
    this->m_pStatusBar->addPermanentWidget(m_pStatusBarLabel);
    this->setStatusBar(this->m_pStatusBar);

    // show the last game played or the default game if we don't have a last played.
    // we need to create the board before adding the menus.  This is done.  So, the
    // initial check state of the game selected will reflect the initial game.
    int gameId=m_settings.value(GameIdStr, GameMgr::DefaultGame).toInt();
    GameBoard * pGameBoard=m_gameMgr.getGame((GameMgr::GameId)gameId);

    // add the menus for the window
    this->addMenuItems();

    // finally setup the board.
    this->setupGameBoard(pGameBoard);
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
MainWindow::~MainWindow()
{
    // save the current settings for the game
    this->m_settings.beginGroup(this->m_pGameBoard->gameSettingsId());
    this->m_pGameBoard->saveSettings(this->m_settings);
    this->m_settings.endGroup();

    // save the position of the game window
    this->m_settings.setValue(SizeStr,this->size());
    this->m_settings.setValue(PtStr,this->pos());
    
    // save the current game being played
    this->m_settings.setValue(GameIdStr,this->m_gameMgr.getGameId());

    // save if animation is enabled or not
    this->m_settings.setValue(AnimationStr,this->m_pAnimationAction->isChecked());
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::slotNewGame()
{
    if (NULL!=this->m_pGameBoard)
    {
        this->m_pGameBoard->newGame();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::slotRestartGame()
{
    if (NULL!=this->m_pGameBoard)
    {
        this->m_pGameBoard->restartGame();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::slotSelectGame(QAction * pAction)
{
    if (NULL!=pAction)
    {
        bool ok=false;
        int gameId=pAction->data().toInt(&ok);

        // if we got the gameid and it is not the same as the current game
        // set the game as the current.
        if (ok && gameId!=this->m_gameMgr.getGameId())
        {
            this->setupGameBoard(this->m_gameMgr.getGame((GameMgr::GameId)gameId));
        }
    }
}

void MainWindow::slotAnimation(bool checked)
{
    CardAnimationLock::getInst().enableAnimations(checked);

    if (this->m_pGameBoard->hasDemo() && checked)
    {
	this->m_pDemoAction->setEnabled(true);
    }
    else
    {
	this->m_pDemoAction->setEnabled(false);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::slotCheat(bool checked)
{
    if (NULL!=this->m_pGameBoard)
    {
        this->m_pGameBoard->setCheat(checked);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::slotUndo()
{
    if (NULL!=this->m_pGameBoard)
    {
        if (this->m_pGameBoard->canUndoMove())
        {
            this->m_pGameBoard->undoMove();
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::slotRedo()
{
    if (NULL!=this->m_pGameBoard)
    {
        if (this->m_pGameBoard->canRedoMove())
        {
            this->m_pGameBoard->redoMove();
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::slotHint()
{
    if (NULL!=this->m_pGameBoard)
    {
        this->m_pGameBoard->showHint();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::slotUndoAvail(bool avail)
{
    if (this->m_pUndoAction && !m_pDemoAction->isChecked())
    {
        this->m_pRestartAction->setEnabled(avail);

        this->m_pUndoAction->setEnabled(avail);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::slotRedoAvail(bool avail)
{
    if (this->m_pRedoAction)
    {
        this->m_pRedoAction->setEnabled(avail);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::slotScoreChanged(int score,const QString & info)
{
    if (this->m_pStatusBar && !m_pDemoAction->isChecked())
    {
        QString statusMsg(tr("%1    Score:  %2").arg(info).arg(QString::number(score)).trimmed());

        this->m_pStatusBarLabel->setText(statusMsg);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::slotAbout()
{
    if (NULL==this->m_aboutWindow)
    {
        this->m_aboutWindow=new About(this);
        connect(this->m_aboutWindow.data(),SIGNAL(showLink(QString)),
                this,SLOT(slotShowHelp(QString)));
    }
    this->m_aboutWindow->show();
    this->m_aboutWindow->activateWindow();
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::slotHelp()
{
    if (this->m_pGameBoard)
    {
        this->slotShowHelp(this->m_pGameBoard->helpFile());
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::slotHelpClosed(int finishCode)
{
    Q_UNUSED(finishCode);

    this->m_settings.beginGroup(HelpStr);
    this->m_settings.setValue(SizeStr,this->m_helpWindow->size());
    this->m_settings.setValue(PtStr,this->m_helpWindow->pos());
    this->m_settings.endGroup();
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::slotShowHelp(const QString & helpFile)
{
    if (NULL==this->m_helpWindow)
    {
        this->m_helpWindow=new Help(this,helpFile);
        this->m_settings.beginGroup(HelpStr);
        this->m_helpWindow->resize(this->m_settings.value(SizeStr,QSize(640,480)).toSize());
        this->m_helpWindow->move(this->m_settings.value(PtStr,QPoint(200,200)).toPoint());
        this->m_settings.endGroup();
        this->connect(this->m_helpWindow,SIGNAL(finished(int)),this,SLOT(slotHelpClosed(int)));
        this->m_helpWindow->show();
    }
    else
    {
        this->m_helpWindow->setHelpFile(helpFile);
        this->m_helpWindow->show();
        this->m_helpWindow->activateWindow();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::slotAnimationStarted()
{
   m_pRestartAction->setEnabled(false);
   m_pDemoAction->setEnabled(false);
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::slotAnimationComplete()
{
   m_pRestartAction->setEnabled(true);
   m_pDemoAction->setEnabled(true);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// this timeout is just to delay slightly calling newGame for a board.  So, the
// animation will start with the QGraphicsScene visible and the board will already be
// completely drawn.
/////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::slotNewGameTimer()
{
    if (this->m_pGameBoard)
    {
	this->m_pMenuBar->setEnabled(true);
	this->m_pGameBoard->newGame();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::slotDemoStarted()
{
    m_pNewGameAction->setEnabled(false);
    m_pRestartAction->setEnabled(false);
    m_pUndoAction->setEnabled(false);
    m_pRedoAction->setEnabled(false);
    m_pHintAction->setEnabled(false);
    m_pAnimationAction->setEnabled(false);
    m_pGameSelectMenu->setEnabled(false);
    m_pGameOptionsMenu->setEnabled(false);
    m_pCheatAction->setEnabled(false);

    m_pDemoAction->setChecked(true);
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::slotDemoStopped()
{
    m_pNewGameAction->setEnabled(true);
    m_pRestartAction->setEnabled(true);
    m_pHintAction->setEnabled(true);
    m_pAnimationAction->setEnabled(true);
    m_pGameSelectMenu->setEnabled(true);
    m_pGameOptionsMenu->setEnabled(true);
    m_pCheatAction->setEnabled(true);


    if (this->m_pGameBoard->canUndoMove())
    {
	m_pUndoAction->setEnabled(true);
    }

    if (this->m_pGameBoard->canRedoMove())
    {
	m_pRedoAction->setEnabled(true);
    }

    m_pDemoAction->setChecked(false);

}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::slotDemoActionItem(bool checked)
{
    if (this->m_pGameBoard)
    {
	if (checked)
	{
	    this->m_pGameBoard->startDemo();
	}
	else
	{
	    this->m_pGameBoard->stopDemo();
	}
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
// we are just using the showEvent to make sure that we don't start the animation of the dealing
// of cards for the game until the window has been shown.
/////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::showEvent(QShowEvent * pShowEvent)
{
    Q_UNUSED(pShowEvent);
    if (!m_firstShow && NULL!=this->m_pGameBoard)
    {
	this->m_pGameBoard->newGame();
	
	m_firstShow=true;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::addMenuItems()
{
    // create the menu for the app.
    this->m_pMenuBar=new QMenuBar(this);

    // create a game menu
    QMenu * pGameMenu=new QMenu(tr("Game").trimmed(),this->m_pMenuBar);

    m_pNewGameAction=new QAction(tr("New Game").trimmed(),pGameMenu);
    m_pNewGameAction->setShortcuts(QKeySequence::New);
    this->connect(m_pNewGameAction,SIGNAL(triggered()),
                  this,SLOT(slotNewGame()));
    pGameMenu->addAction(m_pNewGameAction);

    this->m_pRestartAction=new QAction(tr("Restart Game").trimmed(),pGameMenu);
    this->m_pRestartAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_R));
    this->connect(this->m_pRestartAction,SIGNAL(triggered()),
                  this,SLOT(slotRestartGame()));

    pGameMenu->addAction(this->m_pRestartAction);

    pGameMenu->addSeparator();

    this->m_pGameSelectMenu=new QMenu(tr("Select Game").trimmed(),pGameMenu);
    // add the available games to the menu.
    this->m_gameMgr.buildGameListMenu(*this->m_pGameSelectMenu);
    // connecting this one up a little different.  Since, we are building
    // a generic menu where we don't know what the contents are connect up
    // to the menus triggered event.  We can then get the action and from it's
    // user data the id of the game selected.
    this->connect(this->m_pGameSelectMenu,SIGNAL(triggered(QAction*)),
                  this,SLOT(slotSelectGame(QAction*)));

    pGameMenu->addMenu(this->m_pGameSelectMenu);

#if !(defined Q_WS_MAC)
    pGameMenu->addSeparator();
#endif


    QAction * pQuit=new QAction(tr("Quit").trimmed(),pGameMenu);
    pQuit->setMenuRole(QAction::QuitRole);
    pQuit->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q));
    this->connect(pQuit,SIGNAL(triggered()),
                  this,SLOT(close()));
    pGameMenu->addAction(pQuit);

    this->m_pMenuBar->addMenu(pGameMenu);

    ///////////////////////////////////////////////////////////////////////////
    // create a control menu
    // for the cheat and undo menu items
    ///////////////////////////////////////////////////////////////////////////
    QMenu * pCtrlMenu=new QMenu(tr("Control").trimmed(),this->m_pMenuBar);

    m_pUndoAction=new QAction(tr("Undo").trimmed(),pCtrlMenu);
    m_pUndoAction->setShortcuts(QKeySequence::Undo);
    this->connect(m_pUndoAction,SIGNAL(triggered()),
                  this,SLOT(slotUndo()));
    pCtrlMenu->addAction(m_pUndoAction);

    m_pRedoAction=new QAction(tr("Redo").trimmed(),pCtrlMenu);
    m_pRedoAction->setShortcuts(QKeySequence::Redo);
    this->connect(m_pRedoAction,SIGNAL(triggered()),
                  this,SLOT(slotRedo()));
    pCtrlMenu->addAction(m_pRedoAction);

    m_pHintAction=new QAction(tr("Hint").trimmed(),pCtrlMenu);
    m_pHintAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_H));
    this->connect(m_pHintAction,SIGNAL(triggered()),
                  this,SLOT(slotHint()));
    pCtrlMenu->addAction(m_pHintAction);

    pCtrlMenu->addSeparator();

    m_pAnimationAction=new QAction(tr("Animation").trimmed(),pCtrlMenu);
    m_pAnimationAction->setCheckable(true);
    this->connect(m_pAnimationAction,SIGNAL(triggered(bool)),
                  this,SLOT(slotAnimation(bool)));
    pCtrlMenu->addAction(m_pAnimationAction);

    m_pAnimationAction->setChecked(m_settings.value(AnimationStr, true).toBool());

    CardAnimationLock::getInst().enableAnimations(m_pAnimationAction->isChecked());


    m_pDemoAction=new QAction(tr("Demo").trimmed(),pCtrlMenu);
    m_pDemoAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_D));
    m_pDemoAction->setCheckable(true);
    this->connect(m_pDemoAction,SIGNAL(triggered(bool)),
                  this,SLOT(slotDemoActionItem(bool)));
    pCtrlMenu->addAction(m_pDemoAction);


    pCtrlMenu->addSeparator();

    m_pCheatAction=new QAction(tr("Cheat").trimmed(),pCtrlMenu);
    m_pCheatAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_C));
    m_pCheatAction->setCheckable(true);
    this->connect(m_pCheatAction,SIGNAL(triggered(bool)),
                  this,SLOT(slotCheat(bool)));
    pCtrlMenu->addAction(m_pCheatAction);


    this->m_pMenuBar->addMenu(pCtrlMenu);


    ///////////////////////////////////////////////////////////////////////////
    // create a game options menu
    // this menu will get the specific game option settings to add to the menu
    // from the GameBoard class.  We will not add it to the menu at this time.
    // It will be added and/or removed when the game board is setup.  It will
    // only be added if it is needed.
    ///////////////////////////////////////////////////////////////////////////
    m_pGameOptionsMenu=new QMenu(tr("&Options").trimmed(),this->m_pMenuBar);

    ////////////////////////////////////////////////////////////////////////////
    // add the help menu
    ////////////////////////////////////////////////////////////////////////////
    this->m_pHelpMenu=new QMenu(tr("&Help").trimmed(),this->m_pMenuBar);

    QAction * pHelp=new QAction(tr("Game Help").trimmed(),this->m_pHelpMenu);
    pHelp->setShortcut(QKeySequence(Qt::Key_F1));
    this->connect(pHelp,SIGNAL(triggered()),
                  this,SLOT(slotHelp()));
    this->m_pHelpMenu->addAction(pHelp);

#if !(defined Q_WS_MAC)
    this->m_pHelpMenu->addSeparator();
#endif

    QAction * pAbout=new QAction(tr("&About").trimmed(),this->m_pHelpMenu);
    pAbout->setMenuRole(QAction::AboutRole);
    this->connect(pAbout,SIGNAL(triggered()),
                  this,SLOT(slotAbout()));
    this->m_pHelpMenu->addAction(pAbout);

    this->m_pMenuBar->addMenu(this->m_pHelpMenu);



    this->setMenuBar(this->m_pMenuBar);

    // also go ahead and hook up the slots for the signals that animation is in progress
    // now only used by the restart game menu item.  Need the undo stack to be updated before
    // we can restart a game.
    connect(&CardAnimationLock::getInst(),SIGNAL(animationStarted()),
            this,SLOT(slotAnimationStarted()));
    
    connect(&CardAnimationLock::getInst(),SIGNAL(animationComplete()),
            this,SLOT(slotAnimationComplete()));
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Set a gameboard and hook up the menus, set the icon, etc... for the new board
// if there was a previous gameboard.  When the QMainWindow::setCentralWidget function is called
// the old game board will be deleted by Qt.
/////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::setupGameBoard(GameBoard * pGameBoard)
{
    if (pGameBoard)
    {
	// save settings of the current game if we have one
	if (NULL!=this->m_pGameBoard)
	{
	    this->m_settings.beginGroup(this->m_pGameBoard->gameSettingsId());
	    this->m_pGameBoard->saveSettings(this->m_settings);
	    this->m_settings.endGroup();
	}

        this->m_pGameBoard=pGameBoard;

        // connect the gameboards score changed signal to our slot so we
        // can get score changes and display them.  But first clear the
        // current contents of the statusbar to get ride of old info if necessary.
        this->m_pStatusBar->showMessage("");
        this->m_pStatusBarLabel->setText("");
        this->connect(this->m_pGameBoard,SIGNAL(scoreChanged(int,QString)),
                      this,SLOT(slotScoreChanged(int,QString)));

        this->m_settings.beginGroup(this->m_pGameBoard->gameSettingsId());
        this->m_pGameBoard->loadSettings(m_settings);
        this->m_settings.endGroup();


        this->setCentralWidget(this->m_pGameBoard);
        this->setWindowTitle(tr("QSoloCards:  %1").arg(this->m_pGameBoard->gameName()).trimmed());

        // connect up the game board to the menus
        m_pUndoAction->setEnabled(this->m_pGameBoard->canUndoMove());
        m_pRedoAction->setEnabled(this->m_pGameBoard->canRedoMove());
        m_pRestartAction->setEnabled(this->m_pGameBoard->canUndoMove());

        this->m_pMenuBar->removeAction(this->m_pGameOptionsMenu->menuAction());

        this->m_pGameOptionsMenu->clear();
        this->m_pGameBoard->addGameMenuItems(*this->m_pGameOptionsMenu);

        if (!this->m_pGameOptionsMenu->isEmpty())
        {
            this->m_pMenuBar->insertMenu(this->m_pHelpMenu->menuAction(),this->m_pGameOptionsMenu);
        }

        // set the window icon for the game.
        this->setWindowIcon(this->m_pGameBoard->getGamePixmap());


        ///////////////////////////////////////////////////////////////////////////
        // connect up the slots so we will know when undo and redo actions are available
        ///////////////////////////////////////////////////////////////////////////
        this->connect(this->m_pGameBoard,SIGNAL(undoAvail(bool)),
                      this,SLOT(slotUndoAvail(bool)));

        this->connect(this->m_pGameBoard,SIGNAL(redoAvail(bool)),
                      this,SLOT(slotRedoAvail(bool)));

	if (this->isVisible())
	{
	    // disable the menus while we wait for the timer to pop.
	    this->m_pMenuBar->setEnabled(false);
	    QTimer::singleShot(100,this,SLOT(slotNewGameTimer()));
	}

	this->m_pDemoAction->setChecked(false);

	if (this->m_pGameBoard->hasDemo() && CardAnimationLock::getInst().animationsEnabled())
	{
	    this->m_pDemoAction->setEnabled(true);

	    this->connect(this->m_pGameBoard,SIGNAL(demoStarted()),
			  this,SLOT(slotDemoStarted()));
	    
	    this->connect(this->m_pGameBoard,SIGNAL(demoStopped()),
			  this,SLOT(slotDemoStopped()));
	}
	else
	{
	    this->m_pDemoAction->setEnabled(false);
	}

    }
}

