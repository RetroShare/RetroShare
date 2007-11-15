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
#include <QDateTime>
#include <QTimer>
#include <stdlib.h>
#include <vector>

#include "bgboard.h"

/* Labelling convention:
	players are 0 and 1
	points on board are 1 (top right) counterclockwise to 24 (bottom right)
	home is 1-6 for player 0, 19-24 for player 1
	delta = endpoint - startpoint < 0 for player 0
				  > 0 for player 1
	define activePlayerSign() = -1 for player 0, +1 for player 1 (same sign as delta)
	PointCount[i] is < 0 for player 0's checkers, >0 for player 1's checkers
	home board is 0 for player 0, 25 for player 1   ( ie 25 * activePlayer )
	centre board is 26 for player 0, 27 for player 1  ( ie 26 + activePlayer )
 */

using namespace std;

BgBoard::BgBoard( QWidget *parent )
	: QFrame( parent )
{
	setFrameStyle(QFrame::Panel | QFrame::Sunken);
	setFocusPolicy(Qt::StrongFocus);
	
	m_ClickTimeout = false;  // used to determine whether player clicks or drags
	
	definePoints();
	
	setColor( 0, Qt::red ); //FIXME enum
	setColor( 1, Qt::blue );
	setColor( 2, QColor( 128, 115, 130 ) );
	setColor( 3, QColor( 200, 66, 66 ) );
	setColor( 4, QColor( 10, 90, 5 ) );
	setColor( 5, QColor( 6, 60, 0 ) );
	
	m_draggingChecker = -1;  // the point from which we picked up a checker

	diceInitial.push_back( 0 );
	diceInitial.push_back( 0 );
	
	startNewGame();
}

void BgBoard::setColor( int item, QColor color ) //FIXME make an array of colors
{
	bgColor[ item ] = color;
	bgBrush[ item ] = QBrush( bgColor[ item ] );

	if ( item > 1 ) {
		bgBrush[ item ].setStyle( Qt::SolidPattern );
	}

}

QColor BgBoard::getColor( int item )
{
	return bgColor[ item ];
}

void BgBoard::setPlayerType( int player, int opponent )
{ // set whether the player is human or computer
	playerType[ player ] = opponent;
}

int BgBoard::getPlayerType( int player )
{ 
	return playerType[ player ];
}

void BgBoard::setAutoRoll( int player, bool value )
{//if value = true, the player's dice will be rolled automatically
	autoRoll[ player ] = value;
}

bool BgBoard::getAutoRoll( int player )
{// return true if autoRoll[ player ] is true
	return autoRoll[ player ];
}
	
QSize BgBoard::sizeHint() const
{
	return QSize( 300, 400 );
}

int BgBoard::activePlayer()
{
	return m_activePlayer;
}

void BgBoard::definePoints()
{
	//define the points     Point 1 is top right, counts counterclockwise
	// point 0 is player 0 home; point 25 is player 1 home (i.e., 0,1 and 24,25 are adjacent
	// centre board is 26 for player 0, 27 for player 1
	//checkerPosAllowed is the centre of the rectangle where we will draw each checker
	float x = 2 * home_width + 11 * point_width;
	float y = case_width;
	for ( int i = 0; i < 2; ++i ) { //top or bottom
		for ( int j = 0; j < 2; ++j ){ //left or right
			for ( int k = 0; k < 6; ++k ) {
				int ii = i * 12 + j * 6 + k;
				cellRect[ii+1].setRect( int( x ), int( y ), int( point_width ), int( point_height ) );
				for ( int jj = 0; jj < 5; ++jj ) {	// this QPoint is where we draw the checkers
					checkerPosAllowed[ii+1][jj] = QPoint( int( x + point_width / 2 +1), int( y + jj*(1 - 2*i) * checker_size + i*(point_height - checker_size -1) +1) ); // +1 is a fudge factor
				} 
				
				x += point_width * (2 * i - 1);
			}	
			x += home_width * (2 * i - 1);
		}
		x = home_width;
		y = board_height - case_width - point_height;
	}
	// define home boards
	
	cellRect[0].setRect( int( board_width - home_width + case_width ), int( case_width ), int( home_width - case_width ), int( board_height/2 - 2* case_width ) );
	cellRect[25].setRect( int( board_width - home_width + case_width ), int( case_width + board_height /2), int( home_width - case_width ), int( board_height/2 - 2* case_width ) );
	for ( int i = 0; i < 5; ++i ) { 
		checkerPosAllowed[0][i] = QPoint( int( board_width - home_width/2 ), int( case_width + i * ( checker_size + 2 ) ) ); 
		checkerPosAllowed[25][i] = QPoint( int( board_width - home_width/2 ), int( board_height - case_width - (i+1) * (checker_size + 2) ) ); 
		checkerPosAllowed[26][i] = QPoint( int( board_width/2 ), int( case_width + (i) * checker_size ) );
		checkerPosAllowed[27][i] = QPoint( int( board_width/2 ), int( board_height - case_width - (i+1) * checker_size ) );
	}
	cellRect[26].setRect( int( board_width/2 - home_width/2 ), int( case_width ), int( home_width ), int( board_height /2 - 3 * die_size ) );
	cellRect[27].setRect( int( board_width/2 - home_width/2 ), int( board_height/2 + 3 * die_size ), int( home_width ), int( board_height /2 - 3 * die_size ) );
}

void BgBoard::mousePressEvent( QMouseEvent *event )  
{
	m_ClickTimeout = false;
	m_clicked = true;
	
	if ( !diceRolled ) 
		return;
	if ( diceLeft.size() == 0 && diceRolled ) {
		return;	
	}
	for ( int i = 0; i < 28; ++i ) {
		staticPointCount[ i ] = PointCount[ i ];
	}
	
	QTimer *timer = new QTimer(this);
	timer->setSingleShot( true );
	connect(timer, SIGNAL(timeout()), this, SLOT( unsetSingleClick())); //determine whether this is a single click or a drag
	timer->start(150);//TODO: kde parameter?

	m_draggingChecker = checkerHit( event->pos() );
	if ( m_draggingChecker < 0 )
				return;     // m_draggingChecker is the point from which the checker was taken
	if ( 1 ) { // FIXME
		staticPointCount[ m_draggingChecker ] -= activePlayerSign(); //
		checkerBeingDragged = true;
	}
}

void BgBoard::unsetSingleClick() // if this slot is called the player is dragging, not single clicking
{
	m_ClickTimeout = true;
}

void BgBoard::mouseMoveEvent( QMouseEvent *event )
{
	if ( m_draggingChecker < 0)
		return;
	if ( !m_ClickTimeout )
		return;
	QPoint pos = event->pos();
	if (pos.x() <= 0 )
		pos.setX( 1 );
	if (pos.y() >= height() )
		pos.setY( int( height() -  1 ) );
	checkerPos.setX( int( pos.x() ) );
	checkerPos.setY( int( pos.y() - checker_size / 2 ) );
	update();
}

void BgBoard::mouseDoubleClickEvent( QMouseEvent *event ) 
{// not sure how to handle mouse clicks the best way
	// at present the single click has been handled already, so one checker has been moved
	// here we just move the second one
	Move move;
	if ( !diceRolled )
		return;  //can't move before rolling dice
		
	if ( diceLeft.size() == 0 && diceRolled ) {
		return;
	}
	m_draggingChecker = checkerHit( event->pos() );
	if ( m_draggingChecker < 0 )
		return; 
	if ( diceLeft.size() > 0 ) {
		move.startPoint = m_draggingChecker;
		move.dieValue = diceLeft.at( 0 ) * activePlayerSign();
		moveChecker( move );
		return;
	}
	return;
}

