/*******************************************************************************
 * gui/common/RsLogo.h                                                         *
 *                                                                             *
 * Copyright (C) 2026, Retroshare Team <retroshare.project@gmail.com>          *
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

#ifndef RS_LOGO_H
#define RS_LOGO_H

#include <QColor>
#include <QIcon>
#include <QPixmap>

/**
 * Renders the RetroShare logo from the SVG symbol
 * (:/icons/svg/retroshare-symbol_gradients.svg) with user defined
 * background circle and symbol (foreground) colors, so several running
 * RetroShare instances can be told apart in the system tray and the task bar.
 *
 * litNodes is the number of highlighted (blue) nodes of the symbol:
 * 0..2 are used by the tray icon to show the number of connected friends,
 * 3 (RsLogo::AllNodes) is the normal logo.
 */
namespace RsLogo
{
	static const int AllNodes = 3;

	/** Default background color of the logo */
	QColor defaultBackgroundColor();

	/** Default color of the symbol (the RetroShare blue) */
	QColor defaultForegroundColor();

	/** Background color currently configured in the settings */
	QColor backgroundColor();

	/** Symbol color currently configured in the settings */
	QColor foregroundColor();

	/** Logo icon using the configured colors */
	QIcon icon(int litNodes = AllNodes);

	/** Logo icon using the given colors */
	QIcon icon(const QColor& background, const QColor& foreground, int litNodes = AllNodes);

	/** Square pixmap of the logo using the configured colors */
	QPixmap pixmap(int size, int litNodes = AllNodes);

	/** Square pixmap of the logo using the given colors */
	QPixmap pixmap(const QColor& background, const QColor& foreground, int size, int litNodes = AllNodes);

	/**
	 * Sets the logo as application icon and replaces the stock logo window
	 * icon of all (current and future) top level windows. Call again after
	 * the background color has been changed.
	 */
	void applyToApplication();
}

#endif // RS_LOGO_H
