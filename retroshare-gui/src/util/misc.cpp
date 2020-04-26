/*******************************************************************************
 * util/misc.cpp                                                               *
 *                                                                             *
 * Copyright (c) 2008, defnax           <retroshare.project@gmail.com>         *
 * Copyright (C) 2006  Christophe Dumez                                        *
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

#include <QString>
#include <QDir>
#include <QFileDialog>
#include <QByteArray>
#include <QBuffer>
#include <time.h>
#include <QFontDialog>

#include "misc.h"

// return best userfriendly storage unit (B, KiB, MiB, GiB, TiB)
// use Binary prefix standards from IEC 60027-2
// see http://en.wikipedia.org/wiki/Kilobyte
// value must be given in bytes
QString misc::friendlyUnit(float val)
{
    if(val < 0) {
        return tr("Unknown", "Unknown (size)");
    }
    const QString units[6] = {tr(" B", "bytes"), tr(" KB", "kilobytes (1024 bytes)"), tr(" MB", "megabytes (1024 kilobytes)"), tr(" GB", "gigabytes (1024 megabytes)"), tr(" TB", "terabytes (1024 gigabytes)"), tr(" PB", "petabytes (1024 terabytes)") };
    for(unsigned int i=0; i<6; ++i) {
        if (val < 1024.) {
            return QString(QByteArray::number(val, 'f', 1)) + units[i];
        }
        val /= 1024.;
    }
    return  QString(QByteArray::number(val, 'f', 1)) + tr(" TB", "terabytes (1024 gigabytes)");
}

QString misc::fingerPrintStyleSplit(const QString& in)
{
	QString rest = in ;
	QString res ;

	if(in.isNull())
		return in ;

	for(int i=0;i<in.length();i+=4)
		res += rest.mid(i,4)+' ' ;

	return res.left(res.length()-1) ;
}

bool misc::isPreviewable(QString extension)
{
    extension = extension.toUpper();
    if(extension == "AVI") return true;
    if(extension == "MP3") return true;
    if(extension == "OGG") return true;
    if(extension == "OGM") return true;
    if(extension == "WMV") return true;
    if(extension == "WMA") return true;
    if(extension == "MPEG") return true;
    if(extension == "MPG") return true;
    if(extension == "ASF") return true;
    if(extension == "QT") return true;
    if(extension == "RM") return true;
    if(extension == "RMVB") return true;
    if(extension == "RMV") return true;
    if(extension == "SWF") return true;
    if(extension == "FLV") return true;
    if(extension == "WAV") return true;
    if(extension == "MOV") return true;
    if(extension == "VOB") return true;
    if(extension == "MID") return true;
    if(extension == "AC3") return true;
    if(extension == "MP4") return true;
    if(extension == "MP2") return true;
    if(extension == "AVI") return true;
    if(extension == "FLAC") return true;
    if(extension == "AU") return true;
    if(extension == "MPE") return true;
    if(extension == "MOV") return true;
    if(extension == "MKV") return true;
    if(extension == "AIF") return true;
    if(extension == "AIFF") return true;
    if(extension == "AIFC") return true;
    if(extension == "RA") return true;
    if(extension == "RAM") return true;
    if(extension == "M4P") return true;
    if(extension == "M4A") return true;
    if(extension == "3GP") return true;
    if(extension == "AAC") return true;
    if(extension == "SWA") return true;
    if(extension == "MPC") return true;
    if(extension == "MPP") return true;
    return false;
}

bool misc::isPicture(QString extension)
{
    extension = extension.toUpper();
    if(extension == "PNG") return true;
    if(extension == "GIF") return true;
    if(extension == "JPG") return true;
    if(extension == "JPEG") return true;
    if(extension == "TIFF") return true;
    if(extension == "WEBP") return true;
    return false;
}

bool misc::isVideo(QString extension)
{
    extension = extension.toUpper();
    if(extension == "MP4") return true;
    return false;
}

// return qBittorrent config path
QString misc::qBittorrentPath()
{
    QString qBtPath = QDir::homePath()+QDir::separator()+QString::fromUtf8(".qbittorrent") + QDir::separator();
    // Create dir if it does not exist
    if(!QFile::exists(qBtPath)){
        QDir dir(qBtPath);
        dir.mkpath(qBtPath);
    }
    return qBtPath;
}

QString misc::findFileInDir(QString dir_path, QString fileName)
{
    QDir dir(dir_path);
    if(dir.exists(fileName)) {
        return dir.filePath(fileName);
    }
    QStringList subDirs = dir.entryList(QDir::Dirs);
    QString subdir_name;
    foreach(subdir_name, subDirs) {
        QString result = findFileInDir(dir.path()+QDir::separator()+subdir_name, fileName);
        if(!result.isNull()) {
            return result;
        }
    }
    return QString();
}

// Can't use template class for QString because >,< use unicode code for sorting
// which is not what a human would expect when sorting strings.
void misc::insertSortString(QList<QPair<int, QString> > &list, QPair<int, QString> value, Qt::SortOrder sortOrder)
{
    int i = 0;
    if(sortOrder == Qt::AscendingOrder) {
        while(i < list.size() and QString::localeAwareCompare(value.second, list.at(i).second) > 0) {
            ++i;
        }
    }else{
        while(i < list.size() and QString::localeAwareCompare(value.second, list.at(i).second) < 0) {
            ++i;
        }
    }
    list.insert(i, value);
}

float misc::getPluginVersion(QString filePath)
{
    QFile plugin(filePath);
    if(!plugin.exists()){
        qDebug("%s plugin does not exist, returning 0.0", filePath.toUtf8().data());
        return 0.0;
    }
    if(!plugin.open(QIODevice::ReadOnly | QIODevice::Text)){
        return 0.0;
    }
    float version = 0.0;
    while (!plugin.atEnd()){
        QByteArray line = plugin.readLine();
        if(line.startsWith("#VERSION: ")){
            line = line.split(' ').last();
            line.replace("\n", "");
            version = line.toFloat();
            qDebug("plugin %s version: %.2f", filePath.toUtf8().data(), version);
            break;
        }
    }
    return version;
}

// Take a number of seconds and return an user-friendly
// time duration like "1d 2h 10m".
QString misc::userFriendlyDuration(qlonglong seconds)
{
    if(seconds < 0) {
        return tr("Unknown");
    }
    if(seconds < 60) {
        return tr("< 1m", "< 1 minute");
    }
    int minutes = seconds / 60;
    if(minutes < 60) {
        return tr("%1 minutes","e.g: 10minutes").arg(minutes);
    }
    int hours = minutes / 60;
    minutes = minutes - hours*60;
    if(hours < 24) {
        return tr("%1h %2m", "e.g: 3hours 5minutes").arg(hours).arg(minutes);
    }
    int days = hours / 24;
    hours = hours - days * 24;
    if(days < 365) {
        return tr("%1d %2h", "e.g: 2days 10hours").arg(days).arg(hours);
    }
    int years = days / 365;
    days = days - years * 365;
    return tr("%1y %2d", "e.g: 2 years 2days ").arg(years).arg(days);
}

QString misc::timeRelativeToNow(uint32_t mtime)
{
	if( mtime == 0)
		return QString() ;

	time_t now = time(NULL) ;
	if(mtime > now)
		return misc::userFriendlyDuration(mtime - (int)now) + " (ahead of now)";
	else
		return misc::userFriendlyDuration(now - (int)mtime) ;
}

QString misc::userFriendlyUnit(double count, unsigned int decimal, double factor)
{
    if (count <= 0.0) {
        return "0";
    }

    QString output;

    int i;
    for (i = 0; i < 5; ++i) {
        if (count < factor) {
            break;
        }

        count /= factor;
    }

    QString unit;
    switch (i) {
    case 0:
        decimal = 0; // no decimal
        break;
    case 1:
        unit = tr("k", "e.g: 3.1 k");
        break;
    case 2:
        unit = tr("M", "e.g: 3.1 M");
        break;
    case 3:
        unit = tr("G", "e.g: 3.1 G");
        break;
    default: // >= 4
        unit = tr("T", "e.g: 3.1 T");
    }

    return QString("%1 %2").arg(count, 0, 'f', decimal).arg(unit);
}

QString misc::removeNewLine(const QString &text)
{
    return QString(text).replace("\n", " ");
}

QString misc::removeNewLine(const std::string &text)
{
    return QString::fromUtf8(text.c_str()).replace("\n", " ");
}

QString misc::removeNewLine(const std::wstring &text)
{
    return QString::fromStdWString(text).replace("\n", " ");
}

/*!
* Let's the user choose an avatar picture file, which is returned as a PNG thumbnail
* in a byte array
*
* return false, if the user canceled the dialog, otherwise true
*/
bool misc::getOpenAvatarPicture(QWidget *parent, QByteArray &image_data)
{
	QPixmap picture = getOpenThumbnailedPicture(parent, tr("Load avatar image"), 96, 96);

	if (picture.isNull())
		return false;

	// save image in QByteArray
	QBuffer buffer(&image_data);
	buffer.open(QIODevice::WriteOnly);
	picture.save(&buffer, "PNG"); // writes image into ba in PNG format

	return true;
}