void BgBoard::mouseReleaseEvent( QMouseEvent *event )
{
	Move move;
	int dieValueWanted = 0;
	int dieValueNotWanted = 0;
	
	int delta;  // calculated move length
	
	m_clicked = false;

	int pointReleased = -1;
	QPoint pos = event->pos();
	if ( !m_ClickTimeout ) {  // mouse was clicked, rather than dragging
		if ( dieRect[0].contains( pos ) || dieRect[1].contains( pos ) ) { 
			m_draggingChecker = -1;
			 //if mouse is on the player's dice, she wants to roll or end turn
			if ( diceRolled && ( ( diceLeft.size() == 0 ) || ( !movePossible( PointCount, diceLeft, m_activePlayer ) ) ) ) {
				// either dice are finished, or now valid moves left
				//if this player is a person we need to check if the move combination is valid
				// the computer engine does it itself
				if ( ( playerType[ m_activePlayer ] == 0 ) ) {// if the player is human we need to check move is valid
					if ( isValidTotalMove( initialPointCount, movesMade, maxMovePossible ) ) {
						//player has to discover on their own what is wrong with their move
						changePlayer();		//if all dice are used, and player has actually rolled change player
						return;
					}
					else {
						QString s1 = QString::number( diceRolled );
						QString s2 = QString::number( diceLeft.size() );
						//QString s3 = QString::number( movePossible( PointCount, diceLeft, m_activePlayer ) );
						QString s4 = QString::number( maxMovePossible );
						error("1: diceRolled = "+s1+", diceLeft.size() = "+s2+", mP = "+/*s3+*/", mmP = "+s4);
						return;
					}
				}
				else {
					changePlayer();
					return; 
				}
			}
			else if ( !diceRolled ) {  // roll the dice. Only called from here at beginning of game, otherwise in ChangePlayer (if autoroll==true) //FIXME
				newGame = false;
				rollDice();
				maxMovePossible = getMaxMovePossible( initialPointCount, diceInitial, m_activePlayer );
				if ( playerType[ m_activePlayer ] == 1 ) { //player is the computer
					moveComputer();
					changePlayer();
				}
				//error("max move poss = " + QString::number(maxMovePossible) );

				return;
			}
			return;
		}
		
		if ( !diceRolled )
			return;  //can't move before rolling dice
		
		if ( diceLeft.size() == 0 && diceRolled ) {
			return;
		}
		
		if ( m_draggingChecker < 0 ) 
			return;
		
		// move a piece by clicking on it.  Left click, largest die value possible; right click, smallest 
		if ( event->button() == Qt::LeftButton ) {
			dieValueWanted = maxDieValue( diceLeft );
			dieValueNotWanted = minDieValue( diceLeft );
		}
		else if ( event->button() == Qt::RightButton ) {
			dieValueWanted = minDieValue( diceLeft );
			dieValueNotWanted = maxDieValue( diceLeft );
		}
		delta = dieValueWanted * activePlayerSign();
		
		move.startPoint = m_draggingChecker;
		move.dieValue = delta;
		if ( moveChecker( move ) == false ) {  //if the move corresponding to the L/R button isn't allowed, try the R/L button
			delta = dieValueNotWanted * activePlayerSign();
			move.dieValue = delta;
			moveChecker( move ); //FIXME this doesn't work for some reason
		}
		update();
	}
	
	else if ( m_draggingChecker > 0 ) { // we are moving a checker by dragging it
		for ( int i = 0; i < 26; ++i ) {
			if ( cellRect[i].contains( pos ) ) {
				pointReleased = i;
				break;
			}
		}
		delta = pointReleased - m_draggingChecker; // -ve for player 0
		if ( m_draggingChecker > 25 ) {
			delta = pointReleased - 25 * ( 1 - m_activePlayer );
		}
		move.startPoint = m_draggingChecker;
		move.dieValue = delta;
		moveChecker( move );
		update();
	}
}

bool BgBoard::moveChecker( Move move )
{	// move a checker
	// return true if the checker was moved, false otherwise
	
	bool checkerMoved = false;
	bool allowed = false;
	int endpoint = move.startPoint + move.dieValue;
	vector<int> diceTemp;  // in case move is eventually rejected
	diceTemp.assign( diceLeft.begin(), diceLeft.end() );
	vector<int>::iterator diceIt1;
	// is there a checker on startpoint?
	if ( PointCount[ move.startPoint ]  * activePlayerSign() < 0 ) {
		return false;
	}
	
	//check basic arithmetic
	int numDiceLeft = diceTemp.size();
	if ( move.startPoint > 25 ) { // dragging from center board
		endpoint = 25 * ( 1 - m_activePlayer ) + move.dieValue;
	}
	if ( ( endpoint > 0 ) && ( endpoint < 25 ) && ( move.startPoint < 28 ) && ( move.dieValue != 0 ) ) { //released on a point
		for ( diceIt1 = diceTemp.begin(); diceIt1 != diceTemp.end(); diceIt1++ ) {
			if ( ( move.dieValue * activePlayerSign() )  == *diceIt1 ) {
				if ( !allowed ) {		
					diceTemp.erase( diceIt1 );  // once allowed is true, we don't need to iterate over the rest of the loop
					allowed = true;
					break;
				}
			}
		}
		int i,j;
		if ( !allowed && ( numDiceLeft > 1 ) ) { //check combinations of 2 dice //FIXME only if there's no opponent's point in the way!
			vector<int>::iterator diceIt2, marker1, marker2;
			for ( diceIt1 = diceTemp.begin(); diceIt1 != diceTemp.end(); diceIt1++ ) {  
				i = *diceIt1;
				for ( diceIt2 = diceIt1+1; diceIt2 != diceTemp.end(); diceIt2++ ) {
					j = *diceIt2;
					if ( move.dieValue * activePlayerSign() == *diceIt1 + *diceIt2 ) {
						if ( ( PointCount[ move.startPoint + *diceIt1 ] * activePlayerSign() >= 0 ) || ( PointCount[ move.startPoint + *diceIt2 ] * activePlayerSign() >= 0 ) ) { 
							if ( !allowed ) {  // only want to mark dice used once
								marker1 = diceIt1;
								marker2 = diceIt2;
							}
						}
						allowed = true;
						break;
					}	
				}
			}
			if ( allowed ) {
				diceTemp.erase( marker2 );
				diceTemp.erase( marker1 );
			}				
		}
		if ( !allowed && ( numDiceLeft > 2 ) ) { //check combinations of 3 dice
			vector<int>::iterator diceIt2, diceIt3, marker1, marker2, marker3;
			for ( diceIt1 = diceTemp.begin(); diceIt1 != diceTemp.end(); diceIt1++ ) {
				for ( diceIt2 = diceIt1+1; diceIt2 != diceTemp.end(); diceIt2++ ) {
					for ( diceIt3 = diceIt2+1; diceIt3 != diceTemp.end(); diceIt3++ ) {
						if ( move.dieValue * activePlayerSign() == *diceIt1 + *diceIt2 + *diceIt3 ) {
							if ( ( PointCount[ move.startPoint + *diceIt1 ] * activePlayerSign() >= 0 ) && ( PointCount[ move.startPoint + *diceIt1 + *diceIt2 ] * activePlayerSign() >= 0 ) ) { 
								if ( !allowed ) {
									marker1 = diceIt1;
									marker2 = diceIt2;
									marker3 = diceIt3;
								}
							}
							allowed = true;
							break;
						}
					}	
				}
			}
			if ( allowed ) {
				diceTemp.erase( marker3 );
				diceTemp.erase( marker2 );
				diceTemp.erase( marker1 );
			}	
		}	
		if ( !allowed && ( numDiceLeft > 3 ) ) { // all four dice
			if ( move.dieValue * activePlayerSign() == 4 * diceTemp.at( 0 ) ) {
				int die = diceTemp.at( 0 ) * activePlayerSign();
				int start = move.startPoint;
				if ( ( PointCount[ start + die ] * activePlayerSign() >= 0 ) && ( PointCount[ start + 2 * die ] * activePlayerSign() >= 0 ) && ( PointCount[ start + 3 * die ] * activePlayerSign() >= 0 ) ) { 
					allowed = true;
					diceTemp.clear();
				}
			}
		}
	}
	if ( ( endpoint < 1 ) || ( endpoint > 24 ) ) { // bearing off. moves < dice value allowed. Have to be careful to use correct (min) die
		// we check if bearing off is allowed below, in the function isValidSingleMove
		bool tempAllowed = true;
		move.dieValue = ( 25 * m_activePlayer ) - move.startPoint;
		if ( endpoint < 0 )
			endpoint = 0;
		if ( endpoint > 25 )
			endpoint = 25;
		vector<int>::iterator it = diceTemp.begin();
		
				//FIXME the below is now in isValidSingleMove, remove from here [this isn't true! check]
		if ( move.dieValue * activePlayerSign() > maxDieValue( diceTemp ) ) {
			allowed = false;
		}

		else if ( move.dieValue * activePlayerSign() == maxDieValue( diceTemp ) ) {
			vector<int>::iterator ii = maxDie( diceTemp );
			diceTemp.erase( ii );
			allowed = true;
		} 	// say you have checkers on 3 and 5, and roll a 4. you must move 5->1.
		else if ( move.dieValue * activePlayerSign() > minDieValue( diceLeft ) ) {  //maxDieValue > move > minDieValue
			for ( int i = move.dieValue *activePlayerSign() + 1; i < 7; ++i ) {
				if ( PointCount[ 25 * m_activePlayer - i * activePlayerSign() ] * activePlayerSign() > 0 ) {
					tempAllowed = false;
				}
			}				
			if ( tempAllowed ) {
				allowed = true;
				vector<int>::iterator jj = maxDie( diceTemp );
				diceTemp.erase( jj );
			}
		}	
		else if ( move.dieValue * activePlayerSign() == minDieValue( diceLeft ) ) {
			vector<int>::iterator kk = minDie( diceTemp );
			diceTemp.erase(kk);
			allowed = true;
		} 	

		else if ( move.dieValue * activePlayerSign() < minDieValue( diceLeft ) ) {  //maxDieValue > move > minDieValue
			for ( int i = move.dieValue * activePlayerSign() + 1; i < 7; ++i ) {
				if ( PointCount[ 25 * m_activePlayer - i * activePlayerSign() ] * activePlayerSign() > 0 ) {
					tempAllowed = false;
					break;
				}
			}				
			if ( tempAllowed ) {
				vector<int>::iterator kk = minDie( diceTemp );
				diceTemp.erase(kk);
				allowed = true;
			}
		}	
		//FIXME end of previous fixme
		
// TODO: bear off using more than 1 die at a time
	}
		 // no need to check if move is of correct length. more advanced checking done in other functions
	
	if ( allowed ) {
		if ( isValidSingleMove( PointCount, move ) ) {
			checkerMoved = true;
			movesMade.push_back( move );
			if ( PointCount[ endpoint ] == -activePlayerSign() ) {   // opponent had 1 checker on point
				PointCount[ endpoint ] = activePlayerSign();
				PointCount[ move.startPoint ] -= activePlayerSign();
				PointCount[ 27 - m_activePlayer ] -= activePlayerSign();  // put it on centre board
			}
			else { 
				if ( endpoint < 0 ) //put them into the home board!
					endpoint = 0;
				if ( endpoint > 25 )
					endpoint = 25;
				PointCount[ endpoint ] += activePlayerSign(); 
				PointCount[ move.startPoint ] -= activePlayerSign();
			}

			diceLeft.assign( diceTemp.begin(), diceTemp.end() );

			moveDone();
		}
	}
	
	for ( int i = 0; i < 28; ++i ) {
		staticPointCount[ i ] = PointCount[ i ];
	}
	update();
	m_draggingChecker = -1;
	return checkerMoved;
}
	


