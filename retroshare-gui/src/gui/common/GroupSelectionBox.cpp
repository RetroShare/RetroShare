#include <retroshare/rspeers.h>
#include "GroupSelectionBox.h"

GroupSelectionBox::GroupSelectionBox(QWidget *parent)
	: QListWidget(parent)
{
	setSelectionMode(QAbstractItemView::ExtendedSelection) ;

	// Fill with available groups
	
	std::list<RsGroupInfo> lst ;
	rsPeers->getGroupInfoList(lst) ;

	for(std::list<RsGroupInfo>::const_iterator it(lst.begin());it!=lst.end();++it)
		addItem(QString::fromStdString(it->id)) ;

}

std::list<std::string> GroupSelectionBox::selectedGroups() const
{
	QList<QListWidgetItem*> selected_items = selectedItems() ;
	std::list<std::string> out ;

	for(QList<QListWidgetItem*>::const_iterator it(selected_items.begin());it!=selected_items.end();++it)
		out.push_back((*it)->text().toStdString()) ;

	return out ;
}

void GroupSelectionBox::setSelectedGroups(const std::list<std::string>& group_ids)
{
	for(std::list<std::string>::const_iterator it(group_ids.begin());it!=group_ids.end();++it)
	{
		QList<QListWidgetItem*> lst = findItems(QString::fromStdString(*it),Qt::MatchExactly) ;

		setCurrentItem(*lst.begin(),QItemSelectionModel::Select) ;
	}
}
