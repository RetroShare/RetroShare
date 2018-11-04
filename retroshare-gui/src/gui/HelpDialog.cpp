/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006, crypton
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#include "HelpDialog.h"
#include "ui_HelpDialog.h"

#include <retroshare/rsiface.h>
#include <retroshare/rsplugin.h>
#include "rshare.h"

#ifdef ENABLE_WEBUI
#include <microhttpd.h>
#endif // ENABLE_WEBUI

#include <QFile>
#include <QTextStream>

static void addLibraries(QGridLayout *layout, const std::string &name, const std::list<RsLibraryInfo> &libraries)
{
	int row = layout->rowCount();

	QLabel *label = new QLabel(QString::fromUtf8(name.c_str()));
	label->setTextInteractionFlags(label->textInteractionFlags() | Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
	layout->addWidget(label, row++, 0, 1, 3);

	QFont font = label->font();
	font.setBold(true);
	label->setFont(font);

	std::list<RsLibraryInfo>::const_iterator libraryIt;
	for (libraryIt = libraries.begin(); libraryIt != libraries.end(); ++libraryIt) {
		label = new QLabel(QString::fromUtf8(libraryIt->mName.c_str()));
		label->setTextInteractionFlags(label->textInteractionFlags() | Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
		layout->addWidget(label, row, 0);
		label->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

		label = new QLabel(QString::fromUtf8(libraryIt->mVersion.c_str()));
		label->setTextInteractionFlags(label->textInteractionFlags() | Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
		layout->addWidget(label, row++, 1);
	}
}

/** Constructor */
HelpDialog::HelpDialog(QWidget *parent) :
    QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint),
    ui(new(Ui::HelpDialog))
{
	/* Invoke the Qt Designer generated object setup routine */
	ui->setupUi(this);

	//QFile licenseFile(QLatin1String(":/images/COPYING"));
	QFile licenseFile(QLatin1String(":/help/licence.html"));
	if (licenseFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QTextStream in(&licenseFile);
		ui->license->setText(in.readAll());
	}

	QFile authorsFile(QLatin1String(":/help/authors.html"));
	if (authorsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QTextStream in(&authorsFile);
		ui->authors->setText(in.readAll());
	}

	QFile thanksFile(QLatin1String(":/help/thanks.html"));
	if (thanksFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QTextStream in(&thanksFile);
		ui->thanks->setText(in.readAll());
	}

	ui->version->setText(Rshare::retroshareVersion(true));

	/* Add version numbers of libretroshare */
	std::list<RsLibraryInfo> libraries;
	RsControl::instance()->getLibraries(libraries);
	addLibraries(ui->libraryLayout, "libretroshare", libraries);

#ifdef ENABLE_WEBUI
	/* Add version numbers of RetroShare */
	// Add versions here. Find a better place.
	libraries.clear();
	libraries.push_back(RsLibraryInfo("Libmicrohttpd", MHD_get_version()));
	addLibraries(ui->libraryLayout, "RetroShare", libraries);
#endif // ENABLE_WEBUI

	/* Add version numbers of plugins */
	if (rsPlugins) {
		for (int i = 0; i < rsPlugins->nbPlugins(); ++i) {
			RsPlugin *plugin = rsPlugins->plugin(i);
			if (plugin) {
				libraries.clear();
				plugin->getLibraries(libraries);
				addLibraries(ui->libraryLayout, plugin->getPluginName(), libraries);
			}
		}
	}
}

HelpDialog::~HelpDialog()
{
	delete(ui);
}