int BgBoard::checkerHit( const QPoint &pos ) 
{  // return the number of the point player is trying to take a checker from, -1 if not allowed
	int pointPressed = -1;
	for ( int i = 0; i < 28; ++i ) {
		if ( cellRect[i].contains( pos ) ) {
			pointPressed = i;
			break;
		}
	}
	if ( ( PointCount[26 + m_activePlayer] != 0 ) && ( pointPressed != ( 26 + m_activePlayer ) ) )  //have checker on centreboard, must pick up
		return -2;
	
	if ( pointPressed == 25 )
		return -3;	// can't pick up a checker from home
	
	if ( activePlayerSign() * PointCount[ pointPressed ] > 0 ) 
		return pointPressed;
	
	//fell through
	return -4;
}

void BgBoard::moveDone() // check to see if we've won
{
	if ( PointCount[ m_activePlayer * 25 ] * activePlayerSign() == 15 ) { //all checkers on homeboard
		QString mess1 = QString::number( m_activePlayer );
		emit gameOver( 1 );
		update();
	}	
}


bool BgBoard::movePossible( int *points, vector<int> &dice, int player )  //points[i]*activePlayerSign > 0 if activePlayer has checker on point i
		// return true if a player has a valid move left, false otherwise
		// uses getMaxMovePossible to do the hard work
		// FIXME don't have to pas points, dice, player
{	
	int sumMoves = 0;
	vector<Move>::iterator moveIt;
	for ( moveIt = movesMade.begin(); moveIt != movesMade.end(); ++moveIt ) {
		sumMoves += (*moveIt).dieValue;
	}
	if ( sumMoves == maxMovePossible )
		return false;
	if ( sumMoves == -maxMovePossible ) //FIXME do we need the sign?
		return false;	
	else
		return true;
}

bool BgBoard::canBearOff( int *points, int player )
{// return true if the player is allowed to bear off
	int sumBearOff = 0;
	int playerSign = 2 * player - 1;
	
	if ( player == 0 ) { // if all the checkers are in the home court, or home, we are allowed to bear off
		for ( int i = 0; i < 7; ++i ) {
			if ( points[ i ] * playerSign > 0 )
				sumBearOff += points[ i ] * playerSign;
		}
	}
	if ( player == 1 ) {
		for ( int i = 19; i < 25; ++i ) {
			if ( points[ i ] * playerSign > 0 )
				sumBearOff += points[ i ] * playerSign;
		}		
	}	

	if ( sumBearOff == 15 ) {
		return true;
	}
	else 
		return false;
}
	
void BgBoard::changePlayer()
{	
	movesMade.clear();
	if ( m_activePlayer == 0) {
		m_activePlayer = 1;
	}
	else 
		m_activePlayer = 0;
	diceRolled = false;
	for ( int i = 0; i < 28; ++i ) { // in case of undo
		initialPointCount[ i ] = PointCount[ i ];
	}
	if ( autoRoll[ m_activePlayer ] ) {
		rollDice();
		maxMovePossible = getMaxMovePossible( initialPointCount, diceInitial, m_activePlayer );
		//error("max move poss = " + QString::number(maxMovePossible) );
		if ( playerType[ m_activePlayer ] == 1 ) { //player is the computer
			moveComputer();
			update();
			changePlayer();
		}
	//FIXME redraw the dice before calculating computer move
	}
	update();
}

void BgBoard::moveComputer()
{	
	Move move;
	vector<moveScore> bestMoveList;
	vector<Move> bestMove;
	bestMoveList.clear();
	bestMove.clear();
			
	computeMove( PointCount, diceInitial, bestMoveList, m_activePlayer );
	if ( bestMoveList.size() > 0 ) {
		bestMove = bestMoveList.at(0).moves ;
		vector<Move>::iterator it;
		for ( it = bestMove.begin(); it != bestMove.end(); ++it ) {
			move.startPoint = ( *it ).startPoint;
			move.dieValue = ( *it ).dieValue;
			moveChecker( move );
		}
	}
}

int BgBoard::rollDie()
{

	int D = rand() % 6 +1;
	return D;
}

void BgBoard::rollDice() 
{
	diceInitial.clear();
	diceInitial.push_back( rollDie() );
	diceInitial.push_back( rollDie() );
	if ( diceInitial.at(0) == diceInitial.at(1) ) {
		diceInitial.push_back( diceInitial.at(0) );
		diceInitial.push_back( diceInitial.at(0) );
	}
	diceLeft.assign( diceInitial.begin(), diceInitial.end() );
	
	diceRolled = true;
	update();
	rand1 = rand() % 100;//some random numbers for positioning the dice
	rand2 = rand() % 100;
	rand3 = rand() % 100;
	rand4 = rand() % 100;
}

void BgBoard::startNewGame()
{
	
	for ( int i = 0; i < 28; ++i ) {
		PointCount[i] = 0;
	}
	PointCount[1] = 2;
	PointCount[6] = -5;
	PointCount[8] = -3;
	PointCount[12] = 5;
	PointCount[13] = -5;
	PointCount[17] = 3;
	PointCount[19] = 5;
	PointCount[24] = -2;
	
	movesMade.clear();
	
	static bool firstTime = true;
	if (firstTime) {
		firstTime = false;	
		QTime midnight(0, 0, 0);
		srand(midnight.secsTo(QTime::currentTime()));
	}
	int r = rand() % 2;
	if ( r == 0 ) {
		m_activePlayer = 0;
	}
	else {
		m_activePlayer = 1;
	}
	for ( int i = 0; i < 28; ++i ) { // in case of undo
		initialPointCount[ i ] = PointCount[ i ];
		staticPointCount[ i ] = PointCount[ i ];
	}
	newGame = true;
	diceRolled = false;
	diceInitial.clear();
	diceInitial.push_back( 0 );
	diceInitial.push_back( 0 );
}

