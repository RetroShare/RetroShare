
#include "RemoteDirModel.h"
#include "rsfiles.h"

#include <QtGui>


#include <iostream>
#include <sstream>
#include <math.h>

RemoteDirModel::RemoteDirModel(bool mode, QObject *parent)
        : QAbstractItemModel(parent),
         RemoteMode(mode), nIndex(1), indexSet(1)   /* ass zero index cant be used */
{
	setSupportedDragActions(Qt::CopyAction);
}



 bool RemoteDirModel::hasChildren(const QModelIndex &parent) const
 {

#ifdef RDM_DEBUG
     std::cerr << "RemoteDirModel::hasChildren() :" << parent.internalPointer();
     std::cerr << ": ";
#endif

     if (!parent.isValid())
     {
#ifdef RDM_DEBUG
        std::cerr << "root -> true ";
     	std::cerr << std::endl;
#endif
     	return true;
     }

     void *ref = parent.internalPointer();

     DirDetails details;
     uint32_t flags = DIR_FLAGS_CHILDREN;
     if (RemoteMode)
     	flags |= DIR_FLAGS_REMOTE;
     else
     	flags |= DIR_FLAGS_LOCAL;

     if (!rsFiles->RequestDirDetails(ref, details, flags))
     {
     	/* error */
#ifdef RDM_DEBUG
        std::cerr << "lookup failed -> false";
     	std::cerr << std::endl;
#endif
     	return false;
     }
     	
     if (details.type == DIR_TYPE_FILE) 
     {
#ifdef RDM_DEBUG
        std::cerr << "lookup FILE -> false";
     	std::cerr << std::endl;
#endif
        return false;
     }
     /* PERSON/DIR*/
#ifdef RDM_DEBUG
     std::cerr << "lookup PER/DIR #" << details.count;
     std::cerr << std::endl;
#endif
     return (details.count > 0); /* do we have children? */
 }


 int RemoteDirModel::rowCount(const QModelIndex &parent) const
 {
#ifdef RDM_DEBUG
     std::cerr << "RemoteDirModel::rowCount(): " << parent.internalPointer();
     std::cerr << ": ";
#endif

     void *ref = NULL;
     if (parent.isValid())
     {
     	ref = parent.internalPointer();
     }

     DirDetails details;
     uint32_t flags = DIR_FLAGS_CHILDREN;
     if (RemoteMode)
     	flags |= DIR_FLAGS_REMOTE;
     else
     	flags |= DIR_FLAGS_LOCAL;

     if (!rsFiles->RequestDirDetails(ref, details, flags))
     {
#ifdef RDM_DEBUG
        std::cerr << "lookup failed -> 0";
     	std::cerr << std::endl;
#endif
     	return 0;
     }
     if (details.type == DIR_TYPE_FILE)
     {
#ifdef RDM_DEBUG
        std::cerr << "lookup FILE: 0";
        std::cerr << std::endl;
#endif
     	return 0;
     }
	
     /* else PERSON/DIR*/
#ifdef RDM_DEBUG
     std::cerr << "lookup PER/DIR #" << details.count;
     std::cerr << std::endl;
#endif

     return details.count;
 }

 int RemoteDirModel::columnCount(const QModelIndex &parent) const
 {
	return 4;
 }

 QVariant RemoteDirModel::data(const QModelIndex &index, int role) const
 {
#ifdef RDM_DEBUG
     std::cerr << "RemoteDirModel::data(): " << index.internalPointer();
     std::cerr << ": ";
     std::cerr << std::endl;
#endif

     if (!index.isValid())
         return QVariant();

     /* get the data from the index */
     void *ref = index.internalPointer();
     int coln = index.column(); 

     DirDetails details;
     uint32_t flags = DIR_FLAGS_DETAILS;
     if (RemoteMode)
     	flags |= DIR_FLAGS_REMOTE;
     else
     	flags |= DIR_FLAGS_LOCAL;

     if (!rsFiles->RequestDirDetails(ref, details, flags))
     {
     	return QVariant();
     }

     if (role == Qt::BackgroundRole)
     {
     	/*** colour entries based on rank/age/count **/
     	/*** rank (0-10) ***/
	uint32_t r = details.rank;
	if (r > 10) r = 10;
	r = 200 + r * 5; /* 0->250 */

     	/*** age: log2(age) ***
	 *   1 hour = 3,600       -  250      
	 *   1 day  = 86,400      -  200 
	 *   1 week = 604,800     -  100
	 *   1 month = 2,419,200  -   50
	 *
	 *
	 *   250 - log2( 1 + (age / 100) ) * 10
	 *   0       => 1     =>   0   =>  0   => 250
	 *   900     => 10    =>   3.2 => 32   => 220
	 *   3600    => 37    =>   5.2 => 52   => 200
	 *   86400   => 865   =>   9.2 => 92   => 160
	 *   604800  => 6049  =>  12.3 => 120  => 130
	 *   2419200 => 24193 =>  14.4 => 140  => 110
	 *
	 *   value  log2
	 *
	 *   1       0
	 *   2       1
	 *   4       2
	 *   8       3
	 *   16      4
	 *   32      5
	 *   64      6
	 *   128     7
	 *   256     8
	 *   512     9
	 *   1024    10
	 *   2048    11
	 *   4096    12
	 *   8192    13
	 *  16384    14
	 *  32K      15
	 *
	 */

	uint32_t g =  (uint32_t) log2 ( 1.0 + ( details.age / 100 ) ) * 4;
	if (g > 250) g = 250;
	g = 250 - g;

	if (details.type == DIR_TYPE_PERSON)
	{
		return QVariant();
	}
	else if (details.type == DIR_TYPE_DIR)
	{
		uint32_t b =  200 + details.count;
		if (b > 250) b = 250;

		QBrush brush(QColor(r,g,b));
		return brush;
	}
	else if (details.type == DIR_TYPE_FILE)
	{
		uint32_t b =  (uint32_t) (200 + 2 * log2(details.count));
		if (b > 250) b = 250;

		QBrush brush(QColor(r,g,b));
		return brush;
	}
	else
	{
		return QVariant();
	}
     }

     /*************
     Qt::EditRole
     Qt::ToolTipRole
     Qt::StatusTipRole
     Qt::WhatsThisRole
     Qt::SizeHintRole
     ****************/

     if (role == Qt::DisplayRole)
     {

        /* 
	 * Person:  name,  id, 0, 0;
	 * File  :  name,  size, rank, (0) ts
	 * Dir   :  name,  (0) count, (0) path, (0) ts
	 */


	if (details.type == DIR_TYPE_PERSON) /* Person */
	{
		switch(coln)
		{
			case 0:
		return QString::fromStdString(details.name);
			break;
			case 1:
		//return QString("");
		return QString::fromStdString(details.id);
			break;
			default:
		//return QString("");
		return QString::fromStdString("P");
			break;
		}
	}
	else if (details.type == DIR_TYPE_FILE) /* File */
	{
		switch(coln)
		{
			case 0:
		return QString::fromStdString(details.name);
			break;
			case 1:
		{
			std::ostringstream out;
			out << details.count;
			return QString::fromStdString(out.str());
		}
			break;
			case 2:
		{
			std::ostringstream out;
			out << details.rank;
			return QString::fromStdString(out.str());
		}
			break;
			case 3:
		{
			std::ostringstream out;
			out << details.age;
			return QString::fromStdString(out.str());
		}
			break;
			default:
		return QString::fromStdString("FILE");
			break;
		}
	}
	else if (details.type == DIR_TYPE_DIR) /* Dir */
	{
		switch(coln)
		{
			case 0:
		return QString::fromStdString(details.name);
			break;
			case 1:
		//return QString("");
		{
			std::ostringstream out;
			out << details.count;
			return QString::fromStdString(out.str());
		}
			break;
			case 2:
		//return QString("");
		return QString::fromStdString(details.path);
			break;

			default:
		return QString::fromStdString("DIR");
			break;
		}
	}
   } /* end of DisplayRole */
   return QVariant();
 }

 QVariant RemoteDirModel::headerData(int section, Qt::Orientation orientation,
                                      int role) const
 {
     if (role == Qt::SizeHintRole)
     {
     	 int defw = 50;
     	 int defh = 21;
     	 if (section < 2)
	 {
	 	defw = 200;
	 }
         return QSize(defw, defh);
     }

     if (role != Qt::DisplayRole)
         return QVariant();

     if (orientation == Qt::Horizontal)
     {
     	switch(section)
	{
		case 0:
			if (RemoteMode)
			{
				return QString("Remote Directories");
			}
			else
			{
				return QString("Local Directories");
			}
			break;
		case 1:
			return QString("Size");
			break;
		case 2:
			return QString("Rank");
			break;
		case 3:
			return QString("Age");
			break;
	}
        return QString("Column %1").arg(section);
     }
     else
         return QString("Row %1").arg(section);
 }

 QModelIndex RemoteDirModel::index(int row, int column,        
                        const QModelIndex & parent) const
 {
#ifdef RDM_DEBUG
	std::cerr << "RemoteDirModel::index(): " << parent.internalPointer();
	std::cerr << ": row:" << row << " col:" << column << " ";
#endif

	void *ref = NULL;
	
        if (parent.isValid())
	{
		ref = parent.internalPointer();
	}

	/********
		if (!RemoteMode)
		{
			remote = &(rsiface->getLocalDirectoryList());
		}
	********/

     	DirDetails details;
     	uint32_t flags = DIR_FLAGS_CHILDREN;
     	if (RemoteMode)
     		flags |= DIR_FLAGS_REMOTE;
     	else
     		flags |= DIR_FLAGS_LOCAL;

     	if (!rsFiles->RequestDirDetails(ref, details, flags))
     	{
#ifdef RDM_DEBUG
     		std::cerr << "lookup failed -> invalid";
     		std::cerr << std::endl;
#endif
     		return QModelIndex();
     	}

	/* now iterate through the details to 
	 * get the reference number
	 */

	std::list<DirStub>::iterator it;
	int i = 0;
	for(it = details.children.begin();
		(i < row) && (it != details.children.end()); it++, i++);

	if (it == details.children.end()) 
	{ 
#ifdef RDM_DEBUG
     		std::cerr << "wrong number of children -> invalid";
     		std::cerr << std::endl;
#endif
		return QModelIndex(); 
	}

#ifdef RDM_DEBUG
     	std::cerr << "success index(" << row << "," << column << "," << it->ref << ")";
     	std::cerr << std::endl;
#endif

	/* we can just grab the reference now */
	QModelIndex qmi = createIndex(row, column, it->ref);
	return qmi;
 }


 QModelIndex RemoteDirModel::parent( const QModelIndex & index ) const
 {
#ifdef RDM_DEBUG
	std::cerr << "RemoteDirModel::parent(): " << index.internalPointer();
	std::cerr << ": ";
#endif

	/* create the index */
        if (!index.isValid())
	{
#ifdef RDM_DEBUG
     		std::cerr << "Invalid Index -> invalid";
     		std::cerr << std::endl;
#endif
		/* Parent is invalid too */
		return QModelIndex();
	}
	void *ref = index.internalPointer();

     	DirDetails details;
     	uint32_t flags = DIR_FLAGS_PARENT;
     	if (RemoteMode)
     		flags |= DIR_FLAGS_REMOTE;
     	else
     		flags |= DIR_FLAGS_LOCAL;

     	if (!rsFiles->RequestDirDetails(ref, details, flags))
     	{
#ifdef RDM_DEBUG
     		std::cerr << "Failed Lookup -> invalid";
     		std::cerr << std::endl;
#endif
     		return QModelIndex();
     	}

	if (!(details.parent))
	{
#ifdef RDM_DEBUG
     		std::cerr << "success. parent is Root/NULL --> invalid";
     		std::cerr << std::endl;
#endif
     		return QModelIndex();
	}

#ifdef RDM_DEBUG
     	std::cerr << "success index(" << details.prow << ",0," << details.parent << ")";
     	std::cerr << std::endl;

#endif
	return createIndex(details.prow, 0, details.parent);
 }

 Qt::ItemFlags RemoteDirModel::flags( const QModelIndex & index ) const
 {
#ifdef RDM_DEBUG
     	std::cerr << "RemoteDirModel::flags()";
  	std::cerr << std::endl;
#endif

     	if (!index.isValid())
		return (Qt::ItemIsSelectable); // Error.

	void *ref = index.internalPointer();

     	DirDetails details;
     	uint32_t flags = DIR_FLAGS_DETAILS;
     	if (RemoteMode)
     		flags |= DIR_FLAGS_REMOTE;
     	else
     		flags |= DIR_FLAGS_LOCAL;

     	if (!rsFiles->RequestDirDetails(ref, details, flags))
     	{
		return (Qt::ItemIsSelectable); // Error.
     	}

     	if (details.type == DIR_TYPE_PERSON)
	{
     		return (Qt::ItemIsEnabled);
	}
     	else if (details.type == DIR_TYPE_DIR)
	{
     		return ( Qt::ItemIsSelectable | 
			Qt::ItemIsEnabled);

//			Qt::ItemIsDragEnabled |
//			Qt::ItemIsDropEnabled |

	}
	else // (details.type == DIR_TYPE_FILE)
	{
     		return ( Qt::ItemIsSelectable | 
			Qt::ItemIsDragEnabled |
			Qt::ItemIsEnabled);


	}
}

