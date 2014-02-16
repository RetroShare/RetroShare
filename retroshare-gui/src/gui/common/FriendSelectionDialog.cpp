#include <list>

#include <QLayout>
#include <QDialogButtonBox>
#include "FriendSelectionDialog.h"

std::list<RsPgpId> FriendSelectionDialog::selectFriends_PGP(QWidget *parent,const QString& caption,const QString& header_text,
                            FriendSelectionWidget::Modus modus,
                            FriendSelectionWidget::ShowTypes show_type,
                            const std::list<RsPgpId>& pre_selected_ids)
{
    FriendSelectionDialog dialog(parent,header_text,modus,show_type,FriendSelectionWidget::IDTYPE_GPG,pre_selected_ids) ;

	dialog.setWindowTitle(caption) ;

	if(QDialog::Rejected == dialog.exec())
		return std::list<std::string>() ;

    std::list<std::string> sids ;
	dialog.friends_widget->selectedIds(pre_selected_ids,false) ;

    std::list<RsPgpId> ids ;
    for(std::list<std::string>::const_iterator it(sids.begin());it!=sids.end();++it)
    {
        RsPgpId id(*it) ;
        if(!id.isNull())
            ids.push_back(id) ;
        else
            std::cerr << "ERROR in " << __PRETTY_FUNCTION__ << ": string " << *it << " is not a PGP id" << std::endl;
    }
    return ids ;
}
std::list<RsPeerId> FriendSelectionDialog::selectFriends_SSL(QWidget *parent,const QString& caption,const QString& header_text,
                            FriendSelectionWidget::Modus modus,
                            FriendSelectionWidget::ShowTypes show_type,
                            const std::list<RsPeerId>& pre_selected_ids)
{
    FriendSelectionDialog dialog(parent,header_text,modus,show_type,FriendSelectionWidget::IDTYPE_SSL,pre_selected_ids) ;

    dialog.setWindowTitle(caption) ;

    if(QDialog::Rejected == dialog.exec())
        return std::list<std::string>() ;

    std::list<std::string> sids ;
    dialog.friends_widget->selectedIds(pre_selected_ids,false) ;

    std::list<RsPeerId> ids ;
    for(std::list<std::string>::const_iterator it(sids.begin());it!=sids.end();++it)
    {
        RsPeerId id(*it) ;
        if(!id.isNull())
            ids.push_back(id) ;
        else
            std::cerr << "ERROR in " << __PRETTY_FUNCTION__ << ": string " << *it << " is not a SSL id" << std::endl;
    }
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
	friends_widget->start() ;
	friends_widget->setSelectedIds(pre_selected_id_type, pre_selected_ids, false);

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

