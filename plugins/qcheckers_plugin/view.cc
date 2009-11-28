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
#include <QLayout>
#include <QDate>
#include <QDebug>
#include <QTimer>

#include "pdn.h"
#include "echeckers.h"
#include "rcheckers.h"
#include "view.h"
#include "common.h"
#include "history.h"
#include "newgamedlg.h"

#include "player.h"
#include "humanplayer.h"
#include "computerplayer.h"


#define MAX_CMD_LEN		80
#define MOVE_PAUSE		1000

// this class is used to note differencies between moves.
class myDiff {
public:
	myDiff(int pos, int from, int to)
		: m_pos(pos), m_from(from), m_to(to) {}
	int m_pos;
	int m_from;
	int m_to;
};


myView::myView(QWidget* parent)
	: QFrame(parent)
{
	/*
	 * board & info 
	 */
	m_board = new myBoard(this);
	connect(m_board, SIGNAL(fieldClicked(int)),
			this, SLOT(slot_click(int)));

	m_history = new myHistory(this);
	connect(m_history, SIGNAL(previewGame(int)),
			this, SLOT(slot_preview_game(int)));
	connect(m_history, SIGNAL(applyMoves(const QString&)),
			this, SLOT(slot_apply_moves(const QString&)));
	connect(m_history, SIGNAL(newMode(bool, bool)),
			this, SLOT(slot_new_mode(bool, bool)));
	connect(this, SIGNAL(working(bool)),
			m_history, SLOT(slotWorking(bool)));

	QHBoxLayout* hb = new QHBoxLayout(0);
	hb->addWidget(m_board);
	hb->addSpacing(5);
	hb->addWidget(m_history);


	/*
	 *
	 */
	m_log = new QTextEdit(this);
    	m_log->setFixedHeight(100); //FIXME
	m_log->setReadOnly(true);


	/*
	 * it's the final layout.
	 */
	QVBoxLayout* vb = new QVBoxLayout(this);
	vb->addLayout(hb);
	vb->addWidget(m_log);
	vb->setSizeConstraint(QLayout::SetFixedSize);


	/*
	 * game init
	 */
	m_player = m_current = 0;
}


myView::~myView()
{
	if(m_player) {
		delete m_player;
		delete m_player->opponent();
	}
}


void myView::setEnabled(bool b)
{
	m_board->setEnabled(b);
	if(b)
		setCursor(Qt::ArrowCursor);	// should be m_board bound.
	else
		setCursor(Qt::WaitCursor);
}


void myView::setTheme(const QString& path)
{
	m_board->setTheme(path, m_player ? m_player->isWhite() : true);
	m_history->setFixedHeight(m_board->height());
}


void myView::newGame(int rules, bool freeplace,
		const QString& name, bool is_white,
		int opponent, const QString& opp_name, int skill)
{
	m_freeplace_from = -1;

	if(m_player) {
		delete m_player;
		delete m_player->opponent();
	}

	m_board->setColorWhite(is_white);

	// create players
	myPlayer* plr = new myHumanPlayer(name, is_white, false);
	myPlayer* opp = 0;
	if(opponent==HUMAN)
		opp = new myHumanPlayer(opp_name, !is_white, true);
	else
		opp = new myComputerPlayer(opp_name, !is_white, skill);

	emit working(true);


	/*
	 * set up player stuff. slots/signals.
	 */
	m_player = plr;

	plr->setOpponent(opp);
	opp->setOpponent(plr);

	plr->disconnect();
	opp->disconnect();

	connect(plr, SIGNAL(moveDone(const QString&)),
			this, SLOT(slot_move_done(const QString&)));

	connect(opp, SIGNAL(moveDone(const QString&)),
			this, SLOT(slot_move_done(const QString&)));


	/*
	 * create game board.
	 */
	m_board->setGame(rules);

	m_board->reset();
	m_history->clear();

	begin_game(1, freeplace);
}


void myView::begin_game(unsigned int round, bool freeplace)
{
	if(m_clear_log)
		m_log->clear();

	m_board->adjustNotation(m_player->isWhite());

	m_history->newPdn(APPNAME" Game", freeplace);
	m_history->setTag(PdnGame::Type, QString::number(m_board->type()));
	m_history->setTag(PdnGame::Date,
		QDate::currentDate().toString("yyyy.MM.dd"));
	m_history->setTag(PdnGame::Result, "*");
	m_history->setTag(PdnGame::Round, QString::number(round));


	/*
	 * go!
	 */
	myPlayer* last_player = get_first_player()->opponent();

	m_game_over = false;
	m_aborted = false;
	m_current = last_player;

	// setup names
	if(m_player->isWhite()) {
		m_history->setTag(PdnGame::White, m_player->name());
		m_history->setTag(PdnGame::Black, m_player->opponent()->name());
	} else {
		m_history->setTag(PdnGame::White, m_player->opponent()->name());
		m_history->setTag(PdnGame::Black, m_player->name());
	}

	if(m_history->isFreePlacement())
		emit working(false);
	else
		slot_move_done(m_board->game()->toString(false));
}