// The other flags...
//Qt::ItemIsUserCheckable
//Qt::ItemIsEditable
//Qt::ItemIsDropEnabled
//Qt::ItemIsTristate



/* Callback from */
 void RemoteDirModel::preMods()
 {
	std::cerr << "RemoteDirModel::preMods()" << std::endl;
	//modelAboutToBeReset();
	reset();
	layoutAboutToBeChanged();
 }

/* Callback from */
 void RemoteDirModel::postMods()
 {
	std::cerr << "RemoteDirModel::postMods()" << std::endl;
	//modelReset();
	layoutChanged();
	//reset();
 }


void RemoteDirModel::update (const QModelIndex &index )
{
	//std::cerr << "Directory Request(" << id << ") : ";
	//std::cerr << path << std::endl;
	//rsFiles -> RequestDirectories(id, path, 1);
}

void RemoteDirModel::downloadSelected(QModelIndexList list)
{
	if (!RemoteMode)
	{
		std::cerr << "Cannot download from local" << std::endl;
	}

	/* so for all the selected .... get the name out, 
	 * make it into something the RsControl can understand
	 */

	/* Fire off requests */
	QModelIndexList::iterator it;
	for(it = list.begin(); it != list.end(); it++)
	{
		void *ref = it -> internalPointer();

     		DirDetails details;
     		uint32_t flags = DIR_FLAGS_DETAILS;
     		if (RemoteMode)
		{
     			flags |= DIR_FLAGS_REMOTE;
		}
     		else
		{
     			flags |= DIR_FLAGS_LOCAL;
			continue; /* don't try to download local stuff */
		}

     		if (!rsFiles->RequestDirDetails(ref, details, flags))
     		{
			continue;
     		}
		/* only request if it is a file */
		if (details.type == DIR_TYPE_FILE)
		{
			rsFiles -> FileRequest(details.name, details.hash, 
						details.count, "", 0);
		}
	}
}

