/***************************************************************************
 *   Copyright (C) 2008 by normal   *
 *   normal@Desktop2   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef FORM_MAIN_H
#define FORM_MAIN_H

#include <QtGui>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QMenu>
#include <QCursor>
#include <QPoint>
#include <QMouseEvent>
#include <QSystemTrayIcon>
#include <QSettings>
#include <QFileDialog>

#include "ui_form_Main.h"
#include "gui_icons.h"
#include "form_settingsgui.h"
#include "form_newUser.h"
#include "form_DebugMessages.h"
#include "form_chatwidget.h"
#include "form_rename.h"
#include "form_HelpDialog.h"
#include "src/Core.h"
#include "src/User.h"


class form_MainWindow : public QMainWindow, private Ui::form_MainWindow
{
		Q_OBJECT

	public:
		form_MainWindow ( QWidget* parent=0 );
		~form_MainWindow();
		
	protected:
    	//void closeEvent(QCloseEvent *);

	signals:
		void closeAllWindows();

	private slots:
		//Windows
			void openConfigWindow();
			void openAdduserWindow();
			void openDebugMessagesWindow();
			void openAboutDialog();
			void openChatDialog ();
		//Windows end
		void namingMe();
		void copyDestination();
		void SendFile();
		void closeApplication();
		void eventUserChanged();
		void muteSound();
		

		void connecttreeWidgetCostumPopupMenu ( QPoint point );
		void deleteUserClicked();
		void userRenameCLicked();
		//void updateMenu();
		void onlineComboBoxChanged();
        	//void toggleVisibility(QSystemTrayIcon::ActivationReason e);
        	//void toggleVisibilitycontextmenu();
		void OnlineStateChanged();
	private:
		void init();
		void initStyle();
		//void initTryIconMenu();
		//void initTryIcon();
		void initToolBars();

		void fillComboBox();
		
		cCore* Core;
		bool applicationIsClosing;
		
		cConnectionI2P* I2P;
		form_newUserWindow* newUserWindow;
		
		QSystemTrayIcon *trayIcon;
        	QAction* toggleVisibilityAction, *toolAct;
		QAction* toggleMuteAction;
        	QMenu *menu;
		bool Mute;

};
#endif
