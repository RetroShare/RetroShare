/***************************************************************************
 *   Copyright (C) 2002-2003 Andi Peredri                                  *
 *   andi@ukr.net                                                          *
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

#ifndef PDN_H
#define PDN_H


#include <QList>
#include <QWidget>


#define ENOTATION "1 2 3 4 5 6 7 8 9 1011121314151617181920212223242526272829303132"
//#define ENOTATION "32313029282726252423222120191817161514131211109 8 7 6 5 4 3 2 1 "
#define RNOTATION "b8d8f8h8a7c7e7g7b6d6f6h6a5c5e5g5b4d4f4h4a3c3e3g3b2d2f2h2a1c1e1g1"

#define ENGLISH   21
#define RUSSIAN   25


class PdnMove;
class PdnGame;


// Portable Draughts Notation Format File parser
class Pdn
{
public:
    Pdn();
    virtual ~Pdn();

    int count() const { return m_database.count(); }
    PdnGame* game(int i) { return m_database.at(i); }
    PdnGame* newGame();

    void clear() { m_database.clear(); }
    // parent is needed to display a progress dialog.
    bool open(const QString& filename, QWidget* parent,
	    const QString& label, QString& text_to_log);
    bool save(const QString& filename);

private:
    QList<PdnGame*> m_database;
};


/*
 * m_first, m_second are mandatary.
 * comments are optional.
 */
class PdnMove
{
public:
    PdnMove(QString whole_line_of_move);

    QString m_first, m_comfirst;
    QString m_second, m_comsecond;
};


/*
 * represents each game in a pdn-file.
 */
class PdnGame
{
public:
    PdnGame(const QString& game_string, QString& text_to_log);
    ~PdnGame();

    enum Tag { Date, Site, Type, Event, Round, White, Black,Result };

    int item(int i) const { return board[i]; }
    void setItem(int i, int set) { board[i]=set; }

    bool isWhite() const { return white; }
    void setWhite(bool set) { white=set; }

    void set(Tag tag, const QString& string);
    QString get(Tag tag) const;

    int movesCount() const { return m_moves.count(); }
    PdnMove* getMove(int i);
    PdnMove* addMove();
    void clearMoves() { m_moves.clear(); }

    QString toString();

private:
    bool parse(const QString& game_string, QString& log_txt);
    bool parse(const QString& string, bool side);
    bool parse_moves(const QString& moves_line);

private:
    bool white;
    int board[32];
    QString pdnDate;
    QString pdnSite;
    QString pdnType;
    QString pdnEvent;
    QString pdnRound;
    QString pdnWhite;
    QString pdnBlack;
    QString pdnResult;
    QList<PdnMove*> m_moves;
};


#endif

