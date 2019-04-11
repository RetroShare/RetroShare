/*******************************************************************************
 * gui/common/AvatarDialog.h                                                   *
 *                                                                             *
 * Copyright (C) 2012, Robert Fernie <retroshare.project@gmail.com>            *
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
