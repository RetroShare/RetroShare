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

#include <iostream>

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
	blankDetails();
	if (!mPhotoItem)
	{
		return;
	}

	ui.label_Headline->setText(QString("Photo Description"));

	//ui.comboBox_Category= mPhotoItem->mDetails.mCaption;

	if (mPhotoItem->mIsPhoto)
	{
		// THIS is tedious!

		RsPhotoPhoto &photo = mPhotoItem->mPhotoDetails;
		RsPhotoAlbum &album = mPhotoItem->mAlbumDetails;

        	if (photo.mSetFlags & RSPHOTO_FLAGS_ATTRIB_TITLE)
		{
			ui.lineEdit_Title->setText(QString::fromUtf8(photo.mMeta.mMsgName.c_str()));
		}
		else if (album.mSetFlags & RSPHOTO_FLAGS_ATTRIB_TITLE)
		{
			ui.lineEdit_Title->setText(QString::fromUtf8(album.mMeta.mGroupName.c_str()));
		}

        	if (photo.mSetFlags & RSPHOTO_FLAGS_ATTRIB_CAPTION)
		{
			ui.lineEdit_Caption->setText(QString::fromUtf8(photo.mCaption.c_str()));
		}
		else if (album.mSetFlags & RSPHOTO_FLAGS_ATTRIB_CAPTION)
		{
			ui.lineEdit_Caption->setText(QString::fromUtf8(album.mCaption.c_str()));
		}

        	if (photo.mSetFlags & RSPHOTO_FLAGS_ATTRIB_DESC)
		{
			ui.textEdit_Description->setText(QString::fromUtf8(photo.mDescription.c_str()));
		}
		else if (album.mSetFlags & RSPHOTO_FLAGS_ATTRIB_DESC)
		{
			ui.textEdit_Description->setText(QString::fromUtf8(album.mDescription.c_str()));
		}

		if (photo.mSetFlags & RSPHOTO_FLAGS_ATTRIB_PHOTOGRAPHER)
		{
			ui.lineEdit_Photographer->setText(QString::fromUtf8(photo.mPhotographer.c_str()));
		}
		else if (album.mSetFlags & RSPHOTO_FLAGS_ATTRIB_PHOTOGRAPHER)
		{
			ui.lineEdit_Photographer->setText(QString::fromUtf8(album.mPhotographer.c_str()));
		}

        	if (photo.mSetFlags & RSPHOTO_FLAGS_ATTRIB_WHERE)
		{
			ui.lineEdit_Where->setText(QString::fromUtf8(photo.mWhere.c_str()));
		}
		else if (album.mSetFlags & RSPHOTO_FLAGS_ATTRIB_WHERE)
		{
			ui.lineEdit_Where->setText(QString::fromUtf8(album.mWhere.c_str()));
		}

        	if (photo.mSetFlags & RSPHOTO_FLAGS_ATTRIB_WHEN)
		{
			ui.lineEdit_When->setText(QString::fromUtf8(photo.mWhen.c_str()));
		}
		else if (album.mSetFlags & RSPHOTO_FLAGS_ATTRIB_WHEN)
		{
			ui.lineEdit_When->setText(QString::fromUtf8(album.mWhen.c_str()));
		}

        	if (photo.mSetFlags & RSPHOTO_FLAGS_ATTRIB_OTHER)
		{
			ui.lineEdit_Other->setText(QString::fromUtf8(photo.mOther.c_str()));
		}
		else if (album.mSetFlags & RSPHOTO_FLAGS_ATTRIB_OTHER)
		{
			ui.lineEdit_Other->setText(QString::fromUtf8(album.mOther.c_str()));
		}

        	if (photo.mSetFlags & RSPHOTO_FLAGS_ATTRIB_HASHTAGS)
		{
			ui.lineEdit_HashTags->setText(QString::fromUtf8(photo.mHashTags.c_str()));
		}
		else if (album.mSetFlags & RSPHOTO_FLAGS_ATTRIB_HASHTAGS)
		{
			ui.lineEdit_HashTags->setText(QString::fromUtf8(album.mHashTags.c_str()));
		}
	}
	else
	{
		RsPhotoAlbum &album = mPhotoItem->mAlbumDetails;

		if (album.mSetFlags & RSPHOTO_FLAGS_ATTRIB_TITLE)
		{
			ui.lineEdit_Title->setText(QString::fromUtf8(album.mMeta.mGroupName.c_str()));
		}

		if (album.mSetFlags & RSPHOTO_FLAGS_ATTRIB_CAPTION)
		{
			ui.lineEdit_Caption->setText(QString::fromUtf8(album.mCaption.c_str()));
		}

		if (album.mSetFlags & RSPHOTO_FLAGS_ATTRIB_DESC)
		{
			ui.textEdit_Description->setText(QString::fromUtf8(album.mDescription.c_str()));
		}

		if (album.mSetFlags & RSPHOTO_FLAGS_ATTRIB_PHOTOGRAPHER)
		{
			ui.lineEdit_Photographer->setText(QString::fromUtf8(album.mPhotographer.c_str()));
		}

		if (album.mSetFlags & RSPHOTO_FLAGS_ATTRIB_WHERE)
		{
			ui.lineEdit_Where->setText(QString::fromUtf8(album.mWhere.c_str()));
		}

		if (album.mSetFlags & RSPHOTO_FLAGS_ATTRIB_WHEN)
		{
			ui.lineEdit_When->setText(QString::fromUtf8(album.mWhen.c_str()));
		}

		if (album.mSetFlags & RSPHOTO_FLAGS_ATTRIB_OTHER)
		{
			ui.lineEdit_Other->setText(QString::fromUtf8(album.mOther.c_str()));
		}

		if (album.mSetFlags & RSPHOTO_FLAGS_ATTRIB_HASHTAGS)
		{
			ui.lineEdit_HashTags->setText(QString::fromUtf8(album.mHashTags.c_str()));
		}
	}

	const QPixmap *qtn = mPhotoItem->getPixmap();
	QPixmap cpy(*qtn);
	ui.label_Photo->setPixmap(cpy);
}

