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

#include "GameBoard.h"
#include <QtGui/QImage>
#include "CardPixmaps.h"
#include <QtGui/QResizeEvent>
#include <QtGui/QCursor>
#include <QtGui/QMessageBox>

#include "CardAnimationLock.h"

#include <iostream>

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
GameBoard::GameBoard(QWidget * pParent,
                     const QString & gameName,
                     const QString & gameSettingsId)
    :QGraphicsView(pParent),
     m_scene(),
     m_sToSAniMove(),
     m_dealAni(),
     m_gameName(gameName),
     m_gameSettingsId(gameSettingsId),
     m_gamePixmap(":/images/sol32x32.png"),
     m_undoStack(),
     m_redoStack(),
     m_helpFile(""),
     m_numColsOrRows(1),
     m_resizeType(ResizeByWidth),
     m_demoRunning(false),
     m_pDemoSrcPrev(NULL),
     m_pDemoDstPrev(NULL),
     m_demoCardsPrev(),
     m_demoCardAniTime(GameBoard::DemoNormalCardAniTime),
     m_stacksCreated(false)
{
    // set the background image
    this->setBackgroundBrush(QImage(":/images/greenfelt.png"));
    this->setCacheMode(QGraphicsView::CacheBackground);

    // add the scene to the view.
    // and set it to rescale.
    this->setScene(&m_scene);

    // turnoff the scroll bars on the view
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);


    // hook up the animation signal for stack to stack moves.  This will enable us 
    // to update things just like it was a drag and drop move
    this->connect(&m_sToSAniMove,SIGNAL(cardsMoved(CardMoveRecord)),
		  this,SLOT(slotCardsMoved(CardMoveRecord)));

    this->connect(&m_dealAni,SIGNAL(cardsMoved(CardMoveRecord)),
		  this,SLOT(slotCardsMoved(CardMoveRecord)));
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
GameBoard::~GameBoard()
{
    CardAnimationLock::getInst().setDemoMode(false);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void GameBoard::setCardResizeAlg(unsigned int colsOrRows,CardResizeType resizeType)
{
    if (colsOrRows<1)
    {
	colsOrRows=1;
    }
    
    m_numColsOrRows=colsOrRows;

    m_resizeType=resizeType;

    this->updateCardSize(this->size());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void GameBoard::updateCardSize(const QSize  & newSize)
{    
    if (GameBoard::ResizeByHeight==m_resizeType)
    {
	CardPixmaps::getInst().setCardHeight((newSize.height()-(LayoutSpacing*(m_numColsOrRows+1)))/m_numColsOrRows);
    }
    else
    {
	CardPixmaps::getInst().setCardWidth((newSize.width()-(LayoutSpacing*(m_numColsOrRows+1)))/m_numColsOrRows);
    }
}
////////////////////////////////////////////////////////////////////////////////
// ok go through and restore the last state of all stacks
////////////////////////////////////////////////////////////////////////////////
void GameBoard::undoMove()
{
    if (!this->m_undoStack.empty())
    {
        CardMoveRecord moveRecord(m_undoStack.top());

        CardStack::processCardMoveRecord(CardStack::UndoMove,m_undoStack.top());
        m_undoStack.pop();

        // if we no longer have any undo info let users know.
        if (!this->canUndoMove())
        {
            emit undoAvail(false);
        }

        bool canRedo=this->canRedoMove();

        m_redoStack.push(moveRecord);

        // if we didn't have a redo previously and now have one
        // emit a signal to tell listeners about it.
        if (!canRedo)
        {
            emit redoAvail(true);
        }

        // calc the score with the changes
        this->calcScore();
    }
}

////////////////////////////////////////////////////////////////////////////////
// ok go through and redo the last state of all stacks
////////////////////////////////////////////////////////////////////////////////
void GameBoard::redoMove()
{
    if (!this->m_redoStack.empty())
    {
        CardMoveRecord moveRecord(m_redoStack.top());

        CardStack::processCardMoveRecord(CardStack::RedoMove,m_redoStack.top());
        m_redoStack.pop();

        // emit a signal that we are out of redo info
        if (!this->canRedoMove())
        {
            emit redoAvail(false);
        }

        bool canUndo=this->canUndoMove();
        m_undoStack.push(moveRecord);

        // emit a signal that we now have undo info
        if (!canUndo)
        {
            emit undoAvail(true);
        }

        // calc the score with the changes
        this->calcScore();
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool GameBoard::canUndoMove() const
{
    return !m_undoStack.empty();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool GameBoard::canRedoMove() const
{
    return !m_redoStack.empty();
}

////////////////////////////////////////////////////////////////////////////////
// adding some to the undo stack also means that the redo stack is cleared.
// since
////////////////////////////////////////////////////////////////////////////////
void GameBoard::addUndoMove(const CardMoveRecord &moveRecord)
{
    bool canUndo=this->canUndoMove();

    m_undoStack.push(moveRecord);

    // send a signal that we now have undo info
    if (!canUndo)
    {
        emit undoAvail(true);
    }

    // clear the redo stack because we have made a move
    while(!m_redoStack.empty())
    {
        m_redoStack.pop();
    }

    // since we have made a move the redo stack is now gone.
    emit redoAvail(false);

    // calc the score with the changes
    this->calcScore();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void GameBoard::clearUndoRedoStacks()
{
    while (!this->m_undoStack.empty())
    {
        m_undoStack.pop();
    }

    while (!this->m_redoStack.empty())
    {
        m_redoStack.pop();
    }

    emit undoAvail(false);
    emit redoAvail(false);

    // calc the score with the changes
    this->calcScore();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void GameBoard::restartGame()
{
    this->stopDemo();
    while (!this->m_undoStack.empty())
    {
        CardMoveRecord moveRecord(m_undoStack.top());

        CardStack::processCardMoveRecord(CardStack::UndoMove,m_undoStack.top());
        m_undoStack.pop();
    }

    this->clearUndoRedoStacks();

    // update the screen for the new layout
    this->update();

    // calc the score with the changes
    this->calcScore();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void GameBoard::showHint()
{
    unsigned int index;
    CardStack * pFromStack=NULL;
    CardStack * pToStack=NULL;

    // show hint if we found one.
    if (this->getHint(pFromStack,index,pToStack) && pToStack && pFromStack)
    {
        CardStack::showHint(pFromStack,index,pToStack);
    }
    else
    {
        QMessageBox::critical(this,this->gameName(),
                              tr("No move found involving full sets of movable cards.  "
                                 "It might be possible to make a move not using the full set of movable cards.").trimmed());
    }
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void GameBoard::startDemo(GameBoard::DemoCardAniTime demoCardAniTime)
{
    if (!m_demoRunning)
    {
	m_pDemoSrcPrev=NULL;
	m_pDemoDstPrev=NULL;
	m_demoCardsPrev.clear();

	m_demoCardAniTime=demoCardAniTime;

	m_demoRunning=true;
	CardAnimationLock::getInst().setDemoMode(true);
	emit demoStarted();
	runDemo();
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void GameBoard::stopDemo()
{
    if (m_demoRunning)
    {
	m_demoRunning=false;
	CardAnimationLock::getInst().setDemoMode(false);
	emit demoStopped();
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void GameBoard::newGame()
{
    m_dealAni.stopAni();
    m_sToSAniMove.stopAni();

    stopDemo();

    CardStack::clearAllStacks();
    CardStack::updateAllStacks();

    // clear the undo and redo info
    this->clearUndoRedoStacks();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void GameBoard::slotCardsMoved(const CardMoveRecord & moveRecord)
{
    // calc the score with the changes
    this->calcScore();

    // if the move record is not empty add it.
    if (!moveRecord.empty())
    {
	this->addUndoMove(moveRecord);
    }

    if (this->hasDemo()&& this->isDemoRunning())
    {
	runDemo();
    }
    // if all suits are sent home the game is over.  Show a dialog stating that
    // and start the game over.
    if (this->isGameWon())
    {
        QMessageBox::information(this,this->gameName(),tr("Congratulations you won!").trimmed());
        this->newGame();
    }
    else if (CardAnimationLock::getInst().animationsEnabled() && 
	     this->hasDemo() && !this->isDemoRunning() && 
	     this->isGameWonNotComplete())
    {
	this->startDemo(DemoEndGameCardAniTime);
    }

}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void GameBoard::resizeEvent (QResizeEvent * event)
{
    QGraphicsView::resizeEvent(event);

    this->updateCardSize(event->size());
    this->m_scene.setSceneRect(QRectF(QPointF(0,0),event->size()));

    if (!m_stacksCreated)
    {
	m_stacksCreated=true;
	this->createStacks();
    }

    CardStack::updateAllStacks();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool GameBoard::runDemo(bool stopWhenNoMore)
{
    bool rc=false;
    unsigned int index;
    CardStack * pFromStack=NULL;
    CardStack * pToStack=NULL;
    PlayingCardVector moveCards;
    CardMoveRecord    moveRecord;

    // here we are looking to stop moving cards back and forth over and over
    // between two stacks
    if (this->getHint(pFromStack,index,pToStack) && pToStack && pFromStack)
    {
	rc=true;
	if (pFromStack->removeCardsStartingAt(index,moveCards,moveRecord,true))
	{
	    if (m_pDemoSrcPrev==pToStack &&
		m_pDemoDstPrev==pFromStack &&
		m_demoCardsPrev==moveCards)
	    {
		rc=false;
	    }
	}
    }

    // if there is a hint perform the move
    if (rc)
    {
	pToStack->addCards(moveCards,moveRecord,true);
	
	m_pDemoSrcPrev=pFromStack;
	m_pDemoDstPrev=pToStack;
	m_demoCardsPrev=moveCards;
	
	this->m_sToSAniMove.moveCards(moveRecord,m_demoCardAniTime);
    }
    else if (stopWhenNoMore)
    {
	stopDemo();
	rc=false;
    }
    else
    {
	rc=false;
    }

    return rc;
}
