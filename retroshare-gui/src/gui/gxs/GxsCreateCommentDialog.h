#ifndef _MRK_GXS_CREATE_COMMENT_DIALOG_H
#define _MRK_GXS_CREATE_COMMENT_DIALOG_H

#include <QDialog>
#include "retroshare/rsgxscommon.h"
#include "util/TokenQueue.h"

namespace Ui {
	class GxsCreateCommentDialog;
}

class GxsCreateCommentDialog : public QDialog
{
	Q_OBJECT

public:
	explicit GxsCreateCommentDialog(TokenQueue* tokQ, RsGxsCommentService *service, 
		const RsGxsGrpMsgIdPair& parentId, const RsGxsMessageId& threadId, QWidget *parent = 0);
	~GxsCreateCommentDialog();

private slots:

	void createComment();

private:
	Ui::GxsCreateCommentDialog *ui;
	TokenQueue *mTokenQueue;
	RsGxsCommentService *mCommentService;

	RsGxsGrpMsgIdPair mParentId;
	RsGxsMessageId mThreadId;
};

#endif // _MRK_GXS_CREATE_COMMENT_DIALOG_H
