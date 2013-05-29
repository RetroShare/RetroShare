#include <list>

#include <QLayout>
#include <QDialogButtonBox>
#include "FriendSelectionDialog.h"

std::list<std::string> FriendSelectionDialog::selectFriends(QWidget *parent,const QString& caption,const QString& header_text,
																				FriendSelectionWidget::Modus modus,
																				FriendSelectionWidget::ShowTypes show_type,
																				FriendSelectionWidget::IdType pre_selected_id_type,
																				const std::list<std::string>& pre_selected_ids)
{
	FriendSelectionDialog dialog(parent,header_text,modus,show_type,pre_selected_id_type,pre_selected_ids) ;

	dialog.friends_widget->start() ;
	dialog.friends_widget->setSelectedIds(pre_selected_id_type,pre_selected_ids,true) ;

	dialog.setWindowTitle(caption) ;

	if(QDialog::Rejected == dialog.exec())
		return std::list<std::string>() ;

	std::list<std::string> ids ;
	dialog.friends_widget->selectedIds(pre_selected_id_type,ids,false) ;

	return ids ;
}

FriendSelectionDialog::FriendSelectionDialog(QWidget *parent,const QString& header_text,
															FriendSelectionWidget::Modus modus,
															FriendSelectionWidget::ShowTypes show_type,
															FriendSelectionWidget::IdType pre_selected_id_type,
															const std::list<std::string>& pre_selected_ids)
	: QDialog(parent)
{
	friends_widget = new FriendSelectionWidget(this) ;

	friends_widget->setHeaderText(header_text);
	friends_widget->setModus(modus) ;
	friends_widget->setShowType(show_type) ;

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

