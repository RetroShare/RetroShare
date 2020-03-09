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
#include <QByteArray>
#include <QStringList>
#include <QSignalMapper>

#include "PostedCreatePostDialog.h"
#include "ui_PostedCreatePostDialog.h"

#include "util/misc.h"
#include "util/TokenQueue.h"
#include "util/RichTextEdit.h"
#include "gui/feeds/SubFileItem.h"
#include "util/rsdir.h"

#include "gui/settings/rsharesettings.h"
#include <QBuffer>

#include <iostream>
#include <gui/RetroShareLink.h>
#include <util/imageutil.h>

/* View Page */
#define VIEW_POST  1
#define VIEW_IMAGE  2
#define VIEW_LINK  3

PostedCreatePostDialog::PostedCreatePostDialog(TokenQueue* tokenQ, RsPosted *posted, const RsGxsGroupId& grpId, QWidget *parent):
	QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint),
	mPosted(posted), mGrpId(grpId),
	ui(new Ui::PostedCreatePostDialog)
{
	ui->setupUi(this);
	Settings->loadWidgetInformation(this);

	connect(ui->submitButton, SIGNAL(clicked()), this, SLOT(createPost()));
	connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(close()));
	connect(ui->addPicButton, SIGNAL(clicked() ), this , SLOT(addPicture()));
	
	ui->headerFrame->setHeaderImage(QPixmap(":/icons/png/postedlinks.png"));
	ui->headerFrame->setHeaderText(tr("Create a new Post"));

	setAttribute ( Qt::WA_DeleteOnClose, true );

	ui->RichTextEditWidget->setPlaceHolderTextPosted();

	ui->hashBox->setAutoHide(true);
	ui->hashBox->setDefaultTransferRequestFlags(RS_FILE_REQ_ANONYMOUS_ROUTING);
	connect(ui->hashBox, SIGNAL(fileHashingFinished(QList<HashedFile>)), this, SLOT(fileHashingFinished(QList<HashedFile>)));
	ui->sizeWarningLabel->setText(QString("Post size is limited to %1 KB, pictures will be downscaled.").arg(MAXMESSAGESIZE / 1024));
	
	/* fill in the available OwnIds for signing */
	ui->idChooser->loadIds(IDCHOOSER_ID_REQUIRED, RsGxsId());
	
	QSignalMapper *signalMapper = new QSignalMapper(this);
	connect(ui->postButton, SIGNAL(clicked()), signalMapper, SLOT(map()));
	connect(ui->imageButton, SIGNAL(clicked()), signalMapper, SLOT(map()));
	connect(ui->linkButton, SIGNAL(clicked()), signalMapper, SLOT(map()));

	signalMapper->setMapping(ui->postButton, VIEW_POST);
	signalMapper->setMapping(ui->imageButton, VIEW_IMAGE);
	signalMapper->setMapping(ui->linkButton, VIEW_LINK);
	connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(setPage(int)));
	
	ui->removeButton->hide();

	/* load settings */
	processSettings(true);
}

PostedCreatePostDialog::~PostedCreatePostDialog()
{
	Settings->saveWidgetInformation(this);
	
	// save settings
	processSettings(false);

	delete ui;
}

void PostedCreatePostDialog::processSettings(bool load)
{
	Settings->beginGroup(QString("PostedCreatePostDialog"));

	if (load) {
		// load settings
		
		// state of ID Chooser combobox
		int index = Settings->value("IDChooser", 0).toInt();
		ui->idChooser->setCurrentIndex(index);
		
		// load last used Stacked Page
		setPage(Settings->value("viewPage", VIEW_POST).toInt());
	} else {
		// save settings

		// state of ID Chooser combobox
		Settings->setValue("IDChooser", ui->idChooser->currentIndex());
		
		// store last used Page
		Settings->setValue("viewPage", viewMode());
	}

	Settings->endGroup();
}

void PostedCreatePostDialog::createPost()
{
	if(ui->titleEdit->text().isEmpty()) {
		/* error message */
		QMessageBox::warning(this, "RetroShare", tr("Please add a Title"), QMessageBox::Ok, QMessageBox::Ok);
		return; //Don't add  a empty title!!
	}

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
	
	if(!ui->RichTextEditWidget->toPlainText().trimmed().isEmpty()) {
		QString text;
		text = ui->RichTextEditWidget->toHtml();
		post.mNotes = std::string(text.toUtf8());
	}

	post.mMeta.mAuthorId = authorId;
	post.mMeta.mMsgName = std::string(ui->titleEdit->text().toUtf8());
	
	if(imagebytes.size() > 0)
	{
		// send posted image
		post.mImage.copy((uint8_t *) imagebytes.data(), imagebytes.size());
	}	

	int msgsize = post.mLink.length() + post.mMeta.mMsgName.length() + post.mNotes.length() + imagebytes.size();
	if(msgsize > MAXMESSAGESIZE) {
		QString errormessage = QString(tr("Message is too large.<br />actual size: %1 bytes, maximum size: %2 bytes.")).arg(msgsize).arg(MAXMESSAGESIZE);
		QMessageBox::warning(this, "RetroShare", errormessage, QMessageBox::Ok, QMessageBox::Ok);
		return;
	}

	uint32_t token;
	mPosted->createPost(token, post);

	accept();
}

