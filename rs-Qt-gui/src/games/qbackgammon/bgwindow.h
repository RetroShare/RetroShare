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

#ifndef BGWINDOW_H
#define BGWINDOW_H

#include <QMainWindow>

class QAction;
class QLabel;
class QMenu;
class QTextEdit;
class QPushButton;
class BgWidget;

class BgWindow : public QMainWindow
{
	Q_OBJECT

	public:
		//BgWindow();
		
	    /** Default Constructor */
    BgWindow(QWidget *parent = 0, Qt::WFlags flags = 0);
    
        /** Destructor. */
    ~BgWindow();
	
	signals:
		void undoCalled();
		void newGameCalled();
		void WchangeScore();
		void callQuit();
		
protected:
  void closeEvent (QCloseEvent * event);

    public slots:
    /** Called when this dialog is to be displayed */
    void show();
    
	private slots:
		void newGame();
		void undo();
		void quit();
		void resign();
		void about();
		
	private:
		void createActions();
		void createMenus();
		void updateActions();
	
		BgWidget *bgWidget;
		
		QAction *newAct;
		QAction *undoAct;
		QAction *resignAct;
		QAction *hintAct;
		QAction *quitAct;
		QAction *optionsAct;
		QAction *aboutAct;
		
		QMenu *fileMenu;
		

};

#endif