bool myView::check_game_over()
{
	if(m_game_over)		// no further checks
		return true;

	m_game_over = true;

	bool player_can = m_board->game()->checkMove1()
		|| m_board->game()->checkCapture1();
	bool opp_can = m_board->game()->checkMove2()
		|| m_board->game()->checkCapture2();

	// player cannot go but opponent can -> player lost.
	if(/*FIXME*/m_player==m_current && !player_can && opp_can) {
		you_won(false);
		return m_game_over;
	}
	// player can go but opponent cannot -> player won.
	if(/*FIXME*/m_player!=m_current && player_can && !opp_can) {
		you_won(true);
		return m_game_over;
	}
	// neither of the player can go -> draw.
	if(!player_can && !opp_can) {
		add_log(myView::System, tr("Drawn game."));
		m_history->setTag(PdnGame::Result, "1/2-1/2");
		return m_game_over;
	}

	m_game_over = false;
	return m_game_over;
}


void myView::slot_click(int field_num)
{
	if(m_game_over || m_aborted)
		return;

	if(m_history->isPaused()) {
		if(m_history->isFreePlacement()) {
			// FIXME - hightlight fields
			if(m_freeplace_from < 0) {
				m_freeplace_from = field_num;
				m_board->selectField(field_num, true);
			} else {
				m_board->selectField(m_freeplace_from, false);
				m_board->doFreeMove(m_freeplace_from,
						field_num);
				m_freeplace_from = -1;
			}
		}
	} else {
		bool select = false;
		QString err_msg;

		if(!m_current->fieldClicked(field_num, &select, err_msg)) {
			add_log(myView::Warning, m_current->name()+": "
					+ (err_msg.length()
						? err_msg
						: tr("Invalid move.")));
		} else {
			m_board->selectField(field_num, select);
		}
	}
}


void myView::slotNextRound()
{
	if(m_aborted)
		return;

	m_player->setWhite(!m_player->isWhite());
	m_player->opponent()->setWhite(!m_player->isWhite());

	m_board->setColorWhite(m_player->isWhite());
	m_board->reset();

	unsigned int round = m_history->getTag(PdnGame::Round).toUInt() + 1;
	begin_game(round, m_history->isFreePlacement());
}


void myView::slotStopGame()
{
	m_player->stop();
	m_player->opponent()->stop();
}


void myView::stop_game(const QString& msg)
{
	m_game_over = true;
	m_aborted = true;

	QString text(tr("Game aborted.")+(!msg.isEmpty() ? "\n"+msg : ""));
	add_log(myView::System, text);

	emit working(false);
}


void myView::slot_move_done(const QString& board_str)
{
	if(m_history->isPaused())	// FIXME - ???
		return;

	perform_jumps(m_board->game()->toString(false), board_str);

	// show who is next?
	m_current = m_current->opponent();
	m_history->setCurrent(m_current->name());

	if(!m_current->isHuman()) {
		emit working(true);
	} else {
		emit working(false);
	}

	if(m_current->opponent()->isHuman() && !m_current->isHuman())
		QTimer::singleShot(MOVE_PAUSE, this,
				SLOT(slot_move_done_step_two()));
	else
		slot_move_done_step_two();
}

void myView::slot_move_done_step_two()
{
	//
	m_current->yourTurn(m_board->game());

	if(check_game_over())
		emit working(false);
}


void myView::you_won(bool yes)
{
	if(yes&&m_player->isWhite() || !yes&&!m_player->isWhite()) {
		m_history->setTag(PdnGame::Result, "1-0");	// white wins
		add_log(myView::System, tr("White wins!"));
	} else {
		m_history->setTag(PdnGame::Result, "0-1");	// black wins
		add_log(myView::System, tr("Black wins!"));
	}

	emit working(false);
}


bool myView::openPdn(const QString& fn)
{
	emit working(false);

	m_current->stop();

	QString log_text;
	if(!m_history->openPdn(fn, log_text)) {
		return false;
	}

	if(log_text.length()) {
		add_log(myView::System, tr("Opened:")+" "+fn);
		add_log(myView::Error, log_text.trimmed());
		add_log(myView::Warning, tr("Warning! Some errors occured."));
	}

	return true;
}


bool myView::savePdn(const QString& fn)
{
	if(!m_history->savePdn(fn)) {
		qDebug() << __PRETTY_FUNCTION__ << "failed.";
		return false;
	}
	add_log(myView::System, tr("Saved:")+" "+fn);
	return true;
}


