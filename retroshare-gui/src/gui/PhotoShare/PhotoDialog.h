#ifndef PHOTODIALOG_H
#define PHOTODIALOG_H

#include <QDialog>
#include "retroshare/rsphotoV2.h"
#include "util/TokenQueueV2.h"

namespace Ui {
    class PhotoDialog;
}

class PhotoDialog : public QDialog, public TokenResponseV2
{
    Q_OBJECT

public:
    explicit PhotoDialog(RsPhotoV2* rs_photo, const RsPhotoPhoto& photo, QWidget *parent = 0);
    ~PhotoDialog();

private slots:

    void addComment();

public:
    void loadRequest(const TokenQueueV2 *queue, const TokenRequestV2 &req);
private:
    void setUp();
private:
    Ui::PhotoDialog *ui;

    RsPhotoV2* mRsPhoto;
    TokenQueueV2* mPhotoQueue;
    RsPhotoPhoto mPhotoDetails;

};

#endif // PHOTODIALOG_H