/*!
 * Open a QFileDialog to let the user choose a picture file.
 * This picture is converted to a thumbnail and returned as a QPixmap.
 *
 * \return a null pixmap, if the user canceled the dialog, otherwise the chosen picture
 */
QPixmap misc::getOpenThumbnailedPicture(QWidget *parent, const QString &caption, int width, int height)
{
	// Let the user choose an picture file
	QString fileName;
	if (!getOpenFileName(parent, RshareSettings::LASTDIR_IMAGES, caption, tr("Pictures (*.png *.jpeg *.xpm *.jpg *.tiff *.gif *.webp)"), fileName))
		return QPixmap();

    return QPixmap(fileName).scaledToHeight(height, Qt::SmoothTransformation).copy( 0, 0, width, height);
	//return QPixmap(fileName).scaledToHeight(width, height, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
}

bool misc::getOpenFileName(QWidget *parent, RshareSettings::enumLastDir type
                           , const QString &caption, const QString &filter
                           , QString &file, QFileDialog::Options options)
{
    QString lastDir = Settings->getLastDir(type);

#ifdef RS_NATIVEDIALOGS
    file = QFileDialog::getOpenFileName(parent, caption, lastDir, filter, NULL, QFileDialog::DontResolveSymlinks |                                    options);
#else
    file = QFileDialog::getOpenFileName(parent, caption, lastDir, filter, NULL, QFileDialog::DontResolveSymlinks | QFileDialog::DontUseNativeDialog | options);
#endif

    if (file.isEmpty())
        return false;

    lastDir = QFileInfo(file).absoluteDir().absolutePath();
    Settings->setLastDir(type, lastDir);

#ifdef WINDOWS_SYS
    // fix bug in Qt for Windows Vista and higher, convert path from native separators
    if (QSysInfo::WindowsVersion >= QSysInfo::WV_VISTA && (QSysInfo::WindowsVersion & QSysInfo::WV_NT_based))
        file = QDir::fromNativeSeparators(file);
#endif

    return true;
}

