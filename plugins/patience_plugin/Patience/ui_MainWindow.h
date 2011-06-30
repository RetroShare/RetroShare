/********************************************************************************
** Form generated from reading ui file 'MainWindow.ui'
**
** Created: Mon 8. Feb 22:40:48 2010
**      by: Qt User Interface Compiler version 4.5.2
**
** WARNING! All changes made in this file will be lost when recompiling ui file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QGridLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QStatusBar>
#include <QtGui/QWidget>
#include "Viewer.h"

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *action_neues_spiel;
    QAction *action_beenden;
    QAction *action_about;
    QAction *action_about_qt;
    QAction *action_french_;
    QAction *action_german_;
    QAction *action_highscore;
    QAction *action_eine_ziehen_;
    QAction *action_drei_ziehen_;
    QAction *action_french;
    QAction *action_german;
    QAction *action_eine_ziehen;
    QAction *action_drei_ziehen;
    QAction *action_rahmen;
    QAction *action_undo;
    QAction *action_fragen;
    QAction *action_speichern;
    QAction *action_nicht_speichern;
    QWidget *centralwidget;
    QGridLayout *gridLayout;
    Viewer *viewer;
    QMenuBar *menubar;
    QMenu *menuSpiel;
    QMenu *menu_Hilfe;
    QMenu *menu_Einstellungen;
    QMenu *menu_Cardset;
    QMenu *menu_Gametype;
    QMenu *menuSave_Game;
    QMenu *menu_Highscore;
    QMenu *menu_Edit;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(810, 653);
        action_neues_spiel = new QAction(MainWindow);
        action_neues_spiel->setObjectName(QString::fromUtf8("action_neues_spiel"));
        action_beenden = new QAction(MainWindow);
        action_beenden->setObjectName(QString::fromUtf8("action_beenden"));
        action_about = new QAction(MainWindow);
        action_about->setObjectName(QString::fromUtf8("action_about"));
        action_about_qt = new QAction(MainWindow);
        action_about_qt->setObjectName(QString::fromUtf8("action_about_qt"));
        action_french_ = new QAction(MainWindow);
        action_french_->setObjectName(QString::fromUtf8("action_french_"));
        action_french_->setCheckable(true);
        action_german_ = new QAction(MainWindow);
        action_german_->setObjectName(QString::fromUtf8("action_german_"));
        action_german_->setCheckable(true);
        action_highscore = new QAction(MainWindow);
        action_highscore->setObjectName(QString::fromUtf8("action_highscore"));
        action_eine_ziehen_ = new QAction(MainWindow);
        action_eine_ziehen_->setObjectName(QString::fromUtf8("action_eine_ziehen_"));
        action_eine_ziehen_->setCheckable(true);
        action_drei_ziehen_ = new QAction(MainWindow);
        action_drei_ziehen_->setObjectName(QString::fromUtf8("action_drei_ziehen_"));
        action_drei_ziehen_->setCheckable(true);
        action_french = new QAction(MainWindow);
        action_french->setObjectName(QString::fromUtf8("action_french"));
        action_french->setCheckable(true);
        action_german = new QAction(MainWindow);
        action_german->setObjectName(QString::fromUtf8("action_german"));
        action_german->setCheckable(true);
        action_eine_ziehen = new QAction(MainWindow);
        action_eine_ziehen->setObjectName(QString::fromUtf8("action_eine_ziehen"));
        action_eine_ziehen->setCheckable(true);
        action_drei_ziehen = new QAction(MainWindow);
        action_drei_ziehen->setObjectName(QString::fromUtf8("action_drei_ziehen"));
        action_drei_ziehen->setCheckable(true);
        action_rahmen = new QAction(MainWindow);
        action_rahmen->setObjectName(QString::fromUtf8("action_rahmen"));
        action_rahmen->setCheckable(true);
        action_rahmen->setChecked(true);
        action_undo = new QAction(MainWindow);
        action_undo->setObjectName(QString::fromUtf8("action_undo"));
        action_fragen = new QAction(MainWindow);
        action_fragen->setObjectName(QString::fromUtf8("action_fragen"));
        action_fragen->setCheckable(true);
        action_fragen->setChecked(true);
        action_speichern = new QAction(MainWindow);
        action_speichern->setObjectName(QString::fromUtf8("action_speichern"));
        action_speichern->setCheckable(true);
        action_speichern->setChecked(false);
        action_nicht_speichern = new QAction(MainWindow);
        action_nicht_speichern->setObjectName(QString::fromUtf8("action_nicht_speichern"));
        action_nicht_speichern->setCheckable(true);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        gridLayout = new QGridLayout(centralwidget);
        gridLayout->setMargin(5);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        viewer = new Viewer(centralwidget);
        viewer->setObjectName(QString::fromUtf8("viewer"));
        viewer->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        viewer->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        viewer->setRenderHints(QPainter::Antialiasing|QPainter::HighQualityAntialiasing|QPainter::SmoothPixmapTransform|QPainter::TextAntialiasing);
        viewer->setCacheMode(QGraphicsView::CacheBackground);
        viewer->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

        gridLayout->addWidget(viewer, 0, 0, 1, 1);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 810, 22));
        menuSpiel = new QMenu(menubar);
        menuSpiel->setObjectName(QString::fromUtf8("menuSpiel"));
        menu_Hilfe = new QMenu(menubar);
        menu_Hilfe->setObjectName(QString::fromUtf8("menu_Hilfe"));
        menu_Einstellungen = new QMenu(menubar);
        menu_Einstellungen->setObjectName(QString::fromUtf8("menu_Einstellungen"));
        menu_Cardset = new QMenu(menu_Einstellungen);
        menu_Cardset->setObjectName(QString::fromUtf8("menu_Cardset"));
        menu_Gametype = new QMenu(menu_Einstellungen);
        menu_Gametype->setObjectName(QString::fromUtf8("menu_Gametype"));
        menuSave_Game = new QMenu(menu_Einstellungen);
        menuSave_Game->setObjectName(QString::fromUtf8("menuSave_Game"));
        menu_Highscore = new QMenu(menubar);
        menu_Highscore->setObjectName(QString::fromUtf8("menu_Highscore"));
        menu_Edit = new QMenu(menubar);
        menu_Edit->setObjectName(QString::fromUtf8("menu_Edit"));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        MainWindow->setStatusBar(statusbar);

        menubar->addAction(menuSpiel->menuAction());
        menubar->addAction(menu_Edit->menuAction());
        menubar->addAction(menu_Einstellungen->menuAction());
        menubar->addAction(menu_Highscore->menuAction());
        menubar->addAction(menu_Hilfe->menuAction());
        menuSpiel->addAction(action_neues_spiel);
        menuSpiel->addSeparator();
        menuSpiel->addAction(action_beenden);
        menu_Hilfe->addAction(action_about);
        menu_Hilfe->addAction(action_about_qt);
        menu_Einstellungen->addAction(menu_Cardset->menuAction());
        menu_Einstellungen->addAction(menu_Gametype->menuAction());
        menu_Einstellungen->addAction(menuSave_Game->menuAction());
        menu_Einstellungen->addSeparator();
        menu_Einstellungen->addAction(action_rahmen);
        menu_Einstellungen->addSeparator();
        menu_Cardset->addAction(action_french);
        menu_Cardset->addAction(action_german);
        menu_Gametype->addAction(action_eine_ziehen);
        menu_Gametype->addAction(action_drei_ziehen);
        menuSave_Game->addAction(action_speichern);
        menuSave_Game->addAction(action_nicht_speichern);
        menuSave_Game->addSeparator();
        menuSave_Game->addAction(action_fragen);
        menu_Highscore->addAction(action_highscore);
        menu_Edit->addAction(action_undo);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "Patience", 0, QApplication::UnicodeUTF8));
        action_neues_spiel->setText(QApplication::translate("MainWindow", "&New Game", 0, QApplication::UnicodeUTF8));
        action_neues_spiel->setShortcut(QApplication::translate("MainWindow", "Ctrl+N", 0, QApplication::UnicodeUTF8));
        action_beenden->setText(QApplication::translate("MainWindow", "Close", 0, QApplication::UnicodeUTF8));
        action_about->setText(QApplication::translate("MainWindow", "About Patience", 0, QApplication::UnicodeUTF8));
        action_about_qt->setText(QApplication::translate("MainWindow", "About QT", 0, QApplication::UnicodeUTF8));
        action_french_->setText(QApplication::translate("MainWindow", "French", 0, QApplication::UnicodeUTF8));
        action_german_->setText(QApplication::translate("MainWindow", "German", 0, QApplication::UnicodeUTF8));
        action_highscore->setText(QApplication::translate("MainWindow", "&Show", 0, QApplication::UnicodeUTF8));
        action_eine_ziehen_->setText(QApplication::translate("MainWindow", "Take One", 0, QApplication::UnicodeUTF8));
        action_drei_ziehen_->setText(QApplication::translate("MainWindow", "Take Three", 0, QApplication::UnicodeUTF8));
        action_french->setText(QApplication::translate("MainWindow", "French", 0, QApplication::UnicodeUTF8));
        action_german->setText(QApplication::translate("MainWindow", "German", 0, QApplication::UnicodeUTF8));
        action_eine_ziehen->setText(QApplication::translate("MainWindow", "Take One", 0, QApplication::UnicodeUTF8));
        action_drei_ziehen->setText(QApplication::translate("MainWindow", "Take Three", 0, QApplication::UnicodeUTF8));
        action_rahmen->setText(QApplication::translate("MainWindow", "Frame", 0, QApplication::UnicodeUTF8));
        action_undo->setText(QApplication::translate("MainWindow", "Undo", 0, QApplication::UnicodeUTF8));
        action_fragen->setText(QApplication::translate("MainWindow", "Ask", 0, QApplication::UnicodeUTF8));
        action_speichern->setText(QApplication::translate("MainWindow", "Save", 0, QApplication::UnicodeUTF8));
        action_nicht_speichern->setText(QApplication::translate("MainWindow", "Don't save", 0, QApplication::UnicodeUTF8));
        menuSpiel->setTitle(QApplication::translate("MainWindow", "Game", 0, QApplication::UnicodeUTF8));
        menu_Hilfe->setTitle(QApplication::translate("MainWindow", "&Help", 0, QApplication::UnicodeUTF8));
        menu_Einstellungen->setTitle(QApplication::translate("MainWindow", "&Settings", 0, QApplication::UnicodeUTF8));
        menu_Cardset->setTitle(QApplication::translate("MainWindow", "&Cardset", 0, QApplication::UnicodeUTF8));
        menu_Gametype->setTitle(QApplication::translate("MainWindow", "Gametype", 0, QApplication::UnicodeUTF8));
        menuSave_Game->setTitle(QApplication::translate("MainWindow", "Save Game", 0, QApplication::UnicodeUTF8));
        menu_Highscore->setTitle(QApplication::translate("MainWindow", "&Highscore", 0, QApplication::UnicodeUTF8));
        menu_Edit->setTitle(QApplication::translate("MainWindow", "&Edit", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
