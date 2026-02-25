/*******************************************************************************
 * util/FontSizeHandler.h                                                      *
 *                                                                             *
 * Copyright (C) 2025, Retroshare Team <retroshare.project@gmail.com>          *
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

#ifndef _FONTSIZEHANDLER_H
#define _FONTSIZEHANDLER_H

#include <functional>

class QWidget;
class QAbstractItemView;
class FontSizeHandlerObject;

// Class to handle font size and message font size setting
class FontSizeHandlerBase
{
	friend class FontSizeHandlerObject;

public:
	virtual ~FontSizeHandlerBase();

	int getFontSize();

	void registerFontSize(QWidget *widget, std::function<void(QWidget*, int)> callback = nullptr);
	void registerFontSize(QAbstractItemView *view, float iconHeightFactor = 0.0f, std::function<void(QAbstractItemView*, int)> callback = nullptr);

	void updateFontSize(QWidget *widget, std::function<void(QWidget*, int)> callback = nullptr, bool force = false);
	void updateFontSize(QAbstractItemView *view, float iconHeightFactor, std::function<void (QAbstractItemView *, int)> callback, bool force = false);

protected:
	enum Type {
		FONT_SIZE,
		MESSAGE_FONT_SIZE,
		FORUM_FONT_SIZE
	};

	FontSizeHandlerBase(Type type);

private:
	Type mType;
	FontSizeHandlerObject *mObject;
};

// Class to handle font size setting
class FontSizeHandler : public FontSizeHandlerBase
{
public:
	FontSizeHandler() : FontSizeHandlerBase(FONT_SIZE) {}
};

// Class to handle message font size setting
class MessageFontSizeHandler : public FontSizeHandlerBase
{
public:
	MessageFontSizeHandler() : FontSizeHandlerBase(MESSAGE_FONT_SIZE) {}
};

// Class to handle forum font size setting
class ForumFontSizeHandler : public FontSizeHandlerBase
{
public:
	ForumFontSizeHandler() : FontSizeHandlerBase(FORUM_FONT_SIZE) {}
};

#endif // FONTSIZEHANDLER
