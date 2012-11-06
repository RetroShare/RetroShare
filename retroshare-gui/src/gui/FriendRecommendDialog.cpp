/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2012,  RetroShare Team
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

#include <QMessageBox>

#include "FriendRecommendDialog.h"
#include "ui_FriendRecommendDialog.h"
#include "msgs/MessageComposer.h"
#include "settings/rsharesettings.h"

void FriendRecommendDialog::showYourself()
{
	FriendRecommendDialog *dlg = new FriendRecommendDialog();
	dlg->show();
}

FriendRecommendDialog::FriendRecommendDialog() :
	QDialog(NULL, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint),
	ui(new Ui::FriendRecommendDialog)
{
	ui->setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose, true);

	Settings->loadWidgetInformation(this);

	connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(sendMsg()));
	connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

	ui->recommendList->setHeaderText(tr("Recommend friends"));
	ui->recommendList->setModus(FriendSelectionWidget::MODUS_CHECK);
	ui->recommendList->start();

	ui->toList->setHeaderText(tr("To"));
	ui->toList->setModus(FriendSelectionWidget::MODUS_CHECK);
	ui->toList->start();

	ui->messageEdit->setText(MessageComposer::recommendMessage());
}

FriendRecommendDialog::~FriendRecommendDialog()
{
	Settings->saveWidgetInformation(this);

	delete ui;
}

void FriendRecommendDialog::sendMsg()
{
	std::list<std::string> recommendIds;
	ui->recommendList->selectedSslIds(recommendIds, false);

	if (recommendIds.empty()) {
		QMessageBox::warning(this, "RetroShare", tr("Please select at least one friend for recommendation."), QMessageBox::Ok, QMessageBox::Ok);
		return;
	}

	std::list<std::string> toIds;
	ui->toList->selectedSslIds(toIds, false);

	if (toIds.empty()) {
		QMessageBox::warning(this, "RetroShare", tr("Please select at least one friend as recipient."), QMessageBox::Ok, QMessageBox::Ok);
		return;
	}

	std::list<std::string>::iterator toId;
	for (toId = toIds.begin(); toId != toIds.end(); toId++) {
		MessageComposer::recommendFriend(recommendIds, *toId, ui->messageEdit->toHtml(), true);
	}

	done(Accepted);
}
