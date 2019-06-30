/*******************************************************************************
 * gui/common/Emoticons.h                                                      *
 *                                                                             *
 * Copyright (C) 2010, Retroshare Team <retroshare.project@gmail.com>          *
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

#ifndef _EMOTICONS_H
#define _EMOTICONS_H

class QWidget;
class QString;

class Emoticons
{
public:
    static void load();
    static void loadSmiley();
    static void loadSticker(QString foldername);

    static void showSmileyWidget(QWidget *parent, QWidget *button, const char *slotAddMethod, bool above);
    static void showStickerWidget(QWidget *parent, QWidget *button, const char *slotAddMethod, bool above);

//    static void formatText(QString &text);
};

#endif