void BgBoard::paintEvent(QPaintEvent *event /* event */)
{
	QFrame::paintEvent(event);
	
	QPainter painter( this );
	QRect rect = contentsRect();
	
	board_width = rect.width();
	
	board_height = 9/15.6 * board_width; //TODO: put an is_changed flag on this, to minimize redraws
	checker_size = .08 * board_height;

	point_width = board_height / 9;  
	point_height = point_width * 4;
	home_width = 1.2 * point_width;
	die_size = .075 * rect.height();
	die_spot_size = .2 * die_size;
	
	case_width = .02 * rect.height(); //width of small bit of case around board
	
	painter.setRenderHint(QPainter::Antialiasing);

	paintBoard( painter );
	definePoints();
	paintStaticCheckers( painter );

	paintDice( painter, diceInitial );
	if ( m_clicked ) 
		paintChecker( painter );//the moving checker
}

void BgBoard::paintBoard( QPainter &painter )
{
	
	QPoint points[3] = { 
		QPoint( 0, int( case_width ) ),
		QPoint( int( point_width / 2 ), int( point_height ) ),
		QPoint( int( point_width ), int( case_width ) )
	};
	
	QPen casePen;
	casePen.setWidth( 2 );
	casePen.setColor( Qt::black );
	
	painter.setPen( casePen );
	painter.setBrush( bgBrush[ caseColor ] );
	painter.drawRect( 0, 0, int( board_width / 2 ), int( board_height ) );
	painter.drawRect( int( board_width / 2 ), 0, int( board_width /2 ), int( board_height ) );

	painter.setBrush( bgBrush[ feltColor ] );
	painter.drawRect( int(home_width), int( case_width ), int(6 * point_width + 1), int(board_height - 2 * case_width + 1) );
	painter.drawRect( int( 2 * home_width + 6 * point_width ), int( case_width ), int(6 * point_width + 1), int( board_height - 2* case_width + 1) );
	//home
	painter.drawRect( int( case_width ), int( case_width ), int( home_width  - 2*case_width ), int( board_height /2  -2*case_width ) );
	painter.drawRect( int( case_width ), int( board_height/2 + case_width ), int( home_width - 2 * case_width ), int( board_height/2 - 2 * case_width + 1) );
	painter.drawRect( int( board_width - home_width + case_width ), int( case_width ), int( home_width - 2 * case_width ), int( board_height/2 -2 * case_width ));
	painter.drawRect( int( board_width - home_width + case_width ), int( board_height/2 +case_width ), int( home_width-2 * case_width ), int( board_height/2 -2*case_width + 1) );
	painter.save();
	painter.translate( home_width, 0 );
	
	//draw the points
	painter.setPen( Qt::NoPen );
	for (int i = 0; i < 2; ++i ) { 
		for (int j = 0; j < 2; ++j ) {
			for (int k = 0; k < 3; ++k ) {
				painter.setBrush( bgBrush[ point0Color ] );
				painter.drawPolygon( points, 3 );
				painter.translate( point_width, 0 );
				painter.setBrush( bgBrush[ point1Color ] );
				painter.drawPolygon( points, 3 );
				painter.translate( point_width, 0 );
			}
			painter.translate( home_width, 0 );
		}
		painter.translate( -home_width, board_height-1 );
		painter.rotate( 180 );
	}
	painter.restore();
	painter.setBrush( Qt::NoBrush );
	painter.setPen( casePen ); //FIXME do we really have to redraw these rectangles?  How else to get the borders on top?
	painter.drawRect( int(home_width), int( case_width ), int(6 * point_width + 1 ), int(board_height - 2 * case_width + 1 ) );
	painter.drawRect( int( 2 * home_width + 6 * point_width ), int( case_width ), int(6 * point_width + 1 ), int( board_height - 2* case_width + 1 ) );
	
}

void BgBoard::paintChecker( QPainter &painter )
{
	if ( m_draggingChecker < 0 )
		return;
	paintCheck( painter, checkerPos, m_activePlayer );
}

void BgBoard::paintCheck( QPainter &painter, QPoint &pos, int player )
{
	painter.save();
	painter.setPen( Qt::NoPen );
	painter.translate( int( pos.x() - checker_size / 2 ), int( pos.y() ) );
	
	QRadialGradient radialGradient(10, 10, checker_size, int( .33*checker_size ), int( .33*checker_size ) );
	radialGradient.setColorAt(0.0, Qt::white );
	radialGradient.setColorAt(0.2, bgColor[ player ] );
	radialGradient.setColorAt(1.0, Qt::black );
	painter.setBrush(radialGradient);
	
	QRect R = QRect( 0, 0, int( checker_size ), int( checker_size ) );
	painter.drawEllipse( R );
	painter.restore();
}

void BgBoard::paintStaticCheckers( QPainter &painter )
{
	int number;
	int numberToPaint;
	int player;

	painter.setPen( Qt::NoPen );
	for ( int i = 0; i < 28; ++i ) {
		if ( staticPointCount[ i ] == 0 )
			continue;
		if ( staticPointCount[i] < 0 )
			player = 0;
		else
			player = 1;
		number = abs( staticPointCount[ i ] );
		numberToPaint = number;
		if ( number > 5 ) {
			numberToPaint = 5;
		}

		for ( int j = 0; j < numberToPaint; ++j ) {
			QPoint p = checkerPosAllowed[ i ][ j ];
			paintCheck( painter, p, player );
		}
		if ( number > 5 ) {  //only draw 5 checkers per point
			QString txt; 
			txt = QString::number( number );
			int size = int( .85 * checker_size );
			int textSize = int( .6 * checker_size );//FIXME center the text in the box
			QRect textRect = QRect( int (checkerPosAllowed[ i ][ 4 ].x() - size/2 ), checkerPosAllowed[ i ][ 4 ].y(), size, size );
			number = 5;
			painter.save();
			painter.setPen( Qt::white );
			painter.setBrush( Qt::white );
			painter.drawRect( textRect );
			painter.setPen( Qt::blue );
			painter.setFont(QFont("Courier", textSize, QFont::Bold ));  //TODO prettify
			painter.drawText( textRect, txt );
			painter.restore();
		} 
	}
}

