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

#ifndef _GXSFORUM_GROUP_DIALOG_H
#define _GXSFORUM_GROUP_DIALOG_H

#include "gui/gxs/GxsGroupDialog.h"
#include <retroshare/rsgxsforums.h>

class GxsForumGroupDialog : public GxsGroupDialog
{
	Q_OBJECT

public:
	GxsForumGroupDialog(TokenQueue *tokenQueue, QWidget *parent);
	GxsForumGroupDialog(TokenQueue *tokenExternalQueue, RsTokenService *tokenService, Mode mode, RsGxsGroupId groupId, QWidget *parent);

protected:
	virtual void initUi();
	virtual QPixmap serviceImage();
	virtual bool service_CreateGroup(uint32_t &token, const RsGroupMetaData &meta);
	virtual bool service_loadGroup(uint32_t token, Mode mode, RsGroupMetaData& groupMetaData, QString &description);
	virtual bool service_EditGroup(uint32_t &token, RsGroupMetaData &editedMeta);

private:
    RsGxsForumGroup mGroupData;
};

#endif