void PhotoDetailsDialog::blankDetails()
{
	ui.label_Headline->setText(QString("Nothing"));

	//ui.comboBox_Category= mPhotoItem->mDetails.mCaption;

	ui.lineEdit_Title->setText(QString(""));
	ui.lineEdit_Caption->setText(QString(""));
	ui.textEdit_Description->setText(QString(""));
	ui.lineEdit_Photographer->setText(QString(""));
	ui.lineEdit_Where->setText(QString(""));
	ui.lineEdit_When->setText(QString(""));
	ui.lineEdit_Other->setText(QString(""));
	ui.lineEdit_HashTags->setText(QString(""));

        //QPixmap qtn = mPhotoItem->getPixmap();
	//ui.label_Photo->setPixmap(qtn);
}


void PhotoDetailsDialog::updateDetails()
{
	saveDetails();

	// Notify Listeners.
        editingDone();

	hide();
}

void PhotoDetailsDialog::saveDetails()
{

	if(!mPhotoItem)
	{
		return;
	}

	RsPhotoPhoto &photo = mPhotoItem->mPhotoDetails;
	RsPhotoAlbum &album = mPhotoItem->mAlbumDetails;

	std::string txt = ui.lineEdit_Title->text().toUtf8().constData();
	bool setName = false;
	if ((mPhotoItem->mIsPhoto) && (photo.mSetFlags & RSPHOTO_FLAGS_ATTRIB_TITLE))
	{
		if (txt != photo.mMeta.mMsgName)
			setName = true;
	}
	else if (album.mSetFlags & RSPHOTO_FLAGS_ATTRIB_TITLE)
	{
		if (txt != album.mMeta.mGroupName)
			setName = true;
	}
	else if (txt.length() != 0)
	{
		setName = true;
	}

	if (setName)
	{
		if (mPhotoItem->mIsPhoto)
		{
        		photo.mSetFlags |= RSPHOTO_FLAGS_ATTRIB_TITLE;
        		photo.mModFlags |= RSPHOTO_FLAGS_ATTRIB_TITLE;
			photo.mMeta.mMsgName = txt;
		}
		else
		{
        		album.mSetFlags |= RSPHOTO_FLAGS_ATTRIB_TITLE;
        		album.mModFlags |= RSPHOTO_FLAGS_ATTRIB_TITLE;
			album.mMeta.mGroupName = txt;
		}
	}


	txt = ui.lineEdit_Caption->text().toUtf8().constData();
	setName = false;
	if ((mPhotoItem->mIsPhoto) && (photo.mSetFlags & RSPHOTO_FLAGS_ATTRIB_CAPTION))
	{
		if (txt != photo.mCaption)
			setName = true;
	}
	else if (album.mSetFlags & RSPHOTO_FLAGS_ATTRIB_CAPTION)
	{
		if (txt != album.mCaption)
			setName = true;
	}
	else if (txt.length() != 0)
	{
		setName = true;
	}

	if (setName)
	{
		if (mPhotoItem->mIsPhoto)
		{
        		photo.mSetFlags |= RSPHOTO_FLAGS_ATTRIB_CAPTION;
        		photo.mModFlags |= RSPHOTO_FLAGS_ATTRIB_CAPTION;
			photo.mCaption = txt;
		}
		else
		{
        		album.mSetFlags |= RSPHOTO_FLAGS_ATTRIB_CAPTION;
        		album.mModFlags |= RSPHOTO_FLAGS_ATTRIB_CAPTION;
			album.mCaption = txt;
		}
	}


	txt = ui.textEdit_Description->toPlainText().toUtf8().constData();
	setName = false;
	if ((mPhotoItem->mIsPhoto) && (photo.mSetFlags & RSPHOTO_FLAGS_ATTRIB_DESC))
	{
		if (txt != photo.mDescription)
			setName = true;
	}
	else if (album.mSetFlags & RSPHOTO_FLAGS_ATTRIB_DESC)
	{
		if (txt != album.mDescription)
			setName = true;
	}
	else if (txt.length() != 0)
	{
		setName = true;
	}

	if (setName)
	{
		if (mPhotoItem->mIsPhoto)
		{
        		photo.mSetFlags |= RSPHOTO_FLAGS_ATTRIB_DESC;
        		photo.mModFlags |= RSPHOTO_FLAGS_ATTRIB_DESC;
			photo.mDescription = txt;
		}
		else
		{
        		album.mSetFlags |= RSPHOTO_FLAGS_ATTRIB_DESC;
        		album.mModFlags |= RSPHOTO_FLAGS_ATTRIB_DESC;
			album.mDescription = txt;
		}
	}


	txt = ui.lineEdit_Photographer->text().toUtf8().constData();
	setName = false;
	if ((mPhotoItem->mIsPhoto) && (photo.mSetFlags & RSPHOTO_FLAGS_ATTRIB_PHOTOGRAPHER))
	{
		if (txt != photo.mPhotographer)
			setName = true;
	}
	else if (album.mSetFlags & RSPHOTO_FLAGS_ATTRIB_PHOTOGRAPHER)
	{
		if (txt != album.mPhotographer)
			setName = true;
	}
	else if (txt.length() != 0)
	{
		setName = true;
	}

	if (setName)
	{
		if (mPhotoItem->mIsPhoto)
		{
        		photo.mSetFlags |= RSPHOTO_FLAGS_ATTRIB_PHOTOGRAPHER;
        		photo.mModFlags |= RSPHOTO_FLAGS_ATTRIB_PHOTOGRAPHER;
			photo.mPhotographer = txt;
		}
		else
		{
        		album.mSetFlags |= RSPHOTO_FLAGS_ATTRIB_PHOTOGRAPHER;
        		album.mModFlags |= RSPHOTO_FLAGS_ATTRIB_PHOTOGRAPHER;
			album.mPhotographer = txt;
		}
	}


	txt = ui.lineEdit_Where->text().toUtf8().constData();
	setName = false;
	if ((mPhotoItem->mIsPhoto) && (photo.mSetFlags & RSPHOTO_FLAGS_ATTRIB_WHERE))
	{
		if (txt != photo.mWhere)
			setName = true;
	}
	else if (album.mSetFlags & RSPHOTO_FLAGS_ATTRIB_WHERE)
	{
		if (txt != album.mWhere)
			setName = true;
	}
	else if (txt.length() != 0)
	{
		setName = true;
	}

	if (setName)
	{
		if (mPhotoItem->mIsPhoto)
		{
        		photo.mSetFlags |= RSPHOTO_FLAGS_ATTRIB_WHERE;
        		photo.mModFlags |= RSPHOTO_FLAGS_ATTRIB_WHERE;
			photo.mWhere = txt;
		}
		else
		{
        		album.mSetFlags |= RSPHOTO_FLAGS_ATTRIB_WHERE;
        		album.mModFlags |= RSPHOTO_FLAGS_ATTRIB_WHERE;
			album.mWhere = txt;
		}
	}

	txt = ui.lineEdit_When->text().toUtf8().constData();
	setName = false;
	if ((mPhotoItem->mIsPhoto) && (photo.mSetFlags & RSPHOTO_FLAGS_ATTRIB_WHEN))
	{
		if (txt != photo.mWhen)
			setName = true;
	}
	else if (album.mSetFlags & RSPHOTO_FLAGS_ATTRIB_WHEN)
	{
		if (txt != album.mWhen)
			setName = true;
	}
	else if (txt.length() != 0)
	{
		setName = true;
	}

	if (setName)
	{
		if (mPhotoItem->mIsPhoto)
		{
        		photo.mSetFlags |= RSPHOTO_FLAGS_ATTRIB_WHEN;
        		photo.mModFlags |= RSPHOTO_FLAGS_ATTRIB_WHEN;
			photo.mWhen = txt;
		}
		else
		{
        		album.mSetFlags |= RSPHOTO_FLAGS_ATTRIB_WHEN;
        		album.mModFlags |= RSPHOTO_FLAGS_ATTRIB_WHEN;
			album.mWhen = txt;
		}
	}


	txt = ui.lineEdit_HashTags->text().toUtf8().constData();
	setName = false;
	if ((mPhotoItem->mIsPhoto) && (photo.mSetFlags & RSPHOTO_FLAGS_ATTRIB_HASHTAGS))
	{
		if (txt != photo.mHashTags)
			setName = true;
	}
	else if (album.mSetFlags & RSPHOTO_FLAGS_ATTRIB_HASHTAGS)
	{
		if (txt != album.mHashTags)
			setName = true;
	}
	else if (txt.length() != 0)
	{
		setName = true;
	}

	if (setName)
	{
		if (mPhotoItem->mIsPhoto)
		{
        		photo.mSetFlags |= RSPHOTO_FLAGS_ATTRIB_HASHTAGS;
        		photo.mModFlags |= RSPHOTO_FLAGS_ATTRIB_HASHTAGS;
			photo.mHashTags = txt;
		}
		else
		{
        		album.mSetFlags |= RSPHOTO_FLAGS_ATTRIB_HASHTAGS;
        		album.mModFlags |= RSPHOTO_FLAGS_ATTRIB_HASHTAGS;
			album.mHashTags = txt;
		}
	}


	txt = ui.lineEdit_Other->text().toUtf8().constData();
	setName = false;
	if ((mPhotoItem->mIsPhoto) && (photo.mSetFlags & RSPHOTO_FLAGS_ATTRIB_OTHER))
	{
		if (txt != photo.mOther)
			setName = true;
	}
	else if (album.mSetFlags & RSPHOTO_FLAGS_ATTRIB_OTHER)
	{
		if (txt != album.mOther)
			setName = true;
	}
	else if (txt.length() != 0)
	{
		setName = true;
	}

	if (setName)
	{
		if (mPhotoItem->mIsPhoto)
		{
        		photo.mSetFlags |= RSPHOTO_FLAGS_ATTRIB_OTHER;
        		photo.mModFlags |= RSPHOTO_FLAGS_ATTRIB_OTHER;
			photo.mOther = txt;
		}
		else
		{
        		album.mSetFlags |= RSPHOTO_FLAGS_ATTRIB_OTHER;
        		album.mModFlags |= RSPHOTO_FLAGS_ATTRIB_OTHER;
			album.mOther = txt;
		}
	}



	std::cerr << "PhotoDetailsDialog::saveDetails() ";
	if (mPhotoItem->mIsPhoto)
	{
		std::cerr << " photo.mSetFlags: " << mPhotoItem->mPhotoDetails.mSetFlags;
		std::cerr << " photo.mModFlags: " << mPhotoItem->mPhotoDetails.mModFlags;
	}
	std::cerr << " album.mSetFlags: " << mPhotoItem->mAlbumDetails.mSetFlags;
	std::cerr << " album.mModFlags: " << mPhotoItem->mAlbumDetails.mModFlags;
	std::cerr << std::endl;

        //QPixmap qtn = mPhotoItem->getPixmap();
	//ui.label_Photo->setPixmap(qtn);
}


