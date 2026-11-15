/*******************************************************************************
 * retroshare-profile/src/main.cpp                                             *
 *                                                                             *
 * Copyright (c) 2026 RetroShare Team <retroshare.project@gmail.com>           *
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

#include "retroshare/rsinit.h"
#include "retroshare/rsiface.h"

#include "ProfileToolWindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>

/**
 * RetroShare decides "portable" by looking for a file named "portable" in the
 * current directory, and then uses the relative directory "Data". This tool can
 * be started from anywhere, so the data directory is resolved to an absolute
 * path here:
 *  - a RetroShare folder containing "portable" -> <folder>/Data
 *  - anything else                               -> used as it is
 */
static QString resolveDataDirectory(const QString& path)
{
	QDir dir(QDir::cleanPath(QDir(path).absolutePath()));

	if(QFileInfo::exists(dir.filePath("portable")))
		return dir.filePath("Data");

	return dir.absolutePath();
}

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	QApplication::setApplicationName("RetroShare Profile");
	QApplication::setOrganizationName("RetroShare");

	QCommandLineParser parser;
	parser.setApplicationDescription(QApplication::translate("main",
	        "Back up, restore and upgrade RetroShare nodes. Close RetroShare before using it."));
	parser.addHelpOption();
	QCommandLineOption baseDirOption(QStringList() << "c" << "base-dir",
	        QApplication::translate("main", "Use the RetroShare data directory <directory>. For a portable RetroShare, give its folder (the one with the \"portable\" file) or its Data folder."),
	        QApplication::translate("main", "directory"));
	parser.addOption(baseDirOption);
	parser.process(app);

	RsInit::InitRsConfig();
	RsControl::earlyInitNotificationSystem();

	RsConfigOptions conf;
	conf.main_executable_path = argv[0];

	if(parser.isSet(baseDirOption))
		conf.optBaseDir = resolveDataDirectory(parser.value(baseDirOption)).toUtf8().constData();
	else
	{
		// Portable RetroShare: the tool was put next to retroshare.exe, or was
		// started from inside the portable folder.
		const QStringList candidates = QStringList() << QApplication::applicationDirPath() << QDir::currentPath();

		for(const QString& c : candidates)
			if(QFileInfo::exists(QDir(c).filePath("portable")))
			{
				conf.optBaseDir = resolveDataDirectory(c).toUtf8().constData();
				break;
			}
	}

	// Only sets up the base directory, the PGP keyring and the list of nodes:
	// no node is logged in and no service is started.
	int initResult = RsInit::InitRetroShare(conf);

	if(initResult != RS_INIT_OK && initResult != RS_INIT_HAVE_ACCOUNT)
	{
		QMessageBox::critical(nullptr, QApplication::applicationName(),
		        QApplication::translate("main", "Cannot initialise RetroShare (error %1).\n"
		                                        "Check that the data directory is correct.").arg(initResult));
		return 1;
	}

	ProfileToolWindow window;
	window.show();

	return app.exec();
}
