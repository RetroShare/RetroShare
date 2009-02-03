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
#include <QDebug>

#include "board.h"
#include "common.h"
#include "pdn.h"
#include "echeckers.h"
#include "rcheckers.h"
/*
#include "newgamedlg.h"

#include "player.h"
#include "humanplayer.h"
#include "computerplayer.h"
*/


myBoard::myBoard(QWidget* parent)
	: QFrame(parent)
{
	/*
	 * board & info 
	 */
	setFrameStyle(QFrame::Box|QFrame::Plain);
	for(int i=0; i<64; i++)
	m_fields[i] = new Field(this, i);

	QGridLayout* grid = new QGridLayout(this);
	grid->setSpacing(0);
	grid->setMargin(0);
	for(int i=0; i<4; i++) {
	for(int k=0; k<4; k++) {
		grid->addWidget(m_fields[i*8+k+32], i*2,  k*2  );
		grid->addWidget(m_fields[i*8+k   ], i*2,  k*2+1);
		grid->addWidget(m_fields[i*8+k+4 ], i*2+1,k*2  );
		grid->addWidget(m_fields[i*8+k+36], i*2+1,k*2+1);
		}
	}

	for(int i=0; i<32; i++)
		connect(m_fields[i], SIGNAL(click(int)),
				this, SIGNAL(fieldClicked(int)));


	/*
	 * game init
	 */
	m_game = 0;

	xpmPat1 = 0;
	xpmPat2 = 0;
	xpmFrame= 0;
	xpmManBlack = 0;
	xpmManWhite = 0;
	xpmKingBlack= 0;
	xpmKingWhite= 0;
}


myBoard::~myBoard()
{
	if(m_game)
		delete m_game;
}


void myBoard::setTheme(const QString& path, bool set_white)
{
	// delete them later.
	QPixmap* p1 = xpmManWhite;
	QPixmap* p2 = xpmManBlack;
	QPixmap* p3 = xpmKingWhite;
	QPixmap* p4 = xpmKingBlack;
	QPixmap* p5 = xpmPat1;
	QPixmap* p6 = xpmPat2;
	QPixmap* p7 = xpmFrame;

	if(path == DEFAULT_THEME) {
	// just in case no themes installed.
	xpmPat1 = new QPixmap(":/icons/theme/tile1.png");
	xpmPat2 = new QPixmap(":/icons/theme/tile2.png");
	xpmFrame= new QPixmap(":/icons/theme/frame.png");
	xpmManBlack = new QPixmap(":/icons/theme/manblack.png");
	xpmManWhite = new QPixmap(":/icons/theme/manwhite.png");
	xpmKingBlack= new QPixmap(":/icons/theme/kingblack.png");
	xpmKingWhite= new QPixmap(":/icons/theme/kingwhite.png");
	} else {
	xpmPat1 = new QPixmap(path+"/"THEME_TILE1);
	xpmPat2 = new QPixmap(path+"/"THEME_TILE2);
	xpmFrame= new QPixmap(path+"/"THEME_FRAME);
	xpmManBlack
		= new QPixmap(path+"/"THEME_MANBLACK);
	xpmManWhite
		= new QPixmap(path+"/"THEME_MANWHITE);
	xpmKingBlack
		= new QPixmap(path+"/"THEME_KINGBLACK);
	xpmKingWhite
		= new QPixmap(path+"/"THEME_KINGWHITE);
	}

	setColorWhite(set_white);

	for(int i=0; i<32; i++)
		m_fields[i]->setPattern(xpmPat2);
	for(int i=32; i<64; i++)
		m_fields[i]->setPattern(xpmPat1);
	for(int i=0; i<32; i++)
		m_fields[i]->setFrame(xpmFrame);

	setFixedSize(xpmMan1->width()*8 + 2*frameWidth(),
		xpmMan1->height()*8 + 2*frameWidth());

	if(m_game)
		do_draw();

	// now delete.
	if(p1) delete p1;
	if(p2) delete p2;
	if(p3) delete p3;
	if(p4) delete p4;
	if(p5) delete p5;
	if(p6) delete p6;
	if(p7) delete p7;
}


void myBoard::reset()
{
	int new_board[32];

	for(int i=0; i<12; i++)
		new_board[i]=MAN2;
	for(int i=12; i<20; i++)
		new_board[i]=FREE;
	for(int i=20; i<32; i++)
		new_board[i]=MAN1;

	// reset frames.
	for(int i=0; i<32; i++)
		m_fields[i]->showFrame(false);

	if(m_game)
		m_game->setup(new_board);

	do_draw();
}


