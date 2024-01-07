/*******************************************************************************
 * retroshare-gui/src/gui/gxs/GxsStatisticsProvider.cpp                          *
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

#include <QMenu>
#include <QMessageBox>
#include <QToolButton>

#include "GxsStatisticsProvider.h"


#include "gui/notifyqt.h"
#include "gui/common/UserNotify.h"
#include "util/qtthreadsutils.h"
#include "retroshare/rsgxsifacetypes.h"

#define TOKEN_TYPE_GROUP_SUMMARY    1
//#define TOKEN_TYPE_SUBSCRIBE_CHANGE 2
//#define TOKEN_TYPE_CURRENTGROUP     3
#define TOKEN_TYPE_STATISTICS       4

#define MAX_COMMENT_TITLE 32

static const uint32_t DELAY_BETWEEN_GROUP_STATISTICS_UPDATE = 120; // do not update group statistics more often than once every 2 mins

/*
 * Transformation Notes:
 *   there are still a couple of things that the new groups differ from Old version.
 *   these will need to be addressed in the future.
 *     -> Child TS (for sorting) is not handled by GXS, this will probably have to be done in the GUI.
 *     -> Need to handle IDs properly.
 *     -> Much more to do.
 */

/** Constructor */
GxsStatisticsProvider::GxsStatisticsProvider(RsGxsIfaceHelper *ifaceImpl,const QString& settings_name, QWidget *parent,bool allow_dist_sync)
: MainPage(parent),mSettingsName(settings_name)
{
    mDistSyncAllowed = allow_dist_sync;
    mInterface = ifaceImpl;
}

GxsStatisticsProvider::~GxsStatisticsProvider()
{
}