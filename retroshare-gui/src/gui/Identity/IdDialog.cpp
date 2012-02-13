/*
 * Retroshare Identity.
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

#include "IdDialog.h"

#include <retroshare/rspeers.h>
#include <retroshare/rsidentity.h>

#include <iostream>
#include <sstream>

#include <QTimer>

/******
 * #define ID_DEBUG 1
 *****/


/****************************************************************
 */


#define RSID_COL_NICKNAME	0
#define RSID_COL_KEYID		1
#define RSID_COL_IDTYPE		2



/** Constructor */
IdDialog::IdDialog(QWidget *parent)
: MainPage(parent)
{
	ui.setupUi(this);

	mEditDialog = NULL;
	//mPulseSelected = NULL;

	ui.radioButton_ListAll->setChecked(true);
	connect( ui.pushButton_NewId, SIGNAL(clicked()), this, SLOT(OpenOrShowAddDialog()));
	connect( ui.pushButton_EditId, SIGNAL(clicked()), this, SLOT(OpenOrShowEditDialog()));
	connect( ui.treeWidget_IdList, SIGNAL(itemSelectionChanged()), this, SLOT(updateSelection()));

	connect( ui.radioButton_ListYourself, SIGNAL(toggled( bool ) ), this, SLOT(ListTypeToggled( bool ) ) );
	connect( ui.radioButton_ListFriends, SIGNAL(toggled( bool ) ), this, SLOT(ListTypeToggled( bool ) ) );
	connect( ui.radioButton_ListOthers, SIGNAL(toggled( bool ) ), this, SLOT(ListTypeToggled( bool ) ) );
	connect( ui.radioButton_ListPseudo, SIGNAL(toggled( bool ) ), this, SLOT(ListTypeToggled( bool ) ) );
	connect( ui.radioButton_ListAll, SIGNAL(toggled( bool ) ), this, SLOT(ListTypeToggled( bool ) ) );

	QTimer *timer = new QTimer(this);
	timer->connect(timer, SIGNAL(timeout()), this, SLOT(checkUpdate()));
	timer->start(1000);

	rsIdentity->generateDummyData();

}

void IdDialog::ListTypeToggled(bool checked)
{
        if (checked)
        {
                insertIdList();
        }
}



void IdDialog::updateSelection()
{
	/* */
	QTreeWidgetItem *item = ui.treeWidget_IdList->currentItem();

	if (!item)
	{
		/* blank it all - and fix buttons */
		ui.lineEdit_Nickname->setText("");
		ui.lineEdit_KeyId->setText("");
		ui.lineEdit_GpgHash->setText("");
		ui.lineEdit_GpgId->setText("");
		ui.lineEdit_GpgName->setText("");
		ui.lineEdit_GpgEmail->setText("");

		ui.pushButton_Reputation->setEnabled(false);
		ui.pushButton_Delete->setEnabled(false);
		ui.pushButton_EditId->setEnabled(false);
		ui.pushButton_NewId->setEnabled(true);
	}
	else
	{
		/* get details from libretroshare */
		RsIdData data;
		if (!rsIdentity->getIdentity(item->text(RSID_COL_KEYID).toStdString(), data))
		{
			ui.lineEdit_KeyId->setText("ERROR GETTING KEY!");
			return;
		}

                /* get GPG Details from rsPeers */
                std::string gpgid  = rsPeers->getGPGOwnId();
                RsPeerDetails details;
                rsPeers->getPeerDetails(gpgid, details);

                ui.lineEdit_Nickname->setText(QString::fromStdString(data.mNickname));
                ui.lineEdit_KeyId->setText(QString::fromStdString(data.mKeyId));
                ui.lineEdit_GpgHash->setText(QString::fromStdString(data.mGpgIdHash));
                ui.lineEdit_GpgId->setText(QString::fromStdString(data.mGpgId));
                ui.lineEdit_GpgName->setText(QString::fromStdString(data.mGpgName));
                ui.lineEdit_GpgEmail->setText(QString::fromStdString(data.mGpgEmail));

		if (data.mIdType & RSID_RELATION_YOURSELF)
		{
			ui.radioButton_IdYourself->setChecked(true);
		}
		else if (data.mIdType & RSID_TYPE_PSEUDONYM)
		{
			ui.radioButton_IdPseudo->setChecked(true);
		}
		else if (data.mIdType & RSID_RELATION_FRIEND)
		{
			ui.radioButton_IdFriend->setChecked(true);
		}
		else if (data.mIdType & RSID_RELATION_FOF)
		{
			ui.radioButton_IdFOF->setChecked(true);
		}
		else 
		{
			ui.radioButton_IdOther->setChecked(true);
		}

		ui.pushButton_NewId->setEnabled(true);
		if (data.mIdType & RSID_RELATION_YOURSELF)
		{
			ui.pushButton_Reputation->setEnabled(false);
			ui.pushButton_Delete->setEnabled(true);
			ui.pushButton_EditId->setEnabled(true);
		}
		else
		{
			ui.pushButton_Reputation->setEnabled(true);
			ui.pushButton_Delete->setEnabled(false);
			ui.pushButton_EditId->setEnabled(false);
		}
	}
}

