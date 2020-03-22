/*******************************************************************************
 * retroshare-gui/src/gui/gxsforums/GxsForumsDialog.h                          *
 *                                                                             *
 * Copyright 2013 Robert Fernie        <retroshare.project@gmail.com>          *
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

#ifndef _GXSFORUMSDIALOG_H
#define _GXSFORUMSDIALOG_H

#include "gui/gxs/GxsGroupFrameDialog.h"

#define IMAGE_GXSFORUMS         ":/icons/png/forums.png"

class GxsForumsDialog : public GxsGroupFrameDialog
{
	Q_OBJECT

public:
	GxsForumsDialog(QWidget *parent = 0);
	~GxsForumsDialog();

	virtual QIcon iconPixmap() const { return QIcon(IMAGE_GXSFORUMS) ; } //MainPage
	virtual QString pageName() const { return tr("Forums") ; } //MainPage
	virtual QString helpText() const { return ""; } //MainPage

	void shareInMessage(const RsGxsGroupId& forum_id, const QList<RetroShareLink>& file_link) ;
	
protected:
	virtual QString getHelpString() const ;
	virtual RetroShareLink::enumType getLinkType() { return RetroShareLink::TYPE_FORUM; }
	virtual GroupFrameSettings::Type groupFrameSettingsType() { return GroupFrameSettings::Forum; }

	UserNotify *createUserNotify(QObject *parent) override;
	bool getGroupData(std::list<RsGxsGenericGroupData*>& groupInfo) override;
	bool getGroupStatistics(const RsGxsGroupId& groupId,GxsGroupStatistic& stat) override;
	void groupInfoToGroupItemInfo(const RsGxsGenericGroupData *groupData, GroupItemInfo &groupItemInfo) override;

private:
	/* GxsGroupFrameDialog */
	QString text(TextType type)override ;
	QString icon(IconType type)override ;
	QString settingsGroupName() override { return "ForumsDialog"; }

	GxsGroupDialog *createNewGroupDialog() override;
	GxsGroupDialog *createGroupDialog(GxsGroupDialog::Mode mode, RsGxsGroupId groupId) override;

	int shareKeyType() override;
	GxsMessageFrameWidget *createMessageFrameWidget(const RsGxsGroupId &groupId) override;
	uint32_t requestGroupSummaryType() override { return GXS_REQUEST_TYPE_GROUP_DATA; } // request complete group data

	void handleEvent_main_thread(std::shared_ptr<const RsEvent> event);

    RsEventsHandlerId_t mEventHandlerId;
};

#endif
