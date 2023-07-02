/*******************************************************************************
 * gui/TheWire/CustomFrame.h                                                   *
 *                                                                             *
 * Copyright (c) 2012-2020 Robert Fernie   <retroshare.project@gmail.com>      *
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
#ifndef CUSTOMFRAMEH_H
#define CUSTOMFRAMEH_H

#include <QFrame>
#include <QPixmap>

// This class is made to implement the background image in a Qframe or any widget

class CustomFrame : public QFrame
{
    Q_OBJECT

public:
    explicit CustomFrame(QWidget *parent = nullptr);
    void setPixmap(QPixmap pixmap);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QPixmap backgroundImage;
};

#endif //CUSTOMFRAMEH_H
