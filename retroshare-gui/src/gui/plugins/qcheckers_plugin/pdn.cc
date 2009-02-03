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
#include <QFile>
#include <QTextStream>
#include <QProgressDialog>
#include <QDebug>

#include "checkers.h"
#include "pdn.h"


#define PLAYER	true
#define COMPUTER  false

#define END_OF_MOVELINE		"@."


Pdn::Pdn()
{
}


Pdn::~Pdn()
{
	qDeleteAll(m_database);
	m_database.clear();
}


bool Pdn::open(const QString& filename, QWidget* parent,
		const QString& label, QString& text_to_log)
{
	qDeleteAll(m_database);
	m_database.clear();

	QFile file(filename);
	if(!file.open(QFile::ReadOnly)) return false;

	QTextStream ts(&file);

	QString str1, str2;

	QProgressDialog progress(parent);
	progress.setModal(true);
	progress.setLabelText(label);
	progress.setRange(0, file.size());
	progress.setMinimumDuration(0);

	unsigned int line_nr = 1;
	unsigned int game_started = 1;
	bool in_tags = false;
	while(!ts.atEnd()) {
		str1 = ts.readLine().trimmed();
		if(ts.atEnd())
			str2 += str1;

		if((str1.length() && str1[0]=='[') || ts.atEnd()) {
			if(!in_tags) {
				// tags begin again, so a game is ended.
				if(str2.length()) {
					if((m_database.count()%10)==0)
						progress.setValue(file.pos());

					QString log_txt;
					PdnGame* game = new PdnGame(str2, log_txt);
					m_database.append(game);

					if(log_txt.length()) {
						text_to_log
							+= QString("%1. game begins at line %2:\n")
							.arg(m_database.count())
							.arg(game_started);
						text_to_log += log_txt;
					}

					game_started = line_nr;
					str2="";
				}

				in_tags = true;
			}
		} else {
			if(in_tags)
				in_tags = false;
		}

		str2.append(str1+"\n");

		if(progress.wasCanceled())
			break;

		line_nr++;
	}

	file.close();

	return true;
}


bool Pdn::save(const QString& filename)
{
	QFile file(filename);
	if(!file.open(QFile::WriteOnly))
		return false;

	QTextStream ts(&file);

	foreach(PdnGame* game, m_database) {
		ts << game->toString() << endl << endl;
	}

	file.close();
	return true;
}



PdnGame* Pdn::newGame()
{
	QString log_txt;		// here: ignore TODO
	PdnGame* game = new PdnGame("", log_txt);
	m_database.append(game);
	return game;
}


/***************************************************************************
 *																		 *
 *																		 *
 ***************************************************************************/
PdnMove::PdnMove(QString line)
{
	if(line[0]=='{') {
		qDebug("a move must not begin with a comment.");
		return;
	}

	// first move.
	m_first = line.section(' ', 0, 0);
	line = line.mid(m_first.length()).trimmed();

	// check for a first comment.
	if(line[0]=='{') {
		int end = line.indexOf('}', 1);
		if(end>=0) {
			m_comfirst = line.mid(1, end-1);
			line.remove(0, end+1);
			line = line.trimmed();
		} else
			qDebug("no comment ending of the first comment.");
	}

	// second move.
	m_second = line.section(' ', 0, 0);
	line = line.mid(m_second.length()).trimmed();

	// check for a second comment.
	if(line[0]=='{') {
		int end = line.indexOf('}', 1);
		if(end>=0)
			m_comsecond = line.mid(1, end-1);
		else
			qDebug("no comment ending of the second comment.");
	}
}


/***************************************************************************
 *																		 *
 *																		 *
 ***************************************************************************/
PdnGame::PdnGame(const QString& game_string, QString& log_txt)
{
	white = PLAYER;
	for(int i=0;  i<12; i++)
		board[i]=MAN2;
	for(int i=20; i<32; i++)
		board[i]=MAN1;

	if(!parse(game_string, log_txt)) {
		qDebug("  errors occured while processing game.");	// TODO
	}
}


PdnGame::~PdnGame()
{
	qDeleteAll(m_moves);
	m_moves.clear();
}


QString PdnGame::get(Tag tag) const
{
	switch(tag) {
	case Date:  return pdnDate;
	case Site:  return pdnSite;
	case Type:  return pdnType;
	case Event: return pdnEvent;
	case Round: return pdnRound;
	case White: return pdnWhite;
	case Black: return pdnBlack;
	default:	return pdnResult;
	}
}


