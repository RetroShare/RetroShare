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
#include <QProgressDialog>
#include <QHeaderView>
#include <QInputDialog>
#include <QLineEdit>

#include "history.h"
#include "common.h"



#define COL_TAG_NR	0	// m_tags, this col is hidden.
#define COL_TAG_NAME	1
#define COL_TAG_VAL	2

#define COL_MOVE_NR	0	// m_movelist
#define COL_MOVE	1
#define COL_MOVE_COMM	2


myHistory::myHistory(QWidget* parent)
	: QFrame(parent)
{
	setFixedWidth(240);

	m_gamelist = new QComboBox(this);
	connect(m_gamelist, SIGNAL(activated(int)),
		this, SLOT(slot_game_selected(int)));

	m_taglist = new QTreeWidget(this);
	m_taglist->setColumnCount(3);
	m_taglist->header()->hide();
	m_taglist->setColumnHidden(COL_TAG_NR, true);
//	m_taglist->header()->setStretchLastSection(true);
//	m_taglist->header()->setResizeMode(QHeaderView::Stretch);
	m_taglist->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	connect(m_taglist, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)),
		this, SLOT(slot_modify_tag(QTreeWidgetItem*, int)));

	m_movelist = new QTreeWidget(this);
	m_movelist->setColumnCount(3);
	m_movelist->header()->setStretchLastSection(true);
	m_movelist->header()->setMovable(false);
	m_movelist->setRootIsDecorated(false);
	QStringList header;
	header << "#" << tr("Move") << tr("Comment");
	m_movelist->setHeaderLabels(header);
	//
	connect(m_movelist, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)),
		this, SLOT(slot_modify_comment(QTreeWidgetItem*, int)));
	connect(m_movelist,
		SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
		this,
		SLOT(slot_move(QTreeWidgetItem*, QTreeWidgetItem*)));

	// history
	/*
	gameUndo = new QAction(QIcon(":/icons/undo.png"), tr("&Undo"), this);
	connect(gameUndo, SIGNAL(triggered()), m_view, SLOT(slotUndo()));

	gameRedo = new QAction(QIcon(":/icons/redo.png"), tr("&Redo"), this);
	connect(gameRedo, SIGNAL(triggered()), m_view, SLOT(slotRedo()));

	gameContinue = new QAction(QIcon(":/icons/continue.png"),
			tr("&Continue"), this);
	connect(gameContinue, SIGNAL(triggered()), m_view, SLOT(slotContinue()));


	 */
	m_mode_icon = new QLabel(this);
	m_mode_icon->setFrameStyle(QFrame::Panel | QFrame::Sunken);

	m_undo = new QToolButton(this);
	m_undo->setIcon(QIcon(":/icons/undo.png"));
	m_undo->setToolTip(tr("Undo"));
	connect(m_undo, SIGNAL(clicked()), this, SLOT(slot_undo()));

	m_redo = new QToolButton(this);
	m_redo->setIcon(QIcon(":/icons/redo.png"));
	m_redo->setToolTip(tr("Redo"));
	connect(m_redo, SIGNAL(clicked()), this, SLOT(slot_redo()));

	m_cont = new QToolButton(this);
	m_cont->setIcon(QIcon(":/icons/continue.png"));
	m_cont->setToolTip(tr("Continue"));
	connect(m_cont, SIGNAL(clicked()), this, SLOT(slot_continue()));

	m_current = new QLabel(this);

	QHBoxLayout* history = new QHBoxLayout();
	history->addWidget(m_mode_icon);
//TODO	history->addStretch();
	history->addWidget(m_undo);
	history->addWidget(m_redo);
	history->addWidget(m_cont);
	history->addStretch();
	history->addWidget(m_current);

	// layout
	QVBoxLayout* vb = new QVBoxLayout(this);
	vb->setMargin(0);
	vb->addWidget(m_gamelist, 0);
	vb->addWidget(m_taglist, 2);
	vb->addWidget(m_movelist, 4);
	vb->addLayout(history);

	/*
	 * other stuff
	 */
	m_pdn = new Pdn();
	m_disable_moves = false;
	//
	m_paused = true;	// little hack ensures a mode change.
	m_freeplace = false;
	set_mode(false);
}


myHistory::~myHistory()
{
	delete m_pdn;
}


void myHistory::clear()
{
	m_gamelist->clear();
	m_pdn->clear();
	m_taglist->clear();
	m_movelist->clear();
}


void myHistory::setTag(PdnGame::Tag tag, const QString& val)
{
	QTreeWidgetItem* item = 0;
	QList<QTreeWidgetItem*> item_list = m_taglist->findItems(
			tag_to_string(tag), Qt::MatchExactly, COL_TAG_NAME);
	if(item_list.count()) {
		if(item_list.count() == 1)
			item = item_list[0];
		else
			qDebug() << __PRETTY_FUNCTION__ << "ERR";
	}

	if(item) {
		item->setText(COL_TAG_VAL, val);
	} else {
		item = new QTreeWidgetItem(m_taglist);
		item->setText(COL_TAG_NR, QString::number(tag));
		item->setText(COL_TAG_NAME, tag_to_string(tag));
		item->setText(COL_TAG_VAL, val);
	}

	if(tag==PdnGame::Type) {
		item->setText(COL_TAG_VAL, val + " ("
				+ typeToString(QString("%1").arg(val).toInt())
				+ ")");
	}

	m_game->set(tag, val);
//TODO	m_taglist->resizeColumnToContents(COL_TAG_NAME);
}


