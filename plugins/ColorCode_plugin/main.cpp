#include <iostream>
#include <QtGui/QApplication>
#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>
#include "mainwindow.h"

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
                cout << "  -l cc, --lang=cc    use country code cc instead of system locale, accepted values for cc: en|de" << endl;
                cout << "  -h, --help          prints this message ;-)" << endl;
                return 0;
            }
            else if (str == "-l" && i < argc - 1)
            {
                if (std::string(argv[i + 1]) == "de" || std::string(argv[i + 1]) == "en")
                {
                    std::string test(argv[i]);
                    lang = argv[i + 1];
                }
            }
            else if ( str.size() == 9
                      && str.find("--lang=") != string::npos
                      && (str.substr(7) == "en" || str.substr(7) == "de") )
            {
                lang = str.substr(7);
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
        appTranslator.load("trans_" + QLocale::system().name());
    }
    else
    {
        appTranslator.load(":/trans_" + QString::fromStdString(lang));
    }
    app.installTranslator(&appTranslator);

    MainWindow w;
    w.show();
    return app.exec();
}