bool misc::getOpenFileNames(QWidget *parent, RshareSettings::enumLastDir type
                            , const QString &caption, const QString &filter
                            , QStringList &files, QFileDialog::Options options)
{
    QString lastDir = Settings->getLastDir(type);

#ifdef RS_NATIVEDIALOGS
    files = QFileDialog::getOpenFileNames(parent, caption, lastDir, filter, NULL, QFileDialog::DontResolveSymlinks |                                    options);
#else
    files = QFileDialog::getOpenFileNames(parent, caption, lastDir, filter, NULL, QFileDialog::DontResolveSymlinks | QFileDialog::DontUseNativeDialog | options);
#endif

    if (files.isEmpty())
        return false;

    lastDir = QFileInfo(*files.begin()).absoluteDir().absolutePath();
    Settings->setLastDir(type, lastDir);

#ifdef WINDOWS_SYS
    // fix bug in Qt for Windows Vista and higher, convert path from native separators
    if (QSysInfo::WindowsVersion >= QSysInfo::WV_VISTA && (QSysInfo::WindowsVersion & QSysInfo::WV_NT_based))
        for (QStringList::iterator file = files.begin(); file != files.end(); ++file) {
            (*file) = QDir::fromNativeSeparators(*file);
        }
#endif

    return true;
}

bool misc::getSaveFileName(QWidget *parent, RshareSettings::enumLastDir type
													 , const QString &caption , const QString &filter
													 , QString &file, QString *selectedFilter
													 , QFileDialog::Options options)
{
    QString lastDir = Settings->getLastDir(type) + "/" + file;

#ifdef RS_NATIVEDIALOGS
    file = QFileDialog::getSaveFileName(parent, caption, lastDir, filter, selectedFilter,                                    options);
#else
    file = QFileDialog::getSaveFileName(parent, caption, lastDir, filter, selectedFilter, QFileDialog::DontUseNativeDialog | options);
#endif

    if (file.isEmpty())
        return false;

    lastDir = QFileInfo(file).absoluteDir().absolutePath();
    Settings->setLastDir(type, lastDir);

	return true;
}

QFont misc::getFont(bool *ok, const QFont &initial, QWidget *parent, const QString &title)
{
#ifdef RS_NATIVEDIALOGS
		return QFontDialog::getFont(ok, initial, parent, title);
#else
		return QFontDialog::getFont(ok, initial, parent, title, QFontDialog::DontUseNativeDialog);
#endif
}

QString misc::getExistingDirectory(QWidget *parent, const QString &caption, const QString &dir)
{
#ifdef RS_NATIVEDIALOGS
		return QFileDialog::getExistingDirectory(parent, caption, dir, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
#else
		return QFileDialog::getExistingDirectory(parent, caption, dir, QFileDialog::DontUseNativeDialog | QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
#endif
}

/*!
 * Clear a Layout content
 * \param layout: Layout to Clear
 */
void misc::clearLayout(QLayout * layout) {
	if (! layout)
		return;

	while (auto item = layout->takeAt(0))
	{
		if (auto *widget = item->widget())
			widget->deleteLater();
		if (auto *spacer = item->spacerItem())
			delete spacer;

		clearLayout(item->layout());
		delete item;
	}
}