QString myHistory::getTag(PdnGame::Tag tag)
{
	QList<QTreeWidgetItem*> item_list = m_taglist->findItems(
			tag_to_string(tag), Qt::MatchExactly, COL_TAG_NAME);
	if(item_list.count() == 1)
		return item_list[0]->text(COL_TAG_VAL);
	return "";
}


QString myHistory::tag_to_string(PdnGame::Tag tag)
{
	switch(tag) {
	case PdnGame::Date:	return /*tr(*/"Date";//);
	case PdnGame::Site:	return /*tr(*/"Site";//);
	case PdnGame::Type:	return /*tr(*/"Type";//);
	case PdnGame::Event:	return /*tr(*/"Event";//);
	case PdnGame::Round:	return /*tr(*/"Round";//);
	case PdnGame::White:	return /*tr(*/"White";//);
	case PdnGame::Black:	return /*tr(*/"Black";//);
	case PdnGame::Result:	return /*tr(*/"Result";//);
	}

	return "Site";	// FIXME
}


void myHistory::appendMove(const QString& text, const QString& comm)
{
	m_disable_moves = true;

	QTreeWidgetItem* new_item = new QTreeWidgetItem(m_movelist);
	new_item->setText(COL_MOVE, text);
	new_item->setText(COL_MOVE_COMM, comm);

	int move_nr = (m_movelist->topLevelItemCount() - 2) / 2;
	PdnMove* m = m_game->getMove(move_nr);

	if(m_movelist->topLevelItemCount()%2) {
		m->m_second = text;
		m->m_comsecond = comm;
	} else {
		new_item->setText(COL_MOVE_NR, QString("%1.").arg(move_nr+1));
		m->m_first = text;
		m->m_comfirst = comm;
	}

	m_movelist->setCurrentItem(new_item);
	m_movelist->scrollToItem(new_item);

	// TODO
	m_movelist->resizeColumnToContents(COL_MOVE_NR);

	m_disable_moves = false;
}


void myHistory::slot_modify_comment(QTreeWidgetItem* item, int)
{
	if(!item || item==m_movelist->topLevelItem(0) || m_paused)
		return;

	bool ok;
	QString new_text = QInputDialog::getText(this, tr("Set Comment"),//FIXME
		tr("Comment")+":", QLineEdit::Normal, item->text(COL_MOVE_COMM),
		&ok);
	if(!ok)
		return;

	new_text.remove('{');
	new_text.remove('}');
	if(new_text != item->text(COL_MOVE_COMM)) {
		// gui
		item->setText(COL_MOVE_COMM, new_text);

		// pdn
		int index = m_movelist->indexOfTopLevelItem(item);
		PdnMove* move = m_game->getMove((index - 1) / 2);
		if(index%2==1)
			move->m_comfirst = new_text;
		else
			move->m_comsecond = new_text;
	}
}


void myHistory::slot_modify_tag(QTreeWidgetItem* item, int/* col*/)
{
	if(!item || m_paused)
		return;

	PdnGame::Tag tag =(PdnGame::Tag)item->text(COL_TAG_NR).toUInt();
	if(tag==PdnGame::Type) {
		return;
	}

	bool ok;
	QString new_text = QInputDialog::getText(this, tr("Set Tag"),//FIXME
		tr("Tag")+":", QLineEdit::Normal, item->text(COL_TAG_VAL), &ok);
	if(!ok)
		return;

	new_text.remove('"');
	new_text.remove('[');
	new_text.remove(']');

	if(new_text != item->text(COL_TAG_VAL)) {
		item->setText(COL_TAG_VAL, new_text);
		m_game->set(tag, new_text);
		if(tag==PdnGame::Event)
			m_gamelist->setItemText(m_gamelist->currentIndex(),
					new_text);
	}
}


bool myHistory::openPdn(const QString& filename, QString& log_text)
{
	if(!m_pdn->open(filename, this, tr("Reading file..."), log_text)) {
		set_mode(false);
		return false;
	}

	set_mode(true);

	m_gamelist->clear();
	m_movelist->clear();
	m_taglist->clear();

	QProgressDialog progress(this);
	progress.setModal(true);
	progress.setLabelText(tr("Importing games..."));
	progress.setRange(0, m_pdn->count());
	progress.setMinimumDuration(0);

	for(int i=0; i<m_pdn->count(); ++i) {
		if((i%10)==0)
			progress.setValue(i);
		m_gamelist->insertItem(i, m_pdn->game(i)->get(PdnGame::Event));
	}

	slot_game_selected(0);

	return true;
}


bool myHistory::savePdn(const QString& fn)
{
	return m_pdn->save(fn);
}


