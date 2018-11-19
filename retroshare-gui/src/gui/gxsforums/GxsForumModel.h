#ifdef SUSPENDED_CODE
#include "retroshare/rsgxsifacetypes.h"
#else
#include <string>
#include <iostream>

struct RsMsgMetaData
{
	std::string mMsgName ;
	time_t mPublishTs;
	uint32_t mMsgStatus;
	QString mAuthorId;
};

#endif

#include <QModelIndex>

// This class holds the actual hierarchy of posts, represented by identifiers
// It is responsible for auto-updating when necessary and holds a mutex to allow the Model to
// safely access the data.

// The model contains a post in place 0 that is the parent of all posts.

typedef uint32_t ForumModelIndex;

struct ForumPostEntry
{
    std::vector<RsMsgMetaData> meta_versions;	// maybe we don't need all this. Could be too large.

    std::vector<ForumModelIndex> children;
    ForumModelIndex parent;
    int prow ;									// parent row
};

// This class is the item model used by Qt to display the information

class RsGxsForumModel : public QAbstractItemModel
{
	//    Q_OBJECT

public:
	explicit RsGxsForumModel(QObject *parent = NULL);
	~RsGxsForumModel(){}

	enum Roles{ SortRole = Qt::UserRole+1 };

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;

    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const  override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    QVariant sizeHintRole(int col) const;
	QVariant displayRole(const RsMsgMetaData& meta, int col) const;
	QVariant userRole(const RsMsgMetaData& meta, int col) const;
	QVariant decorationRole(const RsMsgMetaData &meta, int col) const;

	void update_posts();

private:
    void *getParentRef(void *ref,int& row) const;
    void *getChildRef(void *ref,int row) const;
    //bool hasIndex(int row,int column,const QModelIndex& parent)const;
    int  getChildrenCount(void *ref) const;

    static bool convertTabEntryToRefPointer(uint32_t entry,void *& ref);
	static bool convertRefPointerToTabEntry(void *ref,uint32_t& entry);

    std::vector<ForumPostEntry> mPosts ; // store the list of posts updated from rsForums.
};
