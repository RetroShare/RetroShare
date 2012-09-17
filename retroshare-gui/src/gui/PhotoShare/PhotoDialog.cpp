#include "PhotoDialog.h"
#include "ui_PhotoDialog.h"

PhotoDialog::PhotoDialog(RsPhotoV2 *rs_photo, const RsPhotoPhoto &photo, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PhotoDialog), mRsPhoto(rs_photo), mPhotoQueue(new TokenQueueV2(mRsPhoto->getTokenService(), this)),
    mPhotoDetails(photo)

{
    ui->setupUi(this);
    setAttribute ( Qt::WA_DeleteOnClose, true );
    setUp();
}

PhotoDialog::~PhotoDialog()
{
    delete ui;
    delete mPhotoQueue;
}

void PhotoDialog::setUp()
{
    QPixmap qtn;
    qtn.loadFromData(mPhotoDetails.mThumbnail.data, mPhotoDetails.mThumbnail.size, mPhotoDetails.mThumbnail.type.c_str());
    ui->label_Photo->setPixmap(qtn);
    ui->lineEdit_Title->setText(QString::fromStdString(mPhotoDetails.mMeta.mMsgName));
}


void PhotoDialog::loadRequest(const TokenQueueV2 *queue, const TokenRequestV2 &req)
{

}
