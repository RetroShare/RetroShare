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
#include <stdlib.h>

#include <QLayout>
#include <QDebug>

#include "newgamedlg.h"
#include "pdn.h"
#include "common.h"
#include "history.h"

#include "player.h"


#define BEGINNER		2
#define NOVICE			4
#define AVERAGE			6
#define GOOD			7
#define EXPERT			8
#define MASTER			9

#define CFG_SKILL		CFG_KEY"Skill"
#define CFG_RULES		CFG_KEY"Rules"
#define CFG_WHITE		CFG_KEY"White"
#define CFG_PLAYER1		CFG_KEY"Player1"
#define CFG_PLAYER2		CFG_KEY"Player2"


myNewGameDlg::myNewGameDlg(QWidget* parent)
	: QDialog(parent)
{
	setModal(true);
	setWindowTitle(tr("New Game")+QString(" - "APPNAME));


	/*
	 * buttons, options.
	 */
	start_button = new QPushButton(tr("&Start"), this);
	start_button->setDefault(true);
	connect(start_button, SIGNAL(clicked()), this, SLOT(slot_start()));

	QPushButton* cn = new QPushButton(tr("&Cancel"), this);
	connect(cn, SIGNAL(clicked()), this, SLOT(slot_reject()));

	// TODO - better text
	m_freeplace = new QCheckBox(tr("Free Men Placement"), this);

	QHBoxLayout* buttons_layout = new QHBoxLayout();
	buttons_layout->addWidget(m_freeplace);
	buttons_layout->addStretch();
	buttons_layout->addWidget(start_button);
	buttons_layout->addWidget(cn);


	/*
	 * global layout.
	 */
	QHBoxLayout* players_layout = new QHBoxLayout();
	players_layout->addWidget(create_player_one());
	players_layout->addWidget(create_player_two());

	QVBoxLayout* global_layout = new QVBoxLayout(this);
	global_layout->addLayout(players_layout);
	global_layout->addLayout(buttons_layout);
}


myNewGameDlg::~myNewGameDlg()
{
}


QWidget* myNewGameDlg::create_human_options()
{
	QFrame* frm = new QFrame();
	QVBoxLayout* frm_layout = new QVBoxLayout(frm);
	frm_layout->addWidget(new QLabel("No options available."));
	frm_layout->addStretch();

	return frm;
}


QWidget* myNewGameDlg::create_player_one()
{
	m_player_one.box = new QGroupBox(tr("Player One"), this);

	// name
	m_player_one.name = new QLineEdit(m_player_one.box);

	// rules group box
	m_player_one.rules = new QGroupBox(tr("Rules"), m_player_one.box);
	m_player_one.rule_english = new QRadioButton(
			myHistory::typeToString(ENGLISH), m_player_one.rules);
	m_player_one.rule_russian = new QRadioButton(
			myHistory::typeToString(RUSSIAN), m_player_one.rules);

	QVBoxLayout* rules_layout = new QVBoxLayout(m_player_one.rules);
	rules_layout->addWidget(m_player_one.rule_english);
	rules_layout->addWidget(m_player_one.rule_russian);

	// play white men?
	m_player_one.white = new QCheckBox(tr("White"), m_player_one.box);

	// layout
	QVBoxLayout* vb1_layout = new QVBoxLayout(m_player_one.box);
	vb1_layout->addWidget(m_player_one.name);
	vb1_layout->addWidget(m_player_one.rules);
	vb1_layout->addWidget(m_player_one.white);

	return m_player_one.box;
}


QWidget* myNewGameDlg::create_player_two()
{
	m_player_two.box = new QGroupBox(tr("Player Two"), this);

	// name
	m_player_two.name = new QLineEdit(m_player_two.box);

	// options
	m_player_two.options = new QTabWidget(m_player_two.box);
	m_player_two.options->insertTab(COMPUTER, create_computer_options(), tr("Computer"));
	m_player_two.options->insertTab(HUMAN, create_human_options(), tr("Human"));
	connect(m_player_two.options, SIGNAL(currentChanged(int)),
			this, SLOT(slot_game(int)));

	/*
	 * frame layout
	 */
	QVBoxLayout* frm_layout = new QVBoxLayout(m_player_two.box);
	frm_layout->addWidget(m_player_two.name);
	frm_layout->addWidget(m_player_two.options);

	return m_player_two.box;
}