/****************************************************************************
 * OLD RECOMMEND SYSTEM - DISABLED
 *
 */

#if 0

void RemoteDirModel::recommendSelected(QModelIndexList list)
{
	std::cerr << "recommendSelected()" << std::endl;
	if (RemoteMode)
	{
		std::cerr << "Cannot recommend remote! (should download)" << std::endl;
	}
	/* Fire off requests */
	QModelIndexList::iterator it;
	for(it = list.begin(); it != list.end(); it++)
	{
		void *ref = it -> internalPointer();

     		DirDetails details;
     		uint32_t flags = DIR_FLAGS_DETAILS;
     		if (RemoteMode)
		{
     			flags |= DIR_FLAGS_REMOTE;
			continue; /* don't recommend remote stuff */
		}
     		else
		{
     			flags |= DIR_FLAGS_LOCAL;
		}

     		if (!rsFiles->RequestDirDetails(ref, details, flags))
     		{
			continue;
     		}

		std::cerr << "::::::::::::FileRecommend:::: " << std::endl;
		std::cerr << "Name: " << details.name << std::endl;
		std::cerr << "Hash: " << details.hash << std::endl;
		std::cerr << "Size: " << details.count << std::endl;
		std::cerr << "Path: " << details.path << std::endl;

		rsFiles -> FileRecommend(details.name, details.hash, details.count);
	}
	std::cerr << "::::::::::::Done FileRecommend" << std::endl;
}