void myView::slot_new_mode(bool paused, bool freeplace)
{
	if(paused) {
		if(freeplace)
			m_board->setCursor(Qt::PointingHandCursor);
		else
			m_board->setCursor(Qt::ForbiddenCursor);
	} else {
		m_board->setCursor(Qt::ArrowCursor);
	}

	// resume game: ask info for who is next, black or white.XXX FIXME TODO
	if(!paused) {
		myPlayer* next = 0;
		if(m_history->moveCount()%2==0)
			next = get_first_player();
		else
			next = get_first_player()->opponent();

		m_current = next->opponent();
		slot_move_done(m_board->game()->toString(false));
	}
}


void myView::slot_preview_game(int rules)
{
	if(rules!=RUSSIAN && rules!=ENGLISH) {
		qDebug() << __PRETTY_FUNCTION__ << rules << "Wrong game type.";
		return;
	}

	m_board->setGame(rules);

	if(m_player->isWhite() && rules==RUSSIAN) {
		m_player->setName(m_history->getTag(PdnGame::White));
		m_player->opponent()->setName(m_history->getTag(PdnGame::Black));
	} else {
		m_player->setName(m_history->getTag(PdnGame::Black));
		m_player->opponent()->setName(m_history->getTag(PdnGame::White));
	}

	// FIXME
	m_player->setWhite(rules==RUSSIAN);// FIXME TODO
	m_player->opponent()->setWhite(!m_player->isWhite());
	m_board->setColorWhite(m_player->isWhite());
	m_board->adjustNotation(m_player->isWhite());
}


void myView::slot_apply_moves(const QString& moves)
{
	QStringList move_list= moves.split(MOVE_SPLIT, QString::SkipEmptyParts);

	m_board->reset();

	bool white_player = get_first_player()->isWhite();
	foreach(QString move, move_list) {
		m_board->doMove(move, white_player);
		white_player = !white_player;
	}

	// set current player who pulls next. assume is white.
	m_current = (m_player->isWhite() ? m_player : m_player->opponent());
	if(!white_player)
		m_current = m_current->opponent();
	m_history->setCurrent(m_current->name());
}


void myView::add_log(enum LogType type, const QString& text)
{
	QString str = text;
	str = str.replace('<', "&lt;");
	str = str.replace('>', "&gt;");

	QString tag_b, tag_e;
	switch(type) {
	case Error:	tag_b="<ul><pre>"; tag_e="</pre></ul>"; break;
	case Warning:	tag_b="<b>"; tag_e="</b>"; break;
	case System:	tag_b="<font color=\"blue\">"; tag_e="</font>"; break;
	default:	break;
	}

	m_log->append(tag_b + str + tag_e);

	m_log->ensureCursorVisible();
}


void myView::perform_jumps(const QString& from_board, const QString& to_board)
{
	if(from_board==to_board) {
		return;
	}

	QString new_to_board = to_board;

	//qDebug("F:%s\nT:%s", from_board.latin1(), new_to_board.latin1());

	// diff
	QList<myDiff*> diff_list;

	// collect information
	for(int i=0; i<32; i++) {
		if(from_board[2*i]!=new_to_board[2*i]
				|| from_board[2*i+1]!=new_to_board[2*i+1]) {
			myDiff* diff = new myDiff(i,
					from_board.mid(2*i, 2).toInt(),
					new_to_board.mid(2*i, 2).toInt());
			diff_list.append(diff);

			//qDebug(">%d: %d->%d", diff->m_pos, diff->m_from, diff->m_to);
		}
	}

	int from_pos = -1;
	int to_pos = -1;
	bool captured = (diff_list.count()>2);

	int man = -1;
	// find the dest. first: so we have the man moved.
	foreach(myDiff* diff, diff_list) {
		if(diff->m_to!=FREE) {
			man = diff->m_to;
			to_pos = diff->m_pos;
			break;
		}
	}

	int king = -1;
	switch(man) {
	case MAN1:	king=KING1; break;
	case KING1:	king=MAN1; break;
	case MAN2:	king=KING2; break;
	case KING2:	king=MAN2; break;
	}
	// find src.
	foreach(myDiff* diff, diff_list) {
		if(diff->m_to==FREE) {
			if(diff->m_from==man || diff->m_from==king) {
				from_pos = diff->m_pos;
				break;
			}
		}
	} 

	/*
	  qDebug("  to_pos=%d with man/king=%d from=%d", to_pos, man,
	  from_pos);
	  */

	// finally - animate :)
	QString move = m_board->doMove(from_pos, to_pos, m_current->isWhite());
	m_history->appendMove(move.replace("?", captured ? "x" : "-" ), "");

	qDeleteAll(diff_list);
}
 
void myView::setNotation(bool enabled, bool show_above)
{
	// TODO - intermediate function - remove somehow!
	m_board->setNotation(enabled, show_above);
}


void myView::setNotationFont(const QFont& f)
{
	// TODO - intermediate function - remove somehow!
	m_board->setNotationFont(f);
}


myPlayer* myView::get_first_player() const
{
	bool white = m_board->type()==RUSSIAN ? true : false;
	// it is white.
	if((white && m_player->isWhite()) || (!white && !m_player->isWhite()))
		return m_player;
	return m_player->opponent();
}


