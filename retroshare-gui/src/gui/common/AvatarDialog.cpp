/*******************************************************************************
 * gui/common/AvatarDialog.cpp                                                 *
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

#include <QBuffer>

#include "AvatarDialog.h"
#include "ui_AvatarDialog.h"
#include "AvatarDefs.h"
#include "util/misc.h"

/** Constructor */
AvatarDialog::AvatarDialog(QWidget *parent) :
    QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint),
    ui(new(Ui::AvatarDialog))
{
	/* Invoke Qt Designer generated QObject setup routine */
	ui->setupUi(this);

	ui->headerFrame->setHeaderImage(QPixmap(":/images/no_avatar_70.png"));
	ui->headerFrame->setHeaderText(tr("Set your Avatar picture"));

	connect(ui->avatarButton, SIGNAL(clicked(bool)), this, SLOT(changeAvatar()));
	connect(ui->removeButton, SIGNAL(clicked(bool)), this, SLOT(removeAvatar()));

	connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
	connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

	updateInterface();
}

const int AvatarDialog::RS_AVATAR_IMAGE_W = 128;
const int AvatarDialog::RS_AVATAR_IMAGE_H = 128;

AvatarDialog::~AvatarDialog()
{
	delete(ui);
}

void AvatarDialog::changeAvatar()
{
	QPixmap img = misc::getOpenThumbnailedPicture(this, tr("Load Avatar"), RS_AVATAR_IMAGE_W,RS_AVATAR_IMAGE_H);

	if (img.isNull())
		return;

	ui->avatarLabel->setPixmap(img);
	updateInterface();
}

void AvatarDialog::removeAvatar()
{
	ui->avatarLabel->setPixmap(QPixmap());
	updateInterface();
}

void AvatarDialog::updateInterface()
{
	const QPixmap *pixmap = ui->avatarLabel->pixmap();
	if (pixmap && !pixmap->isNull()) {
		ui->removeButton->setEnabled(true);
	} else {
		ui->removeButton->setEnabled(false);
	}
}

void AvatarDialog::setAvatar(const QPixmap &avatar)
{
	ui->avatarLabel->setPixmap(avatar);
	updateInterface();
}

void AvatarDialog::getAvatar(QPixmap &avatar)
{
	const QPixmap *pixmap = ui->avatarLabel->pixmap();
	if (!pixmap) {
		avatar = QPixmap();
		return;
	}

	avatar = *pixmap;
}

void AvatarDialog::getAvatar(QByteArray &avatar)
{
	const QPixmap *pixmap = ui->avatarLabel->pixmap();
	if (!pixmap) {
		avatar.clear();
		return;
	}

	QBuffer buffer(&avatar);

	buffer.open(QIODevice::WriteOnly);
	pixmap->save(&buffer, "PNG"); // writes image into ba in PNG format
}
