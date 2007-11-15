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

#ifndef BGWIDGET_H
#define BGWIDGET_H

#include <QFrame>
#include <QWidget>

class BgBoard;
class QLabel;
class QLCDNumber;
class QGroupBox;
class QLineEdit;
class OptionsDlg;
class QColor;

class BgWidget : public QWidget
{
	Q_OBJECT

	public:
		BgWidget();
			
	public slots:
		void undo();
		void newGame();
		void changeScore( int );
		void hint();
		void options();
			
	signals:
		void widget_undoCalled();
		void widget_newGame();
		void widget_hint();
		
	private:
		BgBoard *bgBoard;		
		QLabel *labela;
		QLabel *labelb;
		
		QString name [ 2];
		QGroupBox *horizontalGroupBox;
		
		QGroupBox *gridGroupBox;
		QGroupBox *mainGroupBox;
		QLabel *nameLabel [2];
		
		QLabel *scoreLabel [2];
		QLineEdit *lineEdits [2];
		
		int score [2];
		
		OptionsDlg *optionsDlg;
};


#endif // BGWIDGET_H

