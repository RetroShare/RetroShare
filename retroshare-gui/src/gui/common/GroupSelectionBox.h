#include <QListWidget>

class GroupSelectionBox: public QListWidget
{
	Q_OBJECT

public:
	GroupSelectionBox(QWidget *parent);

	void selectedGroupIds(std::list<std::string> &groupIds) const;
	void selectedGroupNames(QList<QString> &groupNames) const;

	void setSelectedGroupIds(const std::list<std::string> &groupIds);

private slots:
	void fillGroups();
};