void BgBoard::paintDice( QPainter &painter, vector<int> dice )
{
	if ( dice.size() == 0 )
		return;
	float y [2];
	int x[2];
	float ds = die_spot_size ;
	if ( m_activePlayer == 0 ) {
		painter.setBrush( bgBrush[ checker0Color ] );
	}
	else {
		painter.setBrush( bgBrush[ checker1Color ] );
	}	
	
	painter.setPen( Qt::black );
	
	if ( !diceRolled ) {
		y[0] = 1.1 * board_height;
		y[1] = y[0];
		if ( m_activePlayer == 0 ) {
			x[0] = int( case_width );
			x[1] = int( 2* case_width + die_size );
		}
		if ( m_activePlayer == 1 ) {
			x[0] = int( board_width - case_width - die_size );
			x[1] = int( board_width - 2 * case_width - 2 * die_size );
		}
		dieRect[0] = QRect( x[0], int( y[0] ), int( die_size ), int( die_size ) );
		dieRect[1] = QRect( x[1], int( y[1] ), int( die_size ), int( die_size ) );
	}
	
	else {
		y[0] = (.47 + rand1 / 1666) * board_height - die_size/2;
		y[1] = (.47 + rand2 / 1666) * board_height - die_size/2;
		if ( m_activePlayer == 0 ) {
			x[0] = int( home_width + (1 + rand3 /100) * die_size );
			x[1] = int( x[0] + (1.2 + rand4/100) * die_size );
		}
		if ( m_activePlayer == 1 ) {
			x[0] = int( board_width - home_width - ( 2 + rand3 / 100 ) * die_size );
			x[1] = int( x[0] - ( 1.2 + rand4/100) * die_size );
		}

		dieRect[0] = QRect( x[0], int( y[0] ), int( die_size ), int( die_size ) );
		dieRect[1] = QRect( x[1], int( y[1] ), int( die_size ), int( die_size ) );
	}
		
	painter.drawRect( dieRect[0] );
	painter.drawRect( dieRect[1] );
	
	painter.setBrush( Qt::white );
	
	
	for ( int i = 0; i < 2; ++i ) { // TODO: fix positions!
		switch ( dice.at(i) ) {
			case 5:
				painter.drawEllipse( int( x[i] + .25 * die_size - ds/2), int( y[i] + .25 * die_size -ds/2 ), int(ds), int(ds) );
				painter.drawEllipse( int( x[i] + .75 * die_size - ds/2), int( y[i] + .75 * die_size -ds/2 ), int(ds), int(ds) );
			case 3:
				painter.drawEllipse( int( x[i] + .75 * die_size - ds/2), int( y[i] + .25 * die_size -ds/2 ), int(ds), int(ds) );
				painter.drawEllipse( int( x[i] + .25 * die_size - ds/2), int( y[i] + .75 * die_size -ds/2 ), int(ds), int(ds) );
			case 1:
				painter.drawEllipse( int( x[i] + .5 * die_size - ds/2 ), int( y[i] + .5 * die_size -ds/2 ), int(ds), int(ds) );
				break;
			case 6:
				painter.drawEllipse( int( x[i] + .25 * die_size - ds/2 ), int ( y[i] + .5 * die_size - ds/2 ), int(ds), int(ds) );
				painter.drawEllipse( int( x[i] + .75 * die_size - ds/2 ), int ( y[i] + .5 * die_size - ds/2 ), int(ds), int(ds) );
			case 4:
				painter.drawEllipse( int( x[i] + .25 * die_size - ds/2 ), int ( y[i] + .25 * die_size - ds/2 ), int(ds), int(ds) );
				painter.drawEllipse( int( x[i] + .75 * die_size - ds/2 ), int ( y[i] + .75 * die_size - ds/2 ), int(ds), int(ds) );
			case 2:
				painter.drawEllipse( int( x[i] + .25 * die_size - ds/2 ), int ( y[i] + .75 * die_size - ds/2 ), int(ds), int(ds) );
				painter.drawEllipse( int( x[i] + .75 * die_size - ds/2 ), int ( y[i] + .25 * die_size - ds/2 ), int(ds), int(ds) );
				break;
		}
	}
}

int BgBoard::maxDieValue( vector<int> &dice ) //helper functions to determine if moves are possible
{
	vector<int>::iterator it;
	int value = 0;
	for ( it = dice.begin(); it != dice.end(); ++it ) {
		if ( *it > value )
			value = *it;
	}
	return value;
}

int BgBoard::minDieValue( vector<int> &dice )
{
	vector<int>::iterator it;
	int value = 7;
	for ( it = dice.begin(); it != dice.end(); ++it ) {
		if ( *it < value )
			value = *it;
	}
	if ( value == 7 )
		return 0;  // all dice are used
	return value;
}	

vector<int>::iterator BgBoard::maxDie( vector<int> &dice )
{
	vector<int>::iterator it, it1;
	int dieVal = 0;
	for ( it = dice.begin() ; it != dice.end(); ++it ) {
		if ( *it > dieVal ) {
			dieVal = *it;
			it1 = it;
		}
	}
	return it1;
}

vector<int>::iterator BgBoard::minDie( vector<int> &dice )
{
	vector<int>::iterator it, it1;
	int dieVal = 7;
	for ( it = dice.begin(); it != dice.end(); ++it ) {
		if ( *it < dieVal ) {
			dieVal = *it;
			it1 = it;
		}
	}
	return it1;
}

void BgBoard::undo() 
{
	for ( int i = 0; i < 28; ++i ) {
		PointCount[ i ] = initialPointCount[ i ];
		staticPointCount[ i ] = initialPointCount[ i ];
	}
	diceLeft.assign( diceInitial.begin(), diceInitial.end() );
	movesMade.clear();
	update();
}
 
					
void BgBoard::error( QString string )
{
	QMessageBox::StandardButton reply;
	reply = QMessageBox::information(this, tr("QBackgammon"), string );
	return;
}		
						

bool BgBoard::isValidSingleMove( int *points, Move move )
		//return true if the single move, from startpoint a distance dieval, is allowed
		//sign of dieval determines orientation
		//note: the player with points > 0 has dieval > 0
		// to bear off, you can move past the home board
{
	bool allowed = false;
	bool tempAllowed = true;
	
	int startpoint, endpoint;
	
	int playerSign = 1;// -1 for player 0; 1 for player 1
	int player = 1;   //  0 for player 0; 1 for player 1
	
	if ( move.dieValue < 0 ) {
		playerSign = -1;
		player = 0;
	}		
	
	if ( move.startPoint == 0 || move.startPoint == 25 )//can't move a checker from home
		return false;
	
	if ( points[ 26 + player ] > 0 ) {// point on center board, must move it
		if ( move.startPoint < 26 ) {
			return false;
		}
	}
	
	if ( move.startPoint > 25 ) { //from centre board
		startpoint = ( 25 * ( 1 - player ) );
	}
	else
		startpoint = move.startPoint;
	
	endpoint = startpoint + move.dieValue;
	
	if ( endpoint > 25 ) //bearing off
		endpoint = 25;
	if ( endpoint < 0 )
		endpoint = 0;	
	if ( ( playerSign * points[ endpoint ] ) < -1 ) // opponent has > 1 checker on point
		return false;
	
	int delta = ( endpoint - startpoint ) * playerSign;
	
	int sumBearOff = 0; // to keep this general, do not use m_active 
	if ( player == 0 ) { // if all the checkers are in the home court, or home, we are allowed to bear off
		for ( int i = 0; i < 7; ++i ) {
			if ( points[ i ] * playerSign > 0 )
				sumBearOff += points[ i ] * playerSign;//FIXME combine into 1
		}
	}
	if ( player == 1 ) {
		for ( int i = 19; i < 26; ++i ) {
			if ( points[ i ] * playerSign > 0 )//FIXME only check this if we try to bear off
				sumBearOff += points[ i ] * playerSign;//FIXME call canBearOff
		}		
	}	
	if ( ( sumBearOff < 15 ) && ( ( endpoint == 0 ) || ( endpoint == 25 ) ) )//trying to bear off when not allowed
		return false;	
	
	if ( ( sumBearOff == 15 ) && ( endpoint == ( 25 * player ) ) ) {// bearing off
		// we need to check the following doesn't happen. say points[6] = -1, points[3] = 2, points[2] = 1, die = 3. We are not allowed to move.
				
		if ( delta  > move.dieValue * playerSign ) {
			allowed = false;
		}

		else if ( delta == move.dieValue * playerSign ) {  //FIXME these elses might not all be necessary
			allowed = true;
		} 	// say you have checkers on 3 and 5, and roll a 4. you must move 5->1.

		else if ( delta < move.dieValue * playerSign ) {  //maxDieValue > move > minDieValue
			for ( int i = startpoint + 1; i < 7; ++i ) {
				if ( points[ 25 * player - i * playerSign ] * playerSign > 0 ) {
					tempAllowed = false;
					break;
				}
			}				
			if ( tempAllowed ) {
				allowed = true;
			}
		}	
		
		if ( allowed )
			return true;
		else
			return false;
	}
	
	if ( ( startpoint == ( 25 * (1 - player ) ) ) && ( endpoint == ( 25 * (1 - player) + move.dieValue ) ) ) // checker from player i's centre board
		return true;
	
	if ( ( startpoint > 0 ) && ( startpoint < 25 ) && ( endpoint > 0 ) && ( endpoint < 25 ) ) // nothing left to check for ordinary moves
		return true;
	
	return false;  //
}
						
