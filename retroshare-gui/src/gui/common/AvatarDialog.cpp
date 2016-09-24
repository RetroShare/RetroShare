/*
 * Avatar Dialog
 *
 * Copyright 2015 by RetroShare Team
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

AvatarDialog::~AvatarDialog()
{
	delete(ui);
}

void AvatarDialog::changeAvatar()
{
	QPixmap img = misc::getOpenThumbnailedPicture(this, tr("Load Avatar"), 128, 128);

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
