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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui/QMainWindow>
#include <QtGui/QMenuBar>
#include <QtGui/QMenu>
#include <QtCore/QSettings>
#include <QtGui/QStatusBar>
#include <QtGui/QLabel>
#include <QtCore/QPointer>

#include "GameBoard.h"
#include "Help.h"
#include "About.h"
#include "GameMgr.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    static const unsigned int MaxWidth;
    static const unsigned int MaxHeight;
    static const QString SizeStr;
    static const QString PtStr;
    static const QString HelpStr;
    static const QString GameIdStr;
    static const QString AnimationStr;

    MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void slotNewGame();
    void slotRestartGame();
    void slotSelectGame(QAction *);
    void slotAnimation(bool);
    void slotCheat(bool);
    void slotUndo();
    void slotRedo();
    void slotHint();

    void slotUndoAvail(bool avail);
    void slotRedoAvail(bool avail);

    void slotScoreChanged(int score,const QString &);

    void slotHelp();
    void slotAbout();

    void slotHelpClosed(int);

    void slotShowHelp(const QString & helpFile);

    void slotAnimationStarted();
    void slotAnimationComplete();

    void slotNewGameTimer();

    void slotDemoStarted();
    void slotDemoStopped();

    void slotDemoActionItem(bool);
protected:
    void showEvent(QShowEvent * pShowEvent);

private:
    void addMenuItems();
    void setupGameBoard(GameBoard * pGameBoard);



    GameBoard *  m_pGameBoard;
    QMenuBar  *  m_pMenuBar;
    QMenu     *  m_pGameSelectMenu;
    QMenu     *  m_pGameOptionsMenu;
    QMenu     *  m_pHelpMenu;
    QSettings    m_settings;

    QAction   *  m_pNewGameAction;
    QAction   *  m_pRestartAction;
    QAction   *  m_pUndoAction;
    QAction   *  m_pRedoAction;
    QAction   *  m_pHintAction;
    QAction   *  m_pAnimationAction;
    QAction   *  m_pDemoAction;
    QAction   *  m_pCheatAction;

    QStatusBar * m_pStatusBar;
    QLabel     * m_pStatusBarLabel;
    QPointer<Help>  m_helpWindow;   // use a QPointer so we don't have to worry about knowing if
                                    // the help window has been deleted.  The Window auto deletes
                                    // when closes.
    QPointer<About> m_aboutWindow;
    GameMgr      m_gameMgr;
    bool         m_firstShow;
};

#endif // MAINWINDOW_H
