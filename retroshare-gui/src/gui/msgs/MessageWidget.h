/*******************************************************************************
 * retroshare-gui/src/gui/msgs/MessageWidget.h                                 *
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

#ifndef _MESSAGEWIDGET_H
#define _MESSAGEWIDGET_H

#include <QWidget>
#include "ui_MessageWidget.h"

class QToolButton;
class QAction;
class QTextEdit;

class MessageWidget : public QWidget
{
	Q_OBJECT

public:
	enum enumActionType {
		ACTION_REMOVE,
		ACTION_REPLY,
		ACTION_REPLY_ALL,
		ACTION_FORWARD,
		ACTION_PRINT,
		ACTION_PRINT_PREVIEW,
		ACTION_SAVE_AS
	};

public:
	MessageWidget(bool controlled, QWidget *parent = 0, Qt::WindowFlags flags = 0);
	~MessageWidget();

	static MessageWidget *openMsg(const std::string &msgId, bool window);

    std::string msgId() { return currMsgId; }
	void connectAction(enumActionType actionType, QToolButton* button);
	void connectAction(enumActionType actionType, QAction* action);

	void fill(const std::string &msgId);
	void processSettings(const QString &settingsGroup, bool load);

	QString subject(bool noEmpty);


private slots:
	void reply();
	void replyAll();
	void forward();
	void remove();
	void print();
	void printPreview();
	void saveAs();
	void refill();
	void sendInvite();


	void msgfilelistWidgetCostumPopupMenu(QPoint);
	void messagesTagsChanged();
	void messagesChanged();

	void togglefileview();
	void getcurrentrecommended();
	void getallrecommended();

	void anchorClicked(const QUrl &url);

	void loadImagesAlways();
	void buttonStyle();
	void viewSource();

private:
	void clearTagLabels();
	void showTagLabels();
	void setToolbarButtonStyle(Qt::ToolButtonStyle style);

	bool isControlled;
	bool isWindow;
	std::string currMsgId;
	unsigned int currMsgFlags;

	QList<QLabel*> tagLabels;

	QToolButton* toolButtonReply;

	/** Qt Designer generated object */
	Ui::MessageWidget ui;
};

#endif
