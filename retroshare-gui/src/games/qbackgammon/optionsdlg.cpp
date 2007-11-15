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

#include "optionsdlg.h"

OptionsDlg::OptionsDlg( QWidget *parent )
	: QDialog( parent )
{
	rowLayout[ 0 ] = new QHBoxLayout;
	rowLayout[ 1 ] = new QHBoxLayout;
	rowLayout[ 2 ] = new QHBoxLayout;
	rowLayout[ 3 ] = new QHBoxLayout;
	
	for ( int i = 0; i < 2; ++i ) {
		QString str = "Player " + QString::number( i + 1 );
		pGroupBox[ i ] = new QGroupBox( str );
		pLineEdit[ i ] = new QLineEdit;
		pColorButton[ i ] = new QPushButton( tr( "Color" ) );
		pComboBox[ i ] = new QComboBox;
		pComboBox[ i ]->addItem( tr( "Human" ) );
		pComboBox[ i ]->addItem( tr( "Computer" ) );
		autoRollCheckBox[ i ] = new QCheckBox( tr( "auto roll" ) );
	
		rowLayout[ i ]->addWidget( pLineEdit[ i ] );
		rowLayout[ i ]->addWidget( pColorButton[ i ] );
		rowLayout[ i ]->addWidget( pComboBox[ i ] );	
		rowLayout[ i ]->addWidget( autoRollCheckBox[ i ] );
		pGroupBox[ i ]->setLayout( rowLayout[ i ] );
	}
	


	colorGroupBox = new QGroupBox( tr( "Colors" ) );
	colorLabel = new QLabel( tr( "Colors:" ) );
	color1Button = new QPushButton( tr( "Points (1)" ) );
	color2Button = new QPushButton( tr( "Points (2)" ) );
	color3Button = new QPushButton( tr( "Board" ) );
	color4Button = new QPushButton( tr( "Case" ) );
	
	rowLayout[ 2 ]->addWidget( color1Button );
	rowLayout[ 2 ]->addWidget( color2Button );
	rowLayout[ 2 ]->addWidget( color3Button );
	rowLayout[ 2 ]->addWidget( color4Button );
	colorGroupBox->setLayout( rowLayout[ 2 ] );

	
	okButton = new QPushButton( tr( "Ok" ) );
	cancelButton = new QPushButton( tr( "Cancel" ) );
	okButton->setDefault( true ); //FIXME why doesn't this work?
	okButton->setFocus();
	
	rowLayout[ 3 ]->addStretch( 1 );
	rowLayout[ 3 ]->addWidget( okButton );
	rowLayout[ 3 ]->addWidget( cancelButton );

	
	QVBoxLayout *mainLayout = new QVBoxLayout;
	
	mainLayout->addWidget( pGroupBox[ 0 ] );
	mainLayout->addWidget( pGroupBox[ 1 ] );
	mainLayout->addWidget( colorGroupBox );
	mainLayout->addLayout( rowLayout[ 3 ] );

	setLayout( mainLayout );
	
	connect( okButton, SIGNAL( clicked() ), this, SLOT( accept() ) );
	connect( cancelButton, SIGNAL( clicked() ), this, SLOT( reject() ) );
	connect( pColorButton[ 0 ], SIGNAL( clicked() ), this, SLOT( setColor0() ) );
	connect( pColorButton[ 1 ], SIGNAL( clicked() ), this, SLOT( setColor1() ) );
	connect( color1Button, SIGNAL( clicked() ), this, SLOT( setColor2() ) );
	connect( color2Button, SIGNAL( clicked() ), this, SLOT( setColor3() ) );
	connect( color3Button, SIGNAL( clicked() ), this, SLOT( setColor4() ) );
	connect( color4Button, SIGNAL( clicked() ), this, SLOT( setColor5() ) );
	
}


OptionsDlg::~OptionsDlg()
{
}

void OptionsDlg::setColor0()//FIXME how to make these functions into a single 1? (problem, how to connect the pushbuttons)
{
	QColor newColor = QColorDialog::getColor( color[ 0 ], this);
	if ( newColor.isValid() ) 
		color[ 0 ] = newColor;
}

void OptionsDlg::setColor1()
{
	QColor newColor = QColorDialog::getColor( color[ 1 ], this);
	if ( newColor.isValid() ) 
		color[ 1 ] = newColor;
}

void OptionsDlg::setColor2()
{
	QColor newColor = QColorDialog::getColor( color[ 2 ], this);
	if ( newColor.isValid() ) 
		color[ 2 ] = newColor;
}

void OptionsDlg::setColor3()
{
	QColor newColor = QColorDialog::getColor( color[ 3 ], this);
	if ( newColor.isValid() ) 
		color[ 3 ] = newColor;
}

void OptionsDlg::setColor4()
{
	QColor newColor = QColorDialog::getColor( color[ 4 ], this);
	if ( newColor.isValid() ) 
		color[ 4 ] = newColor;
}

void OptionsDlg::setColor5()
{
	QColor newColor = QColorDialog::getColor( color[ 5 ], this);
	if ( newColor.isValid() ) 
		color[ 5 ] = newColor;
}
