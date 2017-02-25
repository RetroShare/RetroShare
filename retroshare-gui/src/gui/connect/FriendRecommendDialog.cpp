#include <QMessageBox>

#include "FriendRecommendDialog.h"
#include "gui/msgs/MessageComposer.h"

FriendRecommendDialog::FriendRecommendDialog(QWidget *parent)
	: QDialog(parent),  ui(new Ui::FriendRecommendDialog)
{
	ui->setupUi(this) ;
}
FriendRecommendDialog::~FriendRecommendDialog()
{
}

void FriendRecommendDialog::load()
{
    ui->frec_recommendList->setHeaderText(tr("Recommend friends"));
    ui->frec_recommendList->setModus(FriendSelectionWidget::MODUS_CHECK);
    ui->frec_recommendList->setShowType(FriendSelectionWidget::SHOW_GROUP | FriendSelectionWidget::SHOW_SSL);
    ui->frec_recommendList->start();

    ui->frec_toList->setHeaderText(tr("To"));
    ui->frec_toList->setModus(FriendSelectionWidget::MODUS_CHECK);
    ui->frec_toList->start();

    ui->frec_messageEdit->setText(MessageComposer::recommendMessage());
}

void FriendRecommendDialog::showIt()
{
	FriendRecommendDialog *dialog = instance();

	dialog->load();

	dialog->show();
	dialog->raise();
	dialog->activateWindow();
}

FriendRecommendDialog *FriendRecommendDialog::instance()
{
    static FriendRecommendDialog *d = NULL ;

    if(d == NULL)
        d = new FriendRecommendDialog(NULL);

    return d;
}

void FriendRecommendDialog::accept()
{
	std::set<RsPeerId> recommendIds;
	ui->frec_recommendList->selectedIds<RsPeerId,FriendSelectionWidget::IDTYPE_SSL>(recommendIds, false);

	if (recommendIds.empty()) {
		QMessageBox::warning(this, "RetroShare", tr("Please select at least one friend for recommendation."), QMessageBox::Ok, QMessageBox::Ok);
		return ;
	}

	std::set<RsPeerId> toIds;
	ui->frec_toList->selectedIds<RsPeerId,FriendSelectionWidget::IDTYPE_SSL>(toIds, false);

	if (toIds.empty()) {
		QMessageBox::warning(this, "RetroShare", tr("Please select at least one friend as recipient."), QMessageBox::Ok, QMessageBox::Ok);
		return ;
	}

	std::set<RsPeerId>::iterator toId;
	for (toId = toIds.begin(); toId != toIds.end(); ++toId) {
		MessageComposer::recommendFriend(recommendIds, *toId, ui->frec_messageEdit->toHtml(), true);
	}

    QDialog::accept() ;

    QMessageBox::information(NULL,tr("Recommendation messages sent!"),tr("A recommendation message was sent to each of the chosen friends!")) ;
}

