#include <list>

#include <QLayout>
#include <QDialogButtonBox>
#include "FriendSelectionDialog.h"

std::list<std::string> FriendSelectionDialog::selectFriends()
{
	FriendSelectionDialog dialog ;
	dialog.friends_widget->start() ;
	dialog.setWindowTitle(tr("Choose some friends")) ;

	if(QDialog::Rejected == dialog.exec())
		return std::list<std::string>() ;

	std::list<std::string> ids ;
	dialog.friends_widget->selectedSslIds(ids,false) ;

	return ids ;
}

FriendSelectionDialog::FriendSelectionDialog(QWidget *parent)
	: QDialog(parent)
{
	friends_widget = new FriendSelectionWidget(this) ;

	friends_widget->setHeaderText(tr("Contacts:"));
	friends_widget->setModus(FriendSelectionWidget::MODUS_CHECK);
	friends_widget->setShowType(FriendSelectionWidget::SHOW_GROUP | FriendSelectionWidget::SHOW_SSL);

	QLayout *l = new QVBoxLayout ;
	setLayout(l) ;
	
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

	connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

	l->addWidget(friends_widget) ;
	l->addWidget(buttonBox) ;
	l->update() ;
}

FriendSelectionDialog::~FriendSelectionDialog()
{
	delete friends_widget ;
}

