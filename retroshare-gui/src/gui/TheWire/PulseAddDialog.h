/*
 * Retroshare Photo Plugin.
 *
 * Copyright 2012-2012 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

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

