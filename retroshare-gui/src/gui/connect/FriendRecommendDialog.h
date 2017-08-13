#pragma once

#include <QDialog>
#include "ui_FriendRecommendDialog.h"

class FriendRecommendDialog: public QDialog
{
public:
	explicit FriendRecommendDialog(QWidget *parent) ;
	virtual ~FriendRecommendDialog() ;

	static void showIt();

private:
	static FriendRecommendDialog *instance();

    virtual void accept() ;

	void load();

	Ui::FriendRecommendDialog *ui;
};
