#include <iostream>
#include <QVBoxLayout>
#include "PhotoDialog.h"
#include "ui_PhotoDialog.h"
#include "retroshare/rsidentity.h"
#include "AddCommentDialog.h"

PhotoDialog::PhotoDialog(RsPhotoV2 *rs_photo, const RsPhotoPhoto &photo, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PhotoDialog), mRsPhoto(rs_photo), mPhotoQueue(new TokenQueue(mRsPhoto->getTokenService(), this)),
    mPhotoDetails(photo)
{
    ui->setupUi(this);
    setAttribute ( Qt::WA_DeleteOnClose, true );

    connect(ui->pushButton_AddComment, SIGNAL(clicked()), this, SLOT(createComment()));
    connect(ui->pushButton_AddCommentDlg, SIGNAL(clicked()), this, SLOT(addComment()));
    connect(ui->fullscreenButton, SIGNAL(clicked()),this, SLOT(setFullScreen()));

#if QT_VERSION >= 0x040700
    ui->lineEdit->setPlaceholderText(tr("Write a comment...")) ;
#endif

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

    requestComments();
}

void PhotoDialog::addComment()
{
    AddCommentDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        RsPhotoComment comment;
        comment.mComment = dlg.getComment().toUtf8().constData();

        uint32_t token;
        comment.mMeta.mGroupId = mPhotoDetails.mMeta.mGroupId;
        comment.mMeta.mParentId = mPhotoDetails.mMeta.mOrigMsgId;
        mRsPhoto->submitComment(token, comment);
        mPhotoQueue->queueRequest(token, TOKENREQ_MSGINFO, RS_TOKREQ_ANSTYPE_ACK, 0);
    }
}

void PhotoDialog::clearComments()
{
    //QLayout* l = ui->scrollAreaWidgetContents->layout();
    QSetIterator<PhotoCommentItem*> sit(mComments);
    while(sit.hasNext())
    {
        PhotoCommentItem* item = sit.next();
        ui->verticalLayout->removeWidget(item);
        item->setParent(NULL);
        delete item;
    }

    mComments.clear();
}

void PhotoDialog::resetComments()
{
    QSetIterator<PhotoCommentItem*> sit(mComments);
    //QLayout* l = ui->scrollAreaWidgetContents->layout();
    while(sit.hasNext())
    {
        PhotoCommentItem* item = sit.next();
        ui->verticalLayout->insertWidget(0,item);
    }
}

void PhotoDialog::requestComments()
{
    RsTokReqOptions opts;
    opts.mMsgFlagMask = RsPhotoV2::FLAG_MSG_TYPE_MASK;
    opts.mMsgFlagFilter = RsPhotoV2::FLAG_MSG_TYPE_PHOTO_COMMENT;

    opts.mReqType = GXS_REQUEST_TYPE_MSG_IDS;
    opts.mOptions = RS_TOKREQOPT_MSG_PARENT | RS_TOKREQOPT_MSG_LATEST;
    RsGxsGrpMsgIdPair msgId;
    uint32_t token;
    msgId.first = mPhotoDetails.mMeta.mGroupId;
    msgId.second = mPhotoDetails.mMeta.mMsgId;
    uint32_t anstype = RS_TOKREQ_ANSTYPE_LIST;
    mPhotoQueue->requestMsgRelatedInfo(token, anstype, opts, msgId, 0);
}

void PhotoDialog::createComment()
{
    RsPhotoComment comment;
    QString commentString = ui->lineEdit->text();

    comment.mComment = commentString.toUtf8().constData();

    uint32_t token;
    comment.mMeta.mGroupId = mPhotoDetails.mMeta.mGroupId;
    comment.mMeta.mParentId = mPhotoDetails.mMeta.mOrigMsgId;
    mRsPhoto->submitComment(token, comment);
    mPhotoQueue->queueRequest(token, TOKENREQ_MSGINFO, RS_TOKREQ_ANSTYPE_ACK, 0);

    ui->lineEdit->clear();
}


/*************** message loading **********************/

void PhotoDialog::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
    std::cerr << "PhotoShare::loadRequest()";
    std::cerr << std::endl;

    if (queue == mPhotoQueue)
    {
        /* now switch on req */
        switch(req.mType)
        {
            case TOKENREQ_MSGINFO:
            {
                switch(req.mAnsType)
                {
                    case RS_TOKREQ_ANSTYPE_DATA:
                        loadComment(req.mToken);
                        break;
                    case RS_TOKREQ_ANSTYPE_LIST:
                        loadList(req.mToken);
                        break;
                    case RS_TOKREQ_ANSTYPE_ACK:
                        acknowledgeComment(req.mToken);
                        break;
                    default:
                        std::cerr << "PhotoShare::loadRequest() ERROR: MSG INVALID TYPE";
                        std::cerr << std::endl;
                        break;
                }
                break;
            }

            default:
            {
                std::cerr << "PhotoShare::loadRequest() ERROR: INVALID TYPE";
                std::cerr << std::endl;
                break;
            }
        }
    }

}

void PhotoDialog::loadComment(uint32_t token)
{

    clearComments();

    PhotoCommentResult results;
    mRsPhoto->getPhotoComment(token, results);

    PhotoCommentResult::iterator mit = results.begin();

    for(; mit != results.end(); mit++)
    {
        const std::vector<RsPhotoComment>& commentV = mit->second;
        std::vector<RsPhotoComment>::const_iterator vit = commentV.begin();

        for(; vit != commentV.end(); vit++)
        {
            addComment(*vit);
        }
    }

    resetComments();
}

void PhotoDialog::loadList(uint32_t token)
{
    GxsMsgReq msgIds;
    mRsPhoto->getMsgList(token, msgIds);
    RsTokReqOptions opts;

    // just use data as no need to worry about getting comments
    opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;
    uint32_t reqToken;
    mPhotoQueue->requestMsgInfo(reqToken, RS_TOKREQ_ANSTYPE_DATA, opts, msgIds, 0);
}

void PhotoDialog::addComment(const RsPhotoComment &comment)
{
    PhotoCommentItem* item = new PhotoCommentItem(comment);
    mComments.insert(item);
}

void PhotoDialog::acknowledgeComment(uint32_t token)
{
    RsGxsGrpMsgIdPair msgId;
    mRsPhoto->acknowledgeMsg(token, msgId);

    if(msgId.first.empty() || msgId.second.empty()){

    }else
    {
        requestComments();
    }
}

void PhotoDialog::setFullScreen()
{
  if (!isFullScreen()) {
    // hide menu & toolbars

#ifdef Q_WS_X11
    show();
    raise();
    setWindowState( windowState() | Qt::WindowFullScreen );
#else
    setWindowState( windowState() | Qt::WindowFullScreen );
    show();
    raise();
#endif
  } else {

    setWindowState( windowState() ^ Qt::WindowFullScreen );
    show();
  }
}
