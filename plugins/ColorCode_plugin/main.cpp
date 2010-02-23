/* ColorCode, a free MasterMind clone with built in solver
 * Copyright (C) 2009  Dirk Laebisch
 * http://www.laebisch.com/
 *
 * ColorCode is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ColorCode is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ColorCode. If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <QtGui/QApplication>
#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>
#include "colorcode.h"

int main(int argc, char* argv[])
{
    using namespace std;    

    string lang("");
    if (argc > 1)
    {
        string str;
        for (int i = 1; i < argc; ++i)
        {
            str = argv[i];
            if (str == "-h" || str == "--help")
            {
                cout << "usage: ColorCode [options]" << endl;
                cout << "  options:" << endl;
                cout << "  -l cc, --lang=cc    use country code cc instead of system locale, accepted values for cc: en|de|cs|fr|hu" << endl;
                cout << "  -h, --help          prints this message ;-)" << endl;
                return 0;
            }
            else if (str == "-l" && i < argc - 1)
            {
                if (std::string(argv[i + 1]) == "de" || std::string(argv[i + 1]) == "en" || std::string(argv[i + 1]) == "cs" || std::string(argv[i + 1]) == "fr" || std::string(argv[i + 1]) == "hu")
                {
                    std::string test(argv[i]);
                    lang = argv[i + 1];
                    cerr << "Lang: " << lang << endl;
                }
            }
            else if ( str.size() == 9
                      && str.find("--lang=") != string::npos
                      && (str.substr(7) == "en" || str.substr(7) == "de" || str.substr(7) == "cs" || str.substr(7) == "fr" || str.substr(7) == "hu") )
            {
                lang = str.substr(7);
                cerr << "Lang: " << lang << endl;
            }
        }
    }

    QApplication app(argc, argv);

    QTranslator qtTranslator;
    if (lang == "")
    {
        qtTranslator.load("qt_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    }
    else
    {
        qtTranslator.load("qt_" + QString::fromStdString(lang), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    }
    app.installTranslator(&qtTranslator);

    QTranslator appTranslator;
    if (lang == "")
    {
        appTranslator.load(":/trans_" + QLocale::system().name());
    }
    else
    {
        appTranslator.load(":/trans_" + QString::fromStdString(lang));
    }
    app.installTranslator(&appTranslator);

    ColorCode w;
    w.show();
    return app.exec();
}
