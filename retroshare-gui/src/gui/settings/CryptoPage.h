/*******************************************************************************
 * gui/settings/CryptoPage.h                                                   *
 *                                                                             *
 * Copyright 2006, Retroshare Team <retroshare.project@gmail.com>              *
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

#ifndef _CRYPTOPAGE_H
#define _CRYPTOPAGE_H

#include "retroshare-gui/configpage.h"
#include "ui_CryptoPage.h"
#include "gui/common/FilesDefs.h"

class NewFriendList;

class CryptoPage : public ConfigPage
{
  Q_OBJECT

  public:
      /** Default Constructor */
      CryptoPage(QWidget * parent = 0, Qt::WindowFlags flags = 0);
      /** Default Destructor */
      ~CryptoPage();

      /** Loads the settings for this page */

        virtual QPixmap iconPixmap() const { return FilesDefs::getPixmapFromQtResourcePath(":/icons/settings/profile.svg") ; }
		virtual QString pageName() const { return tr("Node") ; }
		virtual QString helpText() const { return ""; }
	
	NewFriendList *friendslist;

  private slots:
      void exportProfile();
      virtual void load();
      void copyPublicKey();
	  void copyRSLink() ;
	  virtual void showEvent ( QShowEvent * event );
//	  void profilemanager();
      bool fileSave();
      bool fileSaveAs();
      void showStats();

  private:
      QString fileName;

      /** Qt Designer generated object */
      Ui::CryptoPage ui;
};

#endif

