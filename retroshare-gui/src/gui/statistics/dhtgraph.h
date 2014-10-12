/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (C) 2014 RetroShare Team
 * Copyright (c) 2006-2007, crypton
 * Copyright (c) 2006, Matt Edman, Justin Hipple
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

#pragma once

#include <QApplication>
#include <gui/common/RSGraphWidget.h>

#define HOR_SPC       2   /** Space between data points */
#define SCALE_WIDTH   75  /** Width of the scale */
#define MINUSER_SCALE 2000  /** 2000 users is the minimum scale */  
#define SCROLL_STEP   4   /** Horizontal change on graph update */

#define BACK_COLOR    Qt::white
#define SCALE_COLOR   Qt::black
#define GRID_COLOR    Qt::black
#define RSDHT_COLOR   Qt::magenta
#define ALLDHT_COLOR  Qt::yellow

#define FONT_SIZE     11

class DhtGraph : public RSGraphWidget
{
public:
    DhtGraph(QWidget *parent = 0);

    /** Show the respective lines and counters. */
    bool _showRSDHT;
    bool _showALLDHT;
};