#if 0

void IdDialog::notifySelection(PulseItem *item, int ptype)
{
        std::cerr << "IdDialog::notifySelection() from : " << ptype << " " << item;
        std::cerr << std::endl;

	notifyPulseSelection(item);

	switch(ptype)
	{
		default:
		case PHOTO_ITEM_TYPE_ALBUM:
			notifyAlbumSelection(item);
			break;
		case PHOTO_ITEM_TYPE_PHOTO:
			notifyPhotoSelection(item);
			break;
	}
}

void IdDialog::notifyPulseSelection(PulseItem *item)
{
	std::cerr << "IdDialog::notifyPulseSelection() from : " << item;
	std::cerr << std::endl;
	
	if (mPulseSelected)
	{
		std::cerr << "IdDialog::notifyPulseSelection() unselecting old one : " << mPulseSelected;
		std::cerr << std::endl;
	
		mPulseSelected->setSelected(false);
	}
	
	mPulseSelected = item;
}

#endif

void IdDialog::checkUpdate()
{
	/* update */
	if (!rsIdentity)
		return;

	if (rsIdentity->updated())
	{
		insertIdList();
	}
	return;
}


/*************** New Photo Dialog ***************/

void IdDialog::OpenOrShowAddDialog()
{
	if (!mEditDialog)
	{
		mEditDialog = new IdEditDialog(NULL);
	}
	bool pseudo = false;
	mEditDialog->setupNewId(pseudo);

	mEditDialog->show();

}


void IdDialog::OpenOrShowEditDialog()
{
	if (!mEditDialog)
	{
		mEditDialog = new IdEditDialog(NULL);
	}


	/* */
	QTreeWidgetItem *item = ui.treeWidget_IdList->currentItem();

	if (!item)
	{
		std::cerr << "IdDialog::OpenOrShowEditDialog() Invalid item";
		std::cerr << std::endl;
		return;
	}

	std::string keyId = item->text(RSID_COL_KEYID).toStdString();
	mEditDialog->setupExistingId(keyId);

	mEditDialog->show();
}


bool IdDialog::matchesAlbumFilter(const RsPhotoAlbum &album)
{

	return true;
}

double IdDialog::AlbumScore(const RsPhotoAlbum &album)
{
	return 1;
}


bool IdDialog::matchesPhotoFilter(const RsPhotoPhoto &photo)
{

	return true;
}

double IdDialog::PhotoScore(const RsPhotoPhoto &photo)
{
	return 1;
}


bool IdDialog::FilterNSortAlbums(const std::list<std::string> &albumIds, std::list<std::string> &filteredAlbumIds, int count)
{
#if 0
	std::multimap<double, std::string> sortedAlbums;
	std::multimap<double, std::string>::iterator sit;
	std::list<std::string>::const_iterator it;
	
	for(it = albumIds.begin(); it != albumIds.end(); it++)
	{
		RsPhotoAlbum album;
		rsPhoto->getAlbum(*it, album);

		if (matchesAlbumFilter(album))
		{
			double score = AlbumScore(album);

			sortedAlbums.insert(std::make_pair(score, *it));
		}
	}

	int i;
	for (sit = sortedAlbums.begin(), i = 0; (sit != sortedAlbums.end()) && (i < count); sit++, i++)
	{
		filteredAlbumIds.push_back(sit->second);
	}
#endif
	return true;
}


