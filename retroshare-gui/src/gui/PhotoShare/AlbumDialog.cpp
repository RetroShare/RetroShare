#include <QPixmap>

#include "AlbumDialog.h"
#include "ui_AlbumDialog.h"
#include "gxs/rsgxsflags.h"

AlbumDialog::AlbumDialog(const RsPhotoAlbum& album, TokenQueue* photoQueue, RsPhotoV2* rs_Photo, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AlbumDialog), mRsPhoto(rs_Photo), mPhotoQueue(photoQueue), mAlbum(album), mPhotoSelected(NULL)
{
    ui->setupUi(this);
    
    ui->headerFrame->setHeaderImage(QPixmap(":/images/kview_64.png"));
    ui->headerFrame->setHeaderText(tr("Album"));

    connect(ui->pushButton_PublishPhotos, SIGNAL(clicked()), this, SLOT(updateAlbumPhotos()));
    connect(ui->pushButton_DeletePhoto, SIGNAL(clicked()), this, SLOT(deletePhoto()));

    mPhotoDrop = ui->scrollAreaWidgetContents;

    if(!(mAlbum.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN))
    {
        ui->scrollAreaPhotos->setEnabled(false);
        ui->pushButton_DeletePhoto->setEnabled(false);
    }
    mPhotoDrop->setPhotoItemHolder(this);

    setUp();
}


void AlbumDialog::setUp()
{
    ui->lineEdit_Title->setText(QString::fromStdString(mAlbum.mMeta.mGroupName));
    ui->lineEdit_Caption->setText(QString::fromStdString(mAlbum.mCaption));
    ui->lineEdit_Category->setText(QString::fromStdString(mAlbum.mCategory));
    ui->lineEdit_Identity->setText(QString::fromStdString(mAlbum.mMeta.mAuthorId));

    QPixmap qtn;
    qtn.loadFromData(mAlbum.mThumbnail.data, mAlbum.mThumbnail.size, mAlbum.mThumbnail.type.c_str());
    ui->label_thumbNail->setPixmap(qtn);
}

void AlbumDialog::updateAlbumPhotos(){

    QSet<PhotoItem*> photos;

    mPhotoDrop->getPhotos(photos);

    QSetIterator<PhotoItem*> sit(photos);

    while(sit.hasNext())
    {
        PhotoItem* item = sit.next();
        uint32_t token;
        RsPhotoPhoto photo = item->getPhotoDetails();
        photo.mMeta.mGroupId = mAlbum.mMeta.mGroupId;
        mRsPhoto->submitPhoto(token, photo);
        mPhotoQueue->queueRequest(token, TOKENREQ_MSGINFO, RS_TOKREQ_ANSTYPE_ACK, 0);
    }
    close();
}

void AlbumDialog::deletePhoto(){

    if(mPhotoSelected)
    {
        mPhotoSelected->setSelected(false);
        mPhotoDrop->deletePhoto(mPhotoSelected);
    }

}

void AlbumDialog::editPhoto()
{

}

AlbumDialog::~AlbumDialog()
{
    delete ui;
}

void AlbumDialog::notifySelection(PhotoShareItem *selection)
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
