/*******************************************************************************
 * idle/idle.h                                                                 *
 *                                                                             *
 * Copyright (C) 2003  Justin Karneges <retroshare.project@gmail.com>          *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#ifndef IDLE_H
#define IDLE_H

#include <QObject>
#include <QCursor>
#include <QDateTime>
#include <QTimer>

class IdlePlatform;

class Idle : public QObject
{
	Q_OBJECT
public:
	Idle();
	~Idle();

	bool isActive() const;
	bool usingPlatform() const;
	void start();
	void stop();
	int secondsIdle();

signals:
	void secondsIdle(int);

private slots:
	void doCheck();

private:
	class Private;
	Private *d;
};

class IdlePlatform
{
public:
	IdlePlatform();
	~IdlePlatform();

	bool init();
	int secondsIdle();

private:
	class Private;
	Private *d;
};

class Idle::Private
{
public:
	Private() {}

	QPoint lastMousePos;
	QDateTime idleSince;

	bool active;
	int idleTime;
	QDateTime startTime;
	QTimer checkTimer;
};

#endif