bool BgBoard::isValidTotalMove( int *startpoints, vector<Move> &moves, int maxMovePossible )
{//both moves may be individually valid, but not the combination. eg when bearing off, need to use max value
	// pass maxMovePossible to avoid determining it many times
	//FIXME startpoints not needed
	int playerSign = 1;// -1 for player 0; 1 for player 1
	int player = 1;   //  0 for player 0; 1 for player 1
	
	if (moves.size() > 0 ) {
		if ( moves.at(0).dieValue < 0 ) {
			playerSign = -1;
			player = 0;
		}	
	}
	
	int sumMoves = 0;
	int delta;
	int endpoint, startpoint;
	vector<Move>::iterator moveIt;
	
	
	
	for ( moveIt = moves.begin(); moveIt < moves.end(); moveIt++ ) {
		if ( (*moveIt).startPoint > 25 ) { //from centre board
			startpoint = ( 25 * ( 1 - player ) );
		}
		else {
			startpoint = (*moveIt).startPoint;
		}
	
		endpoint = startpoint + (*moveIt).dieValue;
	
		if ( endpoint > 25 ) //bearing off
			endpoint = 25;
		if ( endpoint < 0 )
			endpoint = 0;	
		
		delta = endpoint - startpoint;
		sumMoves += delta;
	}
	if (sumMoves == maxMovePossible) {
		return true;
	}
	else if (sumMoves == -maxMovePossible) {  //FIXME not keeping track of sign in this function
		return true;
	}
	else {
		return false;
	}
}

int BgBoard::getMaxMovePossible( int *points, vector<int> &dice, int player )
{
	// given a checker configuration points[28], the dice, and which player it is, we calculate the largest move (which is the only valid move length)
	Move move_temp;
	vector<int>::iterator it;
	vector<int>::iterator itD1, itD2, itD3, itD4; // iterators over dice
	
	vector<Move> bestMove; 
	vector<Move> move;
	vector<Move> tempMove1;
	vector<int> tempDice1;
	int tempPoints1[28];

	vector<Move> tempMove2;
	vector<int> tempDice2;
	int tempPoints2[28];

	vector<Move> tempMove3;
	vector<int> tempDice3;
	int tempPoints3[28];
	
	vector<Move> tempMove4;
	vector<int> tempDice4;
	int tempPoints4[28];

	vector<int> newDice;
	
	int sumDice = 0;  // sum of rolled dice
	for ( it = dice.begin(); it != dice.end(); ++it ) {
		newDice.push_back( *it );
		sumDice += *it;
	}
	
	tempMove1.clear();
	move.clear();
	int tempScore1 = 0;
	int tempScore2 = 0;
	int tempScore3 = 0;
	int tempScore4 = 0;
	int score = 0;  
	int bestScore = 0;
	
	int playerSign = 1;
	if ( player == 0 ) {
		playerSign = -1;
	}

	int jj,kk;
	
	bool movePoss2 = false;
	bool movePoss3 = false;
	bool movePoss4 = false;
	int bestScore1 = 0;
	int bestScore2 = 0;
	int bestScore3 = 0;

	for ( itD1 = newDice.begin(); itD1 != newDice.end(); ++itD1 ) {
		for ( int i = 1; i < 27 + player; ++i ) {
			if ( points[ 26 + player ] * playerSign > 0 ) { // if there is a checker on center board we must move it
				i = 26 + player;
			}
			movePoss2 = false;
			if ( points[ i ] * playerSign > 0 ) {
				move_temp.dieValue = *itD1 * playerSign;
				move_temp.startPoint = i;
				if ( isValidSingleMove( points, move_temp  ) ) {
					tempScore1 = updateTemps( points, tempPoints1, newDice, tempDice1, i, itD1, player );	
					for ( itD2 = tempDice1.begin(); itD2 != tempDice1.end(); ++itD2 ) {
						for ( int ii = 1; ii < 27 + player; ++ii ) {
							if ( tempPoints1[ 26 + player ] * playerSign > 0 ) { // if there is a checker on center board we must move it
								ii = 26 + player;
							}
							if ( tempPoints1[ ii ] * playerSign > 0 ) {
								move_temp.dieValue = *itD2 * playerSign;
								move_temp.startPoint = ii;
								if ( isValidSingleMove( tempPoints1, move_temp ) ) {
									movePoss2 = true;
									movePoss3 = false;
									tempScore2 = updateTemps( tempPoints1, tempPoints2, tempDice1, tempDice2, ii, itD2, player );
									
									if ( newDice.size() > 2 ) { //doubles were rolled, go two more steps
									for ( itD3 = tempDice2.begin(); itD3 != tempDice2.end(); ++itD3 ) {
									for ( int iii = 1; iii < 27 + player; ++iii ) {
									if ( tempPoints2[ 26 + player ] * playerSign > 0 ) { // if there is a checker on center board we must move it
									iii = 26 + player;
									}
									if ( tempPoints2[ iii ] * playerSign > 0 ) {
									move_temp.dieValue = *itD3 * playerSign;
									move_temp.startPoint = iii;
									if ( isValidSingleMove( tempPoints2, move_temp ) ) {
									movePoss3 = true;
									movePoss4 = false;
									tempScore3 = updateTemps( tempPoints2, tempPoints3, tempDice2, tempDice3, iii, itD3, player );
									// no iteration over dice as there's only one left
									for ( int iiii = 1; iiii < 27 + player; ++iiii ) {
									if ( tempPoints3[ 26 + player ] * playerSign > 0 ) { // if there is a checker on center board we must move it
									iiii = 26 + player;
									}
									if ( tempPoints3[ iiii ] * playerSign > 0 ) {
									move_temp.dieValue = tempDice3.at(0) * playerSign;
									move_temp.startPoint = iiii;
									if ( isValidSingleMove( tempPoints3, move_temp ) ) {
									itD4 = tempDice3.begin();
									movePoss4 = true;
									tempScore4 = updateTemps( tempPoints3, tempPoints4, tempDice3, tempDice4, iiii, itD4, player );
					
									score = tempScore1 + tempScore2 + tempScore3 + tempScore4;
									if ( score > bestScore ) {
											bestScore = score;
									}	
									if ( bestScore == sumDice ) {	
										return bestScore;
									}
									}
										
									}
									}// end of iiii loop
									if ( !movePoss4 ) {// have made it through third die without finding a valid move
										bestScore3 = tempScore1 + tempScore2 + tempScore3;
									}
									
									}
									}
									}
									}
									// end of for (itD3)
									if ( !movePoss3 ) {// have made it through third die without finding a valid move
										if ( tempScore1 + tempScore2 > bestScore3 ) {
										bestScore2 = tempScore1 + tempScore2;
										}
									}
									}
								
									else { //doubles not rolled ( dice.size() !> 2 )
										jj = tempPoints2[ 26 + player ] * playerSign;
										kk = points[ 26 + player ] * playerSign;
										score = tempScore1 + tempScore2;
										if ( score > bestScore ) {
											bestScore = score;
											if ( bestScore == sumDice ) 
												return bestScore;
										
									}
									}
								}
							}
						}
					}
					if ( !movePoss2 ) {// have made it through second die without finding a valid move
						if ( tempScore1 > bestScore1 ) {
							bestScore1 = tempScore1;
						}
					}
				}
			}
		}
	}//have looped over all possible moves

	
	if ( bestScore3 > bestScore )
		return bestScore3;
	if ( bestScore2 > bestScore )
		return bestScore2;
	if ( bestScore1 > bestScore )
		return bestScore1;
	return bestScore;
}



int BgBoard::updateTemps( int *oldPoints, int *newPoints, vector<int> &oldDice, vector<int> &newDice, int startpoint, vector<int>::iterator &die, int player )  //FIXME playerSign isn't necessary, pass a signed die
		//return value is the number of points moved
{// a helper function for getMaxMovePossible
	// after a virtual move is made, call this function to get the new dice, points, checkers, and move
	int endpoint;
	int playerSign = 1;
	int delta = 0;
	
	if ( player == 0 )
		playerSign = -1;
	
	vector<int>::iterator it;
	
	newDice.clear();
	for ( it = oldDice.begin(); it != oldDice.end(); ++it ) {
		if ( it != die ) {
			newDice.push_back( *it );
		}
	}
	
	for ( int i = 0; i < 28; ++i ) {// reset the temporary points, for evaluating next move
		newPoints[ i ] = oldPoints[ i ];
	}
	
	newPoints[ startpoint ] -= playerSign;
	
	if ( startpoint > 25 ) { // coming from centre board
		if ( player == 1 ) {
			startpoint = 0;
		}
		else {
			startpoint = 25;
		}
	}				
				
	endpoint = startpoint + *die * playerSign;
	if ( ( endpoint < 0 ) || ( endpoint > 25 ) ) { // want to move to the home board
		if ( player == 1 ) {
			endpoint = 25;
		}
		else
			endpoint = 0;	
	} 
	delta = ( endpoint - startpoint ) * playerSign;
	
	if ( oldPoints[ endpoint ] == -playerSign ) { // opponent had 1 checker on point
		newPoints[ 27 - player ] -= playerSign;
		newPoints[ endpoint ] = 0; // will be incremented below
	}
	
	newPoints[ endpoint ] += playerSign;
	return delta;
}

