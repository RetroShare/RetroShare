/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2006-2007, crypton
 * Copyright (c) 2006, Matt Edman, Justin Hipple
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

#include "rshare.h"
#include "GeneralDialog.h"
#include <QSystemTrayIcon>

/** Constructor */
GeneralDialog::GeneralDialog(QWidget *parent)
: ConfigPage(parent)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

 /* Create RshareSettings object */
  _settings = new RshareSettings();
  
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
  
  if (QSystemTrayIcon::isSystemTrayAvailable()){

    /* Check if we are supposed to show our main window on startup */
    ui.chkShowOnStartup->setChecked(_settings->showMainWindowAtStart());
    if (ui.chkShowOnStartup->isChecked())
      show();
  } else {
    /* Don't let people hide the main window, since that's all they have. */
    ui.chkShowOnStartup->hide();
    //show();
  }
    
}

/** Destructor */
GeneralDialog::~GeneralDialog()
{
  delete _settings;
}

/** Saves the changes on this page */
bool
GeneralDialog::save(QString &errmsg)
{
  Q_UNUSED(errmsg);
  QString languageCode =
    LanguageSupport::languageCode(ui.cmboLanguage->currentText());
  
  _settings->setLanguageCode(languageCode);
  _settings->setInterfaceStyle(ui.cmboStyle->currentText());
  _settings->setSheetName(ui.styleSheetCombo->currentText());
  
  _settings->setValue(QString::fromUtf8("StartMinimized"), startMinimized());

 
  /* Set to new style */
  Rshare::setStyle(ui.cmboStyle->currentText());
  return true;
}
  
/** Loads the settings for this page */
void
GeneralDialog::load()
{
  int index = ui.cmboLanguage->findData(_settings->getLanguageCode());
  ui.cmboLanguage->setCurrentIndex(index);
  
  index = ui.cmboStyle->findData(Rshare::style().toLower());
  ui.cmboStyle->setCurrentIndex(index);
  
  ui.checkStartMinimized->setChecked(_settings->value(QString::fromUtf8("StartMinimized"), false).toBool());

  
  ui.styleSheetCombo->setCurrentIndex(ui.styleSheetCombo->findText(_settings->getSheetName())); 
  
    /** load saved internal styleSheet **/
    //QFile file(":/qss/" + (_settings->getSheetName().toLower()) + ".qss");
    
    /** load saved extern Stylesheets **/
    QFile file(QApplication::applicationDirPath() + "/qss/" + (_settings->getSheetName().toLower()) + ".qss");
    
    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());
    qApp->setStyleSheet(styleSheet);

}

void GeneralDialog::on_styleSheetCombo_activated(const QString &sheetName)
{
    loadStyleSheet(sheetName);
}

void GeneralDialog::loadStyleSheet(const QString &sheetName)
{
     /** internal Stylesheets **/
    //QFile file(":/qss/" + sheetName.toLower() + ".qss");
    
    /** extern Stylesheets **/
    QFile file(QApplication::applicationDirPath() + "/qss/" + sheetName.toLower() + ".qss");
    
    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());

    
    qApp->setStyleSheet(styleSheet);
    
}

void GeneralDialog::loadqss()
{

 QFileInfoList slist = QDir(QApplication::applicationDirPath() + "/qss/").entryInfoList();
 foreach(QFileInfo st, slist)
 {
  if(st.fileName() != "." && st.fileName() != ".." && st.isFile())
  ui.styleSheetCombo->addItem(st.fileName().remove(".qss"));
 }
 
}

bool GeneralDialog::startMinimized() const {
  if(ui.checkStartMinimized->isChecked()) return true;
  return ui.checkStartMinimized->isChecked();
}
