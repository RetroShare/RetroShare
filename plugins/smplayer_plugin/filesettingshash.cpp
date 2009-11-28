/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2008 Ricardo Villalba <rvm@escomposlinux.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "filesettingshash.h"
#include "mediasettings.h"
#include "osparser.h" // hash function
#include <QSettings>
#include <QFile>
#include <QDir>

FileSettingsHash::FileSettingsHash(QString directory) : FileSettingsBase(directory) 
{
	base_dir = directory + "/file_settings";
}

FileSettingsHash::~FileSettingsHash() {
}


QString FileSettingsHash::configFile(const QString & filename, QString * output_dir) {
	QString res;

	QString hash = OSParser::calculateHash(filename);
	if (!hash.isEmpty()) {
		if (output_dir != 0) (*output_dir) = hash[0];
		res = base_dir +"/"+ hash[0] +"/"+ hash + ".ini";
	}
	return res;
}

bool FileSettingsHash::existSettingsFor(QString filename) {
	qDebug("FileSettingsHash::existSettingsFor: '%s'", filename.toUtf8().constData());

	QString config_file = configFile(filename);

	qDebug("FileSettingsHash::existSettingsFor: config_file: '%s'", config_file.toUtf8().constData());

	return QFile::exists(config_file);
}

void FileSettingsHash::loadSettingsFor(QString filename, MediaSettings & mset) {
	qDebug("FileSettings::loadSettingsFor: '%s'", filename.toUtf8().constData());

	QString config_file = configFile(filename);

	qDebug("FileSettingsHash::loadSettingsFor: config_file: '%s'", config_file.toUtf8().constData());

	mset.reset();

	if ((!config_file.isEmpty()) && (QFile::exists(config_file))) {
		QSettings settings(config_file, QSettings::IniFormat);

		settings.beginGroup("file_settings");
		mset.load(&settings);
		settings.endGroup();
	}
}

void FileSettingsHash::saveSettingsFor(QString filename, MediaSettings & mset) {
	qDebug("FileSettingsHash::saveSettingsFor: '%s'", filename.toUtf8().constData());

	QString output_dir;
	QString config_file = configFile(filename, &output_dir);

	qDebug("FileSettingsHash::saveSettingsFor: config_file: '%s'", config_file.toUtf8().constData());
	qDebug("FileSettingsHash::saveSettingsFor: output_dir: '%s'", output_dir.toUtf8().constData());

	if (!config_file.isEmpty()) {
		QDir d(base_dir);
		if (!d.exists(output_dir)) {
			if (!d.mkpath(output_dir)) {
				qWarning("FileSettingsHash::saveSettingsFor: can't create directory '%s'", QString(base_dir + "/" + output_dir).toUtf8().constData());
				return;
			}
		}

		QSettings settings(config_file, QSettings::IniFormat);

		/* settings.setValue("filename", filename); */

		settings.beginGroup("file_settings");
		mset.save(&settings);
		settings.endGroup();
		settings.sync();
	}
}

