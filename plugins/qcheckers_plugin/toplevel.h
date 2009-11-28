/***************************************************************************
 *   Copyright (C) 2002-2003 Andi Peredri                                  *
 *   andi@ukr.net                                                          *
 *   Copyright (C) 2004-2005 Artur Wiebe                                   *
 *   wibix@gmx.de                                                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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
#ifndef _TOPLEVEL_H_
#define _TOPLEVEL_H_

#include <QMainWindow>
#include <QAction>
#include <QMap>


class myView;
class myNewGameDlg;


class myTopLevel : public QMainWindow
{
    Q_OBJECT

public:
    myTopLevel();

    void open(const QString& filename);

protected:
    void closeEvent(QCloseEvent*);

private slots:
    void slot_help();
    void slot_about();

    void slot_new_game();
    void slot_open_game();
    void slot_save_game();
    void slot_next_round();

    void slot_notation(bool);
    void slot_notation_font();

    void slot_working(bool);

    void set_theme(QAction*);

    void warning(const QString& text);


private:
    void make_actions();
    void make_central_widget();
    void restore_settings();
    void store_settings();

    // add themes to this menu.
    void read_themes(QActionGroup*, QMenu*, const QString& path);

    void information(const QString& caption, const QString& text);

    // returns true if the user wishes to keep current game
    bool keep_game();

private:
    QMenu* viewMenu;
    //
    QAction* gameNew;
    QAction* gameStop;
    QAction* gameOpen;
    QAction* gameSave;
    QAction* gameNextRound;
    //
    QAction* viewNotation;
    QAction* viewNotationAbove;
    //
    QAction* settingsKeep;
    QAction* settingsClearLog;

    QString filename;      // PDN File Name

    myView* m_view;
    myNewGameDlg* m_newgame;

    typedef QMap<QAction*, QString> myThemeMap;
    myThemeMap m_themes;
};

#endif

