#include "AlbumItem.h"
#include "ui_AlbumItem.h"
#include <iostream>

#include <QMouseEvent>

AlbumItem::AlbumItem(const RsPhotoAlbum &album, PhotoShareItemHolder *albumHolder, QWidget *parent) :
    QWidget(NULL),
    ui(new Ui::AlbumItem), mAlbum(album), mAlbumHolder(albumHolder)
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
    
    if(mAlbum.mThumbnail.size != 0)
    {
		ui->label_Thumbnail->setPixmap(qtn);
    }
    else
    {
		// display a default Album icon when album has no Thumbnail
		ui->label_Thumbnail->setPixmap(QPixmap(":/images/album_default_128.png"));
    }
}

void AlbumItem::mousePressEvent(QMouseEvent *event)
{
    QPoint pos = event->pos();

    std::cerr << "AlbumItem::mousePressEvent(" << pos.x() << ", " << pos.y() << ")";
    std::cerr << std::endl;

    if(mAlbumHolder)
        mAlbumHolder->notifySelection(this);
    else
        setSelected(true);

    QWidget::mousePressEvent(event);
}

void AlbumItem::setSelected(bool on)
{
    mSelected = on;
    if (mSelected)
    {
            ui->albumFrame->setStyleSheet("QFrame#albumFrame{border: 2px solid #55CC55;\nbackground: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #55EE55, stop: 1 #CCCCCC);\nborder-radius: 10px}");
    }
    else
    {
            ui->albumFrame->setStyleSheet("QFrame#albumFrame{border: 2px solid #CCCCCC;\nbackground: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #EEEEEE, stop: 1 #CCCCCC);\nborder-radius: 10px}");
    }
    update();
}

const RsPhotoAlbum& AlbumItem::getAlbum()
{
    return mAlbum;
}
