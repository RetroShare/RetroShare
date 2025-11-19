/*******************************************************************************
 * retroshare-gui/src/gui/msgs/MessageWindow.h                                 *
 *                                                                             *
 * Copyright (C) 2011 by Retroshare Team     <retroshare.project@gmail.com>    *
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

    MessageWindow(QWidget *parent = 0, Qt::WindowFlags flags = Qt::WindowFlags());
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
