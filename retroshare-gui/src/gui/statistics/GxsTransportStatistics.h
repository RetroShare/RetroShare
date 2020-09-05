/*******************************************************************************
 * gui/statistics/GxsTransportStatistics.h                                     *
 *                                                                             *
 * Copyright (c) 2011 Retroshare Team <retroshare.project@gmail.com>           *
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

#pragma once

#include <map>

#include <QPoint>
#include <retroshare/rsgrouter.h>
#include <retroshare/rstypes.h>
#include <retroshare/rsgxstrans.h>

#include <retroshare-gui/RsAutoUpdatePage.h>

#include "ui_GxsTransportStatistics.h"
#include "gui/gxs/RsGxsUpdateBroadcastPage.h"
#include "util/rstime.h"

class GxsTransportStatisticsWidget ;
class UIStateHelper;

class GxsTransportStatistics: public MainPage, public Ui::GxsTransportStatistics
{
	Q_OBJECT

public:
	GxsTransportStatistics(QWidget *parent = NULL) ;
	~GxsTransportStatistics();

	// Cache for peer names.
	static QString getPeerName(const RsPeerId& peer_id) ;

	void updateContent() ;

private slots:
	/** Create the context popup menu and it's submenus */
	void CustomPopupMenu( QPoint point );
	void CustomPopupMenuGroups( QPoint point ) ;

	void personDetails();
	void showAuthorInPeople();

private:
	void updateDisplay(bool complete) ;
	void loadGroups();

	void processSettings(bool bLoad);
	bool m_bProcessSettings;

	GxsTransportStatisticsWidget *_tst_CW ;
	UIStateHelper *mStateHelper;
	uint32_t mLastGroupReqTS ;

	// temporary storage of retrieved data, for display (useful because this is obtained from the async token system)

	std::map<RsGxsGroupId,RsGxsTransGroupStatistics> mGroupStats ;	// stores the list of active groups and statistics about each of them.
} ;