void PdnGame::set(Tag tag, const QString& string)
{
	switch(tag) {
	case Date:  pdnDate=string;		break;
	case Site:  pdnSite=string;		break;
	case Type:  pdnType=string;		break;
	case Event: pdnEvent=string;break;
	case Round: pdnRound=string;break;
	case White: pdnWhite=string;break;
	case Black: pdnBlack=string;break;
	default:	pdnResult=string;
	}
}


bool PdnGame::parse_moves(const QString& line)
{
	qDeleteAll(m_moves);
	m_moves.clear();

	QStringList list = line.split(' ');

	QString current_move;
	int move_num = 0;
	bool in_comment = false;
	foreach(QString str, list) {
		if(str.startsWith("{"))
			in_comment = true;
		if(str.endsWith("}"))
			in_comment = false;
		
		if(str.endsWith(".") && !in_comment) {
			if(str!=END_OF_MOVELINE) {
				if((move_num+1) != str.mid(0, str.length()-1).toInt()) {
					qDebug() << "Move num expected:" << move_num+1
						<< "received:" << str;
					return false;
				}
				move_num++;
			}

			current_move = current_move.trimmed();
			if(current_move.length()) {
				m_moves.append(new PdnMove(current_move));
				current_move = "";
			}
			continue;
		}

			if(str.isEmpty())
			current_move += " ";
		else
			current_move += str + " ";
	}

	return true;
}


bool PdnGame::parse(const QString& pdngame, QString& log_txt)
{
	QString fen;
	QString moves;
	int num = pdngame.count("\n");	// Number of lines

	for(int i=0; i<=num; i++) {
		QString line = pdngame.section('\n',i ,i);
		if(!line.length())
			continue;

		if(line.startsWith("[")) {
			line.remove(0, 1);
			line = line.trimmed();

			if(line.startsWith("GameType"))	  pdnType=line.section('"',1,1);
			else if(line.startsWith("FEN"))		  fen=line.section('"',1,1);
			else if(line.startsWith("Date"))	 pdnDate=line.section('"',1,1);
			else if(line.startsWith("Site"))	 pdnSite=line.section('"',1,1);
			else if(line.startsWith("Event"))   pdnEvent=line.section('"',1,1);
			else if(line.startsWith("Round"))   pdnRound=line.section('"',1,1);
			else if(line.startsWith("White"))   pdnWhite=line.section('"',1,1);
			else if(line.startsWith("Black"))   pdnBlack=line.section('"',1,1);
			else if(line.startsWith("Result")) pdnResult=line.section('"',1,1);
			else ;  // Skip other unsupported tags

		} else {
			moves += " " + line;
		}
	}

	// parse move section.
	if(moves.endsWith(pdnResult))
		moves.truncate(moves.length()-pdnResult.length());
	else {
		log_txt += "  +Different result at the end of the movelist:\n"
			+ QString("	  \"%1\" expected, got \"%2\"\n")
			.arg(pdnResult)
			.arg(moves.right(pdnResult.length()));

		// need to remove the incorrect result.
		if(moves.endsWith(" *")) {
			log_txt += "	  => Ignoring \" *\" from the end.\n";
			moves.truncate(moves.length()-2);
		} else {
			int pos = moves.lastIndexOf('-') - 1;
			bool skip_ws = true;
			for(int i=pos; i>=0; i--) {
				if(moves[i]==' ') {
					if(!skip_ws) {
						log_txt += "	  => Ignoring \""
							+ moves.right(moves.length()-i-1)
							+ "\" from the end.\n",
						moves.truncate(i+1);
						break;
					}
				} else {
					skip_ws = false;
				}
			}
		}
	}

	if(!parse_moves(moves+" "END_OF_MOVELINE)) {		// :)
		log_txt += "\n +parsing moves failed.";
		return false;
	}

	// Translation of the GameType tag
	switch(pdnType.toInt()) {
	case ENGLISH:
	case RUSSIAN:
		break;
	default:
//		log_txt += "\n +setting game type to english.";
		pdnType.setNum(ENGLISH);
		break;
	}

	// Parsing of the Forsyth-Edwards Notation (FEN) tag
	if(fen.isNull())
		return true;

	fen=fen.trimmed();

	for(int i=fen.indexOf(" "); i!=-1; i=fen.indexOf(" "))
		fen=fen.remove(i,1);

	if(fen.startsWith("W:W"))
		white=PLAYER;
	else if(fen.startsWith("B:W"))
		white=COMPUTER;
	else
		return false;

	QString string = fen.mid(3).section(":B",0,0);
	if(!parse(string, white))
		return false;

	string=fen.section(":B",1,1);
	if(string.endsWith("."))
		string.truncate(string.length()-1);
	if(!parse(string, !white))
		return false;

	return true;
}