void myBoard::adjustNotation(bool bottom_is_white)
{
	if(!m_game)
		return;

	QString notation = (m_game->type()==ENGLISH
			? ENOTATION : QString(RNOTATION).toUpper());

	if(bottom_is_white) {
		for(int i=0; i<32; i++)
			m_fields[i]->setLabel(notation.mid(i*2,2).trimmed());
	} else {
		for(int i=0; i<32; i++)
			m_fields[i]->setLabel(notation.mid(62-i*2,2).trimmed());
	}
}


void myBoard::do_draw()
{
	for(int i=0; i<32; i++) {
	switch(m_game->item(i)) {
	case MAN1:
		m_fields[i]->setPicture(xpmMan1);
		break;
	case MAN2:
		m_fields[i]->setPicture(xpmMan2);
		break;
	case KING1:
		m_fields[i]->setPicture(xpmKing1);
		break;
	case KING2:
		m_fields[i]->setPicture(xpmKing2);
		break;
	default:
		m_fields[i]->setPicture(NULL);
	}
	}
}


void myBoard::setColorWhite(bool b)
{
	if(b) {
	xpmMan1 = xpmManWhite;
	xpmMan2 = xpmManBlack;
	xpmKing1= xpmKingWhite;
	xpmKing2= xpmKingBlack;
	} else {
	xpmMan1 = xpmManBlack;
	xpmMan2 = xpmManWhite;
	xpmKing1= xpmKingBlack;
	xpmKing2= xpmKingWhite;
	}
}

void myBoard::setNotation(bool s, bool above)
{
	for(int i=0; i<32; i++)
	m_fields[i]->showLabel(s, above);
}

/*
void myBoard::do_move(const QString& move)
{
	qDebug() << __PRETTY_FUNCTION__;
	if(!m_current->isHuman()) {
	add_log(myBoard::Warning, tr("It's not your turn."));
	return;
	}

	int from_num, to_num;
	if(extract_move(move, &from_num, &to_num)) {
	slot_click(from_num);
		slot_click(to_num);
	} else
	add_log(myBoard::Warning, tr("Syntax error. Usage: /from-to"));
}
	*/



bool myBoard::convert_move(const QString& move_orig, int* from_num, int* to_num)
{
	QString move = move_orig.toUpper().replace('X', '-');
	QString from;
	QString to;
	int sect = move.count('-');

	*from_num = *to_num = -1;

	from = move.section('-', 0, 0);
	to = move.section('-', sect, sect);

	if(from!=QString::null && to!=QString::null) {
		for(int i=0; i<32; i++) {
			if(m_fields[i]->label()==from)
				*from_num = m_fields[i]->number();
			if(m_fields[i]->label()==to)
				*to_num = m_fields[i]->number();
		}

		if(*from_num>=0 && *to_num>=0)
			return true;
	}

	return false;
}


void myBoard::setNotationFont(const QFont& f)
{
	setFont(f);
	for(int i=0; i<32; i++)
		m_fields[i]->fontUpdate();
}


void myBoard::setGame(int rules)
{
	if(m_game)
		delete m_game;

	if(rules==ENGLISH) {
		m_game = new ECheckers();
	} else {
		m_game = new RCheckers();
	}

	reset();
}


void myBoard::selectField(int field_num, bool is_on)
{
	for(int i=0; i<32; i++) {
		if(i==field_num)
			m_fields[i]->showFrame(is_on);
		else
			m_fields[i]->showFrame(false);
	}
}


QString myBoard::doMove(int from_num, int to_num, bool white_player)
{
	bool bottom_player = (white_player && (xpmMan1==xpmManWhite))
		|| (!white_player && (xpmMan1==xpmManBlack));

	int from_pos = from_num;
	int to_pos = to_num;

	if(!bottom_player) {
		from_pos = 31-from_pos;
		to_pos = 31-to_pos;
		m_game->fromString(m_game->toString(true));
	}
	if(!m_game->go1(from_pos, to_pos)) {
		return QString::null;
		/*
		qDebug() << __PRETTY_FUNCTION__
			<< from_pos << "," << to_pos
			<< " could not move.";
		*/
	}
	if(!bottom_player) {
		m_game->fromString(m_game->toString(true));
	}

	do_draw();

	return QString("%1?%3")
		.arg(m_fields[from_num]->label())
		.arg(m_fields[to_num]->label());
}


bool myBoard::doMove(const QString& move, bool white_player)
{
	int from_pos, to_pos;
	if(convert_move(move, &from_pos, &to_pos)) {
		doMove(from_pos, to_pos, white_player);
		return true;
	}
	return false;
}


void myBoard::doFreeMove(int from, int to)
{
	int old_to = m_game->item(to);
	int old_from = m_game->item(from);
	m_game->setItem(to, old_from);
	m_game->setItem(from, old_to);
	do_draw();
}

