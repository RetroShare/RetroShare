/***************************************************************************
 *   Copyright (C) 2007 by Balázs Béla                                     *
 *   balazsbela@gmail.com                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#include <QApplication>
#include <QSplashScreen>
#include <QTranslator>
#include "qorganizer.h"
#include <QSettings>
#include <QString>

//Config variable to know which language to use and other things needed to be done in main
QString C_LANGUAGE;
QString globalWDirPath;
QSize C_WINDOWSIZE;
bool C_MAXIMIZED;

//Read it from the registry, easyer than re-reading the config file

void getWDirPath()
{
 QSettings settings("qOrganizer","qOrganizer");
 globalWDirPath = settings.value("pathtowdir","home").toString();  
 if(globalWDirPath=="home") globalWDirPath=QDir::homePath()+QDir::separator()+".qOrganizer";
  else globalWDirPath=globalWDirPath+QDir::separator()+".qOrganizer";
}

void readSettings()
{
 getWDirPath();
 QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,globalWDirPath);
 QSettings *settings = new QSettings(QSettings::IniFormat,QSettings::UserScope,"qOrganizer", "qOrganizer");
 C_LANGUAGE = settings -> value("language").toString();
 C_WINDOWSIZE = settings -> value("windowsize").toSize();
 C_MAXIMIZED = settings -> value("windowmaximized", true).toBool();
}

int main(int argc, char *argv[])
{
      Q_INIT_RESOURCE(application);
      QApplication app(argc, argv);
      QTranslator *translator = new QTranslator;
      app.installTranslator(translator);  
      readSettings(); //Read the language
      translator->load(C_LANGUAGE,":/lang"); //set it
      QSplashScreen *splash = new QSplashScreen;
      splash -> setPixmap(QPixmap(":/images/splash.png"));
      splash -> show();
      qOrganizer mw; 
      if(!C_WINDOWSIZE.isEmpty())
       mw.resize(C_WINDOWSIZE);
      if(C_MAXIMIZED) 
       mw.showMaximized();
        else
         mw.show();
      //sleep(1);
      splash -> hide();
      splash -> deleteLater();
      return app.exec();
      
}