void BgBoard::hint()
{
// 	QString message = "";
// 	QString s0 = QString::number( m_activePlayer );
// 	QString s1 = QString::number( diceRolled );
// 	QString s2 = QString::number( diceLeft.size() );
// 	QString s3 = QString::number( movePossible( PointCount, diceLeft, m_activePlayer ) );
// 	QString s4 = QString::number( maxMovePossible );
// 	QString m1 = "";
 	vector<Move>::iterator it;
// 	for ( it = movesMade.begin(); it != movesMade.end(); ++it ) {
// 		m1.append( QString::number( (*it).startPoint ) );
// 		m1.append( ", ");
// 		m1.append( QString::number( (*it).dieValue ) );
// 		m1.append( "; ");
// 	}
// 	
 	vector<moveScore> list;
 	computeMove( initialPointCount, diceInitial, list, m_activePlayer );
	
	QString mess;
	
	for ( int i = 0; i < 10; ++i ) {
		if ( list.size() > i ) {
			mess.append( QString::number( list.at(i).score ) + "; " );
			for ( it = list.at(i).moves.begin(); it != list.at(i).moves.end(); ++it ) { 
				mess.append( QString::number( (*it).startPoint ) + ", " );
				mess.append( QString::number( (*it).dieValue ) + "; " );
			}
			mess.append("\n");
		}
	}
		
// 	message = "player: "+s0+"; diceRolled = "+s1+", diceLeft.size() = "+s2+", mP = "+s3+", mmP = "+s4+"\n";
// 	message.append( "moves made: " +m1+"\n" );
// 	message.append(mess);
// 	
// 	error( message );
	error( mess );
	return;
}
		
//FIXME display player X wins after updating point counts

void BgBoard::computeMove( int *points, vector<int> &dice, vector<moveScore> &movesList, int player )
{
	// given a checker configuration points[28], the dice, and which player it is, we calculate the largest move 
	
	Move move_temp;
	moveScore moveScore_temp;
	
	vector<int>::iterator it;
	vector<moveScore>::iterator listIt;
	
	vector<int>::iterator itD1, itD2, itD3, itD4; // iterators over dice

	vector<Move> move, tempMove1, tempMove2, tempMove3, tempMove4;
	vector<int> tempDice1, tempDice2, tempDice3, tempDice4;
	int tempPoints1[28], tempPoints2[28], tempPoints3[28], tempPoints4[28];

	vector<int> newDice;
	
	int sumDice = 0;  // sum of rolled dice
	for ( it = dice.begin(); it != dice.end(); ++it ) {
		newDice.push_back( *it );
		sumDice += *it;
	}
	
	int playerSign = 1;
	if ( player == 0 ) {
		playerSign = -1;
	}

	bool movePoss2 = false;
	bool movePoss3 = false;
	bool movePoss4 = false;
	
	int maxMovePossible;
	int start;
	maxMovePossible = getMaxMovePossible( points, dice, player );
	
	//FIXME this is horrible, rewrite as recursive function calls
	for ( itD1 = newDice.begin(); itD1 != newDice.end(); ++itD1 ) {
		for ( int i = 1; i < 27 + player; ++i ) {
			if ( points[ 26 + player ] * playerSign > 0 ) { // if there is a checker on center board we must move it
				i = 26 + player;
			}
			tempMove1.clear();
			if ( points[ i ] * playerSign > 0 ) {
				move_temp.dieValue = *itD1 * playerSign;
				move_temp.startPoint = i;
				if ( isValidSingleMove( points, move_temp  ) ) {
					updateTemps( points, tempPoints1, newDice, tempDice1, i, itD1, player );	
					tempMove1.push_back( move_temp );

					for ( itD2 = tempDice1.begin(); itD2 != tempDice1.end(); ++itD2 ) {
						// have to start from 1, sometimes valid moves are otherwise blocked
						start = 1;
						for ( int ii = start; ii < 27 + player; ++ii ) {
							if ( tempPoints1[ 26 + player ] * playerSign > 0 ) { // if there is a checker on center board we must move it
								ii = 26 + player;
							}
							if ( tempPoints1[ ii ] * playerSign > 0 ) {
								move_temp.dieValue = *itD2 * playerSign;
								move_temp.startPoint = ii;
								tempMove2.clear();
								tempMove2.assign( tempMove1.begin(), tempMove1.end() );
								if ( isValidSingleMove( tempPoints1, move_temp ) ) {
									movePoss2 = true;
									tempMove2.push_back( move_temp );
									updateTemps( tempPoints1, tempPoints2, tempDice1, tempDice2, ii, itD2, player );
									if ( newDice.size() > 2 ) { //doubles were rolled, go two more steps
					
										////////////////////////////////////////////
					
										for ( itD3 = tempDice2.begin(); itD3 != tempDice2.end(); ++itD3 ) {
										start = 1;
										for ( int iii = start; iii < 27 + player; ++iii ) {
										if ( tempPoints2[ 26 + player ] * playerSign > 0 ) { // if there is a checker on center board we must move it
										iii = 26 + player;
										}
										tempMove3.clear();
										tempMove3.assign( tempMove2.begin(), tempMove2.end() );
										if ( tempPoints2[ iii ] * playerSign > 0 ) {
										move_temp.dieValue = *itD3 * playerSign;
										move_temp.startPoint = iii;
										if ( isValidSingleMove( tempPoints2, move_temp ) ) {
										movePoss3 = true;
										tempMove3.push_back( move_temp );
										updateTemps( tempPoints2, tempPoints3, tempDice2, tempDice3, iii, itD3, player );
						// no iteration over dice as there's only one left
										start = 1;
										for ( int iiii = start; iiii < 27 + player; ++iiii ) {
									
										if ( tempPoints3[ 26 + player ] * playerSign > 0 ) { // if there is a checker on center board we must move it
										iiii = 26 + player; 
										}
					
										tempMove4.clear();
										tempMove4.assign( tempMove3.begin(), tempMove3.end() );
										if ( tempPoints3[ iiii ] * playerSign > 0 ) {
										move_temp.dieValue = tempDice3.at(0) * playerSign;
										move_temp.startPoint = iiii;
											
										if ( isValidSingleMove( tempPoints3, move_temp ) ) {
										itD4 = tempDice3.begin();
										movePoss4 = true;
										tempMove4.push_back( move_temp );
										updateTemps( tempPoints3, tempPoints4, tempDice3, tempDice4, iiii, itD4, player );
										if ( isValidTotalMove( points, tempMove4, maxMovePossible ) ) {
										pushMove( tempPoints4, tempMove4, movesList );
										}
										}
										}
										}// end of iiii loop
										if ( !movePoss4 ) {// have made it through third die without finding a valid move
										if ( isValidTotalMove( points, tempMove3, maxMovePossible ) ) {
										pushMove( tempPoints3, tempMove3, movesList );
										}
										}//end of if (!movePoss4 )
										}
										}
										} //end of iii loop
										}// end of for (itD3)
										if ( !movePoss3 ) {// have made it through third die without finding a valid move
										if ( isValidTotalMove( points, tempMove2, maxMovePossible ) ) {
										pushMove( tempPoints2, tempMove2, movesList );
										}
										} // end of ( !movePoss3 )
									}  // end of ( newDice.size() > 2 )
				
									////////////////////////////////////////////////////////////////
								
									else { //doubles not rolled ( dice.size() !> 2 )
										if ( isValidTotalMove( points, tempMove2, maxMovePossible ) ) {
										pushMove( tempPoints2, tempMove2, movesList );
										}
									}										
								} //end of if ( isValidSingleMove( tempPoints1, move_temp ) ) {
							}
						}
					}
					if ( !movePoss2 ) {// have made it through second die without finding a valid move
						if ( isValidTotalMove( points, tempMove1, maxMovePossible ) ) {
							pushMove( tempPoints1, tempMove1, movesList );
						}	
					}
				}
			}
		}
		
	}//have looped over all possible moves
	return;
}