void PostedCreatePostDialog::fileHashingFinished(QList<HashedFile> hashedFiles)
{
	if(hashedFiles.length() > 0) { //It seems like it returns 0 if hashing cancelled
		HashedFile hashedFile = hashedFiles[0]; //Should be exactly one file
		RetroShareLink link;
		link = RetroShareLink::createFile(hashedFile.filename, hashedFile.size, QString::fromStdString(hashedFile.hash.toStdString()));
		ui->linkEdit->setText(link.toString());
	}
	ui->submitButton->setEnabled(true);
	ui->addPicButton->setEnabled(true);
}

void PostedCreatePostDialog::addPicture()
{	
	imagefilename = "";
	imagebytes.clear();
	QPixmap empty;
	ui->imageLabel->setPixmap(empty);

	// select a picture file
	if (misc::getOpenFileName(window(), RshareSettings::LASTDIR_IMAGES, tr("Load Picture File"), "Pictures (*.png *.xpm *.jpg *.jpeg *.gif *.webp )", imagefilename)) {
		QString encodedImage;
		QImage image;
		if (image.load(imagefilename) == false) {
			fprintf (stderr, "RsHtml::makeEmbeddedImage() - image \"%s\" can't be load\n", imagefilename.toLatin1().constData());
			imagefilename = "";
			return;
		}

		QImage opt;
		if(ImageUtil::optimizeSizeBytes(imagebytes, image, opt, 640*480, MAXMESSAGESIZE - 2000)) { //Leave space for other stuff
			ui->imageLabel->setPixmap(QPixmap::fromImage(opt));
			ui->stackedWidgetPicture->setCurrentIndex(1);
			ui->removeButton->show();
		} else {
			imagefilename = "";
			imagebytes.clear();
			return;
		}
	}

	//Do we need to hash the image?
	QMessageBox::StandardButton answer;
	answer = QMessageBox::question(this, tr("Post image"), tr("Do you want to share and link the original image?"), QMessageBox::Yes|QMessageBox::No);
	if (answer == QMessageBox::Yes) {
		if(!ui->linkEdit->text().trimmed().isEmpty()) {
			answer = QMessageBox::question(this, tr("Post image"), tr("You already added a link.<br />Do you want to replace it?"), QMessageBox::Yes|QMessageBox::No);
		}
	}

	//If still yes then link it
	if(answer == QMessageBox::Yes) {
		ui->submitButton->setEnabled(false);
		ui->addPicButton->setEnabled(false);
		QStringList files;
		files.append(imagefilename);
		ui->hashBox->addAttachments(files,RS_FILE_REQ_ANONYMOUS_ROUTING);
	}
	

}

int PostedCreatePostDialog::viewMode()
{
	if (ui->postButton->isChecked()) {
		return VIEW_POST;
	} else if (ui->imageButton->isChecked()) {
		return VIEW_IMAGE;
	} else if (ui->linkButton->isChecked()) {
		return VIEW_LINK;
	}

	/* Default */
	return VIEW_POST;
}

void PostedCreatePostDialog::setPage(int viewMode)
{
	switch (viewMode) {
	case VIEW_POST:
		ui->stackedWidget->setCurrentIndex(0);

		ui->postButton->setChecked(true);
		ui->imageButton->setChecked(false);
		ui->linkButton->setChecked(false);

		break;
	case VIEW_IMAGE:
		ui->stackedWidget->setCurrentIndex(1);

		ui->imageButton->setChecked(true);
		ui->postButton->setChecked(false);
		ui->linkButton->setChecked(false);

		break;
	case VIEW_LINK:
		ui->stackedWidget->setCurrentIndex(2);

		ui->linkButton->setChecked(true);
		ui->postButton->setChecked(false);
		ui->imageButton->setChecked(false);

		break;
	default:
		setPage(VIEW_POST);
		return;
	}
}

void PostedCreatePostDialog::on_removeButton_clicked()
{
	imagefilename = "";
	imagebytes.clear();
	QPixmap empty;
	ui->imageLabel->setPixmap(empty);
	ui->removeButton->hide();
	ui->stackedWidgetPicture->setCurrentIndex(0);
}