QWidget* myNewGameDlg::create_computer_options()
{
	QFrame* frm = new QFrame();

	// skills
	QGroupBox* skills = new QGroupBox(tr("Skill"), frm);
	m_player_two.computer.skills[BEGINNER] = new QRadioButton(tr("Beginner"),
			skills);
	m_player_two.computer.skills[NOVICE] = new QRadioButton(tr("Novice"),
			skills);
	m_player_two.computer.skills[AVERAGE] = new QRadioButton(tr("Average"),
			skills);
	m_player_two.computer.skills[GOOD] = new QRadioButton(tr("Good"),
			skills);
	m_player_two.computer.skills[EXPERT] = new QRadioButton(tr("Expert"),
			skills);
	m_player_two.computer.skills[MASTER] = new QRadioButton(tr("Master"),
			skills);

	QGridLayout* skills_layout = new QGridLayout(skills);
	int row = 0;
	int col = 0;
	foreach(QRadioButton* rb, m_player_two.computer.skills) {
		skills_layout->addWidget(rb, row++, col);
		connect(rb, SIGNAL(clicked()), this, SLOT(slot_skills()));
		if(row > 2) {
			row = 0;
			col = 1;
		}
	}

	// layout
	QHBoxLayout* frm_layout = new QHBoxLayout(frm);
	frm_layout->addWidget(skills);

	return frm;
}


void myNewGameDlg::slot_reject()
{
	reject();
}


void myNewGameDlg::slot_start()
{
	accept();
}


void myNewGameDlg::slot_skills()
{
	QRadioButton* skill = 0;
	foreach(QRadioButton* rb, m_player_two.computer.skills) {
		if(rb->isChecked()) {
			skill = rb;
			break;
		}
	}

	if(skill)
			m_player_two.name->setText("*"+skill->text()+"*");
}


void myNewGameDlg::slot_game_start(int id)
{
	slot_game(id);
	slot_start();
}


void myNewGameDlg::slot_game(int id)
{
	start_button->setEnabled(true);
	m_player_one.box->setEnabled(true);
	m_player_two.options->setEnabled(true);

	if(m_player_two.last_game_index==HUMAN) {
		m_cfg_player2 = m_player_two.name->text();
	}

	m_player_two.last_game_index = id;

	switch(id) {
	case COMPUTER:
		m_player_two.name->setReadOnly(true);
		slot_skills();

		m_player_one.rules->setEnabled(true);
		m_player_one.white->setEnabled(true);
		break;

	case HUMAN:
		m_player_two.name->setReadOnly(false);
		m_player_two.name->setText(m_cfg_player2);

		m_player_one.rules->setEnabled(true);
		m_player_one.white->setEnabled(true);
		break;

	default:
		qDebug() << __PRETTY_FUNCTION__ << "ERR";
		break;
	}
}


void myNewGameDlg::writeSettings(QSettings* cfg)
{
	cfg->setValue(CFG_SKILL, skill());
	cfg->setValue(CFG_RULES, rules());
	cfg->setValue(CFG_WHITE, m_player_one.white->isChecked());

	cfg->setValue(CFG_PLAYER1, m_player_one.name->text());
	cfg->setValue(CFG_PLAYER2, m_cfg_player2);
}


void myNewGameDlg::readSettings(QSettings* cfg)
{
	int skills = cfg->value(CFG_SKILL, BEGINNER).toInt();
	QMap<int, QRadioButton*>::iterator it;
	it = m_player_two.computer.skills.find(skills);
	if(it != m_player_two.computer.skills.end())
		it.value()->setChecked(true);
	else
		m_player_two.computer.skills[BEGINNER]->setChecked(true);
	slot_skills();

	int rules = cfg->value(CFG_RULES, ENGLISH).toInt();
	if(rules == ENGLISH)
		m_player_one.rule_english->setChecked(true);
	else
		m_player_one.rule_russian->setChecked(true);

	m_player_one.white->setChecked(cfg->value(CFG_WHITE, false).toBool());

	m_player_one.name->setText(cfg->value(CFG_PLAYER1,
				getenv("USER")).toString());
	m_cfg_player2 = cfg->value(CFG_PLAYER2, "Player2").toString();
}


int myNewGameDlg::skill() const
{
	QMap<int, QRadioButton*>::const_iterator it;
	it = m_player_two.computer.skills.begin();
	for(; it!=m_player_two.computer.skills.end(); ++it) {
		if(it.value()->isChecked())
			return it.key();
	}

	qDebug() << __PRETTY_FUNCTION__ << "No skill selected.";
	return BEGINNER;
}


int myNewGameDlg::rules() const
{
	if(m_player_one.rule_english->isChecked())
		return ENGLISH;
	return RUSSIAN;
}

