/*

Name: Patience
Autor: Andreas Konarski
Begonnen: 05.08.2009.
Erstellt: 08.02.2010.
Version: 0.81
Lizenz: GPL v3 or later
Plattformen: Alle Systeme, auf denen QT 4.5 verfuegbar ist.

Kontakt: programmieren@konarski-wuppertal.de
home: www.konarski-wuppertal.de

Falls ich mit diesem Programm die Rechte von irgend jemand verletzen sollte, bitte ich um einen Hinweis. Wenn dies Tatsaechlich der Fall ist, entferne ich es schnellstmoeglich von meiner Homepage.


Zu den Kartenbildern:

This PySol cardset was adapted from the game XSkat 4.0

Copyright (C) 2004 Gunter Gerhardt (http://www.xskat.de)

This cardset is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2 of
the License, or (at your option) any later version.

*/

#include <QApplication>
#include <QTranslator>
#include <QLibraryInfo>
#include <QLocale>
#include "MainWindow.h"

using namespace std;

int main(int argc, char *argv[])
{
QApplication anwendung(argc, argv);

// die anwendung uebersetzen
QTranslator uebersetzer, uebersetzer2;

QString dateiname_eigene_uebersetzung;
dateiname_eigene_uebersetzung = QString(":/translations/Uebersetzung_%1").arg(QLocale::system().name());

// die uebersetzungs dateien in die uebersetzer laden
uebersetzer.load("qt_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
uebersetzer2.load(dateiname_eigene_uebersetzung);

// die uebersetzer installieren
anwendung.installTranslator(&uebersetzer);
anwendung.installTranslator(&uebersetzer2);

// den anwendungsnamen setzen
anwendung.setApplicationName("Patience");

// das hauptfenster erzeugen ...
MainWindow hauptfenster;

// ... und anzeigen
hauptfenster.show();

return anwendung.exec();
}