void BgBoard::pushMove( int *newPoints, vector<Move> &move, vector<moveScore> &list ) 
{
	//helper function for computeMove.  Push a move onto the stack of moves
	float score;
	vector<moveScore>::iterator listIt;
	moveScore moveScore_temp;
	score = pointsScore( newPoints, m_activePlayer );
	moveScore_temp.score = score;
	moveScore_temp.moves = move;
	if ( list.size() == 0 ) {
		list.push_back( moveScore_temp );
		return;
	}
	else {
		for ( listIt = list.begin(); listIt != list.end(); ++listIt ) {
			if ( score > (*listIt).score ) {
				list.insert( listIt, moveScore_temp );
				if ( list.size() > 10 ) {
					list.pop_back();
				}
				return;
			}
		}
	}
	// if we've made it here, this is the worst move. add to the back
	list.push_back( moveScore_temp );
	if ( list.size() > 10 ) {//keep the size small
		list.pop_back();
	}
	return;
}


float BgBoard::pointsScore( int *points, int player )
{// given a checker layout points (array[28]), and which player it is, we calculate the score for that position 
	// use to calculate the best move
	//TODO improve!
	//TODO check if the game is a race
	//TODO suggestions for bearing off
	//FIXME get the last two out of there sooner!
	float score = 0;
	float doubleScore = 0;
	
	int p[25];
	
	int playerSign = 1;
	if ( player == 0 ) {
		playerSign = -1;
		for ( int i = 0; i < 25; ++i ) { // point label p is valid for either player
			p[i] = i;     // takes same index as player 0
		}			// home court = 1 through 6 
	}				//  NB home and centre boards unchanged, used 26+player etc
	else {
		for ( int i = 0; i < 25; ++i ) {
			p[i] = 25 - i;
		}
	}
	
		// if the game is a race, move the furthest checkers
	//FIXME for use with doubling cube (deciding when to double) need some way to compare scores calculated different ways
	int sum = 0;
	bool isRace = true;
	for ( int i = 0; i < 19; ++i ) {//just need to check for 1 player!
		if ( ( points[ i ] > 0 ) && ( sum != -15 ) )
			isRace = false;
		if ( points[ i ] < 0 )
			sum += points[ i ];
	}
	if ( sum != -15 ) 
		isRace = false;
	if ( ( points[ 26 ] != 0 ) || ( points[ 27 ] != 0 ) )
		isRace = false;
	// isRace == true, race is on
	// move furthest checkers, bear off if posible
	if ( isRace ) {
		for ( int i = 24; i > 0; --i ) {
			if ( points[ p[ i ] ] * playerSign > 0 ) {
				score -= i * i * points[ p[ i ] ] * playerSign;
			}
		}
		score += points[ 25 * player ] * 100;
		return score;
	}
		
	//doubles are good
	for ( int i = 1; i < 25; ++i ) {   // skip the furthest from home
		if ( points[ p[i] ] * playerSign > 1 )
			score += 10;
	}
	
	//some doubles are even better
	if ( points[ p[3] ] * playerSign > 1 )
		doubleScore += 30;
	if ( points[ p[4] ] * playerSign > 1 )
		doubleScore += 40;
	if ( points[ p[5] ] * playerSign > 1 )
		doubleScore += 60;
	if ( points[ p[6] ] * playerSign > 1 )
		doubleScore += 100;
	if ( points[ p[7] ] * playerSign > 1 )
		doubleScore += 100;
	if ( points[ p[8] ] * playerSign > 1 )
		doubleScore += 40;
	if ( points[ p[9] ] * playerSign > 1 )
		doubleScore += 40;
	if ( points[ p[18] ] * playerSign > 1 )
		doubleScore += 60;
	if ( points[ p[19] ] * playerSign > 1 )
		doubleScore += 60;
	if ( points[ p[20] ] * playerSign > 1 )
		doubleScore += 40;	
	
	for ( int i = 2; i < 12; ++i ) {
		if ( points[ p[i] ] * playerSign > 1 ) {
			doubleScore *= 1.3;  //more blocked points are better
		}
	}
	
	score += doubleScore /2;
	
	//however we don't want too much pileup in the home board
	
	if ( points[ p[1] ] * playerSign > 2 )
		score -= 10 * ( ( points[ p[1] ] * playerSign) - 2 );
	if ( points[ p[2] ] * playerSign > 2 )
		score -= 10 * ( ( points[ p[2] ] * playerSign) - 2 );
	if ( points[ p[3] ] * playerSign > 2 )
		score -= 7 * ( ( points[ p[3] ] * playerSign) - 2 );
	if ( points[ p[4] ] * playerSign > 2 )
		score -= 3 * ( ( points[ p[4] ] * playerSign) - 2 );
	if ( points[ p[5] ] * playerSign > 2 )
		score -= 3 * ( ( points[ p[5] ] * playerSign) - 2 );
	if ( points[ p[6] ] * playerSign > 2 )
		score -= 1 * ( ( points[ p[6] ] * playerSign) - 2 );
	if ( points[ p[7] ] * playerSign > 2 )
		score -= 2 * ( ( points[ p[6] ] * playerSign) - 2 );
	
	// nor in the opponent's home board
	
	if ( points[ p[24] ] * playerSign > 2 )
		score -= 20 * ( ( points[ p[1] ] * playerSign) - 2 );
	if ( points[ p[23] ] * playerSign > 2 )
		score -= 20 * ( ( points[ p[2] ] * playerSign) - 2 );
	if ( points[ p[22] ] * playerSign > 2 )
		score -= 20 * ( ( points[ p[3] ] * playerSign) - 2 );
	if ( points[ p[21] ] * playerSign > 2 )
		score -= 20 * ( ( points[ p[4] ] * playerSign) - 2 );
	if ( points[ p[19] ] * playerSign > 2 )
		score -= 20 * ( ( points[ p[5] ] * playerSign) - 2 );
	if ( points[ p[18] ] * playerSign > 2 )
		score -= 20 * ( ( points[ p[6] ] * playerSign) - 2 );
	
	//opponent's points on centre board is good
	
	if ( points[ 27 - player ] != 0 ) {
		score += 120 * points[ 27 - player ] * points[ 27 - player ];// square takes care of sign--but weights checkers off too heavily
	} // opponents checkers are opposite sign

	//singles within striking distance are bad
	
	for ( int i = 1; i < 25; ++i ) {  
		if ( points[ i ] * playerSign == 1 ) {
			for ( int j = 1; j < 7; ++j ) {
				if ( i + (j*playerSign) < 25 ) { //TODO replace 4 by value prop to probability of value being rolled
					if ( points[ i + (j*playerSign) ] * playerSign < 0 ) { //opponent has checkers there
						score -= (4*i);  // weight, depending on how far from home board checker is
					}
				}
				if ( i + (j*playerSign) == 25) { //coming from opponents centre board
					if ( points[ 27 - player ] * playerSign < 0 ) {
						score -= (4*i);
					}
				}
			}
			for ( int j = 7; j < 13; ++j ) {
				if ( i + (j*playerSign) < 25 ) { 
					if ( points[ i + (j*playerSign) ] * playerSign < 0 ) { //opponent has checkers there
						score -= (2*i);  // weight, depending on how far from home board checker is
					}
				}
				if ( i + (j*playerSign) == 25) {
					if ( points[ 27 - player ] * playerSign < 0 ) {
						score -= (2*i);
					}
				}
			}
		}
	}
	
	// give some incentive to move the furthest two
	
	for ( int i = 13; i < 25; ++i ) {
		if ( points[ p[ i ] ] * playerSign > 0 ) {
			score -= (4*i) * points[ p[ i ] ] * playerSign;
		}
	}
	
	if ( points[ p[24] ] * playerSign < 2 )
		score += 20;
	if ( points[ p[24] ] * playerSign < 1 )
		score += 10;	
	
	return score;				
}
