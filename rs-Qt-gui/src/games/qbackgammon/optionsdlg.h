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

#ifndef OPTIONSDLG_H
#define OPTIONSDLG_H

#include <QDialog>

class QLabel;
class QLineEdit;
class QGroupBox;
class QPushButton;
class QHBoxLayout;
class QComboBox;
class QCheckBox;

class OptionsDlg : public QDialog
{
	Q_OBJECT

	public:
		OptionsDlg( QWidget *parent = 0 );
	
		~OptionsDlg();
		QLineEdit *pLineEdit [2];
		
		QColor color [6];
		QComboBox *pComboBox [2];
		
		QCheckBox *autoRollCheckBox [2];
	
	public slots:
		void setColor0();
		void setColor1();	
		void setColor2();	
		void setColor3();	
		void setColor4();	
		void setColor5();	
	
	private:
		QGroupBox *pGroupBox [2];
		QGroupBox *colorGroupBox;
	
		QPushButton *pColorButton [2];
	
		QLabel *colorLabel;
		QPushButton *color1Button;
		QPushButton *color2Button;
		QPushButton *color3Button;
		QPushButton *color4Button;
	
		
		QPushButton *okButton;
		QPushButton *cancelButton;
			
		QHBoxLayout *rowLayout [4];
};

#endif
