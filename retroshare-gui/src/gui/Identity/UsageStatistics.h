/*******************************************************************************
 * retroshare-gui/src/gui/Identity/UsageStatistics.h                           *
 *                                                                             *
 * Copyright (C) 2026 by Retroshare Team     <retroshare.project@gmail.com>    *
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

#ifndef USAGESTATISTICS_H
#define USAGESTATISTICS_H

#include "gui/gxs/RsGxsUpdateBroadcastPage.h"

#include "retroshare/rsidentity.h"

namespace Ui {
class UsageStatistics;
}


class UsageStatistics : public QWidget
{
	Q_OBJECT

public:
	UsageStatistics(QWidget *parent = 0);
	~UsageStatistics();

    void setUsageData(RsGxsIdGroup data);
    
protected:


private slots:


private:
	QString createUsageString(const RsIdentityUsage& u) const;

	/* UI -  Designer */
	Ui::UsageStatistics *ui;
};

#endif
