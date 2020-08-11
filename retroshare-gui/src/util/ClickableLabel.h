/*******************************************************************************
 * retroshare-gui/src/util/ClickableLabel.h                                    *
 *                                                                             *
 * Copyright (C) 2020 by RetroShare Team       <retroshare.project@gmail.com>  *
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

#ifndef CLICKABLELABEL_H
#define CLICKABLELABEL_H

#include <QLabel>
#include <QWidget>
#include <Qt>

class ClickableLabel : public QLabel { 
    Q_OBJECT 

public:
    explicit ClickableLabel(QWidget* parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags());
    ~ClickableLabel();

    void setUseStyleSheet(bool b){ mUseStyleSheet=b ; update();}
signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent* event);

    void enterEvent(QEvent * /* ev */ ) override { if(mUseStyleSheet) setStyleSheet("QLabel { border: 2px solid #039bd5; }");}
    void leaveEvent(QEvent * /* ev */ ) override { if(mUseStyleSheet) setStyleSheet("QLabel { border: 2px solid #CCCCCC; border-radius: 3px; }");}

    bool mUseStyleSheet;
};

#endif // CLICKABLELABEL_H
