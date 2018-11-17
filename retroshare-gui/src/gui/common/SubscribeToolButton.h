/*******************************************************************************
 * gui/common/SubscribeToolButton.h                                            *
 *                                                                             *
 * Copyright (c) 2018, RetroShare Team <retroshare.project@gmail.com>          *
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

#ifndef SUBSCRIBETOOLBUTTON_H
#define SUBSCRIBETOOLBUTTON_H

#include <QToolButton>

class SubscribeToolButton : public QToolButton
{
	Q_OBJECT

public:
	explicit SubscribeToolButton(QWidget *parent = 0);

	void setSubscribed(bool subscribed);
	void addSubscribedAction(QAction *action);

signals:
	void subscribe(bool subscribe);

private slots:
	void subscribePrivate();
	void unsubscribePrivate();

private:
	void updateUi();

private:
	bool mSubscribed;
	QList<QAction*> mSubscribedActions;
    	QMenu *mMenu ;
};

#endif // SUBSCRIBETOOLBUTTON_H
