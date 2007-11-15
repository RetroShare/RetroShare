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

#include "bgwidget.h"
#include "bgwindow.h"
#include "bgboard.h"
#include "optionsdlg.h"

BgWidget::BgWidget()
{
	bgBoard = new BgBoard;
	
	connect( bgBoard, SIGNAL( gameOver( int ) ), this, SLOT( changeScore( int ) ) );
	connect( bgBoard, SIGNAL( gameOver( int ) ), this, SLOT( newGame() ) );
	
	for ( int i = 0; i < 2; ++i ) {
		score[ i ] = 0;
		name[ i ] = tr( "Player ") + QString::number( i + 1 ) ;
		nameLabel[ i ] = new QLabel( name[ i ] );
		scoreLabel[ i ] = new QLabel( QString::number( score[ i ] ) );

		bgBoard->setAutoRoll( i, false );
		bgBoard->setPlayerType( i, 0 );
	}
	
	mainGroupBox = new QGroupBox;
	QGridLayout *mainLayout = new QGridLayout;
	
	gridGroupBox = new QGroupBox;
	QGridLayout *layout = new QGridLayout;

	layout->addWidget( nameLabel[0], 0, 0 );
	layout->addWidget( nameLabel[1], 1, 0 );
	layout->addWidget( scoreLabel[0], 0, 1 );
	layout->addWidget( scoreLabel[1], 1, 1 );
	
	mainLayout->addWidget(bgBoard,0,0,3,3);
	mainLayout->addLayout(layout,5,1);

	setLayout(mainLayout);
}

void BgWidget::undo()
{
	bgBoard->undo();
}

void BgWidget::newGame()
{
	bgBoard->startNewGame();
	return;
}

void BgWidget::changeScore( int points )
{//points may be positive or negative; if points > 0 current player has won and points added; if points < 0 current player lost, points added to opponent
	int player = bgBoard->activePlayer();
	if ( points > 0 ) {
		score[ player ] += points;
	}
	else {
		player = 1 - player;
		score[ player ] -= points;
	}
	QString s = QString::number( score[ player ] );
	scoreLabel[ player ]->setText(s);

	QMessageBox::StandardButton reply;

	reply = QMessageBox::information(this, tr("QBackgammon"), name[ player ]+tr( " wins!" ) ); 
	
	return;
}

void BgWidget::hint()
{
	bgBoard->hint();
	return;
}

void BgWidget::options()
{
	optionsDlg = new OptionsDlg( );
	for ( int i = 0; i < 6; ++i ) {
		optionsDlg->color[ i ] = bgBoard->getColor( i );
	}

	for ( int i = 0; i < 2; ++i ) {
		optionsDlg->pLineEdit[ i ]->setText( name[ i ] );
		optionsDlg->pComboBox[ i ]->setCurrentIndex( bgBoard->getPlayerType( i ) );


		if ( bgBoard->getAutoRoll( i ) ) {
			optionsDlg->autoRollCheckBox[ i ]->setCheckState( Qt::Checked );
		}
		else {
			optionsDlg->autoRollCheckBox[ i ]->setCheckState( Qt::Unchecked );
		}
	}


	if ( optionsDlg->exec() ) {
		QString str0 = optionsDlg->pLineEdit[ 0 ]->text();
		nameLabel[ 0 ]->setText( str0 );
		name[ 0 ] = str0;
		QString str1 = optionsDlg->pLineEdit[ 1 ]->text();
		nameLabel[ 1 ]->setText( str1 );
		name[ 1 ] = str1;
		
		for ( int i = 0; i < 6; ++i ) {
			bgBoard->setColor( i, optionsDlg->color[ i ] );
		}
			
		for ( int i = 0; i < 2; ++i ) {
			bgBoard->setPlayerType( i, optionsDlg->pComboBox[ i ]->currentIndex() );

			if ( optionsDlg->autoRollCheckBox[ i ]->checkState() == Qt::Checked ) {
				bgBoard->setAutoRoll( i, true );
			}
			else {
				bgBoard->setAutoRoll( i, false );
			}
		}
	}
}

