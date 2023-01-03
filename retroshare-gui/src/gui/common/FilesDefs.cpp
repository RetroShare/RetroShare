/*******************************************************************************
 * gui/common/FilesDefs.cpp                                                    *
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

#include "FilesDefs.h"

#include "RsCollection.h"

#include <QApplication>
#include <QFileInfo>

#include <map>

//#define DEBUG_FILESDEFS 1

static QString getInfoFromFilename(const QString& filename, bool anyForUnknown, bool image)
{
	QString ext = QFileInfo(filename).suffix().toLower();

	if (ext == "jpg" || ext == "jpeg" || ext == "tif" || ext == "tiff" || ext == "png" || ext == "gif" ||
        ext == "bmp" || ext == "ico" || ext == "svg" || ext == "webp") {
		return image ? ":/icons/filetype/picture.svg" : QApplication::translate("FilesDefs", "Picture");
	} else if (ext == "avi" || ext == "mpg" || ext == "mpeg" || ext == "wmv" || ext == "divx" || ext == "ts" ||
			   ext == "mkv" || ext == "mp4" || ext == "flv" || ext == "mov" || ext == "asf" || ext == "xvid" ||
			   ext == "vob" || ext == "qt" || ext == "rm" || ext == "3gp" || ext == "ogm" || ext == "webm") {
		return image ? ":/icons/filetype/video.svg" : QApplication::translate("FilesDefs", "Video");
	} else if (ext == "ogg" || ext == "mp3" || ext == "mp1" || ext == "mp2" || ext == "wav" || ext == "wma" ||
               ext == "m4a" || ext == "flac" || ext == "xpm" || ext == "weba") {
		return image ? ":/icons/filetype/audio.svg" : QApplication::translate("FilesDefs", "Audio");
	} else if (ext == "tar" || ext == "bz2" || ext == "zip" || ext == "gz" || ext == "7z" || ext == "msi" ||
			   ext == "rar" || ext == "rpm" || ext == "ace" || ext == "jar" || ext == "tgz" || ext == "lha" ||
			   ext == "cab" || ext == "cbz"|| ext == "cbr" || ext == "alz" || ext == "sit" || ext == "arj" || ext == "deb") {
		return image ? ":/icons/filetype/archive.svg" : QApplication::translate("FilesDefs", "Archive");
	} else if (ext == "app" || ext == "bat" || ext == "cgi" || ext == "com" ||
			   ext == "exe" || ext == "js" || ext == "pif" ||
			   ext == "py" || ext == "pl" || ext == "sh" || ext == "vb" || ext == "ws") {
		return image ? ":/icons/filetype/program.svg" : QApplication::translate("FilesDefs", "Program");
	} else if (ext == "iso" || ext == "nrg" || ext == "mdf" || ext == "img" || ext == "dmg" || ext == "bin" || ext == "uif") {
		return image ? ":/icons/filetype/img.svg" : QApplication::translate("FilesDefs", "CD/DVD-Image");
	} else if (ext == "txt" || ext == "ui" ||
			   ext == "doc" || ext == "rtf" || ext == "sxw" || ext == "xls" || ext == "pps" || ext == "xml" || ext == "nfo" ||
			   ext == "reg" || ext == "sxc" || ext == "odt" || ext == "ods" || ext == "dot" || ext == "ppt" || ext == "css" || ext == "crt" ||
			   ext == "html" || ext == "htm" || ext == "php") {
		return image ? ":/icons/filetype/document.svg" : QApplication::translate("FilesDefs", "Document");
	} else if (ext == "pdf") {
		return image ? ":/icons/filetype/pdf.svg" : QApplication::translate("FilesDefs", "Document");
	} else if (ext == RsCollection::ExtensionString) {
		return image ? ":/icons/filetype/collection.svg" : QApplication::translate("FilesDefs", "RetroShare collection file");
	} else if (ext == "sub" || ext == "srt") {
		return image ? ":/icons/filetype/any.svg" : QApplication::translate("FilesDefs", "Subtitles");
	} else if (ext == "nds") {
		return image ? ":/icons/filetype/any.svg" : QApplication::translate("FilesDefs", "Nintendo DS Rom");
	} else if (ext == "patch" || ext == "diff") {
		return image ? ":/icons/filetype/patch.svg" : QApplication::translate("FilesDefs", "Patch");
	} else if (ext == "cpp") {
		return image ? ":/icons/filetype/cpp.svg" : QApplication::translate("FilesDefs", "C++");
	} else if (ext == "h") {
		return image ? ":/icons/filetype/h.svg" : QApplication::translate("FilesDefs", "Header");
	} else if (ext == "c") {
		return image ? ":/icons/filetype/c.svg" : QApplication::translate("FilesDefs", "C ");
	} else if (ext == "apk") {
		return image ? ":/icons/filetype/apk.svg" : QApplication::translate("FilesDefs", "APK ");
	} else if (ext == "dll") {
		return image ? ":/icons/filetype/dll.svg" : QApplication::translate("FilesDefs", "DLL ");
	}


	if (anyForUnknown) {
		return image ? ":/icons/filetype/any.svg" : "";
	}

	return "";
}

QString FilesDefs::getImageFromFilename(const QString& filename, bool anyForUnknown)
{
	return getInfoFromFilename(filename, anyForUnknown, true);
}

QPixmap FilesDefs::getPixmapFromQtResourcePath(const QString& resource_path)
{
	static std::map<QString,QPixmap> mPixmapCache;
	QPixmap pixmap;
#ifdef DEBUG_FILESDEFS
    std::cerr << "Creating Pixmap from resource path " << resource_path.toStdString() ;
#endif

	auto item = mPixmapCache.find(resource_path);

	if (item == mPixmapCache.end())
	{
#ifdef DEBUG_FILESDEFS
        std::cerr << "  Not in cache. Creating new one." << std::endl;
#endif
        pixmap = QPixmap(resource_path);
		mPixmapCache[resource_path] = pixmap;
	}
	else
    {
#ifdef DEBUG_FILESDEFS
        std::cerr << "  In cache. " << std::endl;
#endif
		pixmap = item->second;
	}

	return pixmap;
}

QIcon FilesDefs::getIconFromQtResourcePath(const QString& resource_path)
{
	static std::map<QString,QIcon> mIconCache;
	QIcon icon;
#ifdef DEBUG_FILESDEFS
    std::cerr << "Creating Icon from resource path " << resource_path.toStdString() ;
#endif

	auto item = mIconCache.find(resource_path);

	if (item == mIconCache.end())
	{
#ifdef DEBUG_FILESDEFS
        std::cerr << "  Not in cache. Creating new one." << std::endl;
#endif
		icon = QIcon(resource_path);
		mIconCache[resource_path] = icon;
	}
	else
    {
#ifdef DEBUG_FILESDEFS
        std::cerr << "  In cache. " << std::endl;
#endif
		icon = item->second;
	}

	return icon;
}

QIcon FilesDefs::getIconFromGxsIdCache(const RsGxsId& id,const QIcon& setIcon, bool& exist)
{
	static std::map<RsGxsId,QIcon> mIconCache;
	exist = false;
	QIcon icon;
#ifdef DEBUG_FILESDEFS
	std::cerr << "Creating Icon from id " << id.toStdString() ;
#endif
	if (setIcon.isNull())
	{
		if (id.isNull())
			return getIconFromQtResourcePath(":/icons/notification.svg");

		auto item = mIconCache.find(id);

		if (item == mIconCache.end())
		{
#ifdef DEBUG_FILESDEFS
			std::cerr << "  Not in cache. And not setted." << std::endl;
#endif
			icon = getIconFromQtResourcePath(":/icons/png/anonymous.png");
		}
		else
		{
#ifdef DEBUG_FILESDEFS
			std::cerr << "  In cache. " << std::endl;
#endif
			icon = item->second;
			exist = true;
		}
	}
	else
	{
#ifdef DEBUG_FILESDEFS
		std::cerr << "  Have to set it." << std::endl;
#endif
		icon = setIcon;
		mIconCache[id] = icon;
	}
	return icon;
}

QIcon FilesDefs::getIconFromFileType(const QString& filename)
{
    return getIconFromQtResourcePath(getInfoFromFilename(filename,true,true));
}

QString FilesDefs::getNameFromFilename(const QString &filename)
{
	return getInfoFromFilename(filename, false, false);
}
