/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008 Robert Fernie
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

#ifndef _GXSFORUMSDIALOG_H
#define _GXSFORUMSDIALOG_H

#include "gui/gxs/GxsGroupFrameDialog.h"

#define IMAGE_GXSFORUMS         ":/images/konversation.png"

class GxsForumsDialog : public GxsGroupFrameDialog
{
	Q_OBJECT

public:
	GxsForumsDialog(QWidget *parent = 0);
	~GxsForumsDialog();

	virtual QIcon iconPixmap() const { return QIcon(IMAGE_GXSFORUMS) ; } //MainPage
	virtual QString pageName() const { return tr("Forums") ; } //MainPage
	virtual QString helpText() const { return ""; } //MainPage

//	virtual UserNotify *getUserNotify(QObject *parent);

private slots:
	void settingsChanged();

private:
	/* GxsGroupFrameDialog */
	virtual QString text(TextType type);
	virtual QString icon(IconType type);
	virtual QString settingsGroupName() { return "GxsForumsDialog"; }
	virtual GxsGroupDialog *createNewGroupDialog(TokenQueue *tokenQueue);
	virtual GxsGroupDialog *createGroupDialog(TokenQueue *tokenQueue, RsTokenService *tokenService, GxsGroupDialog::Mode mode, RsGxsGroupId groupId);
	virtual int shareKeyType();
	virtual GxsMessageFrameWidget *createMessageFrameWidget(const RsGxsGroupId &groupId);
};

#endif
