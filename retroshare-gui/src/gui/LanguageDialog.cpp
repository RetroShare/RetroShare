/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2006-2008, crypton
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
#include "LanguageDialog.h"


/** Constructor */
LanguageDialog::LanguageDialog(QWidget *parent)
: QWidget(parent)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

 /* Create RshareSettings object */
  _settings = new RshareSettings();
  

  /* Populate combo boxes */
  foreach (QString code, LanguageSupport::languageCodes()) {
    ui.cmboxLanguage->addItem(QIcon(":/images/flags/" + code + ".png"),
                             LanguageSupport::languageName(code),
                             code);
  }
  
   connect(ui.ok_Button, SIGNAL(clicked( bool )), this, SLOT( save(QString &errmsg)) );
   connect(ui.cancel_Button, SIGNAL(clicked( bool )), this, SLOT( cancellanguage()) );
 

}

/** Destructor */
LanguageDialog::~LanguageDialog()
{
  delete _settings;
}

/** Saves the changes on this page */
bool
LanguageDialog::save(QString &errmsg)
{
  Q_UNUSED(errmsg);
  QString languageCode =
    LanguageSupport::languageCode(ui.cmboxLanguage->currentText());
  
  _settings->setLanguageCode(languageCode);

 
}
  
/** Loads the settings for this page */
void
LanguageDialog::load()
{
  int index = ui.cmboxLanguage->findData(_settings->getLanguageCode());
  ui.cmboxLanguage->setCurrentIndex(index);
    
  
}

/** Cancel and close the Language Window. */
void
LanguageDialog::cancellanguage()
{

  QWidget::close();
}

