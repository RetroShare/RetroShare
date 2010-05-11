/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006 - 2009 RetroShare Team
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


#include <rshare.h>
#include "AppearancePage.h"
#include "rsharesettings.h"


/** Constructor */
AppearancePage::AppearancePage(QWidget * parent, Qt::WFlags flags)
    : ConfigPage(parent, flags)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

  connect(ui.styleSheetCombo, SIGNAL(clicked()), this, SLOT(loadStyleSheet()));

  /* Populate combo boxes */
  foreach (QString code, LanguageSupport::languageCodes()) {
    ui.cmboLanguage->addItem(QIcon(":/images/flags/" + code + ".png"),
                             LanguageSupport::languageName(code),
                             code);
  }
  foreach (QString style, QStyleFactory::keys()) {
    ui.cmboStyle->addItem(style, style.toLower());
  }

  ui.styleSheetCombo->setCurrentIndex(ui.styleSheetCombo->findText("Default"));
  //loadStyleSheet("Default");
  loadqss();

  //load();


  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

AppearancePage::~AppearancePage()
{
}

/** Saves the changes on this page */
bool
AppearancePage::save(QString &errmsg)
{
	Q_UNUSED(errmsg);
	QString languageCode =
    LanguageSupport::languageCode(ui.cmboLanguage->currentText());

        RshareSettings settings;
        settings.setLanguageCode(languageCode);
        settings.setInterfaceStyle(ui.cmboStyle->currentText());
        settings.setSheetName(ui.styleSheetCombo->currentText());

    /* Set to new style */
	Rshare::setStyle(ui.cmboStyle->currentText());
	return true;
}



/** Loads the settings for this page */
void
AppearancePage::load()
{
    RshareSettings settings;

        int index = ui.cmboLanguage->findData(settings.getLanguageCode());
	ui.cmboLanguage->setCurrentIndex(index);

	index = ui.cmboStyle->findData(Rshare::style().toLower());
	ui.cmboStyle->setCurrentIndex(index);

    ui.styleSheetCombo->setCurrentIndex(ui.styleSheetCombo->findText(settings.getSheetName()));

    /** load saved internal styleSheet **/
    //QFile file(":/qss/" + (settings.getSheetName().toLower()) + ".qss");

    /** load saved extern Stylesheets **/
    QFile file(QApplication::applicationDirPath() + "/qss/" + (settings.getSheetName().toLower()) + ".qss");

    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());
    qApp->setStyleSheet(styleSheet);

}

void AppearancePage::on_styleSheetCombo_activated(const QString &sheetName)
{
    loadStyleSheet(sheetName);
}

void AppearancePage::loadStyleSheet(const QString &sheetName)
{
     /** internal Stylesheets **/
    //QFile file(":/qss/" + sheetName.toLower() + ".qss");

    /** extern Stylesheets **/
    QFile file(QApplication::applicationDirPath() + "/qss/" + sheetName.toLower() + ".qss");

    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());


    qApp->setStyleSheet(styleSheet);

}

void AppearancePage::loadqss()
{

	QFileInfoList slist = QDir(QApplication::applicationDirPath() + "/qss/").entryInfoList();
	foreach(QFileInfo st, slist)
	{
	if(st.fileName() != "." && st.fileName() != ".." && st.isFile())
	ui.styleSheetCombo->addItem(st.fileName().remove(".qss"));
	}

}