bool IdDialog::FilterNSortPhotos(const std::list<std::string> &photoIds, std::list<std::string> &filteredPhotoIds, int count)
{
#if 0
	std::multimap<double, std::string> sortedPhotos;
	std::multimap<double, std::string>::iterator sit;
	std::list<std::string>::const_iterator it;

	int i = 0;
	for(it = photoIds.begin(); it != photoIds.end(); it++, i++)
	{
		RsPhotoPhoto photo;
		rsPhoto->getPhoto(*it, photo);

		if (matchesPhotoFilter(photo))
		{
			double score = i; //PhotoScore(album);
			sortedPhotos.insert(std::make_pair(score, *it));
		}
	}

	for (sit = sortedPhotos.begin(), i = 0; (sit != sortedPhotos.end()) && (i < count); sit++, i++)
	{
		filteredPhotoIds.push_back(sit->second);
	}
#endif
	return true;
}



void IdDialog::insertIdList()
{
	QTreeWidget *tree = ui.treeWidget_IdList;

	tree->clear();

	std::list<std::string> ids;
	std::list<std::string>::iterator it;

	bool acceptAll = ui.radioButton_ListAll->isChecked();
	bool acceptPseudo = ui.radioButton_ListPseudo->isChecked();
	bool acceptYourself = ui.radioButton_ListYourself->isChecked();
	bool acceptFriends = ui.radioButton_ListFriends->isChecked();
	bool acceptOthers = ui.radioButton_ListOthers->isChecked();

	rsIdentity->getIdentityList(ids);

	for(it = ids.begin(); it != ids.end(); it++)
	{
		RsIdData data;
		if (!rsIdentity->getIdentity(*it, data))
		{
			continue;
		}

		/* do filtering */
		bool ok = false;
		if (acceptAll)
		{
			ok = true;
		}
		else if (data.mIdType & RSID_TYPE_PSEUDONYM)
		{
 			if (acceptPseudo)
			{
				ok = true;
			}

			if ((data.mIdType & RSID_RELATION_YOURSELF) && (acceptYourself))
			{
				ok = true;
			}
		}
		else
		{
			if (data.mIdType & RSID_RELATION_YOURSELF)
			{
 				if (acceptYourself)
				{
					ok = true;
				}
			}
			else if (data.mIdType & (RSID_RELATION_FRIEND | RSID_RELATION_FOF)) 
			{
				if (acceptFriends)
				{
					ok = true;
				}
			}
			else 
			{
				if (acceptOthers)
				{
					ok = true;
				}
			}
		}

		if (!ok)
		{
			continue;
		}


		QTreeWidgetItem *item = new QTreeWidgetItem(NULL);
		item->setText(RSID_COL_NICKNAME, QString::fromStdString(data.mNickname));
		item->setText(RSID_COL_KEYID, QString::fromStdString(data.mKeyId));
		item->setText(RSID_COL_IDTYPE, QString::fromStdString(rsIdTypeToString(data.mIdType)));

                tree->addTopLevelItem(item);
	}

	// fix up buttons.
	updateSelection();
}





void IdDialog::insertPhotosForSelectedAlbum()
{
#if 0
	std::cerr << "IdDialog::insertPhotosForSelectedAlbum()";
	std::cerr << std::endl;

	clearPhotos();

	std::list<std::string> albumIds;
	if (mAlbumSelected)
	{
		albumIds.push_back(mAlbumSelected->mDetails.mAlbumId);

		std::cerr << "IdDialog::insertPhotosForSelectedAlbum() AlbumId: " << mAlbumSelected->mDetails.mAlbumId;
		std::cerr << std::endl;
	}

	insertPhotosForAlbum(albumIds);
#endif
}


void IdDialog::addAlbum(const std::string &id)
{
#if 0
	RsPhotoAlbum album;
	rsPhoto->getAlbum(id, album);


	RsPhotoThumbnail thumbnail;
	rsPhoto->getAlbumThumbnail(id, thumbnail);

	std::cerr << " IdDialog::addAlbum() AlbumId: " << album.mAlbumId << std::endl;

	PulseItem *item = new PulseItem(this, album, thumbnail);
	QLayout *alayout = ui.scrollAreaWidgetContents->layout();
	alayout->addWidget(item);
#endif
}

