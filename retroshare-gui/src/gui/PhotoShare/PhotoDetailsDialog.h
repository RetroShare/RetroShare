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

#ifndef MRK_PHOTO_DETAILS_DIALOG_H
#define MRK_PHOTO_DETAILS_DIALOG_H

#include "ui_PhotoDetailsDialog.h"

class PhotoItem;

class PhotoDetailsDialog : public QWidget
{
  Q_OBJECT

public:
	PhotoDetailsDialog(QWidget *parent = 0);

void	setPhotoItem(PhotoItem *item);

signals:
	void editingDone();

private:
void	saveDetails();
void	refreshDetails();

private slots:
void	updateDetails();
void	blankDetails();

private:
	PhotoItem *mPhotoItem;

	Ui::PhotoDetailsDialog ui;

};

#endif

