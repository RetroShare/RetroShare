/*******************************************************************************
 * retroshare-gui/src/gui/msgs/TagsMenu.h                                      *
 *                                                                             *
 * Copyright (C) 2006-2010 by Retroshare Team  <retroshare.project@gmail.com>  *
 * Copyright (C) 2008-2009 Mehrdad Momeny <mehrdad.momeny@gmail.com>           *
 * Copyright (C) 2008-2009 Golnaz Nilieh <g382nilieh@gmail.com>                *
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

#ifndef TEXTFORMAT_H
#define TEXTFORMAT_H

#include <QTextFormat>

/**
 * Adds some textformat attributes which don't exist in QTextFormat class.
 * this class may be removed in future, if all editor related staff be ordered as a lib.
 *
 @author Mehrdad Momeny <mehrdad.momeny@gmail.com>
 @author Golnaz Nilieh <g382nilieh@gmail.com>
*/
// class TextCharFormat : public QTextCharFormat

class TextFormat
{
public:

    enum Property {
        /// Anchor properties
        AnchorTitle = 0x100010,
        AnchorTarget = 0x100011,

        /// Image properties
        ImageTitle = 0x100020,
        ImageAlternateText = 0x100021,
        
        HasCodeStyle = 0x100030,

        /// Block Properties
        HtmlHeading = 0x100040, //zero if block is not in heading format, 1 for Heading 1, and so on.
        IsBlockQuote = 0x100041,

        IsHtmlTagSign = 0x100042,
    };

};

#endif