void IdDialog::clearAlbums()
{
#if 0
	std::cerr << "IdDialog::clearAlbums()" << std::endl;

	std::list<PulseItem *> photoItems;
	std::list<PulseItem *>::iterator pit;
	
	QLayout *alayout = ui.scrollAreaWidgetContents->layout();
        int count = alayout->count();
	for(int i = 0; i < count; i++)
	{
		QLayoutItem *litem = alayout->itemAt(i);
		if (!litem)
		{
			std::cerr << "IdDialog::clearAlbums() missing litem";
			std::cerr << std::endl;
			continue;
		}
		
		PulseItem *item = dynamic_cast<PulseItem *>(litem->widget());
		if (item)
		{
			std::cerr << "IdDialog::clearAlbums() item: " << item;
			std::cerr << std::endl;
		
			photoItems.push_back(item);
		}
		else
		{
			std::cerr << "IdDialog::clearAlbums() Found Child, which is not a PulseItem???";
			std::cerr << std::endl;
		}
	}
	
	for(pit = photoItems.begin(); pit != photoItems.end(); pit++)
	{
		PulseItem *item = *pit;
		alayout->removeWidget(item);
		delete item;
	}
	mAlbumSelected = NULL;
#endif
}

void IdDialog::clearPhotos()
{
#if 0
	std::cerr << "IdDialog::clearPhotos()" << std::endl;

	std::list<PulseItem *> photoItems;
	std::list<PulseItem *>::iterator pit;
	
	QLayout *alayout = ui.scrollAreaWidgetContents_2->layout();
        int count = alayout->count();
	for(int i = 0; i < count; i++)
	{
		QLayoutItem *litem = alayout->itemAt(i);
		if (!litem)
		{
			std::cerr << "IdDialog::clearPhotos() missing litem";
			std::cerr << std::endl;
			continue;
		}
		
		PulseItem *item = dynamic_cast<PulseItem *>(litem->widget());
		if (item)
		{
			std::cerr << "IdDialog::clearPhotos() item: " << item;
			std::cerr << std::endl;
		
			photoItems.push_back(item);
		}
		else
		{
			std::cerr << "IdDialog::clearPhotos() Found Child, which is not a PulseItem???";
			std::cerr << std::endl;
		}
	}
	
	for(pit = photoItems.begin(); pit != photoItems.end(); pit++)
	{
		PulseItem *item = *pit;
		alayout->removeWidget(item);
		delete item;
	}
	
	mPhotoSelected = NULL;
#endif	
	
}


void IdDialog::insertPhotosForAlbum(const std::list<std::string> &albumIds)
{
#if 0
	/* clear it all */
	clearPhotos();
	//ui.photoLayout->clear();

	/* create a list of albums */

	std::list<std::string> ids;
	std::list<std::string> photoIds;
	std::list<std::string> filteredPhotoIds;
	std::list<std::string>::const_iterator it;

	for(it = albumIds.begin(); it != albumIds.end(); it++)
	{
		rsPhoto->getPhotoList(*it, photoIds);
	}

	/* Filter Albums */ /* Sort Albums */
#define MAX_PHOTOS 50
	
	int count = MAX_PHOTOS;

	FilterNSortPhotos(photoIds, filteredPhotoIds, MAX_PHOTOS);

	for(it = filteredPhotoIds.begin(); it != filteredPhotoIds.end(); it++)
	{
		addPhoto(*it);
	}
#endif
}


void IdDialog::addPhoto(const std::string &id)
{
#if 0
	RsPhotoPhoto photo;
	rsPhoto->getPhoto(id,photo);

	RsPhotoThumbnail thumbnail;
	rsPhoto->getPhotoThumbnail(id, thumbnail);

	std::cerr << "IdDialog::addPhoto() AlbumId: " << photo.mAlbumId;
	std::cerr << " PhotoId: " << photo.mId;
	std::cerr << std::endl;

	PulseItem *item = new PulseItem(this, photo, thumbnail);
	QLayout *alayout = ui.scrollAreaWidgetContents_2->layout();
	alayout->addWidget(item);
#endif
}


#if 0
void IdDialog::deletePulseItem(PulseItem *item, uint32_t type)
{


	return;
}

#endif


