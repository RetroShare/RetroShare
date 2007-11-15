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

#ifndef BGBOARD_H
#define BGBOARD_H

#include <QWidget>
#include <QFrame>
#include <vector>

class BgEngine;

enum { checker0Color, checker1Color, point0Color, point1Color, feltColor, caseColor };
	


struct Move {
	int startPoint;
	int dieValue;
};

struct moveScore {
	std::vector<Move> moves;
	float score;
};

class BgBoard : public QFrame
{
	Q_OBJECT
			
	public:
		BgBoard( QWidget *parent = 0 );
		QSize sizeHint() const;
		int activePlayer();
		void setColor( int item, QColor color );
		QColor getColor( int item );
		void setPlayerType( int player, int opponent );
		int getPlayerType( int player );
		void setAutoRoll( int player, bool value );
		bool getAutoRoll( int player );
		
	public slots:
		void unsetSingleClick();
		void undo();
		void startNewGame();
		void hint();
	
	signals:
		void playerChanged( int player );
		void gameOver( int points );
		
	protected:
				
		void paintEvent( QPaintEvent *event );
		void paintBoard( QPainter &painter );
		void paintChecker( QPainter &painter );  // paint the checker being dragged
		void paintStaticCheckers( QPainter &painter );
		void paintDice( QPainter &painter, std::vector<int> dice );
		void paintCheck( QPainter &painter, QPoint &pos, int player );
		
		void mousePressEvent( QMouseEvent *event );
		void mouseMoveEvent( QMouseEvent *event );
		void mouseReleaseEvent( QMouseEvent *event );
		void mouseDoubleClickEvent( QMouseEvent *event );
	
	private:
		float board_height;
		float point_width;
		float point_height;
		float home_width;
		float board_width;
		float checker_size;
		float case_width;
		float die_size;
		float die_spot_size;
			
		int m_draggingChecker;  //when dragging a checker, the pip from which it came; -1 otherwise
		double checkerX, checkerY;
		
		void moveDone();
		int rollDie();
		void rollDice();
		void changePlayer();
		
		void error( QString string );
		
		void computeMove( int *points, std::vector<int> &dice, std::vector<moveScore> &moveList, int player );
		float pointsScore( int *points, int player );
		bool moveChecker( Move move );
		void moveComputer();
		
		std::vector<int> diceInitial;
		std::vector<int> diceLeft;
		std::vector<Move> movesMade;
		
		bool m_canBearOff [2];
		bool diceRolled;
		bool newGame;
		
		int playerType [2];
		bool autoRoll [2];
		
		void definePoints();
		int checkerHit( const QPoint &pos );
		QRect cellRect[28];
		QPoint checkerPosAllowed[28][5]; // allowed checker positions
		//only display 5 checkers on each pip. 
		QRect dieRect [2]; //where the dice are
		
		QPoint checkerPos;  // current checker position
		QPoint checkerPosInitial; // checker position before beginning move
		int PointCount [28]; //number of checkers on each pip
		int initialPointCount [28];
		int staticPointCount [28]; //this is what we draw
		
		QColor bgColor [6];
		QBrush bgBrush [6];
		
		int m_activePlayer;        //players are 0 and 1
		inline int activePlayerSign() { return ( 2 * m_activePlayer - 1 ) ; }  //their signs are -1 and 1 
		
		int minDieValue( std::vector<int> &dice );
		int maxDieValue( std::vector<int> &dice );
		std::vector<int>::iterator minDie( std::vector<int> &dice );
		std::vector<int>::iterator maxDie( std::vector<int> &dice );
		
		QTimer *m_ClickTimer;
		bool m_ClickTimeout;
		bool checkerBeingDragged;
		bool m_clicked; //button pressed down
		
		int maxMovePossible;
		
		float rand1, rand2, rand3, rand4;
		
		bool isValidSingleMove( int *points, Move move );
		bool isValidTotalMove( int *startpoints, std::vector<Move> &move, int maxMovePossible );
		bool movePossible( int *points, std::vector<int> &dice, int player );
		bool canBearOff( int *points, int player );
		int getMaxMovePossible( int *startpoints, std::vector<int> &dice, int player );
		int updateTemps( int *oldPoints, int *newPoints, std::vector<int> &oldDice, std::vector<int> &newDice, int startPoint, std::vector<int>::iterator &die, int player );
		void pushMove( int *newPoints, std::vector<Move> &move, std::vector<moveScore> &list );
		
};

#endif //BGBOARD_H


