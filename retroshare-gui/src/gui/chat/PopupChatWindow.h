/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006, crypton
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


#ifndef _POPUPCHATWINDOW_H
#define _POPUPCHATWINDOW_H

#include <QTimer>
#include "ui_PopupChatWindow.h"

class ChatDialog;

class PopupChatWindow : public QMainWindow
{
	Q_OBJECT

public:
	static PopupChatWindow *getWindow(bool needSingleWindow);
	static void cleanup();

public:
	void addDialog(ChatDialog *dialog);
	void removeDialog(ChatDialog *dialog);
	void showDialog(ChatDialog *dialog, uint chatflags);
	void alertDialog(ChatDialog *dialog);
	void calculateTitle(ChatDialog *dialog);

protected:
	/** Default constructor */
	PopupChatWindow(bool tabbed, QWidget *parent = 0, Qt::WFlags flags = 0);
	/** Default destructor */
	~PopupChatWindow();

	void showEvent(QShowEvent *event);
	void closeEvent(QCloseEvent *event);

private slots:
	void getAvatar();
	void tabChanged(ChatDialog *dialog);
	void tabInfoChanged(ChatDialog *dialog);
	void tabNewMessage(ChatDialog *dialog);
	void tabClosed(ChatDialog *dialog);
	void dialogClose(ChatDialog *dialog);
	void dockTab();
	void undockTab();
	void setStyle();
	void setOnTop();
	void blink(bool on);

private:
	bool tabbedWindow;
	bool firstShow;
	std::string peerId;
	ChatDialog *chatDialog;
	QIcon mBlinkIcon;
	QIcon *mEmptyIcon;

	ChatDialog *getCurrentDialog();
	void saveSettings();
	void calculateStyle(ChatDialog *dialog);

	/** Qt Designer generated object */
	Ui::PopupChatWindow ui;
};

#endif
