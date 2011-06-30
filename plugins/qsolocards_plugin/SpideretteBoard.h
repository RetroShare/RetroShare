/*
    QSoloCards is a collection of Solitaire card games written using Qt
    Copyright (C) 2009  Steve Moore

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __SPIDERETTEBOARD_H__
#define __SPIDERETTEBOARD_H__

#include "SpiderBoard.h"

class SpideretteBoard:public SpiderBoard
{
    Q_OBJECT
public:
    SpideretteBoard();
    virtual ~SpideretteBoard();

    virtual void newGame();

    virtual void addGameMenuItems(QMenu & menu);

    virtual void loadSettings(const QSettings & settings);
    virtual void saveSettings(QSettings & settings);

protected:
    virtual void resizeEvent (QResizeEvent * event);
    virtual void createStacks();
};

#endif
