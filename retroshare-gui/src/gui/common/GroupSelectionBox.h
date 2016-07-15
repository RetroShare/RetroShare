#include <QListWidget>
#include <retroshare/rsids.h>

class GroupSelectionBox: public QListWidget
{
	Q_OBJECT

public:
	GroupSelectionBox(QWidget *parent);

    void selectedGroupIds(std::list<RsNodeGroupId> &groupIds) const;
	void selectedGroupNames(QList<QString> &groupNames) const;

    void setSelectedGroupIds(const std::list<RsNodeGroupId> &groupIds);

private slots:
	void fillGroups();
};