void myHistory::slot_game_selected(int index)
{
	if(index>=m_pdn->count()) {
		qDebug() << __PRETTY_FUNCTION__ << "Index" << index
			<< "out of range >=" << m_pdn->count();
		return;
	}

	m_game = m_pdn->game(index);
	m_movelist->clear();

	QTreeWidgetItem* root = new QTreeWidgetItem(m_movelist);
	for(int i=0; i<m_game->movesCount(); ++i) {
		PdnMove* m = m_game->getMove(i);

		appendMove(m->m_first, m->m_comfirst);
		if(m->m_second.length())
			appendMove(m->m_second, m->m_comsecond);
	}

	setTag(PdnGame::Site,	m_game->get(PdnGame::Site));
	setTag(PdnGame::Black,	m_game->get(PdnGame::Black));
	setTag(PdnGame::White,	m_game->get(PdnGame::White));
	setTag(PdnGame::Result,	m_game->get(PdnGame::Result));
	setTag(PdnGame::Date,	m_game->get(PdnGame::Date));
	setTag(PdnGame::Site,	m_game->get(PdnGame::Site));
	setTag(PdnGame::Type,	m_game->get(PdnGame::Type));
	setTag(PdnGame::Round,	m_game->get(PdnGame::Round));
	setTag(PdnGame::Event,	m_game->get(PdnGame::Event));

	// signal to view
	if(m_paused && !m_freeplace) {
		emit previewGame(m_game->get(PdnGame::Type).toInt());
	}

	m_movelist->setCurrentItem(root);
	slot_move(root, 0);
}


void myHistory::newPdn(const QString& event, bool freeplace)
{
	m_freeplace = freeplace;
	m_paused = !m_freeplace;	// FIXME - needed to force view update.
	set_mode(m_freeplace);

	PdnGame* game = m_pdn->newGame();
	game->set(PdnGame::Event, event);

	int index = m_gamelist->count();
	m_gamelist->insertItem(index, event);
	m_gamelist->setCurrentIndex(index);

	slot_game_selected(index);
}


QString myHistory::typeToString(int type)
{
	switch(type) {
		case ENGLISH:	   return tr("English draughts");
		case RUSSIAN:	   return tr("Russian draughts");
	};
	return tr("Unknown game type");
}


void myHistory::set_mode(bool paused)
{
	if(m_paused != paused) {
		m_paused = paused;

		if(m_paused) {
			if(m_freeplace) {
				m_mode_icon->setPixmap(QPixmap(":/icons/freeplace.png"));
				m_mode_icon->setToolTip(tr("Free Placement Mode"));
			 } else {
				m_mode_icon->setPixmap(QPixmap(":/icons/paused.png"));
				m_mode_icon->setToolTip(tr("Paused Mode"));
			 }
		} else {
			m_mode_icon->setPixmap(QPixmap(":/icons/logo.png"));
			m_mode_icon->setToolTip(tr("Play Mode"));
		}

		m_gamelist->setEnabled(m_paused);
		//FIXME	m_movelist->setEnabled(yes);

		emit newMode(m_paused, m_freeplace);
	}
}


void myHistory::slot_move(QTreeWidgetItem* item, QTreeWidgetItem*)
{
	// update history buttons.
	bool curr_is_first =
		(m_movelist->topLevelItem(0) == m_movelist->currentItem());
	bool curr_is_last = 
		(m_movelist->indexOfTopLevelItem(m_movelist->currentItem())
			== m_movelist->topLevelItemCount()-1);

	m_undo->setEnabled(!curr_is_first);
	m_redo->setEnabled(!curr_is_last);
	m_cont->setEnabled(m_paused);


	// process
	if(!item || !m_paused || m_disable_moves)
		return;

	do_moves();
}


void myHistory::history_undo(bool move_backwards)
{
	int next = m_movelist->indexOfTopLevelItem(m_movelist->currentItem())
		+ (move_backwards ? -1 : +1);

	if(next>=0 && next<m_movelist->topLevelItemCount())
		m_movelist->setCurrentItem(m_movelist->topLevelItem(next));
}


void myHistory::do_moves()
{
	QString moves;
	QTreeWidgetItem* item = m_movelist->currentItem();
	for(int i=0; i<m_movelist->topLevelItemCount(); ++i) {
		QTreeWidgetItem* cur = m_movelist->topLevelItem(i);
		moves += cur->text(COL_MOVE) + MOVE_SPLIT;
		if(m_movelist->topLevelItem(i)==item)
			break;
	}
	emit applyMoves(moves);
}


void myHistory::delete_moves()
{
	int curr = m_movelist->indexOfTopLevelItem(m_movelist->currentItem());
	while(m_movelist->topLevelItemCount() > curr+1) {
		delete m_movelist->topLevelItem(
				m_movelist->topLevelItemCount()-1);
	}
}


void myHistory::slot_undo()
{
	set_mode(true);
	history_undo(true);
	do_moves();
}

void myHistory::slot_redo()
{
	set_mode(true);
	history_undo(false);
	do_moves();
}


void myHistory::slot_continue()
{
	delete_moves();
	set_mode(false);
}


void myHistory::slotWorking(bool b)
{
	setEnabled(!b);
}

