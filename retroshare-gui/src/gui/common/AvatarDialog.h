/*
 * Avatar Dialog.
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

#ifndef _AVATARDIALOG_H
#define _AVATARDIALOG_H

#include <QDialog>

class QPixmap;
class QByteArray;

namespace Ui {
class AvatarDialog;
}

class AvatarDialog : public QDialog
{
	Q_OBJECT

public:
	AvatarDialog(QWidget *parent = 0);
	~AvatarDialog();

	void setAvatar(const QPixmap &avatar);

	void getAvatar(QPixmap &avatar);
	void getAvatar(QByteArray &avatar);

private slots:
	void changeAvatar();
	void removeAvatar();

private:
	void updateInterface();

private:
	Ui::AvatarDialog *ui;
};

#endif
