/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006 - 2011, RetroShare Team
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

#ifndef _MESSAGEWINDOW_H
#define _MESSAGEWINDOW_H

#include "gui/common/rwindow.h"
#include "ui_MessageWindow.h"

class MessageWidget;

class MessageWindow : public RWindow
{
    Q_OBJECT

public:
    /** Default Constructor */

    MessageWindow(QWidget *parent = 0, Qt::WindowFlags flags = 0);
    ~MessageWindow();

	void addWidget(MessageWidget *widget);

private slots:
	void newmessage();
	void tagAboutToShow();
	void tagSet(int tagId, bool set);
	void tagRemoveAll();

	void buttonStyle();

private:
	void setupFileActions();
	void processSettings(bool load);
	void setToolbarButtonStyle(Qt::ToolButtonStyle style);

	MessageWidget *msgWidget;
	QAction *actionSaveAs;
	QAction *actionPrint;
	QAction *actionPrintPreview;

    /** Qt Designer generated object */
    Ui::MessageWindow ui;
};

#endif
