/*******************************************************************************
 * gui/statusbar/OpModeStatus.h                                                *
 *                                                                             *
 * Copyright (c) 2008 Retroshare Team <retroshare.project@gmail.com>           *
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

#ifndef OP_MODE_STATUS_H
#define OP_MODE_STATUS_H

#include "gui/common/RSComboBox.h"

class OpModeStatus : public RSComboBox
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

public slots:
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
