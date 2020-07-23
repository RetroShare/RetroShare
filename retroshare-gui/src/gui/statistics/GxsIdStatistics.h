/*******************************************************************************
 * gui/statistics/GxsIdStatistics.h                                            *
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

#include <QPoint>
#include <retroshare/rsidentity.h>
#include <retroshare/rstypes.h>

#include "RsAutoUpdatePage.h"
#include "Histogram.h"
#include "ui_GxsIdStatistics.h"

// In this statistics panel we show:
//
// 	- histograms
// 		* age histogram of GXS ids (creation time)
// 		* last usage histogram
// 		* number of IDs used in each service as reported by UsageStatistics
//
//     (note: we could use that histogram class for packets statistics, so we made a separate class)
//
// And general statistics:
//
// 	- total number of IDs
// 	- total number of signed IDs
// 	- total number of own IDs
//
class GxsIdStatisticsWidget ;

class GxsIdStatistics: public RsAutoUpdatePage, public Ui::GxsIdStatistics
{
	Q_OBJECT

	public:
		GxsIdStatistics(QWidget *parent = NULL) ;
		~GxsIdStatistics();

		void updateContent() ;

	private:

		void processSettings(bool bLoad);
		bool m_bProcessSettings;

		virtual void updateDisplay() ;

		GxsIdStatisticsWidget *_tst_CW ;
} ;

class GxsIdStatisticsWidget:  public QWidget
{
	Q_OBJECT

	public:
		GxsIdStatisticsWidget(QWidget *parent = NULL) ;

		virtual void paintEvent(QPaintEvent *event) ;
		virtual void resizeEvent(QResizeEvent *event);

		void updateContent() ;
		void updateData();
	private:
		static QString speedString(float f) ;

		QPixmap pixmap ;
		int mMaxWidth,mMaxHeight ;
		uint32_t mNbWeeks;
		uint32_t mNbHours;
        uint32_t mTotalIdentities;
        Histogram mPublishDateHist ;
        Histogram mLastUsedHist    ;

		std::map<RsIdentityUsage::UsageCode,int> mUsageMap;
		std::map<uint32_t,int> mPerServiceUsageMap;
};