bool PdnGame::parse(const QString& str, bool side)
{
	QString notation;

	if(pdnType.toInt() == ENGLISH)
		notation=QString(ENOTATION);
	else
		notation=QString(RNOTATION);

	QStringList sections = str.split(",");
	foreach(QString pos, sections) {
		bool king=false;

		if(pos.startsWith("K")) {
			pos=pos.remove(0,1);
			king=true;
		}
		if(pos.length()==1)
			pos.append(' ');
		if(pos.length()!=2)
			return false;

		int index = notation.indexOf(pos);
		if(index%2)
			index=notation.indexOf(pos,index+1);
		if(index == -1)
			return false;

		if(white==COMPUTER)
			index=62-index;

		if(side==PLAYER)
			board[index/2]=(king ? KING1 : MAN1);
		else
			board[index/2]=(king ? KING2 : MAN2);
	}
	return true;
}


PdnMove* PdnGame::getMove(int i)
{
	if(i<m_moves.count()) {
		return m_moves.at(i);
	}

	// TODO - do we need this?
	if(i>m_moves.count())
		qDebug("PdnGame::getMove(%u) m_moves.count()=%u",
				i, m_moves.count());

	PdnMove* m = new PdnMove("");
	m_moves.append(m);
	return m;
}


QString PdnGame::toString()
{
	QString fen;
	QString moves;

	/*
	 * fen
	 */
	if(!movesCount()) {
			qDebug("FEN tag with lots of errors.");
		QString string1;
		QString string2;
		QString notation;

		if(pdnType.toInt() == ENGLISH)
			notation=QString(ENOTATION);
		else
				notation=QString(RNOTATION);

		for(int i=0; i<32; i++) {
			int index=i*2;
			if(white==COMPUTER) index=62-index;

			QString pos;

			switch(board[i]) {
			case KING1:
				pos.append('K');
			case MAN1:
				pos.append(notation.mid(index,2).trimmed());
				if(string1.length()) string1.append(',');
				string1.append(pos);
				break;
			case KING2:
				pos.append('K');
			case MAN2:
				pos.append(notation.mid(index,2).trimmed());
				if(string2.length()) string2.append(',');
				string2.append(pos);
			default:
				break;
			}
		}
			if(white==PLAYER)
				fen.append("W:W"+string1+":B"+string2+".");
		else
			fen.append("B:W"+string2+":B"+string1+".");
	}

	/*
	 * moves
	 */
	unsigned int count = 1;
	foreach(PdnMove* move, m_moves) {
		moves += QString("%1. %2 %3%4%5\n")
			.arg(count)
			.arg(move->m_first)
			.arg(move->m_comfirst.length() ? "{"+move->m_comfirst+"} " : "")
			.arg(move->m_second)
			.arg(move->m_comsecond.length() ? " {"+move->m_comsecond+"}" : "");
		count++;
	}


	/*
	 * create format and write tags+fen+moves.
	 */
	QString str;

	if(pdnEvent.length())		str.append("[Event \""+pdnEvent+"\"]\n");
	if(pdnSite.length())		str.append("[Site \"" +pdnSite +"\"]\n");
	if(pdnDate.length())		str.append("[Date \"" +pdnDate +"\"]\n");
	if(pdnRound.length())		str.append("[Round \""+pdnRound+"\"]\n");
	if(pdnWhite.length())		str.append("[White \""+pdnWhite+"\"]\n");
	if(pdnBlack.length())		str.append("[Black \""+pdnBlack+"\"]\n");

	if(fen.length()) {
		str.append("[SetUp \"1\"]\n");
		str.append("[FEN \""+fen+"\"]\n\n");
	}

	str.append("[Result \""+pdnResult+"\"]\n");
	str.append("[GameType \""+pdnType+"\"]\n");
	str.append(moves);
	str.append(pdnResult+"\n");

	return str;
}

