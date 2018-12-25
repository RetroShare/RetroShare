/*******************************************************************************
 * gui/TheWire/PulseAddDialog.h                                                *
 *                                                                             *
 * Copyright (c) 2012 Robert Fernie   <retroshare.project@gmail.com>           *
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

#ifndef MRK_PULSE_ADD_DIALOG_H
#define MRK_PULSE_ADD_DIALOG_H

#include "ui_PulseAddDialog.h"

#include <inttypes.h>

class PhotoDetailsDialog;

class PulseAddDialog : public QWidget
{
  Q_OBJECT

public:
	PulseAddDialog(QWidget *parent = 0);

private slots:
	void showPhotoDetails();
	void updateMoveButtons(uint32_t status);

	void addURL();
	void addTo();
	void postPulse();
	void cancelPulse();
	void clearDialog();

protected:

	PhotoDetailsDialog *mPhotoDetails;
	Ui::PulseAddDialog ui;

};

#endif

