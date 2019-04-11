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

static QString getInfoFromFilename(const QString& filename, bool anyForUnknown, bool image)
{
	QString ext = QFileInfo(filename).suffix().toLower();

	if (ext == "jpg" || ext == "jpeg" || ext == "tif" || ext == "tiff" || ext == "png" || ext == "gif" || ext == "bmp" || ext == "ico" || ext == "svg") {
		return image ? ":/images/FileTypePicture.png" : QApplication::translate("FilesDefs", "Picture");
	} else if (ext == "avi" || ext == "mpg" || ext == "mpeg" || ext == "wmv" || ext == "divx" || ext == "ts" ||
			   ext == "mkv" || ext == "mp4" || ext == "flv" || ext == "mov" || ext == "asf" || ext == "xvid" ||
			   ext == "vob" || ext == "qt" || ext == "rm" || ext == "3gp" || ext == "ogm") {
		return image ? ":/images/FileTypeVideo.png" : QApplication::translate("FilesDefs", "Video");
	} else if (ext == "ogg" || ext == "mp3" || ext == "mp1" || ext == "mp2" || ext == "wav" || ext == "wma" || ext == "m4a" || ext == "flac" ||ext == "xpm") {
		return image ? ":/images/FileTypeAudio.png" : QApplication::translate("FilesDefs", "Audio");
	} else if (ext == "tar" || ext == "bz2" || ext == "zip" || ext == "gz" || ext == "7z" || ext == "msi" ||
			   ext == "rar" || ext == "rpm" || ext == "ace" || ext == "jar" || ext == "tgz" || ext == "lha" ||
			   ext == "cab" || ext == "cbz"|| ext == "cbr" || ext == "alz" || ext == "sit" || ext == "arj" || ext == "deb") {
		return image ? ":/images/FileTypeArchive.png" : QApplication::translate("FilesDefs", "Archive");
	} else if (ext == "app" || ext == "bat" || ext == "cgi" || ext == "com" ||
			   ext == "exe" || ext == "js" || ext == "pif" ||
			   ext == "py" || ext == "pl" || ext == "sh" || ext == "vb" || ext == "ws") {
		return image ? ":/images/FileTypeProgram.png" : QApplication::translate("FilesDefs", "Program");
	} else if (ext == "iso" || ext == "nrg" || ext == "mdf" || ext == "img" || ext == "dmg" || ext == "bin" || ext == "uif") {
		return image ? ":/images/FileTypeCDImage.png" : QApplication::translate("FilesDefs", "CD/DVD-Image");
	} else if (ext == "txt" || ext == "ui" ||
			   ext == "doc" || ext == "rtf" || ext == "sxw" || ext == "xls" || ext == "pps" || ext == "xml" || ext == "nfo" ||
			   ext == "reg" || ext == "sxc" || ext == "odt" || ext == "ods" || ext == "dot" || ext == "ppt" || ext == "css" || ext == "crt" ||
			   ext == "html" || ext == "htm" || ext == "php") {
		return image ? ":/images/FileTypeDocument.png" : QApplication::translate("FilesDefs", "Document");
	} else if (ext == "pdf") {
		return image ? ":/images/mimetypes/pdf.png" : QApplication::translate("FilesDefs", "Document");
	} else if (ext == RsCollection::ExtensionString) {
		return image ? ":/images/mimetypes/rscollection-16.png" : QApplication::translate("FilesDefs", "RetroShare collection file");
	} else if (ext == "sub" || ext == "srt") {
		return image ? ":/images/FileTypeAny.png" : QApplication::translate("FilesDefs", "Subtitles");
	} else if (ext == "nds") {
		return image ? ":/images/FileTypeAny.png" : QApplication::translate("FilesDefs", "Nintendo DS Rom");
	} else if (ext == "patch" || ext == "diff") {
		return image ? ":/images/mimetypes/patch.png" : QApplication::translate("FilesDefs", "Patch");
	} else if (ext == "cpp") {
		return image ? ":/images/mimetypes/source_cpp.png" : QApplication::translate("FilesDefs", "C++");
	} else if (ext == "h") {
		return image ? ":/images/mimetypes/source_h.png" : QApplication::translate("FilesDefs", "Header");
	} else if (ext == "c") {
		return image ? ":/images/mimetypes/source_c.png" : QApplication::translate("FilesDefs", "C ");
	}


	if (anyForUnknown) {
		return image ? ":/images/FileTypeAny.png" : "";
	}

	return "";
}

QString FilesDefs::getImageFromFilename(const QString& filename, bool anyForUnknown)
{
	return getInfoFromFilename(filename, anyForUnknown, true);
}

QIcon FilesDefs::getIconFromFilename(const QString& filename)
{
	QString sImage = getInfoFromFilename(filename, true, true);
	static std::map<QString,QIcon> mIconCache;
	QIcon icon;
	auto item = mIconCache.find(sImage);
	if (item == mIconCache.end())
	{
		icon = QIcon(sImage);
		mIconCache[sImage] = icon;
	}
	else
		icon = item->second;

	return icon;
}

QString FilesDefs::getNameFromFilename(const QString &filename)
{
	return getInfoFromFilename(filename, false, false);
}
