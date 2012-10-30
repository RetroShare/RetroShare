#ifndef PHOTODIALOG_H
#define PHOTODIALOG_H

#include <QDialog>
#include <QSet>
#include "retroshare/rsphotoV2.h"
#include "util/TokenQueue.h"
#include "PhotoCommentItem.h"

namespace Ui {
    class PhotoDialog;
}

class PhotoDialog : public QDialog, public TokenResponse
{
    Q_OBJECT

public:
    explicit PhotoDialog(RsPhotoV2* rs_photo, const RsPhotoPhoto& photo, QWidget *parent = 0);
    ~PhotoDialog();

private slots:

    void addComment();
    void createComment();
	void setFullScreen();

public:
    void loadRequest(const TokenQueue *queue, const TokenRequest &req);
private:
    void setUp();

    /*!
     * clears comments
     * and places them back in dialog
     */
    void resetComments();

    /*!
     * Request comments
     */
    void requestComments();

    /*!
     * Simply removes comments but doesn't place them back in dialog
     */
    void clearComments();

    void acknowledgeComment(uint32_t token);
    void loadComment(uint32_t token);
    void loadList(uint32_t token);
    void addComment(const RsPhotoComment& comment);
private:
    Ui::PhotoDialog *ui;

    RsPhotoV2* mRsPhoto;
    TokenQueue* mPhotoQueue;
    RsPhotoPhoto mPhotoDetails;
    QSet<PhotoCommentItem*> mComments;
};

#endif // PHOTODIALOG_H
