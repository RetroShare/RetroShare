/*******************************************************************************
 * gui/statusbar/peerstatus.h                                                  *
 *                                                                             *
 * Copyright (c) 2008 Retroshare Team <retroshare.project@gmail.com>           *
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

#ifndef PEERSTATUS_H
#define PEERSTATUS_H

#include <QWidget>

class QLabel;

class PeerStatus : public QWidget
{
    Q_OBJECT

public:
    PeerStatus(QWidget *parent = 0);

    void getPeerStatus(unsigned int nFriendCount, unsigned int nOnlineCount);
    void setCompactMode(bool compact) {_compactMode = compact; }

private:
    QLabel *iconLabel, *statusPeers;
    bool _compactMode;
};

#endif
