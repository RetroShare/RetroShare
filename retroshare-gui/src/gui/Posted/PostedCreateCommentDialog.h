#ifndef POSTEDCREATECOMMENTDIALOG_H
#define POSTEDCREATECOMMENTDIALOG_H

#include <QDialog>
#include "retroshare/rsposted.h"
#include "util/TokenQueue.h"

namespace Ui {
    class PostedCreateCommentDialog;
}

class PostedCreateCommentDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PostedCreateCommentDialog(TokenQueue* tokQ, const RsGxsGrpMsgIdPair& parentId, const RsGxsMessageId& threadId, QWidget *parent = 0);
    ~PostedCreateCommentDialog();

private slots:

    void createComment();

private:
    Ui::PostedCreateCommentDialog *ui;
    TokenQueue* mTokenQueue;
    RsGxsGrpMsgIdPair mParentId;
    RsGxsMessageId mThreadId;
};

#endif // POSTEDCREATECOMMENTDIALOG_H
