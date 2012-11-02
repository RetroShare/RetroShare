#include <QListWidget>

class GroupSelectionBox: public QListWidget
{
	public:
		GroupSelectionBox(QWidget *parent) ;

		std::list<std::string> selectedGroups() const ;

		void setSelectedGroups(const std::list<std::string>& selected_group_ids) ;
};
