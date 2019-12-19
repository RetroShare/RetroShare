/*******************************************************************************
 * retroshare-gui/src/gui/Posted/PostedCreatePostDialog.cpp                    *
 *                                                                             *
 * Copyright (C) 2013 by Robert Fernie       <retroshare.project@gmail.com>    *
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
#include <QMessageBox>
#include "PostedCreatePostDialog.h"
#include "ui_PostedCreatePostDialog.h"

#include "util/misc.h"
#include "util/TokenQueue.h"
#include "util/MRichTextEdit.h"
#include "gui/feeds/SubFileItem.h"
#include "util/rsdir.h"

#include "gui/settings/rsharesettings.h"
#include <QBuffer>

#include <iostream>

PostedCreatePostDialog::PostedCreatePostDialog(TokenQueue* tokenQ, RsPosted *posted, const RsGxsGroupId& grpId, QWidget *parent):
	QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint),
	mTokenQueue(tokenQ), mPosted(posted), mGrpId(grpId),
	ui(new Ui::PostedCreatePostDialog)
{
	ui->setupUi(this);
	Settings->loadWidgetInformation(this);

	connect(ui->submitButton, SIGNAL(clicked()), this, SLOT(createPost()));
	connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(close()));
	connect(ui->pushButton, SIGNAL(clicked() ), this , SLOT(addPicture()));
	
	ui->headerFrame->setHeaderImage(QPixmap(":/icons/png/postedlinks.png"));
	ui->headerFrame->setHeaderText(tr("Create a new Post"));

	setAttribute ( Qt::WA_DeleteOnClose, true );

	ui->MRichTextEditWidget->setPlaceHolderTextPosted();
	
	/* fill in the available OwnIds for signing */
	ui->idChooser->loadIds(IDCHOOSER_ID_REQUIRED, RsGxsId());
}

PostedCreatePostDialog::~PostedCreatePostDialog()
{
	Settings->saveWidgetInformation(this);
	delete ui;
}

void PostedCreatePostDialog::createPost()
{
	RsGxsId authorId;
	switch (ui->idChooser->getChosenId(authorId)) {
		case GxsIdChooser::KnowId:
		case GxsIdChooser::UnKnowId:
		break;
		case GxsIdChooser::NoId:
		case GxsIdChooser::None:
		default:
		std::cerr << "PostedCreatePostDialog::createPost() ERROR GETTING AuthorId!, Post Failed";
		std::cerr << std::endl;

		QMessageBox::warning(this, tr("RetroShare"),tr("Please create or choose a Signing Id first"), QMessageBox::Ok, QMessageBox::Ok);

		return;
	}//switch (ui->idChooser->getChosenId(authorId))

	RsPostedPost post;
	post.mMeta.mGroupId = mGrpId;
	post.mLink = std::string(ui->linkEdit->text().toUtf8());
	
	QString text;
	text = ui->MRichTextEditWidget->toHtml();
	post.mNotes = std::string(text.toUtf8());

	post.mMeta.mAuthorId = authorId;

	if(!ui->titleEdit->text().isEmpty())
	{ 
		post.mMeta.mMsgName = std::string(ui->titleEdit->text().toUtf8());
	}else
	{
		post.mMeta.mMsgName = std::string(ui->titleEditLink->text().toUtf8());
	}

	QByteArray ba;
	QBuffer buffer(&ba);
	
	if(!picture.isNull())
	{
		// send posted image

		buffer.open(QIODevice::WriteOnly);
		picture.save(&buffer, "PNG"); // writes image into ba in PNG format
		post.mImage.copy((uint8_t *) ba.data(), ba.size());
	}
	
	if(ui->titleEdit->text().isEmpty()&& ui->titleEditLink->text().isEmpty()) {
		/* error message */
		QMessageBox::warning(this, "RetroShare", tr("Please add a Title"), QMessageBox::Ok, QMessageBox::Ok);
		return; //Don't add  a empty title!!
	}//if(ui->titleEdit->text().isEmpty())

	uint32_t token;
	mPosted->createPost(token, post);
//	mTokenQueue->queueRequest(token, TOKENREQ_MSGINFO, RS_TOKREQ_ANSTYPE_ACK, TOKEN_USER_TYPE_POST);

	accept();
}

void PostedCreatePostDialog::addPicture()
{
	QPixmap img = misc::getOpenThumbnailedPicture(this, tr("Load thumbnail picture"), 800, 600);

	if (img.isNull())
		return;

	picture = img;

	// to show the selected
	ui->imageLabel->setPixmap(picture);
}

void PostedCreatePostDialog::on_postButton_clicked()
{
	ui->stackedWidget->setCurrentIndex(0);
}

void PostedCreatePostDialog::on_imageButton_clicked()
{
	ui->stackedWidget->setCurrentIndex(1);
}

void PostedCreatePostDialog::on_linkButton_clicked()
{
	ui->stackedWidget->setCurrentIndex(2);
}
