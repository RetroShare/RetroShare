/*******************************************************************************
 * gui/Circles/CirclesDialog.h                                                 *
 *                                                                             *
 * LibResAPI: API for local socket server                                      *
 *                                                                             *
 * Copyright (C) 2012, robert Fernie <retroshare.project@gmail.com>            *
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

#ifndef MRK_CIRCLE_DIALOG_H
#define MRK_CIRCLE_DIALOG_H

#include "gui/gxs/RsGxsUpdateBroadcastPage.h"
#include "ui_CirclesDialog.h"

#define IMAGE_CIRCLES           ":/icons/png/circles.png"

class UIStateHelper;

class CirclesDialog : public MainPage
{
	Q_OBJECT

public:
	CirclesDialog(QWidget *parent = 0);
	~CirclesDialog();

	virtual QIcon iconPixmap() const { return QIcon(IMAGE_CIRCLES) ; } //MainPage
	virtual QString pageName() const { return tr("Circles") ; } //MainPage
	virtual QString helpText() const { return ""; } //MainPage

protected:
	virtual void updateDisplay(bool complete);

private slots:
	void todo();
	void createExternalCircle();
	void createPersonalCircle();
	void editExistingCircle();

	void circle_selected();
	void friend_selected();
	void category_selected();

private:
	void reloadAll();

	void requestGroupMeta();
    void loadGroupMeta(const std::list<RsGroupMetaData>& groupInfo);

	UIStateHelper *mStateHelper;

	/* UI - from Designer */
	Ui::CirclesDialog ui;
};

#endif
