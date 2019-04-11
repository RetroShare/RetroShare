/*******************************************************************************
 * retroshare-gui/src/gui/PhotoShare/AlbumCreateDialog.cpp                     *
 *                                                                             *
 * Copyright (C) 2018 by Retroshare Team     <retroshare.project@gmail.com>    *
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

#include "AlbumCreateDialog.h"
#include "ui_AlbumCreateDialog.h"

#include "util/misc.h"
#include "retroshare/rsgxsflags.h"

AlbumCreateDialog::AlbumCreateDialog(TokenQueue *photoQueue, RsPhoto *rs_photo, QWidget *parent):
    QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint),
    ui(new Ui::AlbumCreateDialog), mPhotoQueue(photoQueue), mRsPhoto(rs_photo), mPhotoSelected(NULL)
{
    ui->setupUi(this);
    
	ui->headerFrame->setHeaderImage(QPixmap(":/images/album_create_64.png"));
    ui->headerFrame->setHeaderText(tr("Create Album"));
    

#if QT_VERSION >= 0x040700
    ui->lineEdit_Title_2->setPlaceholderText(tr("Untitle Album"));
    ui->lineEdit_Caption_2->setPlaceholderText(tr("Say something about this album..."));
    //ui->textEdit_Description->setPlaceholderText(tr("Say something about this album...")) ;
    ui->lineEdit_Where->setPlaceholderText(tr("Where were these taken?"));
#endif

	ui->backButton->hide();
	
    connect(ui->publishButton, SIGNAL(clicked()), this, SLOT(publishAlbum()));
    connect(ui->AlbumThumbNail, SIGNAL(clicked()), this, SLOT(addAlbumThumbnail()));
    
    connect(ui->addphotosButton, SIGNAL(clicked()),this, SLOT(changePage()));
    connect(ui->backButton, SIGNAL(clicked()),this, SLOT(backPage()));

    
    mPhotoDrop = ui->scrollAreaWidgetContents;
    mPhotoDrop->setPhotoItemHolder(this);

    
}

AlbumCreateDialog::~AlbumCreateDialog()
{
    delete ui;
}

#define PUBLIC_INDEX 0
#define RESTRICTED_INDEX 1
#define PRIVATE_INDEX 2

void AlbumCreateDialog::publishAlbum()
{
    // get fields for album to publish, publish and then exit dialog
    RsPhotoAlbum album;

    album.mCaption = ui->lineEdit_Caption_2->text().toStdString();
    album.mPhotographer = ui->lineEdit_Photographer->text().toStdString();
    album.mMeta.mGroupName = ui->lineEdit_Title_2->text().toStdString();
    album.mDescription = ui->textEdit_Description->toPlainText().toStdString();
    album.mWhere = ui->lineEdit_Where->text().toStdString();
    album.mPhotographer = ui->lineEdit_Photographer->text().toStdString();
    getAlbumThumbnail(album.mThumbnail);


    int currIndex = ui->privacyComboBox->currentIndex();

    switch(currIndex)
    {
        case PUBLIC_INDEX:
            album.mMeta.mGroupFlags |= GXS_SERV::FLAG_PRIVACY_PUBLIC;
            break;
        case RESTRICTED_INDEX:
            album.mMeta.mGroupFlags |= GXS_SERV::FLAG_PRIVACY_RESTRICTED;
            break;
        case PRIVATE_INDEX:
            album.mMeta.mGroupFlags |= GXS_SERV::FLAG_PRIVACY_PRIVATE;
            break;
    }

    uint32_t token;
    mRsPhoto->submitAlbumDetails(token, album);
    mPhotoQueue->queueRequest(token, TOKENREQ_GROUPINFO, RS_TOKREQ_ANSTYPE_ACK, 0);
    
    publishPhotos();
    
    close();
}

void AlbumCreateDialog::publishPhotos()
{
    // get fields for album to publish, publish and then exit dialog
    RsPhotoAlbum album;

    QSet<PhotoItem*> photos;

    mPhotoDrop->getPhotos(photos);

    QSetIterator<PhotoItem*> sit(photos);

    while(sit.hasNext())
    {
        PhotoItem* item = sit.next();
        uint32_t token;
        RsPhotoPhoto photo = item->getPhotoDetails();
        photo.mMeta.mGroupId = album.mMeta.mGroupId;
        mRsPhoto->submitPhoto(token, photo);
        mPhotoQueue->queueRequest(token, TOKENREQ_MSGINFO, RS_TOKREQ_ANSTYPE_ACK, 0);
    }
    
}

bool AlbumCreateDialog::getAlbumThumbnail(RsPhotoThumbnail &nail)
{
        const QPixmap *tmppix = &mThumbNail;

        QByteArray ba;
        QBuffer buffer(&ba);

        if(!tmppix->isNull())
        {
                // send chan image

                buffer.open(QIODevice::WriteOnly);
                tmppix->save(&buffer, "PNG"); // writes image into ba in PNG format

                RsPhotoThumbnail tmpnail;
                tmpnail.data = (uint8_t *) ba.data();
                tmpnail.size = ba.size();

                nail.copyFrom(tmpnail);

                return true;
        }

        nail.data = NULL;
        nail.size = 0;
        return false;
}

void AlbumCreateDialog::addAlbumThumbnail()
{
    QPixmap img = misc::getOpenThumbnailedPicture(this, tr("Load Album Thumbnail"), 128, 128);

    if (img.isNull())
            return;

    mThumbNail = img;

    // to show the selected
    ui->AlbumThumbNail->setIcon(mThumbNail);
}

void AlbumCreateDialog::changePage()
{
	int nextPage = ui->stackedWidget->currentIndex() + 1;
	if (nextPage >= ui->stackedWidget->count())
	nextPage = 0;
	ui->stackedWidget->setCurrentIndex(nextPage);
	
	ui->backButton->show();
	ui->addphotosButton->hide();
}

void AlbumCreateDialog::backPage()
{
	int nextPage = ui->stackedWidget->currentIndex() - 1;
	if (nextPage >= ui->stackedWidget->count())
	nextPage = 0;
	ui->stackedWidget->setCurrentIndex(nextPage);
	
	ui->backButton->hide();
	ui->addphotosButton->show();
}

void AlbumCreateDialog::notifySelection(PhotoShareItem *selection)
{

    PhotoItem* pItem = dynamic_cast<PhotoItem*>(selection);

    if(mPhotoSelected  == NULL)
    {
        return;
    }
    else
    {
        mPhotoSelected->setSelected(false);
        mPhotoSelected = pItem;
    }

    mPhotoSelected->setSelected(true);
}
