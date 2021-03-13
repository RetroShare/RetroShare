/*******************************************************************************
 * gui/statusbar/torstatus.h                                                   *
 *                                                                             *
 * Copyright (c) 2012 Retroshare Team <retroshare.project@gmail.com>           *
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

#include <QWidget>

class QLabel;

class TorStatus : public QWidget
{
    Q_OBJECT
public:
    TorStatus(QWidget *parent = 0);

    void getTorStatus( );
    void setCompactMode(bool compact) {_compactMode = compact; }
	void reset()
	{
		_updated = false;
	}

private:
    QLabel *torstatusLabel, *statusTor;
    bool _compactMode;
    bool _updated;
};