void RemoteDirModel::recommendSelectedOnly(QModelIndexList list)
{
	std::cerr << "recommendSelectedOnly()" << std::endl;
	if (RemoteMode)
	{
		std::cerr << "Cannot recommend remote! (should download)" << std::endl;
	}
     	rsFiles->ClearInRecommend();

	/* Fire off requests */
	QModelIndexList::iterator it;
	for(it = list.begin(); it != list.end(); it++)
	{
		void *ref = it -> internalPointer();

     		DirDetails details;
     		uint32_t flags = DIR_FLAGS_DETAILS;
     		if (RemoteMode)
		{
     			flags |= DIR_FLAGS_REMOTE;
			continue; /* don't recommend remote stuff */
		}
     		else
		{
     			flags |= DIR_FLAGS_LOCAL;
		}

     		if (!rsFiles->RequestDirDetails(ref, details, flags))
     		{
			continue;
     		}

		std::cerr << "::::::::::::FileRecommend:::: " << std::endl;
		std::cerr << "Name: " << details.name << std::endl;
		std::cerr << "Hash: " << details.hash << std::endl;
		std::cerr << "Size: " << details.count << std::endl;
		std::cerr << "Path: " << details.path << std::endl;

		rsFiles -> FileRecommend(details.name, details.hash, details.count);
     		rsFiles -> SetInRecommend(details.name, true);
	}
	std::cerr << "::::::::::::Done FileRecommend" << std::endl;
}

