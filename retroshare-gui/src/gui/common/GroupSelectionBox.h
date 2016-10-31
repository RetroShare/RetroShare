#include <QListWidget>
#include <QDialog>
#include <retroshare/rsids.h>

class GroupSelectionBox: public QListWidget
{
	Q_OBJECT

public:
	GroupSelectionBox(QWidget *parent);

    static void selectGroups(const std::list<RsNodeGroupId>& default_groups) ;

    void selectedGroupIds(std::list<RsNodeGroupId> &groupIds) const;
	void selectedGroupNames(QList<QString> &groupNames) const;

    void setSelectedGroupIds(const std::list<RsNodeGroupId> &groupIds);

private slots:
	void fillGroups();
};

class GroupSelectionDialog: public QDialog
{
    Q_OBJECT

public:
    GroupSelectionDialog(QWidget *parent) ;
    virtual ~GroupSelectionDialog() ;

    static std::list<RsNodeGroupId> selectGroups(const std::list<RsNodeGroupId>& default_groups) ;

private:
    GroupSelectionBox *mBox ;
};
