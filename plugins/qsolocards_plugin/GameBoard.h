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

#ifndef GAMEBOARD_H
#define GAMEBOARD_H

#include <QtGui/QGraphicsView>
#include <QtCore/QString>
#include <QtGui/QPixmap>
#include <QtGui/QMenu>
#include <QtCore/QSettings>
#include <QtGui/QGraphicsScene>

#include <stack>

#include "CardStack.h"
#include "CardMoveRecord.h"
#include "StackToStackAniMove.h"
#include "DealAnimation.h"

// Generic base class for a games board
class GameBoard : public QGraphicsView
{
    Q_OBJECT

public:
    enum
    {
	LayoutSpacing=10
    };

    enum CardResizeType
    {
	ResizeByWidth=0,
	ResizeByHeight=1
    };

    enum DemoCardAniTime
    {
	DemoEndGameCardAniTime=100,
	DemoNormalCardAniTime=400
    };

    GameBoard(QWidget * pWidget,
	      const QString & gameName,
	      const QString & gameSettingsId);
    virtual ~GameBoard();

    virtual void setGameName(const QString & gameName){m_gameName=gameName;}
    virtual void setGameId(const QString & gameId){ m_gameSettingsId=gameId;}
	
    // this function show be called in the constructor of classes inheriting from this
    // class to set the way that cards will be resized.  
    virtual void setCardResizeAlg(unsigned int colsOrRows,CardResizeType resizeType);

    // this function is called when setCardResizeAlg is called or when the GameBoard is resized.
    virtual void updateCardSize(const QSize & newSize);

    // undoMove and redoMove pop items off there respective stacks
    // and process the move.  They also call calcScore to update the
    // score.
    // signals undoAvail and redoAvail are also emitted if necessary
    virtual void undoMove();
    virtual void redoMove();

    virtual bool canUndoMove() const;
    virtual bool canRedoMove() const;

    // addUndoMove will add the move to the undo stack. clear the redo stack
    // and call calcScore.
    // signals undoAvail and redoAvail are also emitted if necessary
    virtual void addUndoMove(const CardMoveRecord &);

    // will clear all contents of both the redo and undo stacks, call calcScore, and
    // signals undoAvail and redoAvail are also emitted if necessary
    virtual void clearUndoRedoStacks();

    virtual void restartGame();

    virtual void showHint();

    // this function is split into a separate virtual function
    // it will allow the function to be used both for showing a
    // hint and for demo mode.
    virtual bool getHint(CardStack * & pSrcWidget,
			 unsigned int & srcStackIndex,
			 CardStack * & pDstWidget)=0;

    virtual void startDemo(DemoCardAniTime demoCardAniTime=DemoNormalCardAniTime);
    virtual void stopDemo();
    inline bool isDemoRunning() const { return m_demoRunning;}

    inline DemoCardAniTime getDemoCardAniTime()const{return m_demoCardAniTime;}
	
    // override this function if the game is implementing demo mode.
    // The runDemo() function will need to be called after 
    // each move.  Bestway would be to add something at the end of the slot
    // that catches the cardsMoved signal from the move animations.
    // By default runDemo will call getHint and if a move is returned it will
    // perform the move.  If something else is desired the function can be overridden.
    // it also returns a bool.  So, if it is overridden the base class can be called
    // first to see if there is a basic move.  If not something else can be done.
    virtual bool hasDemo() const {return false;}

    virtual void newGame();

    virtual bool isCheating() const=0;

    virtual void setCheat(bool cheat)=0;

    virtual void addGameMenuItems(QMenu &)=0;

    virtual void loadSettings(const QSettings & settings)=0;
    virtual void saveSettings(QSettings & settings)=0;

    virtual bool supportsScore() const=0;

    virtual const QString & helpFile() const { return m_helpFile;}

    const QPixmap & getGamePixmap() const{return m_gamePixmap;}

    const QString & gameName() const{return m_gameName;}

    const QString & gameSettingsId() const {return m_gameSettingsId;}

public slots:
    virtual void slotCardsMoved(const CardMoveRecord &);

signals:
    // this signal should be emitted on a state change of undo from available
    // to unavailable or unavailable to available by sub classes
    void undoAvail(bool avail);

    // this signal should be emitted on a state change of redo from available
    // to unavailable or unavailable to available by sub classes
    void redoAvail(bool avail);

    // signal emitted by a game when its score changed.
    // The string is extra info the game can pass to show
    // additional info.
    void scoreChanged(int score,const QString &);

	
    void demoStarted();
    void demoStopped();

protected:
    virtual void calcScore()=0;  // called when the score is changed by add to rewinding the undo and redo stacks
    // subclasses should override for customizing score calculations

    virtual void setHelpFile(const QString & helpFile){m_helpFile=helpFile;}

    virtual void resizeEvent (QResizeEvent * event);

    // called to create the CardStacks for the game.
    virtual void createStacks()=0;

    // this function can be overloaded for more complex behavior for the demo.  By default it
    // will just peform the hint returned if it found one.
    virtual bool runDemo(bool stopWhenNoMore=true);

    // implement in subclasses to determine if the game is won.
    virtual bool isGameWon()const=0;

    // implement in subclasses when cards are in position that the
    // game will be won.  When this function returns true the demo
    // will be started if it is available to move the remaining cards
    virtual bool isGameWonNotComplete()const=0;


    QGraphicsScene  m_scene;
    StackToStackAniMove   m_sToSAniMove;
    DealAnimation         m_dealAni;

private:
    QString m_gameName;
    QString m_gameSettingsId;
    QPixmap m_gamePixmap;

    std::stack <CardMoveRecord>  m_undoStack;  // stack to keep track of moves
    std::stack <CardMoveRecord>  m_redoStack;  // stack to keep track of moves

    QString m_helpFile;

    unsigned int m_numColsOrRows;
    CardResizeType  m_resizeType;

    bool m_demoRunning;

    CardStack * m_pDemoSrcPrev;
    CardStack * m_pDemoDstPrev;
    PlayingCardVector  m_demoCardsPrev;
    DemoCardAniTime    m_demoCardAniTime;
    
    bool m_stacksCreated; // variable to keep track if we have called createStacks
};

#endif // GAMEBOARD_H
