/*******************************************************************************
 * retroshare-gui/src/gui/gxs/GxsStatisticsProvider.h                            *
 *                                                                             *
 * Copyright 2012-2013  by Robert Fernie      <retroshare.project@gmail.com>   *
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

#ifndef GXSSTATISTICSPROVIDER_H
#define GXSSTATISTICSPROVIDER_H

#include <retroshare-gui/RsAutoUpdatePage.h>

#include "gui/gxs/RsGxsUpdateBroadcastPage.h"
#include "gui/RetroShareLink.h"
#include "gui/settings/rsharesettings.h"
#include "util/RsUserdata.h"

#include <inttypes.h>

#include "GxsIdTreeWidgetItem.h"
#include "GxsGroupDialog.h"

class GroupTreeWidget;
class GroupItemInfo;
class GxsMessageFrameWidget;
class UIStateHelper;
struct RsGxsCommentService;
class GxsCommentDialog;

class GxsStatisticsProvider  : public MainPage
{
    Q_OBJECT

public:
    GxsStatisticsProvider(RsGxsIfaceHelper *ifaceImpl, const QString& settings_name,QWidget *parent = 0,bool allow_dist_sync=false);
    virtual ~GxsStatisticsProvider();

    virtual void getServiceStatistics(GxsServiceStatistic& stats) const = 0;

protected:
    virtual void updateGroupStatistics(const RsGxsGroupId &groupId) = 0;
    virtual void updateGroupStatisticsReal(const RsGxsGroupId &groupId) = 0;

    // This needs to be overloaded by subsclasses, possibly calling the blocking API, since it is used asynchroneously.
    virtual bool getGroupStatistics(const RsGxsGroupId& groupId,GxsGroupStatistic& stat) =0;

    QString mSettingsName;
    RsGxsIfaceHelper *mInterface;
    bool mDistSyncAllowed;
    std::map<RsGxsGroupId,GxsGroupStatistic> mCachedGroupStats;
    bool mShouldUpdateGroupStatistics;
    std::set<RsGxsGroupId> mGroupStatisticsToUpdate;
    bool mCountChildMsgs; // Count unread child messages?

private:
    virtual QString settingsGroupName() = 0;
};
#endif // GXSSTATISTICSPROVIDER_H
