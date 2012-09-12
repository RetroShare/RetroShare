#include "AlbumCreateDialog.h"
#include "ui_AlbumCreateDialog.h"

#include "util/misc.h"

AlbumCreateDialog::AlbumCreateDialog(TokenQueueV2 *photoQueue, RsPhotoV2 *rs_photo, QWidget *parent):
    QDialog(parent),
    ui(new Ui::AlbumCreateDialog), mPhotoQueue(photoQueue), mRsPhoto(rs_photo)
{
    ui->setupUi(this);

    connect(ui->publishButton, SIGNAL(clicked()), this, SLOT(publishAlbum()));
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

    uint32_t token;
    mRsPhoto->submitAlbumDetails(token, album);
    mPhotoQueue->queueRequest(token, TOKENREQ_GROUPINFO, RS_TOKREQ_ANSTYPE_ACK, 0);
    close();
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
