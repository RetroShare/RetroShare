/****************************************************************
 *
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2011, RetroShare Team
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

#ifndef CHATTABWIDGET_H
#define CHATTABWIDGET_H

#include "gui/common/RSTabWidget.h"

namespace Ui {
class ChatTabWidget;
}

class ChatDialog;

class ChatTabWidget : public RSTabWidget
{
	Q_OBJECT

public:
	explicit ChatTabWidget(QWidget *parent = 0);
	~ChatTabWidget();

	void addDialog(ChatDialog *dialog);
	void removeDialog(ChatDialog *dialog);

	void getInfo(bool &isTyping, bool &hasNewMessage, QIcon *icon);
	void setBlinking(int tab, bool blink);

signals:
	void tabChanged(ChatDialog *dialog);
	void tabClosed(ChatDialog *dialog);
	void infoChanged();

private slots:
	void tabClose(int tab);
	void tabChanged(int tab);
	void tabInfoChanged(ChatDialog *dialog);
	void dialogClose(ChatDialog *dialog);
	void blink(bool on);

private:
	QIcon *mEmptyIcon;

	Ui::ChatTabWidget *ui;
};

#endif // CHATTABWIDGET_H
