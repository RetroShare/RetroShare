/*******************************************************************************
 * plugins/VOIP/gui/AudioStats.h                                               *
 *                                                                             *
 * Copyright (C) 2005-2010 Thorvald Natvig <thorvald@natvig.com>               *
 * Copyright (C) 2012 by Retroshare Team <retroshare.project@gmail.com>        *
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
#pragma once

#include <QTimer>
#include <QWidget>

//#include "mumble_pch.hpp"

class AudioBar : public QWidget {
	private:
		Q_OBJECT
		Q_DISABLE_COPY(AudioBar)
	protected:
		void paintEvent(QPaintEvent *event);
	public:
		AudioBar(QWidget *parent = NULL);
		int iMin, iMax;
		int iBelow, iAbove;
		int iValue, iPeak;
                bool highContrast;
		QColor qcBelow, qcInside, qcAbove;

		QList<QColor> qlReplacableColors;
		QList<Qt::BrushStyle> qlReplacementBrushes;
};
/*
class AudioEchoWidget : public QWidget {
	private:
		Q_OBJECT
		Q_DISABLE_COPY(AudioEchoWidget)
	public:
		AudioEchoWidget(QWidget *parent);
	protected slots:
		void paintEvent(QPaintEvent *event);
};

class AudioNoiseWidget : public QWidget {
	private:
		Q_OBJECT
		Q_DISABLE_COPY(AudioNoiseWidget)
	public:
		AudioNoiseWidget(QWidget *parent);
	protected slots:
		void paintEvent(QPaintEvent *event);
};

#include "ui_AudioStats.h"

class AudioStats : public QDialog, public Ui::AudioStats {
	private:
		Q_OBJECT
		Q_DISABLE_COPY(AudioStats)
	protected:
		QTimer *qtTick;
		bool bTalking;
	public:
		AudioStats(QWidget *parent);
		~AudioStats();
	public slots:
		void on_Tick_timeout();
};
*/
