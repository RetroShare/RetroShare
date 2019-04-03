/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008 RetroShare Team
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/
#ifndef RATESSTATUS_H
#define RATESSTATUS_H

#include <QWidget>

class QLabel;

class RatesStatus : public QWidget
{
    Q_OBJECT

public:
    RatesStatus(QWidget *parent = 0);

	void getRatesStatus(float downKb, uint64_t down, float upKb, uint64_t upl);
    void setCompactMode(bool compact) {_compactMode = compact; }

private:
    QLabel *iconLabel, *statusRates;
    bool _compactMode;
};

#endif
