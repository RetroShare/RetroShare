/*******************************************************************************
 * gui/TheWire/PulseItem.h                                                     *
 *                                                                             *
 * Copyright (c) 2012 Robert Fernie   <retroshare.project@gmail.com>           *
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

#ifndef MRK_PULSE_ITEM_H
#define MRK_PULSE_ITEM_H

#include "ui_PulseItem.h"

class PulseItem;

class PulseHolder
{
    public:
virtual void deletePulseItem(PulseItem *, uint32_t ptype) = 0;
virtual void notifySelection(PulseItem *item, int ptype) = 0;
};

class PulseItem : public QWidget, private Ui::PulseItem
{
  Q_OBJECT

public:
    PulseItem(PulseHolder *parent, std::string url);

    void removeItem();

    void setSelected(bool on);
    bool isSelected();

    const QPixmap *getPixmap();

protected:
    void mousePressEvent(QMouseEvent *event);

private:

    PulseHolder *mParent;
    uint32_t     mType;
    bool mSelected;
};

#endif
