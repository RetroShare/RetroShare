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
#ifndef OP_MODE_STATUS_H
#define OP_MODE_STATUS_H

#include <QComboBox>

class OpModeStatus : public QComboBox
{
	Q_OBJECT
	Q_PROPERTY(QColor opMode_Full_Color READ getOpMode_Full_Color WRITE setOpMode_Full_Color DESIGNABLE true)
	Q_PROPERTY(QColor opMode_NoTurtle_Color READ getOpMode_NoTurtle_Color WRITE setOpMode_NoTurtle_Color DESIGNABLE true)
	Q_PROPERTY(QColor opMode_Gaming_Color READ getOpMode_Gaming_Color WRITE setOpMode_Gaming_Color DESIGNABLE true)
	Q_PROPERTY(QColor opMode_Minimal_Color READ getOpMode_Minimal_Color WRITE setOpMode_Minimal_Color DESIGNABLE true)

public:
	OpModeStatus(QWidget *parent = 0);

	QColor getOpMode_Full_Color() const;
	void setOpMode_Full_Color( QColor c );

	QColor getOpMode_NoTurtle_Color() const;
	void setOpMode_NoTurtle_Color( QColor c );

	QColor getOpMode_Gaming_Color() const;
	void setOpMode_Gaming_Color( QColor c );

	QColor getOpMode_Minimal_Color() const;
	void setOpMode_Minimal_Color( QColor c );

private slots:
	void setOpMode();

private:
	void getOpMode();

	QColor opMode_Full_Color;
	QColor opMode_NoTurtle_Color;
	QColor opMode_Gaming_Color;
	QColor opMode_Minimal_Color;
	bool onUpdate;
};

#endif
