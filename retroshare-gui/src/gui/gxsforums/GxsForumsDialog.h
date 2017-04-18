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

#define IMAGE_GXSFORUMS         ":/icons/png/forums.png"

class GxsForumsDialog : public GxsGroupFrameDialog
{
	Q_OBJECT

public:
	explicit GxsForumsDialog(QWidget *parent = 0);
	~GxsForumsDialog();

	virtual QIcon iconPixmap() const { return QIcon(IMAGE_GXSFORUMS) ; } //MainPage
	virtual QString pageName() const { return tr("Forums") ; } //MainPage
	virtual QString helpText() const { return ""; } //MainPage

	virtual UserNotify *getUserNotify(QObject *parent);

protected:
	virtual QString getHelpString() const ;
	virtual RetroShareLink::enumType getLinkType() { return RetroShareLink::TYPE_FORUM; }
	virtual GroupFrameSettings::Type groupFrameSettingsType() { return GroupFrameSettings::Forum; }
	virtual void groupInfoToGroupItemInfo(const RsGroupMetaData &groupInfo, GroupItemInfo &groupItemInfo, const RsUserdata *userdata);

private:
	/* GxsGroupFrameDialog */
	virtual QString text(TextType type);
	virtual QString icon(IconType type);
	virtual QString settingsGroupName() { return "ForumsDialog"; }
	virtual GxsGroupDialog *createNewGroupDialog(TokenQueue *tokenQueue);
	virtual GxsGroupDialog *createGroupDialog(TokenQueue *tokenQueue, RsTokenService *tokenService, GxsGroupDialog::Mode mode, RsGxsGroupId groupId);
	virtual int shareKeyType();
	virtual GxsMessageFrameWidget *createMessageFrameWidget(const RsGxsGroupId &groupId);
	virtual uint32_t requestGroupSummaryType() { return GXS_REQUEST_TYPE_GROUP_DATA; } // request complete group data
	virtual void loadGroupSummaryToken(const uint32_t &token, std::list<RsGroupMetaData> &groupInfo, RsUserdata* &userdata);
};

#endif
