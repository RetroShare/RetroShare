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
#ifndef _HISTORY_H_
#define _HISTORY_H_


#include <QFrame>
#include <QTreeWidget>
#include <QComboBox>
#include <QToolButton>
#include <QLabel>

#include "pdn.h"


#define MOVE_SPLIT	'#'


class myHistory : public QFrame
{
	Q_OBJECT

public:
	myHistory(QWidget* parent);
	~myHistory();

	void newPdn(const QString& event, bool freeplace);
	bool openPdn(const QString& filename, QString& log_text);
	bool savePdn(const QString& fn);

	void clear();

	bool isPaused() const { return m_paused; }
	bool isFreePlacement() const { return m_freeplace; }

	void setTag(PdnGame::Tag, const QString& val);
	QString getTag(PdnGame::Tag);

	void appendMove(const QString& move, const QString& comment);
	// FIXME - provide a function that returns who is next, black or white.
	int moveCount() const { return m_movelist->topLevelItemCount()-1; }

	static QString typeToString(int type);

	void setCurrent(const QString& t) { m_current->setText(t); }

signals:
	void previewGame(int game_type);
	void applyMoves(const QString& moves);
	void newMode(bool paused, bool freeplace);

public slots:
	void slotWorking(bool);

private slots:
	void slot_move(QTreeWidgetItem*, QTreeWidgetItem*);
	void slot_game_selected(int index);
	void slot_modify_tag(QTreeWidgetItem* item, int col);
	void slot_modify_comment(QTreeWidgetItem* item, int col);

	void slot_undo();
	void slot_redo();
	void slot_continue();

private:
	QString tag_to_string(PdnGame::Tag);
	void set_mode(bool);

	void do_moves();
	void history_undo(bool move_backwards);
	void delete_moves();


private:
	QTreeWidget* m_taglist;
	QTreeWidget* m_movelist;
	QComboBox* m_gamelist;
	Pdn* m_pdn;
	PdnGame* m_game;

	bool m_paused;
	bool m_freeplace;
	bool m_disable_moves;

	QToolButton* m_undo;
	QToolButton* m_redo;
	QToolButton* m_cont;

	QLabel* m_mode_icon;
	QLabel* m_current;
};


#endif

