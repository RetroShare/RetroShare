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

#include "gui/PhotoShare/PhotoDetailsDialog.h"
#include "gui/PhotoShare/PhotoItem.h"

/** Constructor */
PhotoDetailsDialog::PhotoDetailsDialog(QWidget *parent)
: QWidget(parent)
{
	ui.setupUi(this);
	connect( ui.pushButton_Update, SIGNAL( clicked() ), this, SLOT( updateDetails () ) );

}


void PhotoDetailsDialog::setPhotoItem(PhotoItem *item)
{
	if (mPhotoItem == item)
	{
		return;
	}

	mPhotoItem = item;

	/* update fields from the edit fields */
	refreshDetails();
}


void PhotoDetailsDialog::refreshDetails()
{

	if(!mPhotoItem)
	{
		blankDetails();
	}

	ui.label_Headline->setText(QString("Photo Description"));

	//ui.comboBox_Category= mPhotoItem->mDetails.mCaption;

	if (mPhotoItem->mIsPhoto)
	{
		ui.lineEdit_Caption->setText(QString::fromStdString(mPhotoItem->mPhotoDetails.mCaption));
		ui.textEdit_Description->setText(QString::fromStdString(mPhotoItem->mPhotoDetails.mDescription));
		ui.lineEdit_Photographer->setText(QString::fromStdString(mPhotoItem->mPhotoDetails.mPhotographer));
		ui.lineEdit_Where->setText(QString::fromStdString(mPhotoItem->mPhotoDetails.mWhere));
		ui.lineEdit_When->setText(QString::fromStdString(mPhotoItem->mPhotoDetails.mWhen));
		ui.lineEdit_Other->setText(QString::fromStdString(mPhotoItem->mPhotoDetails.mOther));
		ui.lineEdit_Title->setText(QString::fromStdString(mPhotoItem->mPhotoDetails.mMeta.mMsgName));
		ui.lineEdit_HashTags->setText(QString::fromStdString(mPhotoItem->mPhotoDetails.mHashTags));
	}
	else
	{
		ui.lineEdit_Caption->setText(QString::fromStdString(mPhotoItem->mAlbumDetails.mCaption));
		ui.textEdit_Description->setText(QString::fromStdString(mPhotoItem->mAlbumDetails.mDescription));
		ui.lineEdit_Photographer->setText(QString::fromStdString(mPhotoItem->mAlbumDetails.mPhotographer));
		ui.lineEdit_Where->setText(QString::fromStdString(mPhotoItem->mAlbumDetails.mWhere));
		ui.lineEdit_When->setText(QString::fromStdString(mPhotoItem->mAlbumDetails.mWhen));
		ui.lineEdit_Other->setText(QString::fromStdString(mPhotoItem->mAlbumDetails.mOther));
		ui.lineEdit_Title->setText(QString::fromStdString(mPhotoItem->mAlbumDetails.mMeta.mGroupName));
		ui.lineEdit_HashTags->setText(QString::fromStdString(mPhotoItem->mAlbumDetails.mHashTags));
	}

	const QPixmap *qtn = mPhotoItem->getPixmap();
	QPixmap cpy(*qtn);
	ui.label_Photo->setPixmap(cpy);
}

void PhotoDetailsDialog::blankDetails()
{
	ui.label_Headline->setText(QString("Nothing"));

	//ui.comboBox_Category= mPhotoItem->mDetails.mCaption;

	ui.lineEdit_Caption->setText(QString("N/A"));
	ui.textEdit_Description->setText(QString("N/A"));
	ui.lineEdit_Photographer->setText(QString("N/A"));
	ui.lineEdit_Where->setText(QString("N/A"));
	ui.lineEdit_When->setText(QString("N/A"));
	ui.lineEdit_Other->setText(QString("N/A"));
	ui.lineEdit_Title->setText(QString("N/A"));
	ui.lineEdit_HashTags->setText(QString("N/A"));

        //QPixmap qtn = mPhotoItem->getPixmap();
	//ui.label_Photo->setPixmap(qtn);
}


void PhotoDetailsDialog::updateDetails()
{
	saveDetails();

	hide();
}

void PhotoDetailsDialog::saveDetails()
{

	if(!mPhotoItem)
	{
		return;
	}

	//mPhotoItem->mDetails.mCaption = ui.comboBox_Category;

	if (mPhotoItem->mIsPhoto)
	{
		mPhotoItem->mPhotoDetails.mCaption = ui.lineEdit_Caption->text().toStdString();
		mPhotoItem->mPhotoDetails.mDescription = ui.textEdit_Description->toPlainText().toStdString();
		mPhotoItem->mPhotoDetails.mPhotographer = ui.lineEdit_Photographer->text().toStdString();
		mPhotoItem->mPhotoDetails.mWhere = ui.lineEdit_Where->text().toStdString();
		mPhotoItem->mPhotoDetails.mWhen = ui.lineEdit_When->text().toStdString();
		mPhotoItem->mPhotoDetails.mOther = ui.lineEdit_Other->text().toStdString();
		mPhotoItem->mPhotoDetails.mMeta.mMsgName = ui.lineEdit_Title->text().toStdString();
		mPhotoItem->mPhotoDetails.mHashTags = ui.lineEdit_HashTags->text().toStdString();
	}
	else
	{
		mPhotoItem->mAlbumDetails.mCaption = ui.lineEdit_Caption->text().toStdString();
		mPhotoItem->mAlbumDetails.mDescription = ui.textEdit_Description->toPlainText().toStdString();
		mPhotoItem->mAlbumDetails.mPhotographer = ui.lineEdit_Photographer->text().toStdString();
		mPhotoItem->mAlbumDetails.mWhere = ui.lineEdit_Where->text().toStdString();
		mPhotoItem->mAlbumDetails.mWhen = ui.lineEdit_When->text().toStdString();
		mPhotoItem->mAlbumDetails.mOther = ui.lineEdit_Other->text().toStdString();
		mPhotoItem->mAlbumDetails.mMeta.mGroupName = ui.lineEdit_Title->text().toStdString();
		mPhotoItem->mAlbumDetails.mHashTags = ui.lineEdit_HashTags->text().toStdString();
	}

        //QPixmap qtn = mPhotoItem->getPixmap();
	//ui.label_Photo->setPixmap(qtn);
}


