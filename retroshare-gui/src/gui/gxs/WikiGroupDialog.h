/*******************************************************************************
 * retroshare-gui/src/gui/gxs/RsGxsUpdateBroadcastWidget.h                     *
 *                                                                             *
 * Copyright 2012-2013 by Robert Fernie     <retroshare.project@gmail.com>     *
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

#ifndef _WIKI_GROUP_DIALOG_H
#define _WIKI_GROUP_DIALOG_H

#include "GxsGroupDialog.h"
#include "retroshare/rswiki.h"

class WikiGroupDialog : public GxsGroupDialog
{
	Q_OBJECT

public:
	WikiGroupDialog(QWidget *parent);
	WikiGroupDialog(Mode mode, RsGxsGroupId groupId, QWidget *parent = NULL);

protected:
	virtual void initUi() override;
	virtual QPixmap serviceImage() override;
	virtual bool service_createGroup(RsGroupMetaData &meta) override;
	virtual bool service_updateGroup(const RsGroupMetaData &editedMeta) override;
	virtual bool service_loadGroup(const RsGxsGenericGroupData *data, Mode mode, QString &description) override;
	virtual bool service_getGroupData(const RsGxsGroupId &groupId, RsGxsGenericGroupData *&data) override;

private:

    RsWikiCollection mGrp;

};

#endif

