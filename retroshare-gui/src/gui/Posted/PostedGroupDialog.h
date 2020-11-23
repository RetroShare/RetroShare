/*******************************************************************************
 * retroshare-gui/src/gui/Posted/PostedGroupDialog.h                           *
 *                                                                             *
 * Copyright (C) 2013 by Robert Fernie       <retroshare.project@gmail.com>    *
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


#ifndef _POSTED_GROUP_DIALOG_H
#define _POSTED_GROUP_DIALOG_H

#include "gui/gxs/GxsGroupDialog.h"
#include <retroshare/rsposted.h>

class PostedGroupDialog : public GxsGroupDialog
{
	Q_OBJECT

public:
	PostedGroupDialog(QWidget *parent);
	PostedGroupDialog(Mode mode, RsGxsGroupId groupId, QWidget *parent);

protected:
	void initUi() override;
	QPixmap serviceImage() override;
	bool service_createGroup(RsGroupMetaData& meta) override;
	bool service_loadGroup(const RsGxsGenericGroupData *data,Mode mode, QString &description) override;
	bool service_updateGroup(const RsGroupMetaData& editedMeta) override;
    bool service_getGroupData(const RsGxsGroupId& grpId,RsGxsGenericGroupData *& data) override;

private:
	void preparePostedGroup(RsPostedGroup &group, const RsGroupMetaData &meta);
};

#endif
