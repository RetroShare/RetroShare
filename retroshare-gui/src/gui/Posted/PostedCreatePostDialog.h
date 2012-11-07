#ifndef POSTEDCREATEPOSTDIALOG_H
#define POSTEDCREATEPOSTDIALOG_H

#include <QDialog>
#include "retroshare/rsposted.h"
#include "util/TokenQueue.h"

namespace Ui {
    class PostedCreatePostDialog;
}

class PostedCreatePostDialog : public QDialog
{
    Q_OBJECT

public:

    /*!
     * @param tokenQ parent callee token
     * @param posted
     */
    explicit PostedCreatePostDialog(TokenQueue* tokenQ, RsPosted* posted, QWidget *parent = 0);
    ~PostedCreatePostDialog();

private:

    void createPost();

private:
    Ui::PostedCreatePostDialog *ui;

    QString mLink;
    QString mNotes;
    RsPosted* mPosted;
    TokenQueue* mTokenQueue;
};

#endif // POSTEDCREATEPOSTDIALOG_H
