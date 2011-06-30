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
#include <QDebug>
#include <QApplication>
#include <QEvent>

#include "computerplayer.h"
#include "pdn.h"
#include "rcheckers.h"
#include "echeckers.h"
#include "checkers.h"


myComputerPlayer::myComputerPlayer(const QString& name, bool white, int skill)
	: myPlayer(name, white)
{
	m_game = 0;
	m_thread = 0;
	m_skill = skill;
}


myComputerPlayer::~myComputerPlayer()
{
	if(m_thread) {
		m_thread->stop();
		// delete m_thread
	}
	delete m_game;
}


void myComputerPlayer::yourTurn(const Checkers* g)
{
	if(m_thread)
		qDebug("myComputerPlayer::yourTurn: a thread exists.");

	// first create it.
	if(!m_game || m_game->type()!=g->type()) {
		delete m_game;
	if(g->type()==RUSSIAN)
		m_game = new RCheckers();
	else
		m_game = new ECheckers();
	}

	m_game->setSkill(m_skill);
	m_game->fromString(g->toString(false));

	m_thread = new myThread(this, m_game);
	m_thread->start();
}


void myComputerPlayer::stop()
{
	if(m_thread) {
		m_thread->stop();
	}
}


void myComputerPlayer::customEvent(QEvent* ev)
{
	if(ev->type() == QEvent::MaxUser) {
		m_thread->wait();

		delete m_thread;
		m_thread = 0;

		emit moveDone(m_game->toString(false));
	}
}



/****************************************************************************
 *
 *
 ***************************************************************************/
void myThread::run()
{
	m_game->go2();
	if(!m_aborted) {
		QEvent* ev = new QEvent(QEvent::MaxUser);
		QApplication::postEvent(m_player, ev);
	} else
		qDebug("thread.aborted.done.");
}


void myThread::stop()
{
	m_aborted = true;
	m_game->setSkill(0);
}

