/***************************************************************************
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

#ifndef _VIEW_H_
#define _VIEW_H_


#include <QFrame>
#include <QTextEdit>
#include <QLineEdit>
#include <QLabel>


#include "board.h"


class Pdn;
class myPlayer;
class myHistory;


class myView : public QFrame
{
	Q_OBJECT

public:
	myView(QWidget* parent);
	~myView();

	void newGame(int rules, bool free_place,
			const QString& name, bool is_white,
			int opponent, const QString& opp_name, int skill);
	bool openPdn(const QString& fn);
	bool savePdn(const QString& fn);

	void setTheme(const QString& theme_path);

	bool isAborted() const { return m_aborted; }

	void setNotation(bool enabled, bool show_above);
	void setNotationFont(const QFont& f);
	QFont notationFont() const { return m_board->font(); }

public slots:
	virtual void setEnabled(bool);

	void slotClearLog(bool b) { m_clear_log = b; }

	void slotStopGame();
	void slotNextRound();

signals:
	void working(bool);

private slots:
	void slot_click(int);

	void slot_move_done(const QString& board_str);
	void slot_move_done_step_two();

	void slot_preview_game(int game_type);
	void slot_apply_moves(const QString& moves);
	void slot_new_mode(bool paused, bool freeplace);

private:
	void begin_game(unsigned int round, bool freeplacement);

	void perform_jumps(const QString& from_board, const QString& to_board);
	bool extract_move(const QString& move, int* from_num, int* to_num);

	void stop_game(const QString&);
	void you_won(bool really);
	bool check_game_over();

	enum LogType {
		None,
		Error,
		Warning,
		System,
		User,
		Opponent,
	};
	void add_log(enum LogType type, const QString& text);

	myPlayer* get_first_player() const;

private:
	bool m_clear_log;

	bool m_game_over;// need this to avoid multiple calls to isGameOver()
	bool m_aborted;

	myPlayer* m_player;
	myPlayer* m_current;

	myBoard* m_board;
	myHistory* m_history;
	QTextEdit* m_log;

	int m_freeplace_from;
};


#endif

