/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008 RetroShare Team
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/
#ifndef HASHINGSTATUS_H
#define HASHINGSTATUS_H

#include <QWidget>

class QLabel;
class ElidedLabel;

class HashingStatus : public QWidget
{
    Q_OBJECT

public:
    HashingStatus(QWidget *parent = 0);
    ~HashingStatus();

    void setCompactMode(bool compact) {_compactMode = compact; }
	void mousePressEvent(QMouseEvent *);

public slots:
    void updateHashingInfo(const QString&) ;

private:
    ElidedLabel *statusHashing;
    QLabel *hashloader;
	QString mLastText ;
    QMovie *movie;
    bool _compactMode;
};

#endif
