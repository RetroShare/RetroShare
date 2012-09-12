#include "AlbumItem.h"
#include "ui_AlbumItem.h"

AlbumItem::AlbumItem(const RsPhotoAlbum& album, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AlbumItem), mAlbum(album)
{
    ui->setupUi(this);
    setUp();
}

AlbumItem::~AlbumItem()
{
    delete ui;
}

void AlbumItem::setUp()
{
    ui->label_AlbumTitle->setText(QString::fromStdString(mAlbum.mMeta.mGroupName));
    ui->label_Photographer->setText(QString::fromStdString(mAlbum.mPhotographer));
    QPixmap qtn;
    qtn.loadFromData(mAlbum.mThumbnail.data, mAlbum.mThumbnail.size, mAlbum.mThumbnail.type.c_str());
    ui->label_Thumbnail->setPixmap(qtn);
}

RsPhotoAlbum AlbumItem::getAlbum()
{
    return mAlbum;
}
