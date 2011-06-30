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

#ifndef _NEWGAMEDLG_H_
#define _NEWGAMEDLG_H_

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QSettings>
#include <QPushButton>
#include <QListWidget>
#include <QTabWidget>
#include <QCheckBox>
#include <QGroupBox>
#include <QRadioButton>


#define COMPUTER	0
#define HUMAN		1


class myPlayer;


class myNewGameDlg : public QDialog
{
	Q_OBJECT

public:
	myNewGameDlg(QWidget* parent);
	~myNewGameDlg();

	// playing against the computer
	int rules() const;
	// return HUMAN/COMPUTER
	int opponent() const { return m_player_two.options->currentIndex(); }
	//
	int skill() const;
	//
	bool freePlacement() const { return m_freeplace->isChecked(); }

	//
	const QString name() const { return m_player_one.name->text(); }
	bool isWhite() const { return m_player_one.white->isChecked(); }
	const QString opponentName() const { return m_player_two.name->text(); }

	// for settings
	void writeSettings(QSettings*);
	void readSettings(QSettings*);

private slots:
	void slot_game(int id);		// ListWidget
	void slot_game_start(int id);
	void slot_skills();

	void slot_reject();
	void slot_start();

private:
	QWidget* create_player_one();
	QWidget* create_player_two();

	QWidget* create_human_options();
	QWidget* create_computer_options();

private:
	struct player_one_struct {
		QGroupBox* box;
		QLineEdit* name;
		QGroupBox* rules;
		QRadioButton* rule_english;
		QRadioButton* rule_russian;
		QCheckBox* white;
	};
	struct player_one_struct m_player_one;

	struct player_two_struct {
		QGroupBox* box;
		QLineEdit* name;
		int last_game_index;
		QTabWidget* options;
		// human options
		// computer options
		struct computer_stuct {
			QMap<int, QRadioButton*> skills;
		};
		struct computer_stuct computer;
	};
	struct player_two_struct m_player_two;

	QCheckBox* m_freeplace;
	QPushButton* start_button;

	QString m_cfg_player2;
};

#endif