#endif
/****************************************************************************
 * OLD RECOMMEND SYSTEM - DISABLED
 ******/

void RemoteDirModel::openSelected(QModelIndexList list)
{
	//recommendSelected(list);
}


void RemoteDirModel::getFilePaths(QModelIndexList list, std::list<std::string> &fullpaths)
{
	std::cerr << "RemoteDirModel::getFilePaths()" << std::endl;
	if (RemoteMode)
	{
		std::cerr << "No File Paths for remote files" << std::endl;
		return;
	}
	/* translate */
	QModelIndexList::iterator it;
	for(it = list.begin(); it != list.end(); it++)
	{
		void *ref = it -> internalPointer();

     		DirDetails details;
     		uint32_t flags = DIR_FLAGS_DETAILS;
     		flags |= DIR_FLAGS_LOCAL;

     		if (!rsFiles->RequestDirDetails(ref, details, flags))
     		{
			std::cerr << "getFilePaths() Bad Request" << std::endl;
			continue;
     		}

		if (details.type != DIR_TYPE_FILE)
		{
			std::cerr << "getFilePaths() Not File" << std::endl;
			continue; /* not file! */
		}

		std::cerr << "::::::::::::File Details:::: " << std::endl;
		std::cerr << "Name: " << details.name << std::endl;
		std::cerr << "Hash: " << details.hash << std::endl;
		std::cerr << "Size: " << details.count << std::endl;
		std::cerr << "Path: " << details.path << std::endl;

		std::string filepath = details.path + "/";
		filepath += details.name;

		std::cerr << "Constructed FilePath: " << filepath << std::endl;
		if (fullpaths.end() == std::find(fullpaths.begin(), fullpaths.end(), filepath))
		{
			fullpaths.push_back(filepath);
		}
	}
	std::cerr << "::::::::::::Done getFilePaths" << std::endl;
}

  /* Drag and Drop Functionality */
QMimeData * RemoteDirModel::mimeData ( const QModelIndexList & indexes ) const
{
	/* extract from each the member text */
	std::string  text;
	QModelIndexList::const_iterator it;
	std::map<std::string, uint64_t> drags;
	std::map<std::string, uint64_t>::iterator dit;

	for(it = indexes.begin(); it != indexes.end(); it++)
	{
		void *ref = it -> internalPointer();

     		DirDetails details;
     		uint32_t flags = DIR_FLAGS_DETAILS;
     		if (RemoteMode)
		{
     			flags |= DIR_FLAGS_REMOTE;
		}
     		else
		{
     			flags |= DIR_FLAGS_LOCAL;
		}

     		if (!rsFiles->RequestDirDetails(ref, details, flags))
     		{
			continue;
     		}

		std::cerr << "::::::::::::FileDrag:::: " << std::endl;
		std::cerr << "Name: " << details.name << std::endl;
		std::cerr << "Hash: " << details.hash << std::endl;
		std::cerr << "Size: " << details.count << std::endl;
		std::cerr << "Path: " << details.path << std::endl;

		if (details.type != DIR_TYPE_FILE)
		{
			std::cerr << "RemoteDirModel::mimeData() Not File" << std::endl;
			continue; /* not file! */
		}

		if (drags.end() != (dit = drags.find(details.hash)))
		{
			std::cerr << "RemoteDirModel::mimeData() Duplicate" << std::endl;
			continue; /* duplicate */
		}

		drags[details.hash] = details.count;

		std::string line = details.name;
		line += "/";
		line += details.hash;
		line += "/";

		{
			std::ostringstream out;
			out << details.count;
			line += out.str();
			line += "/";
		}

		if (RemoteMode)
		{
			line += "Remote";
		}
		else
		{
			line += "Local";
		}
		line += "/\n";

		text += line;
	}

	std::cerr << "Created MimeData:";
	std::cerr << std::endl;

	std::cerr << text;
	std::cerr << std::endl;

	QMimeData *data = new QMimeData();
	data->setData("application/x-rsfilelist", QByteArray(text.c_str()));

	return data;


}

QStringList RemoteDirModel::mimeTypes () const
{
	QStringList list;
	list.push_back("application/x-rsfilelist");

	return list;
}


