#include <QBuffer>

#include "AlbumCreateDialog.h"
#include "ui_AlbumCreateDialog.h"

#include "util/misc.h"

AlbumCreateDialog::AlbumCreateDialog(TokenQueueV2 *photoQueue, RsPhotoV2 *rs_photo, QWidget *parent):
    QDialog(parent),
    ui(new Ui::AlbumCreateDialog), mPhotoQueue(photoQueue), mRsPhoto(rs_photo)
{
    ui->setupUi(this);

    connect(ui->publishButton, SIGNAL(clicked()), this, SLOT(publishAlbum()));
    connect(ui->AlbumThumbNail, SIGNAL(clicked()), this, SLOT(addAlbumThumbnail()));
}

AlbumCreateDialog::~AlbumCreateDialog()
{
    delete ui;
}

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

    uint32_t token;
    mRsPhoto->submitAlbumDetails(token, album);
    mPhotoQueue->queueRequest(token, TOKENREQ_GROUPINFO, RS_TOKREQ_ANSTYPE_ACK, 0);
    close();
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
    QPixmap img = misc::getOpenThumbnailedPicture(this, tr("Load Album Thumbnail"), 64, 64);

    if (img.isNull())
            return;

    mThumbNail = img;

    // to show the selected
    ui->AlbumThumbNail->setIcon(mThumbNail);
}
