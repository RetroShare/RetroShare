/*******************************************************************************
 * gui/chat/PopupChatWindow.h                                                  *
 *                                                                             *
 * LibResAPI: API for local socket server                                      *
 *                                                                             *
 * Copyright (C) 2006, Crypton <retroshare.project@gmail.com>                  *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#ifndef _POPUPCHATWINDOW_H
#define _POPUPCHATWINDOW_H

#include <QTimer>
#include "ui_PopupChatWindow.h"
#include <retroshare/rstypes.h>
#include <retroshare/rsmsgs.h>
Q_DECLARE_METATYPE(RsGxsId)
Q_DECLARE_METATYPE(QList<RsGxsId>)

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
	PopupChatWindow(bool tabbed, QWidget *parent = 0, Qt::WindowFlags flags = Qt::WindowFlags());
	/** Default destructor */
	~PopupChatWindow();

	void showEvent(QShowEvent *event);
	void closeEvent(QCloseEvent *event);

private slots:
	void setStyle();
	void getAvatar();
	void tabChanged(ChatDialog *dialog);
	void tabInfoChanged(ChatDialog *dialog);
	void tabNewMessage(ChatDialog *dialog);
	void tabClosed(ChatDialog *dialog);
	void dialogClose(ChatDialog *dialog);
	void dockTab();
	void undockTab();
	void setOnTop();
	void blink(bool on);
	void showContextMenu(QPoint p);
	void voteParticipant();

private:
	bool tabbedWindow;
	bool firstShow;
    ChatId chatId;
	ChatDialog *chatDialog;
	QIcon mBlinkIcon;
	QIcon *mEmptyIcon;
	QAction* votePositive;
	QAction* voteNegative;
	QAction* voteNeutral;

	ChatDialog *getCurrentDialog();
	void saveSettings();
	void calculateStyle(ChatDialog *dialog);

	/** Qt Designer generated object */
	Ui::PopupChatWindow ui;
};

#endif
