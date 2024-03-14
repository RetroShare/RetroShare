/*******************************************************************************
 * gui/common/RsCollection.h                                                   *
 *                                                                             *
 * Copyright (C) 2011, Retroshare Team <retroshare.project@gmail.com>          *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

// Implements a RetroShare collection file. Such a file contains
//
// - a directory structure
// - retroshare links to put files in
//

#pragma once

#include <QObject>
#include <QString>
#include <QDomDocument>
#include <QFile>
#include <retroshare/rsfiles.h>
#include <QMetaType>

class QDomElement ;
class QWidget;

class ColFileInfo
{
	public:
	ColFileInfo(): name(""), size(0), path(""), hash(""), type(0), filename_has_wrong_characters(false), checked(false) {}

public:
	QString name ;
	qulonglong size ;
	QString path ;
	QString hash ;
	uint8_t type;
	bool filename_has_wrong_characters ;
	std::vector<ColFileInfo> children;
	bool checked;
};
Q_DECLARE_METATYPE(ColFileInfo)

class RsCollection : public QObject
{
	Q_OBJECT

public:
    enum class RsCollectionErrorCode:uint8_t {
        NO_ERROR                      = 0x00,
        UNKNOWN_ERROR                 = 0x01,
        FILE_READ_ERROR               = 0x02,
        FILE_CONTAINS_HARMFUL_STRINGS = 0x03,
        INVALID_ROOT_NODE             = 0x04,
        XML_PARSING_ERROR             = 0x05,
    };

	RsCollection(QObject *parent = 0) ;
	// create from list of files and directories
	RsCollection(const std::vector<DirDetails>& file_entries, FileSearchFlags flags, QObject *parent = 0) ;
    RsCollection(const RsFileTree& ft);
    RsCollection(const QString& filename,RsCollectionErrorCode& error_code);

    static QString errorString(RsCollectionErrorCode code);

    virtual ~RsCollection() ;

    void merge_in(const QString& fname,uint64_t size,const RsFileHash& hash,RsFileTree::DirIndex parent_index=0) ;
    void merge_in(const RsFileTree& tree,RsFileTree::DirIndex parent_index=0) ;

	static const QString ExtensionString ;

#ifdef TO_REMOVE
	bool load(QWidget *parent);
    bool save(QWidget *parent) const ;
#endif
	// Save to disk
	bool save(const QString& fileName) const ;

    // returns the file tree
    const RsFileTree& fileTree() const { return *mFileTree; }
    // total size of files in the collection
    qulonglong size();
    // total number of files in the collection
    qulonglong count() const;

	// Download the content.
	void downloadFiles() const ;
	// Auto Download all the content.
	void autoDownloadFiles() const ;

	static bool isCollectionFile(const QString& fileName);

    void updateHashes(const std::map<RsFileHash,RsFileHash>& old_to_new_hashes);
private:

    bool recursExportToXml(QDomDocument& doc,QDomElement& e,const RsFileTree::DirData& dd) const;
    bool recursParseXml(QDomDocument& doc, const QDomNode &e, RsFileTree::DirIndex dd) ;

    // This function is used to populate a RsCollection from locally or remotly shared files.
    void recursAddElements(RsFileTree::DirIndex parent, const DirDetails& dd, FileSearchFlags flags) ;

    // This function is used to merge an existing RsFileTree into the RsCollection
    void recursMergeTree(RsFileTree::DirIndex parent, const RsFileTree& tree, const RsFileTree::DirData &dd);

#ifdef TO_REMOVE
	void recursAddElements(QDomDocument&,const ColFileInfo&,QDomElement&) const;
	void recursAddElements(
	        QDomDocument& doc, const RsFileTree& ft, uint32_t index,
	        QDomElement& e ) const;

	void recursCollectColFileInfos(const QDomElement&,std::vector<ColFileInfo>& colFileInfos,const QString& current_dir,bool bad_chars_in_parent) const ;
#endif

    // check that the file is a valid rscollection file, and not a lol bomb or some shit like this
    static bool checkFile(const QString &fileName, RsCollectionErrorCode &error);

	// Auto Download recursively.
	void autoDownloadFiles(ColFileInfo colFileInfo, QString dlDir) const ;

    std::unique_ptr<RsFileTree> mFileTree;
    std::map<RsFileHash,RsFileTree::FileIndex> mHashes;	// used to efficiently update files being hashed

#ifdef TO_REMOVE
	QDomDocument _xml_doc ;
	QString _fileName ;
	bool _saved;
	QDomElement _root ;
#endif

	friend class RsCollectionDialog ;
};

