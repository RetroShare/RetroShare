/*******************************************************************************
 * retroshare-gui/src/gui/gxs/GxsUserNotify.cpp                                *
 *                                                                             *
 * Copyright 2014 Retroshare Team           <retroshare.project@gmail.com>     *
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

#include "GxsUserNotify.h"
//#include "gui/gxs/RsGxsUpdateBroadcastBase.h"

#include "retroshare/rsgxsifacehelper.h"
#include "util/qtthreadsutils.h"

#define TOKEN_TYPE_STATISTICS  1

GxsUserNotify::GxsUserNotify(RsGxsIfaceHelper */*ifaceImpl*/, const GxsStatisticsProvider *g,QObject *parent) : UserNotify(parent), mGxsStatisticsProvider(g)
{
	mNewThreadMessageCount = 0;
	mNewChildMessageCount = 0;
	mCountChildMsgs = false;
}

GxsUserNotify::~GxsUserNotify() {}

void GxsUserNotify::startUpdate()
{
	mNewThreadMessageCount = 0;
	mNewChildMessageCount = 0;


	GxsServiceStatistic stats;
    mGxsStatisticsProvider->getServiceStatistics(stats);

	/* Here it goes any code you want to be executed on the Qt Gui
			 * thread, for example to update the data model with new information
			 * after a blocking call to RetroShare API complete */

	mNewThreadMessageCount = stats.mNumThreadMsgsNew;
	mNewChildMessageCount = stats.mNumChildMsgsNew;

	update();
}

